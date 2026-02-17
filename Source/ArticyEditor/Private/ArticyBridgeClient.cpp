//  
// Copyright (c) 2025 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyBridgeClient.h"
#include "Common/TcpSocketBuilder.h"
#include "IPAddress.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Modules/ModuleManager.h"
#include "ArticyEditorFunctionLibrary.h"
#include "Networking.h"
#include "UObject/UnrealType.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "ArticyImportData.h"
#include "ArticyTexts.h"
#include "ArticyBridgeDiscoveryDialog.h"
#include "Dom/JsonObject.h"
#include "ArticyLocalizerSystem.h"
#include "CodeGeneration/CodeGenerator.h"
#include "ArticyBuiltinTypes.h"
#include "ArticyPins.h"
#include "ArticyEditorModule.h"

TArray<uint8> DataBuffer;

static const int32 HEADER_SIZE = 48;  // 4 + 32 + 8 + 4

// Helper function to discover server via UDP advertisement.
bool UArticyBridgeClientCommands::DiscoverServerAdvertisement(FString & OutHostname, int32 & OutPort)
{
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get();
    if (!SocketSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("No SocketSubsystem available."));
        return false;
    }

    FSocket* UDPSocket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("AdvertisementSocket"), false);
    if (!UDPSocket)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create UDP socket for advertisement."));
        return false;
    }

    // Make multicast more resilient
    UDPSocket->SetReuseAddr(true);
    UDPSocket->SetBroadcast(true);
    UDPSocket->SetNonBlocking(true);
    UDPSocket->SetRecvErr(false);
    int32 Buf = 256 * 1024;
    UDPSocket->SetReceiveBufferSize(Buf, Buf);

    // Bind to ANY:3334
    TSharedRef<FInternetAddr> ListenAddr = SocketSubsystem->CreateInternetAddr();
    ListenAddr->SetAnyAddress();
    ListenAddr->SetPort(3334);
    if (!UDPSocket->Bind(*ListenAddr))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to bind UDP advertisement socket."));
        SocketSubsystem->DestroySocket(UDPSocket);
        return false;
    }

    // Multicast group 239.255.0.1:3334
    TSharedRef<FInternetAddr> MulticastAddr = SocketSubsystem->CreateInternetAddr();
    bool bGroupValid = false;
    MulticastAddr->SetIp(TEXT("239.255.0.1"), bGroupValid);
    MulticastAddr->SetPort(3334);

    // Enable loopback so same-host adverts are received
    UDPSocket->SetMulticastLoopback(true);
    UDPSocket->SetMulticastTtl(1);

    if (bGroupValid)
    {
        // Join on loopback explicitly (same-PC case)
        {
            TSharedRef<FInternetAddr> LoopIf = SocketSubsystem->CreateInternetAddr();
            bool bLoopOk = false;
            LoopIf->SetIp(TEXT("127.0.0.1"), bLoopOk);
            if (bLoopOk)
            {
                if (!UDPSocket->JoinMulticastGroup(*MulticastAddr, *LoopIf))
                {
                    UE_LOG(LogTemp, Verbose, TEXT("JoinMulticastGroup loopback failed (ok to ignore on some OSes)."));
                }
            }
        }

        // Join on every local adapter
        TArray<TSharedPtr<FInternetAddr>> LocalIfs;
        SocketSubsystem->GetLocalAdapterAddresses(LocalIfs);

        int32 Joined = 0;
        for (const TSharedPtr<FInternetAddr>& IfAddr : LocalIfs)
        {
            if (!IfAddr.IsValid()) { continue; }
            if (UDPSocket->JoinMulticastGroup(*MulticastAddr, *IfAddr))
            {
                ++Joined;
                UE_LOG(LogTemp, Verbose, TEXT("Joined 239.255.0.1:3334 via %s"), *IfAddr->ToString(false));
            }
        }

        // Best-effort fallback: join without specifying interface
        if (Joined == 0)
        {
            if (!UDPSocket->JoinMulticastGroup(*MulticastAddr))
            {
                UE_LOG(LogTemp, Warning, TEXT("Failed to join multicast group on any interface."));
            }
        }
    }

    const float TimeoutSeconds = 5.0f;
    const double StartTime = FPlatformTime::Seconds();
    TArray<uint8> ReceivedData;
    ReceivedData.SetNumUninitialized(64 * 1024);
    TSharedRef<FInternetAddr> Sender = SocketSubsystem->CreateInternetAddr();

    while ((FPlatformTime::Seconds() - StartTime) < TimeoutSeconds)
    {
        uint32 Pending = 0;
        if (UDPSocket->HasPendingData(Pending) && Pending > 0)
        {
            int32 BytesRead = 0;
            if (UDPSocket->RecvFrom(ReceivedData.GetData(), ReceivedData.Num(), BytesRead, *Sender) && BytesRead >= HEADER_SIZE)
            {
                if (FMemory::Memcmp(ReceivedData.GetData(), "RTCB", 4) != 0)
                {
                    continue;
                }

                int32 MessageLength = 0;
                FMemory::Memcpy(&MessageLength, ReceivedData.GetData() + 44, sizeof(int32));
                if (MessageLength <= 0 || HEADER_SIZE + MessageLength > BytesRead)
                {
                    continue;
                }

                const char* JsonStart = reinterpret_cast<const char*>(ReceivedData.GetData() + HEADER_SIZE);
                FUTF8ToTCHAR Conv(JsonStart, MessageLength);
                FString JsonString(Conv.Length(), Conv.Get());

                TSharedPtr<FJsonObject> Obj;
                auto Reader = TJsonReaderFactory<>::Create(JsonString);
                if (FJsonSerializer::Deserialize(Reader, Obj) && Obj.IsValid())
                {
                    OutPort = Obj->GetIntegerField(TEXT("ServerPort"));
                    OutHostname = Sender->ToString(false); // source IP
                    SocketSubsystem->DestroySocket(UDPSocket);
                    return true;
                }
            }
        }
        FPlatformProcess::Sleep(0.05f);
    }

    SocketSubsystem->DestroySocket(UDPSocket);

    // Fallback when multicast is blocked
    OutHostname = TEXT("127.0.0.1");
    OutPort = 9870;
    UE_LOG(LogTemp, Warning, TEXT("Discovery timed out; falling back to %s:%d"), *OutHostname, OutPort);
    return true;
}

void FArticyBridgeClientRunnable::SendHandshake()
{
    FString JsonString;

    FString ComputerName = FPlatformProcess::ComputerName();
    auto ImportDataPtr = UArticyImportData::GetImportData();
    UArticyImportData* ImportData = ImportDataPtr.IsValid() ? ImportDataPtr.Get() : nullptr;

    FString RuleSetIdStr = TEXT("");
    FString RuleSetChecksumStr = TEXT("");
    if (ImportData)
    {
        const FAdiSettings& S = ImportData->GetSettings();
        RuleSetIdStr = S.RuleSetId;
        RuleSetChecksumStr = S.RuleSetChecksum;
    }

    // 1) Build the JSON payload
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    Writer->WriteObjectStart();
    Writer->WriteValue(TEXT("ClientName"), ComputerName);
    Writer->WriteValue(TEXT("Type"), TEXT("Unreal"));
    Writer->WriteValue(TEXT("Version"), 1);
    Writer->WriteValue(TEXT("NewSession"), !bSessionEstablished);
    Writer->WriteValue(TEXT("UsedRuleSetId"), RuleSetIdStr);
    Writer->WriteValue(TEXT("UsedRuleSetChecksum"), RuleSetChecksumStr);
    Writer->WriteObjectEnd();
    Writer->Close();

    // 2) Frame the packet
    const ANSICHAR* TypeName = "ClientBridgeSessionData";
    const ANSICHAR* DataType = "Json";
    FTCHARToUTF8 Utf8(JsonString);
    int32 BodyLen = Utf8.Length();

    TArray<uint8> Packet;
    Packet.SetNumUninitialized(HEADER_SIZE + BodyLen);

    // RTCB tag
    FMemory::Memcpy(Packet.GetData() + 0, "RTCB", 4);
    // MessageType (32 bytes, zero-padded)
    FMemory::Memset(Packet.GetData() + 4, 0, 32);
    FMemory::Memcpy(Packet.GetData() + 4, TypeName, FMath::Min<int32>(FCStringAnsi::Strlen(TypeName), 32));
    // DataType (8 bytes, zero-padded)
    FMemory::Memset(Packet.GetData() + 36, 0, 8);
    FMemory::Memcpy(Packet.GetData() + 36, DataType, FCStringAnsi::Strlen(DataType));
    // Length (4 bytes little endian)
    FMemory::Memcpy(Packet.GetData() + 44, &BodyLen, sizeof(int32));
    // JSON body
    FMemory::Memcpy(Packet.GetData() + HEADER_SIZE, Utf8.Get(), BodyLen);

    // 3) Send it
    int32 Sent = 0;
    if (!Socket->Send(Packet.GetData(), Packet.Num(), Sent))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed sending handshake"));
    }
    else
    {
        bSessionEstablished = true;
    }
}

