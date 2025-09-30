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

TArray<uint8> DataBuffer;

static const int32 HEADER_SIZE = 48;  // 4 + 32 + 8 + 4

// Helper function to discover server via UDP advertisement.
bool UArticyBridgeClientCommands::DiscoverServerAdvertisement(FString& OutHostname, int32& OutPort)
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

void SetPropertyValue(FString Property, FArticyTexts& ArticyText, const FString& Value)
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

void FArticyBridgeClientRunnable::UpdateAssets(UArticyImportData* ImportData)
{
    ImportData->BuildCachedVersion();
    CodeGenerator::GenerateAssets(ImportData);
    ImportData->PostImport();
}

void FArticyBridgeClientRunnable::ProcessMessage(const FString& MessageType, const TSharedPtr<FJsonObject> Message, UArticyImportData* ImportData)
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

    if (MessageType.Equals(TEXT("CreatedFirstClassObject"))) {
        /*
        {"Type":"Dialogue","Properties":{"Id":"0x0100000000003279","TechnicalName":"Dlg_8C2E87FB","Parent":"0x0300000043CE43F7","DisplayName":{"LId":"Dlg_8C2E87FB.DisplayName","Values":{"en":"Untitled dialogue"}},"Attachments":[],"PreviewImageAsset":null,"Color":{"a":1.0,"r":0.78431374,"g":0.8862745,"b":0.90588236},"Text":{"LId":"Dlg_8C2E87FB.Text","Values":{}},"ExternalId":"","ShortId":2351859707,"TemplateName":"","Position":{"x":442.0,"y":300.0},"ZIndex":11,"Size":{"w":291.0,"h":200.0},"InputPins":[{"Id":"0x010000000000327C","Owner":"0x0100000000003279"}],"OutputPins":[{"Id":"0x010000000000327D","Owner":"0x0100000000003279"}]},"Packages":["72057594037939252"]}
        */
        return;
    }

    if (MessageType.Equals(TEXT("GlobalVariableCreated"))) {
        return;
    }

    if (MessageType.Equals(TEXT("CreatedConnection"))) {
        /* {"Source":"0x010000010000029E","Target":"0x0100000100000B92","SourcePin":"0x01000001000002A4","TargetPin":"0x0100000100000B97"} */
        return;
    }

    if (MessageType.Equals(TEXT("RemovedConnection"))) {
        /* { "Source":"0x010000010000029E", "Target" : "0x0100000100000B92", "SourcePin" : "0x01000001000002A4", "TargetPin" : "0x0100000100000B97" } */
        return;
    }

    if (MessageType.Equals(TEXT("ObjectsDeleted"))) {
        const TArray<TSharedPtr<FJsonValue>>* ObjectsArrayPtr;
        if (Message->TryGetArrayField(TEXT("Objects"), ObjectsArrayPtr))
        {
            for (const TSharedPtr<FJsonValue>& IdValue : *ObjectsArrayPtr)
            {
                FString DeletedId = IdValue->AsString();

                // Attempt to remove from packages
                for (const TSoftObjectPtr<UArticyPackage>& Package : Packages)
                {
                    Package->RemoveAssetById(DeletedId);
                    break;
                }
            }
        }
        UpdateAssets(ImportData);
        return;
    }

    if (MessageType.Equals(TEXT("ChangedBasicProperty"))) {
        return;
    }

    if (MessageType.Equals(TEXT("ChangedLocalizableText")))
    {
        FString LangId;
        Message->TryGetStringField(TEXT("LanguageIso"), LangId);

        TMap<FString, FArticyTexts> UpdatedTexts = CurrentPackageDef.GetTexts();

        FString Key = FString::Printf(TEXT("%s.%s"), *TechnicalName.ToString(), *Property);
        UpdatedTexts[Key].Content[LangId].Text = Value;

        const FString StringTableFileName = PackageName.Replace(TEXT(" "), TEXT("_"));
        const FArticyLanguageDef LangDef = Languages[LangId];
        TPair<FString, FArticyLanguageDef> Lang(LangId, LangDef);

        StringTableGenerator(StringTableFileName, LangId,
            [&](StringTableGenerator* CsvOutput)
            {
                return UArticyImportData::ProcessStrings(CsvOutput, UpdatedTexts, Lang);
            });

        UArticyLocalizerSystem::Get()->Reload();
        return;
    }
}

void FArticyBridgeClientRunnable::ParseReceivedData(const TArray<uint8>& Data)
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

void FArticyBridgeClientRunnable::HandleServerMessage(const TSharedPtr<FJsonObject>& Msg)
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

void UArticyBridgeClientCommands::StartBridgeConnection(const TArray<FString>& Args)
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

void UArticyBridgeClientCommands::GetCurrentBridgeTarget(FString& OutHost, int32& OutPort)
{
    OutHost.Empty();
    OutPort = 0;
    if (ClientRunnable.IsValid())
    {
        ClientRunnable->GetCurrentTarget(OutHost, OutPort);
    }
}