void UArticyBridgeClientCommands::ShowBridgeDialog(const TArray<FString>&)
{
    if (!FSlateApplication::IsInitialized())
        return;
    TSharedRef<SWindow> Window = SNew(SWindow)
        .Title(FText::FromString(TEXT("Connect to Articy Bridge")))
        .ClientSize(FVector2D(400, 350))
        .SupportsMinimize(false)
        .SupportsMaximize(false);
    Window->SetContent(
        SNew(SBridgeDiscoveryDialog)
        .DialogWindow(Window)
    );
    FSlateApplication::Get().AddWindow(Window);
}


void UArticyBridgeClientCommands::RegisterConsoleCommands()
{
    IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("StartBridgeConnection"),
        TEXT("Starts the bridge connection with specified parameters, or discovers server via UDP if not specified."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&UArticyBridgeClientCommands::StartBridgeConnection),
        ECVF_Default
    );

    IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("StopBridgeConnection"),
        TEXT("Stops the bridge connection."),
        FConsoleCommandDelegate::CreateStatic(&UArticyBridgeClientCommands::StopBridgeConnection),
        ECVF_Default
    );

    IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("ShowBridgeDialog"),
        TEXT("Opens the Bridge discovery and connect dialog."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&UArticyBridgeClientCommands::ShowBridgeDialog),
        ECVF_Default
    );
}

void UArticyBridgeClientCommands::UnregisterConsoleCommands()
{
    IConsoleManager::Get().UnregisterConsoleObject(TEXT("StartBridgeConnection"), false);
    IConsoleManager::Get().UnregisterConsoleObject(TEXT("StopBridgeConnection"), false);
    IConsoleManager::Get().UnregisterConsoleObject(TEXT("ShowBridgeDialog"), false);
}

TUniquePtr<FArticyBridgeClientRunnable> ClientRunnable;

FArticyBridgeClientRunnable::FArticyBridgeClientRunnable(FString InHostname, int32 InPort)
    : Hostname(InHostname), Port(InPort), bRun(true), bShouldReconnect(true)
{
    Thread = FRunnableThread::Create(this, TEXT("TCPClientThread"), 0, TPri_Normal);
}

FArticyBridgeClientRunnable::~FArticyBridgeClientRunnable()
{
    bRun = false;
    bShouldReconnect = false;

    CloseSocket_NoLock();

    if (Thread)
    {
        Thread->Kill(true);
    }
}

bool FArticyBridgeClientRunnable::Init() { return true; }

uint32 FArticyBridgeClientRunnable::Run()
{
    UE_LOG(LogTemp, Log, TEXT("Bridge Client Runnable State - bRun: %s, bShouldReconnect: %s"),
        bRun ? TEXT("true") : TEXT("false"),
        bShouldReconnect ? TEXT("true") : TEXT("false"));

    while (bRun)
    {
        if (bReconnectRequested.Load())
        {
            {
                FScopeLock Lock(&TargetMutex);
                CloseSocket_NoLock();           // drop existing connection
                bReconnectRequested = false;    // clear flag; Hostname/Port already updated
            }
            // loop around; next iteration will call Connect() with new target
        }

        if (!Socket.IsValid())
        {
            Connect();
        }
        else
        {
            uint32 DataSize = 0;
            while (Socket->HasPendingData(DataSize))
            {
                TArray<uint8> ReceivedData;
                ReceivedData.SetNumUninitialized(DataSize);
                int32 BytesRead = 0;
                if (Socket->Recv(ReceivedData.GetData(), ReceivedData.Num(), BytesRead))
                {
                    ParseReceivedData(ReceivedData);
                }
            }
            FPlatformProcess::Sleep(0.1f);
        }
    }
    return 0;
}

void SetPropertyValue(FString Property, FArticyTexts & ArticyText, const FString & Value)
{
    FProperty* FoundProperty = FindFProperty<FStrProperty>(FArticyTexts::StaticStruct(), *Property);
    if (FoundProperty)
    {
        void* PropertyPtr = FoundProperty->ContainerPtrToValuePtr<void>(&ArticyText);
        if (PropertyPtr)
        {
            static_cast<FStrProperty*>(FoundProperty)->SetPropertyValue(PropertyPtr, Value);
        }
    }
}

void FArticyBridgeClientRunnable::UpdateAssets(UArticyImportData * ImportData)
{
    ImportData->BuildCachedVersion();
    CodeGenerator::GenerateAssets(ImportData);
    ImportData->PostImport();
}

void FArticyBridgeClientRunnable::UpdateAssetsAndCode(UArticyImportData * ImportData)
{
    const bool bAnyCodeGenerated = CodeGenerator::GenerateCode(ImportData);

    if (bAnyCodeGenerated)
    {
        static FDelegateHandle PostImportHandle;

        if (PostImportHandle.IsValid())
        {
            FArticyEditorModule::Get().OnCompilationFinished.Remove(PostImportHandle);
            PostImportHandle.Reset();
        }

        // Avoid dangling pointer if the object gets GC'd before the callback fires
        TWeakObjectPtr<UArticyImportData> WeakImportData = ImportData;

        PostImportHandle = FArticyEditorModule::Get().OnCompilationFinished.AddLambda(
            [WeakImportData](UArticyImportData* FinishedImportData)
            {
                // Prefer the still-valid captured object; otherwise fall back to the parameter
                UArticyImportData* Data = WeakImportData.IsValid() ? WeakImportData.Get() : FinishedImportData;
                if (!Data) return;

                Data->BuildCachedVersion();
                CodeGenerator::GenerateAssets(Data);
                Data->PostImport();
            });

        CodeGenerator::Recompile(ImportData);
    }
}

FString FArticyBridgeClientRunnable::MakeCppIdentifier(const FString& In)
{
    FString Out;
    Out.Reserve(In.Len());

    for (TCHAR C : In)
    {
        if (FChar::IsAlnum(C) || C == TEXT('_'))
        {
            Out.AppendChar(C);
        }
        else
        {
            Out.AppendChar(TEXT('_'));
        }
    }

    // Must not start with a digit
    if (Out.IsEmpty() || FChar::IsDigit(Out[0]))
    {
        Out = TEXT("_") + Out;
    }

    return Out;
}

bool FArticyBridgeClientRunnable::ApplyBridgeTypeAndDefault(FArticyGVar& Var, const FString& DataType, const TSharedPtr<FJsonObject>& Msg)
{
    Var.Type = EArticyType::ADT_String;
    Var.BoolValue = false;
    Var.IntValue = 0;
    Var.StringValue.Reset();

    // Type
    if (DataType.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase))
    {
        Var.Type = EArticyType::ADT_Boolean;

        bool B = false;
        if (Msg->TryGetBoolField(TEXT("DefaultValue"), B))
        {
            Var.BoolValue = B;
        }
        return true;
    }

    if (DataType.Equals(TEXT("Integer"), ESearchCase::IgnoreCase))
    {
        Var.Type = EArticyType::ADT_Integer;

        double Num = 0.0;
        if (Msg->TryGetNumberField(TEXT("DefaultValue"), Num))
        {
            Var.IntValue = (int32)Num;
            return true;
        }

        FString S;
        if (Msg->TryGetStringField(TEXT("DefaultValue"), S))
        {
            Var.IntValue = FCString::Atoi(*S);
            return true;
        }

        return true;
    }

    // Treat anything else as string
    Var.Type = EArticyType::ADT_String;

    FString Str;
    if (Msg->TryGetStringField(TEXT("DefaultValue"), Str))
    {
        Var.StringValue = Str;
        return true;
    }

    // If DefaultValue is not a string, coerce from raw JSON Value
    const TSharedPtr<FJsonValue>* Raw = Msg->Values.Find(TEXT("DefaultValue"));
    if (Raw && Raw->IsValid())
    {
        if ((*Raw)->Type == EJson::Boolean) Var.StringValue = (*Raw)->AsBool() ? TEXT("true") : TEXT("false");
        else if ((*Raw)->Type == EJson::Number) Var.StringValue = FString::SanitizeFloat((*Raw)->AsNumber());
        else Var.StringValue = TEXT("");
    }

    return true;
}

EArticyType FArticyBridgeClientRunnable::BridgeTypeToArticyType(const FString& In)
{
    if (In.Equals(TEXT("Boolean"), ESearchCase::IgnoreCase)) return EArticyType::ADT_Boolean;
    if (In.Equals(TEXT("Integer"), ESearchCase::IgnoreCase)) return EArticyType::ADT_Integer;
    if (In.Equals(TEXT("String"), ESearchCase::IgnoreCase)) return EArticyType::ADT_String;
    if (In.Equals(TEXT("MultiLanguageString"), ESearchCase::IgnoreCase)) return EArticyType::ADT_MultiLanguageString;
    return EArticyType::ADT_String;
}

FString FArticyBridgeClientRunnable::MakeCppSafeIdentifier(const FString& In, const TCHAR* Fallback)
{
    FString Out;
    Out.Reserve(In.Len());

    for (int32 i = 0; i < In.Len(); ++i)
    {
        const TCHAR C = In[i];
        const bool bAlphaNum = FChar::IsAlnum(C);
        Out.AppendChar(bAlphaNum ? C : TEXT('_'));
    }

    if (Out.IsEmpty())
        Out = Fallback;

    // C++ identifier must not start with a digit
    if (Out.Len() > 0 && FChar::IsDigit(Out[0]))
        Out = FString(TEXT("_")) + Out;

    return Out;
}

FArticyGVNamespace* FArticyBridgeClientRunnable::FindGVNamespace(FArticyGVInfo& GV, const FString& VariableSet)
{
    for (FArticyGVNamespace& Ns : GV.Namespaces)
    {
        if (Ns.Namespace.Equals(VariableSet, ESearchCase::CaseSensitive) ||
            Ns.Namespace.Equals(VariableSet, ESearchCase::IgnoreCase))
        {
            return &Ns;
        }
    }
    return nullptr;
}

int32 FArticyBridgeClientRunnable::FindGVVarIndex(const FArticyGVNamespace& Ns, const FString& VarName)
{
    for (int32 i = 0; i < Ns.Variables.Num(); ++i)
    {
        if (Ns.Variables[i].Variable.Equals(VarName, ESearchCase::IgnoreCase))
            return i;
    }
    return INDEX_NONE;
}

void FArticyBridgeClientRunnable::ProcessMessage(const FString & MessageType, const TSharedPtr<FJsonObject> Message, UArticyImportData * ImportData)
{
    if (!ImportData)
    {
        UE_LOG(LogTemp, Error, TEXT("No import cache. Need to fix this."));
        return;
    }

    auto& Packages = ImportData->GetPackages();
    auto& PackageDefs = ImportData->GetPackageDefs();
    auto& Objects = ImportData->GetObjectDefs();
    auto& Types = Objects.GetTypes();
    auto& Languages = ImportData->Languages.Languages;

    FString Id = TEXT("");
    FName TechnicalName = TEXT("");
    FString PackageName = TEXT("");
    TSoftObjectPtr<UArticyPackage> CurrentPackage = nullptr;
    FArticyPackageDef CurrentPackageDef;

    UArticyObject* ObjectRef = nullptr;
    if (Message->TryGetStringField(TEXT("Id"), Id))
    {
        for (const TSoftObjectPtr<UArticyPackage> Package : Packages)
        {
            UArticyObject* PackageAsset = Package->GetAssetById(Id);
            if (PackageAsset)
            {
                ObjectRef = PackageAsset;
                TechnicalName = PackageAsset->GetTechnicalName();
                PackageName = Package->Name;
                CurrentPackage = nullptr;
                for (FArticyPackageDef& PackageDef : PackageDefs.GetPackages())
                {
                    if (PackageDef.GetName().Replace(TEXT(" "), TEXT("_")).Equals(PackageName.Replace(TEXT(" "), TEXT("_"))))
                    {
                        CurrentPackageDef = PackageDef;
                        break;
                    }
                }
                break;
            }
        }
    }

    FString Property;
    Message->TryGetStringField(TEXT("Property"), Property);
    FString Value;
    Message->TryGetStringField(TEXT("Value"), Value);

    if (MessageType.Equals(TEXT("CreatedFirstClassObject")))
    {
        // 1) Parse basic fields
        FString CreatedType;
        const TSharedPtr<FJsonObject>* PropsPtr = nullptr;
        const TArray<TSharedPtr<FJsonValue>>* JsonPkgIds = nullptr;

        if (!Message->TryGetStringField(TEXT("Type"), CreatedType) || CreatedType.IsEmpty())
        {
            UE_LOG(LogTemp, Error, TEXT("CreatedFirstClassObject: Missing 'Type'."));
            return;
        }
        if (!Message->TryGetObjectField(TEXT("Properties"), PropsPtr) || !PropsPtr || !PropsPtr->IsValid())
        {
            UE_LOG(LogTemp, Error, TEXT("CreatedFirstClassObject: Missing 'Properties'."));
            return;
        }
        const TSharedPtr<FJsonObject>& Props = *PropsPtr;

        FString NewId;             Props->TryGetStringField(TEXT("Id"), NewId);
        FString NewTechnicalName;  Props->TryGetStringField(TEXT("TechnicalName"), NewTechnicalName);

        // 2) Resolve the target package definition (prefer the one listed in the message)
        //    Message "Packages" is an array of numeric ids (as strings).
        Message->TryGetArrayField(TEXT("Packages"), JsonPkgIds);

        FArticyPackageDef* TargetPkgDef = nullptr;
        FString TargetCsvBase;

        auto Norm = [](const FString& S) { return S.Replace(TEXT(" "), TEXT("_")); };

        // (a) Try by numeric id from the message
        if (JsonPkgIds && JsonPkgIds->Num() > 0)
        {
            // Usually one entry; we use the first
            const FString DeclaredIdStr = (*JsonPkgIds)[0]->AsString();
            for (FArticyPackageDef& PkgDef : PackageDefs.GetPackages())
            {
                const FString PkgIdStr = FString::Printf(TEXT("%llu"), (unsigned long long)PkgDef.GetId());
                if (PkgIdStr == DeclaredIdStr)
                {
                    TargetPkgDef = &PkgDef;
                    TargetCsvBase = Norm(PkgDef.GetName());
                    break;
                }
            }
        }

        // (b) Fall back to the package you discovered earlier in the outer code
        if (!TargetPkgDef)
        {
            for (FArticyPackageDef& PkgDef : PackageDefs.GetPackages())
            {
                if (Norm(PkgDef.GetName()).Equals(Norm(PackageName)))
                {
                    TargetPkgDef = &PkgDef;
                    TargetCsvBase = Norm(PkgDef.GetName());
                    break;
                }
            }
        }

        // (c) Last resort: use the first package def
        if (!TargetPkgDef)
        {
            auto& All = PackageDefs.GetPackages();
            if (All.Num() == 0)
            {
                UE_LOG(LogTemp, Error, TEXT("CreatedFirstClassObject: No package defs available."));
                return;
            }
            TargetPkgDef = &All[0];
            TargetCsvBase = Norm(TargetPkgDef->GetName());
            UE_LOG(LogTemp, Warning, TEXT("CreatedFirstClassObject: Falling back to first package '%s'."), *TargetPkgDef->GetName());
        }

        // 3) Prepare text map + helpers
        TMap<FString, FArticyTexts> UpdatedTexts = TargetPkgDef->GetTexts();
        TSet<FString> LanguagesToRebuild;

        auto UpdateFromLocalizableField = [&](const TCHAR* FieldName)
            {
                const TSharedPtr<FJsonObject>* LocObjPtr = nullptr;
                if (!Props->TryGetObjectField(FieldName, LocObjPtr) || !LocObjPtr || !LocObjPtr->IsValid())
                {
                    return; // field absent or not localizable
                }

                const TSharedPtr<FJsonObject>& LocObj = *LocObjPtr;
                FString LId;
                if (!LocObj->TryGetStringField(TEXT("LId"), LId) || LId.IsEmpty())
                {
                    return; // no key to write
                }

                const TSharedPtr<FJsonObject>* ValuesObjPtr = nullptr;
                if (!LocObj->TryGetObjectField(TEXT("Values"), ValuesObjPtr) || !ValuesObjPtr || !ValuesObjPtr->IsValid())
                {
                    return; // no values (allowed—fresh objects often start empty)
                }

                FArticyTexts& Entry = UpdatedTexts.FindOrAdd(LId);

                for (const auto& KV : (*ValuesObjPtr)->Values)
                {
                    const FString LangId = KV.Key;
                    if (LangId.IsEmpty()) continue;

                    FString TextValue;
                    if (KV.Value.IsValid())
                    {
                        if (KV.Value->Type == EJson::String)
                        {
                            TextValue = KV.Value->AsString();
                        }
                        else if (!KV.Value->TryGetString(TextValue))
                        {
                            double Num; bool b;
                            if (KV.Value->TryGetNumber(Num)) TextValue = FString::SanitizeFloat(Num);
                            else if (KV.Value->TryGetBool(b)) TextValue = b ? TEXT("true") : TEXT("false");
                            else TextValue = TEXT("");
                        }
                    }

                    FArticyTextDef& LangEntry = Entry.Content.FindOrAdd(LangId);
                    LangEntry.Text = TextValue;
                    LanguagesToRebuild.Add(LangId);
                }
            };

        // 4) Apply for known localizable fields present in Articy fragments
        UpdateFromLocalizableField(TEXT("Text"));
        UpdateFromLocalizableField(TEXT("MenuText"));
        UpdateFromLocalizableField(TEXT("DisplayName"));

        // 5) Rebuild string tables for just the languages we touched
        for (const FString& LangId : LanguagesToRebuild)
        {
            const FArticyLanguageDef* LangDefPtr = Languages.Find(LangId);
            if (!LangDefPtr)
            {
                UE_LOG(LogTemp, Warning, TEXT("CreatedFirstClassObject: Unknown language '%s' (skipping CSV)."), *LangId);
                continue;
            }
            const FArticyLanguageDef LangDef = *LangDefPtr;
            TPair<FString, FArticyLanguageDef> LangPair(LangId, LangDef);

            StringTableGenerator(TargetCsvBase, LangId,
                [&](StringTableGenerator* CsvOutput)
                {
                    return UArticyImportData::ProcessStrings(CsvOutput, UpdatedTexts, LangPair);
                });
        }

        // 7) Refresh the in-engine view of assets
        UpdateAssetsAndCode(ImportData);

        UE_LOG(LogTemp, Log, TEXT("CreatedFirstClassObject: Type=%s, Id=%s, TechnicalName=%s, PackageCsv=%s, TouchedLangs=%d"),
            *CreatedType, *NewId, *NewTechnicalName, *TargetCsvBase, LanguagesToRebuild.Num());

        return;
    }

    if (MessageType.Equals(TEXT("GlobalVariableCreated")))
    {
        FString VarSet, VarName, DataType, Desc;

        if (!Message->TryGetStringField(TEXT("VariableSet"), VarSet) || VarSet.IsEmpty() ||
            !Message->TryGetStringField(TEXT("Variable"), VarName) || VarName.IsEmpty() ||
            !Message->TryGetStringField(TEXT("DataType"), DataType) || DataType.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("GlobalVariableCreated: Missing VariableSet/Variable/DataType."));
            return;
        }

        Message->TryGetStringField(TEXT("Description"), Desc);

        FArticyGVInfo& GV = ImportData->GetGlobalVars();
        TArray<FArticyGVNamespace>& Namespaces = GV.Namespaces;

        // Find or create namespace
        FArticyGVNamespace* Ns = nullptr;
        for (FArticyGVNamespace& Existing : Namespaces)
        {
            if (Existing.Namespace.Equals(VarSet))
            {
                Ns = &Existing;
                break;
            }
        }

        if (!Ns)
        {
            const int32 Idx = Namespaces.AddDefaulted();
            Ns = &Namespaces[Idx];
            Ns->Namespace = VarSet;
            Ns->Description = TEXT("");
            Ns->CppTypename = MakeCppIdentifier(VarSet);
        }

        // Find or create variable
        FArticyGVar* Var = nullptr;
        for (FArticyGVar& ExistingVar : Ns->Variables)
        {
            if (ExistingVar.Variable.Equals(VarName))
            {
                Var = &ExistingVar;
                break;
            }
        }

        if (!Var)
        {
            const int32 VIdx = Ns->Variables.AddDefaulted();
            Var = &Ns->Variables[VIdx];
            Var->Variable = VarName;
        }

        Var->Description = Desc;
        ApplyBridgeTypeAndDefault(*Var, DataType, Message);

        ImportData->GetSettings().SetObjectDefinitionsNeedRebuild();
        UpdateAssetsAndCode(ImportData);

        UE_LOG(LogTemp, Log, TEXT("GlobalVariableCreated: %s.%s (%s)"), *VarSet, *VarName, *DataType);
        return;
    }

    if (MessageType.Equals(TEXT("GlobalVariableRenamed")))
    {
        FString VariableSet, OldVariable, NewVariable, DataType, Description;
        if (!Message->TryGetStringField(TEXT("VariableSet"), VariableSet) || VariableSet.IsEmpty())
            return;
        if (!Message->TryGetStringField(TEXT("OldVariable"), OldVariable) || OldVariable.IsEmpty())
            return;
        if (!Message->TryGetStringField(TEXT("Variable"), NewVariable) || NewVariable.IsEmpty())
            return;

        Message->TryGetStringField(TEXT("DataType"), DataType);
        Message->TryGetStringField(TEXT("Description"), Description);

        // DefaultValue can be bool/int/string depending on DataType
        const TSharedPtr<FJsonValue>* DefaultValuePtr = Message->Values.Find(TEXT("DefaultValue"));

        FArticyGVInfo& GV = ImportData->GetGlobalVars();
        FArticyGVNamespace* Ns = FindGVNamespace(GV, VariableSet);
        if (!Ns)
        {
            UE_LOG(LogTemp, Warning, TEXT("GlobalVariableRenamed: Namespace '%s' not found."), *VariableSet);
            return;
        }

        const int32 OldIdx = FindGVVarIndex(*Ns, OldVariable);
        if (OldIdx == INDEX_NONE)
        {
            UE_LOG(LogTemp, Warning, TEXT("GlobalVariableRenamed: Variable '%s' not found in '%s'."),
                *OldVariable, *VariableSet);
            return;
        }

        // Rename
        FArticyGVar& Var = Ns->Variables[OldIdx];
        Var.Variable = NewVariable;
        Var.Description = Description;

        if (!DataType.IsEmpty())
            Var.Type = BridgeTypeToArticyType(DataType);

        if (DefaultValuePtr && DefaultValuePtr->IsValid())
        {
            const TSharedPtr<FJsonValue>& DV = *DefaultValuePtr;

            // Reset stored defaults first
            Var.BoolValue = false;
            Var.IntValue = 0;
            Var.StringValue.Empty();

            switch (Var.Type)
            {
            case EArticyType::ADT_Boolean:
                Var.BoolValue = (DV->Type == EJson::Boolean) ? DV->AsBool() : false;
                break;

            case EArticyType::ADT_Integer:
                if (DV->Type == EJson::Number) Var.IntValue = (int)DV->AsNumber();
                else if (DV->Type == EJson::String) LexFromString(Var.IntValue, *DV->AsString());
                break;

            case EArticyType::ADT_String:
            case EArticyType::ADT_MultiLanguageString:
                if (DV->Type == EJson::String) Var.StringValue = DV->AsString();
                else if (DV->Type == EJson::Number) Var.StringValue = FString::SanitizeFloat(DV->AsNumber());
                else if (DV->Type == EJson::Boolean) Var.StringValue = DV->AsBool() ? TEXT("true") : TEXT("false");
                break;
            }
        }

        if (Ns->CppTypename.IsEmpty())
        {
            Ns->CppTypename = MakeCppSafeIdentifier(Ns->Namespace, TEXT("GVNamespace"));
        }

        ImportData->GetSettings().SetObjectDefinitionsNeedRebuild();
        UpdateAssetsAndCode(ImportData);

        UE_LOG(LogTemp, Log, TEXT("GlobalVariableRenamed: %s.%s -> %s.%s"),
            *VariableSet, *OldVariable, *VariableSet, *NewVariable);
        return;
    }

    if (MessageType.Equals(TEXT("GlobalVariableDeleted")))
    {
        FString VariableSet, Variable;
        if (!Message->TryGetStringField(TEXT("VariableSet"), VariableSet) || VariableSet.IsEmpty())
            return;
        if (!Message->TryGetStringField(TEXT("Variable"), Variable) || Variable.IsEmpty())
            return;

        FArticyGVInfo& GV = ImportData->GetGlobalVars();
        FArticyGVNamespace* Ns = FindGVNamespace(GV, VariableSet);
        if (!Ns)
        {
            UE_LOG(LogTemp, Warning, TEXT("GlobalVariableDeleted: Namespace '%s' not found."), *VariableSet);
            return;
        }

        const int32 Before = Ns->Variables.Num();
        Ns->Variables.RemoveAll([&](const FArticyGVar& V)
            {
                return V.Variable.Equals(Variable, ESearchCase::IgnoreCase);
            });
        const int32 Removed = Before - Ns->Variables.Num();

        if (Removed == 0)
        {
            UE_LOG(LogTemp, Verbose, TEXT("GlobalVariableDeleted: %s.%s not found (no-op)."), *VariableSet, *Variable);
            return;
        }

        ImportData->GetSettings().SetObjectDefinitionsNeedRebuild();
        UpdateAssetsAndCode(ImportData);

        UE_LOG(LogTemp, Log, TEXT("GlobalVariableDeleted: Removed %d variable(s): %s.%s"), Removed, *VariableSet, *Variable);
        return;
    }

    if (MessageType.Equals(TEXT("CreatedConnection")))
    {
        // Payload example:
        // {"Source":"0x010000010000029E","Target":"0x0100000100000B92","SourcePin":"0x01000001000002A4","TargetPin":"0x0100000100000B97"}

        FString SourceNodeHex, TargetNodeHex, SourcePinHex, TargetPinHex;
        if (!Message->TryGetStringField(TEXT("Source"), SourceNodeHex) ||
            !Message->TryGetStringField(TEXT("Target"), TargetNodeHex) ||
            !Message->TryGetStringField(TEXT("SourcePin"), SourcePinHex) ||
            !Message->TryGetStringField(TEXT("TargetPin"), TargetPinHex))
        {
            UE_LOG(LogTemp, Error, TEXT("CreatedConnection: Missing one of Source/Target/SourcePin/TargetPin."));
            return;
        }

        FArticyId SourceNodeId = FArticyId(SourceNodeHex);
        FArticyId TargetNodeId = FArticyId(TargetNodeHex);
        FArticyId SourcePinId = FArticyId(SourcePinHex);
        FArticyId TargetPinId = FArticyId(TargetPinHex);

        // --- Locate the source node (owner of the output pin) ---
        UArticyObject* SourceNode = nullptr;
        for (const TSoftObjectPtr<UArticyPackage>& Pkg : Packages)
        {
            if (!Pkg.IsValid()) continue;
            if (UArticyObject* Obj = Pkg->GetAssetById(SourceNodeHex))
            {
                SourceNode = Obj;
                break;
            }
        }
        if (!SourceNode)
        {
            UE_LOG(LogTemp, Warning, TEXT("CreatedConnection: Source node %s not found."), *SourceNodeHex);
            UpdateAssetsAndCode(ImportData);
            return;
        }

        // --- Fetch the source output pin subobject ---
        UArticyOutputPin* SourcePin = Cast<UArticyOutputPin>(SourceNode->GetSubobject(SourcePinId));
        if (!SourcePin)
        {
            UE_LOG(LogTemp, Warning, TEXT("CreatedConnection: Source pin %s not found on node %s."), *SourcePinHex, *SourceNodeHex);
            UpdateAssetsAndCode(ImportData);
            return;
        }

        // Validate target side for sanity & logs
        UArticyObject* TargetNode = nullptr;
        for (const TSoftObjectPtr<UArticyPackage>& Pkg : Packages)
        {
            if (!Pkg.IsValid()) continue;
            if (UArticyObject* Obj = Pkg->GetAssetById(TargetNodeHex))
            {
                TargetNode = Obj;
                break;
            }
        }
        UArticyInputPin* TargetPin = nullptr;
        if (TargetNode)
        {
            TargetPin = Cast<UArticyInputPin>(TargetNode->GetSubobject(TargetPinId));
            if (!TargetPin)
            {
                UE_LOG(LogTemp, Verbose, TEXT("CreatedConnection: Target node found but target pin %s missing (node %s)."),
                    *TargetPinHex, *TargetNodeHex);
            }
        }
        else
        {
            UE_LOG(LogTemp, Verbose, TEXT("CreatedConnection: Target node %s not found (will still attach outgoing connection)."), *TargetNodeHex);
        }

        // --- Create and attach the outgoing connection on the source pin ---
        UArticyOutgoingConnection* NewConn = NewObject<UArticyOutgoingConnection>(SourcePin);
        if (!ensure(NewConn))
        {
            UE_LOG(LogTemp, Error, TEXT("CreatedConnection: Failed to allocate UArticyOutgoingConnection."));
            return;
        }

        // Fill connection endpoints
        NewConn->Target = TargetNodeId;
        NewConn->TargetPin = TargetPinId;

        SourcePin->Connections.Add(NewConn);

#if WITH_EDITOR
        SourcePin->Modify();
#endif

        // --- Refresh any derived/cached views so exploration sees the new edge ---
        UpdateAssetsAndCode(ImportData);

        UE_LOG(LogTemp, Log, TEXT("CreatedConnection: %s[%s] -> %s[%s] (connections on src now: %d)"),
            *SourceNodeHex, *SourcePinHex, *TargetNodeHex, *TargetPinHex, SourcePin->Connections.Num());

        return;
    }

    if (MessageType.Equals(TEXT("RemovedConnection")))
    {
        // Payload example:
        // { "Source":"0x010000010000029E", "Target":"0x0100000100000B92",
        //   "SourcePin":"0x01000001000002A4", "TargetPin":"0x0100000100000B97" }

        FString SourceNodeHex, TargetNodeHex, SourcePinHex, TargetPinHex;
        if (!Message->TryGetStringField(TEXT("Source"), SourceNodeHex) ||
            !Message->TryGetStringField(TEXT("Target"), TargetNodeHex) ||
            !Message->TryGetStringField(TEXT("SourcePin"), SourcePinHex) ||
            !Message->TryGetStringField(TEXT("TargetPin"), TargetPinHex))
        {
            UE_LOG(LogTemp, Error, TEXT("RemovedConnection: Missing one of Source/Target/SourcePin/TargetPin."));
            return;
        }

        FArticyId SourceNodeId = FArticyId(SourceNodeHex);
        FArticyId TargetNodeId = FArticyId(TargetNodeHex);
        FArticyId SourcePinId = FArticyId(SourcePinHex);
        FArticyId TargetPinId = FArticyId(TargetPinHex);

        // --- Locate the source node (owner of the output pin) ---
        UArticyObject* SourceNode = nullptr;
        for (const TSoftObjectPtr<UArticyPackage>& Pkg : Packages)
        {
            if (!Pkg.IsValid()) continue;
            if (UArticyObject* Obj = Pkg->GetAssetById(SourceNodeHex))
            {
                SourceNode = Obj;
                break;
            }
        }
        if (!SourceNode)
        {
            UE_LOG(LogTemp, Warning, TEXT("RemovedConnection: Source node %s not found."), *SourceNodeHex);
            UpdateAssetsAndCode(ImportData);
            return;
        }

        // --- Fetch the source output pin subobject ---
        UArticyOutputPin* SourcePin = Cast<UArticyOutputPin>(SourceNode->GetSubobject(SourcePinId));
        if (!SourcePin)
        {
            UE_LOG(LogTemp, Warning, TEXT("RemovedConnection: Source pin %s not found on node %s."), *SourcePinHex, *SourceNodeHex);
            UpdateAssetsAndCode(ImportData);
            return;
        }

        // --- Remove matching connections ---
        const int32 Before = SourcePin->Connections.Num();
        SourcePin->Connections.RemoveAll([&](UArticyOutgoingConnection* Conn)
            {
                return Conn &&
                    Conn->Target == TargetNodeId &&
                    Conn->TargetPin == TargetPinId;
            });
        const int32 Removed = Before - SourcePin->Connections.Num();

#if WITH_EDITOR
        if (Removed > 0) SourcePin->Modify();
#endif

        if (Removed == 0)
        {
            UE_LOG(LogTemp, Verbose, TEXT("RemovedConnection: No matching connection found from %s[%s] to %s[%s]."),
                *SourceNodeHex, *SourcePinHex, *TargetNodeHex, *TargetPinHex);
        }

        // --- Refresh derived/cached views so exploration reflects the removal ---
        UpdateAssetsAndCode(ImportData);

        UE_LOG(LogTemp, Log, TEXT("RemovedConnection: %s[%s] -X-> %s[%s] (removed %d, remaining on src: %d)"),
            *SourceNodeHex, *SourcePinHex, *TargetNodeHex, *TargetPinHex, Removed, SourcePin->Connections.Num());

        return;
    }

    if (MessageType.Equals(TEXT("ObjectsDeleted")))
    {
        const TArray<TSharedPtr<FJsonValue>>* ObjectsArrayPtr = nullptr;
        if (!Message->TryGetArrayField(TEXT("Objects"), ObjectsArrayPtr) || !ObjectsArrayPtr)
        {
            UpdateAssetsAndCode(ImportData);
            return;
        }

        // --- Collect deleted ids (raw hex + parsed) ---
        TSet<FString>          DeletedHexIds;
        TSet<FArticyId>        DeletedIds;

        for (const TSharedPtr<FJsonValue>& IdValue : *ObjectsArrayPtr)
        {
            const FString DeletedHex = IdValue->AsString();
            DeletedHexIds.Add(DeletedHex);
            FArticyId DeletedId = FArticyId(DeletedHex);
            DeletedIds.Add(DeletedId);
        }

        // --- Helpers to fetch InputPins/OutputPins from a node via reflection ---
        auto GetInputPinsPtrFrom = [](UArticyObject* Obj) -> const TArray<UArticyInputPin*>*
            {
                if (!Obj) return nullptr;
                if (IArticyReflectable* Refl = Cast<IArticyReflectable>(Obj))
                {
                    static const FName NameInputPins(TEXT("InputPins"));
                    return Refl->GetPropPtr<TArray<UArticyInputPin*>>(NameInputPins);
                }
                return nullptr;
            };
        auto GetOutputPinsPtrFrom = [](UArticyObject* Obj) -> const TArray<UArticyOutputPin*>*
            {
                if (!Obj) return nullptr;
                if (IArticyReflectable* Refl = Cast<IArticyReflectable>(Obj))
                {
                    static const FName NameOutputPins(TEXT("OutputPins"));
                    return Refl->GetPropPtr<TArray<UArticyOutputPin*>>(NameOutputPins);
                }
                return nullptr;
            };

        // --- Resolve which deleted ids are nodes/pins and collect impacted pin ids ---
        TSet<FArticyId> DeletedNodeIds;
        TSet<FArticyId> DeletedInputPinIds;
        TSet<FArticyId> DeletedOutputPinIds;

        // Try to resolve objects BEFORE we remove them from packages
        auto ResolveObjectByHex = [&](const FString& Hex) -> UArticyObject*
            {
                for (const TSoftObjectPtr<UArticyPackage>& Package : Packages)
                {
                    if (!Package.IsValid()) continue;
                    if (UArticyObject* Obj = Package->GetAssetById(Hex))
                        return Obj;
                }
                return nullptr;
            };

        for (const FString& Hex : DeletedHexIds)
        {
            UArticyObject* Obj = ResolveObjectByHex(Hex);
            if (!Obj)
                continue;

            // Check whether this is a pin (input/output) or an owning node
            if (UArticyFlowPin* AnyPin = Cast<UArticyFlowPin>(Obj))
            {
                // We don't expect pins as "assets" typically, but handle it anyway
                if (UArticyInputPin* In = Cast<UArticyInputPin>(AnyPin))
                {
                    DeletedInputPinIds.Add(In->GetId());
                }
                else if (UArticyOutputPin* Out = Cast<UArticyOutputPin>(AnyPin))
                {
                    DeletedOutputPinIds.Add(Out->GetId());
                }
            }
            else
            {
                // Treat as "node": collect its input pins (incoming edges point here)
                DeletedNodeIds.Add(Obj->GetId());

                if (const TArray<UArticyInputPin*>* InPins = GetInputPinsPtrFrom(Obj))
                {
                    for (UArticyInputPin* Pin : *InPins)
                    {
                        if (Pin) DeletedInputPinIds.Add(Pin->GetId());
                    }
                }

                // For completeness, also collect its output pins, so we can clear them too
                if (const TArray<UArticyOutputPin*>* OutPins = GetOutputPinsPtrFrom(Obj))
                {
                    for (UArticyOutputPin* Pin : *OutPins)
                    {
                        if (Pin) DeletedOutputPinIds.Add(Pin->GetId());
                    }
                }
            }
        }

        // --- 1) Clear outgoing connections on any deleted OUTPUT pins themselves ---
        for (TObjectIterator<UArticyOutputPin> It; It; ++It)
        {
            UArticyOutputPin* OutPin = *It;
            if (!IsValid(OutPin)) continue;

            if (DeletedOutputPinIds.Contains(OutPin->GetId()))
            {
                OutPin->Connections.Reset();
#if WITH_EDITOR
                OutPin->Modify();
#endif
            }
        }

        // --- 2) Remove incoming edges that target any deleted NODE or INPUT PIN ---
        // Source pins (outgoing lists) are authoritative, so scan all of them:
        for (TObjectIterator<UArticyOutputPin> It; It; ++It)
        {
            UArticyOutputPin* OutPin = *It;
            if (!IsValid(OutPin)) continue;

            const int32 Before = OutPin->Connections.Num();
            OutPin->Connections.RemoveAll([&](UArticyOutgoingConnection* Conn)
                {
                    if (!Conn) return true; // prune nulls defensively
                    const bool bTargetNodeGone = DeletedIds.Contains(Conn->Target) || DeletedNodeIds.Contains(Conn->Target);
                    const bool bTargetPinGone = DeletedIds.Contains(Conn->TargetPin) || DeletedInputPinIds.Contains(Conn->TargetPin);
                    return bTargetNodeGone || bTargetPinGone;
                });
            if (OutPin->Connections.Num() != Before)
            {
#if WITH_EDITOR
                OutPin->Modify();
#endif
            }
        }

        // --- 3) Now remove the assets themselves from packages ---
        for (const FString& DeletedHex : DeletedHexIds)
        {
            for (const TSoftObjectPtr<UArticyPackage>& Package : Packages)
            {
                if (!Package.IsValid()) continue;
                Package->RemoveAssetById(DeletedHex);
                // Do NOT 'break' here — same id could exist across multiple loaded packages in rare scenarios.
            }
        }

        // --- 4) Rebuild code/assets (keeps DB caches & string tables consistent) ---
        UpdateAssetsAndCode(ImportData);

        UE_LOG(LogTemp, Log, TEXT("ObjectsDeleted: %d ids. Removed incoming edges to %d nodes / %d input pins, cleared %d output pins."),
            DeletedHexIds.Num(), DeletedNodeIds.Num(), DeletedInputPinIds.Num(), DeletedOutputPinIds.Num());

        return;
    }

    if (MessageType.Equals(TEXT("ChangedBasicProperty")))
    {
        // ---- 1) Parse Id / Property / generic Value (as raw JSON) ----
        FString TargetId;
        FString PropName;

        if (!Message->TryGetStringField(TEXT("Id"), TargetId) || TargetId.IsEmpty())
        {
            UE_LOG(LogTemp, Error, TEXT("ChangedBasicProperty: Missing 'Id'."));
            return;
        }
        if (!Message->TryGetStringField(TEXT("Property"), PropName) || PropName.IsEmpty())
        {
            UE_LOG(LogTemp, Error, TEXT("ChangedBasicProperty: Missing 'Property'."));
            return;
        }

        // Pull the raw 'Value' as a generic JSON value so we can coerce based on FProperty type
        const TSharedPtr<FJsonValue>* RawValuePtr = Message->Values.Find(TEXT("Value"));
        if (!RawValuePtr || !RawValuePtr->IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("ChangedBasicProperty: 'Value' missing for Id=%s, Property=%s"), *TargetId, *PropName);
            return;
        }
        const TSharedPtr<FJsonValue>& RawValue = *RawValuePtr;

        // ---- 2) Locate target object from your packages ----
        UArticyObject* TargetObj = nullptr;
        for (const TSoftObjectPtr<UArticyPackage>& Pkg : Packages)
        {
            if (!Pkg.IsValid()) continue;
            if (UArticyObject* Candidate = Pkg->GetAssetById(TargetId))
            {
                TargetObj = Candidate;
                break;
            }
        }
        if (!TargetObj)
        {
            UE_LOG(LogTemp, Warning, TEXT("ChangedBasicProperty: Object Id=%s not found in any loaded package."), *TargetId);
            return;
        }

        // ---- 3) Find destination property by name on the object class ----
        FProperty* DestProp = TargetObj->GetClass()->FindPropertyByName(FName(*PropName));
        if (!DestProp)
        {
            UE_LOG(LogTemp, Warning, TEXT("ChangedBasicProperty: Property '%s' not found on class %s (Id=%s)."),
                *PropName, *TargetObj->GetClass()->GetName(), *TargetId);
            return;
        }

        // ---- 4) Helpers to coerce JSON -> native type and assign ----
        auto SetStringLike = [&](FProperty* P, void* Addr, const FString& S)
            {
                if (FStrProperty* SP = CastField<FStrProperty>(P))
                {
                    SP->SetPropertyValue(Addr, S);
                    return true;
                }
                if (FNameProperty* NP = CastField<FNameProperty>(P))
                {
                    NP->SetPropertyValue(Addr, FName(*S));
                    return true;
                }
                if (FTextProperty* TP = CastField<FTextProperty>(P))
                {
                    TP->SetPropertyValue(Addr, FText::FromString(S));
                    return true;
                }
                return false;
            };

        auto SetBoolLike = [&](FProperty* P, void* Addr, bool bVal)
            {
                if (FBoolProperty* BP = CastField<FBoolProperty>(P))
                {
                    BP->SetPropertyValue(Addr, bVal);
                    return true;
                }
                return false;
            };

        auto SetNumericLike = [&](FProperty* P, void* Addr, const TSharedPtr<FJsonValue>& Jv)
            {
                if (FNumericProperty* NP = CastField<FNumericProperty>(P))
                {
                    // Try number first; if string, try to parse
                    if (Jv->Type == EJson::Number)
                    {
                        if (NP->IsInteger())
                        {
                            int64 I = (int64)Jv->AsNumber();
                            NP->SetIntPropertyValue(Addr, I);
                        }
                        else
                        {
                            double D = Jv->AsNumber();
                            NP->SetFloatingPointPropertyValue(Addr, D);
                        }
                        return true;
                    }
                    else if (Jv->Type == EJson::String)
                    {
                        const FString S = Jv->AsString();
                        if (NP->IsInteger())
                        {
                            int64 I = 0;
                            LexFromString(I, *S);
                            NP->SetIntPropertyValue(Addr, I);
                        }
                        else
                        {
                            double D = 0.0;
                            LexFromString(D, *S);
                            NP->SetFloatingPointPropertyValue(Addr, D);
                        }
                        return true;
                    }
                }
                return false;
            };

        auto TrySetStruct = [&](FStructProperty* SP, void* Addr, const TSharedPtr<FJsonValue>& Jv)
            {
                if (Jv->Type != EJson::Object) return false;
                const TSharedPtr<FJsonObject> Obj = Jv->AsObject();
                if (!Obj.IsValid()) return false;

                UScriptStruct* SS = SP->Struct;
                if (!SS) return false;

                // FLinearColor {a,r,g,b}
                if (SS == TBaseStructure<FLinearColor>::Get())
                {
                    FLinearColor Color;
                    Obj->TryGetNumberField(TEXT("a"), Color.A);
                    Obj->TryGetNumberField(TEXT("r"), Color.R);
                    Obj->TryGetNumberField(TEXT("g"), Color.G);
                    Obj->TryGetNumberField(TEXT("b"), Color.B);
                    *reinterpret_cast<FLinearColor*>(Addr) = Color;
                    return true;
                }

                // FVector2D {x,y}  (useful for Position / Size if mapped)
                if (SS == TBaseStructure<FVector2D>::Get())
                {
                    double X = 0, Y = 0;
                    Obj->TryGetNumberField(TEXT("x"), X);
                    Obj->TryGetNumberField(TEXT("y"), Y);
                    *reinterpret_cast<FVector2D*>(Addr) = FVector2D((float)X, (float)Y);
                    return true;
                }

                // TODO: Handle other structs

                return false;
            };

        auto AssignJsonToUProperty = [&](UObject* Obj, FProperty* P, const TSharedPtr<FJsonValue>& Jv) -> bool
            {
                void* ValueAddr = P->ContainerPtrToValuePtr<void>(Obj);

                // 1) Strings / Names / Text
                if (Jv->Type == EJson::String && SetStringLike(P, ValueAddr, Jv->AsString()))
                    return true;

                // 2) Numbers (int/float/double)
                if (Jv->Type == EJson::Number && SetNumericLike(P, ValueAddr, Jv))
                    return true;

                // 3) Bool
                if (Jv->Type == EJson::Boolean && SetBoolLike(P, ValueAddr, Jv->AsBool()))
                    return true;

                // 4) Structs we know how to parse (Color/Position/Size etc.)
                if (FStructProperty* SP = CastField<FStructProperty>(P))
                {
                    if (TrySetStruct(SP, ValueAddr, Jv))
                        return true;
                }

                // 5) Name/Text from non-string JSON (coerce)
                if (FStrProperty* StrP = CastField<FStrProperty>(P))
                {
                    // Coerce anything to string for last resort
                    FString S;
                    if (Jv->Type == EJson::Number) { S = FString::SanitizeFloat(Jv->AsNumber()); }
                    else if (Jv->Type == EJson::Boolean) { S = Jv->AsBool() ? TEXT("true") : TEXT("false"); }
                    else if (Jv->Type == EJson::Object) { S = TEXT("<object>"); }
                    else if (Jv->Type == EJson::Array) { S = TEXT("<array>"); }
                    else                                 S = TEXT("");
                    StrP->SetPropertyValue(ValueAddr, S);
                    return true;
                }

                UE_LOG(LogTemp, Warning, TEXT("ChangedBasicProperty: Unsupported assignment for property '%s' on %s (JSON type=%d)."),
                    *P->GetName(), *Obj->GetClass()->GetName(), (int)Jv->Type);
                return false;
            };

        // ---- 5) Apply the change ----
        if (!AssignJsonToUProperty(TargetObj, DestProp, RawValue))
        {
            UE_LOG(LogTemp, Warning, TEXT("ChangedBasicProperty: Failed to assign Value to '%s' (Id=%s)."),
                *PropName, *TargetId);
            return;
        }

#if WITH_EDITOR
        // Notify editor systems if you’re in editor; otherwise omit
        TargetObj->Modify();
        FPropertyChangedEvent Ev(DestProp, EPropertyChangeType::ValueSet);
        TargetObj->PostEditChangeProperty(Ev);
#endif

        TargetObj->MarkPackageDirty();

        // If property changes affect graph/layout etc., refresh assets
        UpdateAssetsAndCode(ImportData);

        UE_LOG(LogTemp, Log, TEXT("ChangedBasicProperty: Set %s.%s for Id=%s"), *TargetObj->GetClass()->GetName(), *PropName, *TargetId);
        return;
    }

    if (MessageType.Equals(TEXT("ChangedLocalizableText")))
    {
        // Value is an object: { "LId": "...", "Values": { "en": "..." , ... } }
        const TSharedPtr<FJsonObject>* ValueObjPtr = nullptr;
        if (!Message->TryGetObjectField(TEXT("Value"), ValueObjPtr) || !ValueObjPtr || !ValueObjPtr->IsValid())
        {
            UE_LOG(LogTemp, Error, TEXT("ChangedLocalizableText: 'Value' is missing or not an object."));
            return;
        }
        const TSharedPtr<FJsonObject>& ValueObj = *ValueObjPtr;

        // LId is the key for the text entry (e.g. "DFr_Awake.Text")
        FString LId;
        if (!ValueObj->TryGetStringField(TEXT("LId"), LId) || LId.IsEmpty())
        {
            UE_LOG(LogTemp, Error, TEXT("ChangedLocalizableText: 'LId' missing in 'Value'."));
            return;
        }

        // Values is an object whose keys are language codes (en, de, fr, ...)
        const TSharedPtr<FJsonObject>* ValuesObjPtr = nullptr;
        if (!ValueObj->TryGetObjectField(TEXT("Values"), ValuesObjPtr) || !ValuesObjPtr || !ValuesObjPtr->IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("ChangedLocalizableText: 'Values' missing; nothing to update."));
            return;
        }
        const TSharedPtr<FJsonObject>& ValuesObj = *ValuesObjPtr;

        // Prepare current text map
        TMap<FString, FArticyTexts> UpdatedTexts = CurrentPackageDef.GetTexts();

        // Ensure the key exists
        FArticyTexts& TextsEntry = UpdatedTexts.FindOrAdd(LId);

        // Update all provided languages
        // ValuesObj->Values: TMap<FString, TSharedPtr<FJsonValue>>
        for (const auto& Pair : ValuesObj->Values)
        {
            const FString LangId = Pair.Key; // e.g. "en"
            FString NewText;

            // Extract the string safely
            if (Pair.Value.IsValid())
            {
                if (Pair.Value->Type == EJson::String)
                {
                    NewText = Pair.Value->AsString();
                }
                else
                {
                    // Coerce to string just in case
                    TSharedPtr<FJsonObject> AsObj;
                    double AsNumber;
                    bool AsBool;
                    if (Pair.Value->TryGetString(NewText))
                    {
                        // ok
                    }
                    else if (Pair.Value->TryGetNumber(AsNumber))
                    {
                        NewText = FString::SanitizeFloat(AsNumber);
                    }
                    else if (Pair.Value->TryGetBool(AsBool))
                    {
                        NewText = AsBool ? TEXT("true") : TEXT("false");
                    }
                }
            }

            // Update the language content if we have a language code
            if (!LangId.IsEmpty())
            {
                FArticyTextDef& LangEntry = TextsEntry.Content.FindOrAdd(LangId);
                LangEntry.Text = NewText;

                // Generate/update the string table for this language immediately
                const FString StringTableFileName = PackageName.Replace(TEXT(" "), TEXT("_"));

                const FArticyLanguageDef* LangDefPtr = Languages.Find(LangId);
                if (!LangDefPtr)
                {
                    UE_LOG(LogTemp, Warning, TEXT("ChangedLocalizableText: Unknown language '%s' for LId '%s'."), *LangId, *LId);
                    continue;
                }

                const FArticyLanguageDef LangDef = *LangDefPtr;
                TPair<FString, FArticyLanguageDef> LangPair(LangId, LangDef);

                StringTableGenerator(StringTableFileName, LangId,
                    [&](StringTableGenerator* CsvOutput)
                    {
                        // Note: We pass the whole UpdatedTexts map so the CSV can be rebuilt consistently.
                        return UArticyImportData::ProcessStrings(CsvOutput, UpdatedTexts, LangPair);
                    });
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("ChangedLocalizableText: Empty language id for LId '%s'."), *LId);
            }
        }

        // Finally reload localizer
        UArticyLocalizerSystem::Get()->Reload();
        return;
    }
}

void FArticyBridgeClientRunnable::ParseReceivedData(const TArray<uint8>&Data)
{
    DataBuffer.Empty();
    DataBuffer.Append(Data);

    FString HeaderCheck = FString(ANSI_TO_TCHAR(reinterpret_cast<const char*>(DataBuffer.GetData()))).Left(4);
    if (HeaderCheck != "RTCB")
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid message header."));
        DataBuffer.Empty();
        return;
    }

    const FString MessageType = FString(ANSI_TO_TCHAR(reinterpret_cast<const char*>(DataBuffer.GetData() + 4))).Left(32).TrimStartAndEnd();
    const FString DataType = FString(ANSI_TO_TCHAR(reinterpret_cast<const char*>(DataBuffer.GetData() + 36))).Left(8).TrimStartAndEnd();

    int32 MessageLength = *reinterpret_cast<const int32*>(DataBuffer.GetData() + 44);

    if (MessageLength <= 0 || 48 + MessageLength > DataBuffer.Num())
    {
        return; // Wait for more data
    }

    const char* JsonStart = reinterpret_cast<const char*>(DataBuffer.GetData() + 48);
    FUTF8ToTCHAR Converter(JsonStart, MessageLength);
    FString JsonData(Converter.Length(), Converter.Get());

    UE_LOG(LogTemp, Log, TEXT("Received Message Type: %s"), *MessageType);
    UE_LOG(LogTemp, Log, TEXT("Received JSON Data: %s"), *JsonData);

    if (MessageType == TEXT("BridgeServerMessage"))
    {
        // parse JSON and call handler on GameThread
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonData);
        TSharedPtr<FJsonObject> JsonObject;
        if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
        {
            AsyncTask(ENamedThreads::GameThread, [this, JsonObject]() {
                HandleServerMessage(JsonObject);
                });
        }
    }
    else
    {
        AsyncTask(ENamedThreads::GameThread, [JsonData, MessageType, this]() {
            TWeakObjectPtr<UArticyImportData> ImportData = UArticyImportData::GetImportData();
            if (!ImportData.IsValid()) {
                // Build cache if needed
                FArticyEditorFunctionLibrary::ReimportChanges();
                ImportData = UArticyImportData::GetImportData();
            }
            auto Reader = TJsonReaderFactory<>::Create(JsonData);
            TSharedPtr<FJsonObject> JsonObject;
            if (FJsonSerializer::Deserialize(Reader, JsonObject))
            {
                ProcessMessage(MessageType, JsonObject, ImportData.Get());
            }
            });
    }
}

void FArticyBridgeClientRunnable::HandleServerMessage(const TSharedPtr<FJsonObject>&Msg)
{
    // 1) Extract the Event number
    int32 EventId = 0;
    if (!Msg->TryGetNumberField(TEXT("Event"), EventId))
    {
        UE_LOG(LogTemp, Warning, TEXT("BridgeServerMessage missing Event field"));
        return;
    }

    // 2) Pull out arguments - can be null or an object.
    TSharedPtr<FJsonValue> ArgsValue;
    if (Msg->HasField(TEXT("Arguments")))
    {
        ArgsValue = Msg->GetField<EJson::None>(TEXT("Arguments"));
    }

    // 3) Handle the known events
    switch (EventId)
    {
    case 0: // ServerShutdown
        UE_LOG(LogTemp, Warning, TEXT("Bridge Server Shutdown event received."));
        // cleanly stop client loop:
        this->bRun = false;
        this->bShouldReconnect = false;
        break;

    default:
        UE_LOG(LogTemp, Warning, TEXT("Unknown BridgeServerMessage Event: %d"), EventId);
        break;
    }
}

void FArticyBridgeClientRunnable::Connect()
{
    UE_LOG(LogTemp, Log, TEXT("Attempting to connect to: %s:%d"), *Hostname, Port);

    if (ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get())
    {
        TSharedPtr<FInternetAddr> Addr = SocketSubsystem->CreateInternetAddr();
        bool bIsValid = false;

        Addr->SetIp(*Hostname, bIsValid);
        if (!bIsValid)
        {
            FAddressInfoResult AddressInfoResult = SocketSubsystem->GetAddressInfo(*Hostname, nullptr, EAddressInfoFlags::Default, NAME_None);
            if (AddressInfoResult.Results.Num() > 0)
            {
                Addr = AddressInfoResult.Results[0].Address;
                bIsValid = true;
            }
        }

        if (!bIsValid)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to resolve hostname: %s"), *Hostname);
            return;
        }

        Addr->SetPort(Port);
        Socket = MakeShareable(SocketSubsystem->CreateSocket(NAME_Stream, TEXT("TCPClientSocket"), false));
        if (!Socket.IsValid() || !Socket->Connect(*Addr))
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to connect to %s:%d"), *Hostname, Port);
            Socket.Reset();
            if (bShouldReconnect) { FPlatformProcess::Sleep(1.0f); }
            return;
        }

        SendHandshake();

        UE_LOG(LogTemp, Log, TEXT("Connected to %s:%d"), *Hostname, Port);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get SocketSubsystem"));
    }
}

void FArticyBridgeClientRunnable::StopRunning()
{
    bRun = false;
    bShouldReconnect = false;
}

void UArticyBridgeClientCommands::StartBridgeConnection(const TArray<FString>&Args)
{
    FString Hostname;
    int32 Port = 0;

    if (Args.Num() > 0)
    {
        Hostname = Args[0];
        Port = (Args.Num() > 1) ? FCString::Atoi(*Args[1]) : 9870;
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("No address specified. Attempting to discover server via UDP advertisement..."));
        if (!UArticyBridgeClientCommands::DiscoverServerAdvertisement(Hostname, Port))
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to discover server via advertisement."));
            return;
        }
    }

    if (!ClientRunnable.IsValid())
    {
        ClientRunnable = MakeUnique<FArticyBridgeClientRunnable>(Hostname, Port);
        UE_LOG(LogTemp, Log, TEXT("Started Bridge Connection to %s:%d"), *Hostname, Port);
        return;
    }

    FString CurHost; int32 CurPort = 0;
    ClientRunnable->GetCurrentTarget(CurHost, CurPort);

    if (!CurHost.Equals(Hostname, ESearchCase::IgnoreCase) || CurPort != Port)
    {
        UE_LOG(LogTemp, Log, TEXT("Switching Bridge Connection %s:%d -> %s:%d"),
            *CurHost, CurPort, *Hostname, Port);
        ClientRunnable->RequestSwitchServer(Hostname, Port);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Bridge Connection already on %s:%d"), *Hostname, Port);
    }
}

void UArticyBridgeClientCommands::StopBridgeConnection()
{
    if (ClientRunnable.IsValid())
    {
        ClientRunnable->StopRunning();
        ClientRunnable.Reset();
        UE_LOG(LogTemp, Log, TEXT("Stopped Bridge Connection"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No Bridge Connection to stop"));
    }
}

bool UArticyBridgeClientCommands::IsBridgeRunning()
{
    return ClientRunnable.IsValid();
}

void UArticyBridgeClientCommands::GetCurrentBridgeTarget(FString & OutHost, int32 & OutPort)
{
    OutHost.Empty();
    OutPort = 0;
    if (ClientRunnable.IsValid())
    {
        ClientRunnable->GetCurrentTarget(OutHost, OutPort);
    }
}
