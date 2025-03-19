#include "ArticyBridgeClient.h"
#include "Common/TcpSocketBuilder.h"
#include "IPAddress.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Modules/ModuleManager.h"
#include "ArticyImportData.h"
#include "ArticyEditorFunctionLibrary.h"
#include "Networking.h"
#include "UObject/UnrealType.h"

TArray<uint8> DataBuffer;

// Helper function to discover server via UDP advertisement.
bool DiscoverServerAdvertisement(FString& OutHostname, int32& OutPort)
{
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get();
    if (!SocketSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("No SocketSubsystem available."));
        return false;
    }

    // Create a UDP socket.
    FSocket* UDPSocket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("AdvertisementSocket"), false);
    if (!UDPSocket)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create UDP socket for advertisement."));
        return false;
    }

    // Bind the UDP socket to any address on port 3334.
    TSharedRef<FInternetAddr> ListenAddr = SocketSubsystem->CreateInternetAddr();
    ListenAddr->SetAnyAddress();
    ListenAddr->SetPort(3334);
    if (!UDPSocket->Bind(*ListenAddr))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to bind UDP advertisement socket."));
        SocketSubsystem->DestroySocket(UDPSocket);
        return false;
    }

    // Join the IPv4 multicast group (239.255.0.1).
    TSharedRef<FInternetAddr> MulticastAddr = SocketSubsystem->CreateInternetAddr();
    bool bIsValid = false;
    MulticastAddr->SetIp(TEXT("239.255.0.1"), bIsValid);
    MulticastAddr->SetPort(3334);
    if (bIsValid)
    {
        // Note: JoinMulticastGroup returns false on failure.
        if (!UDPSocket->JoinMulticastGroup(*MulticastAddr))
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to join multicast group. Continuing without group membership."));
        }
    }

    // Use non-blocking mode with a short wait loop.
    UDPSocket->SetNonBlocking(true);

    const float TimeoutSeconds = 5.0f;
    const double StartTime = FPlatformTime::Seconds();
    bool bReceived = false;
    TArray<uint8> ReceivedData;
    ReceivedData.SetNumUninitialized(1024);
    TSharedRef<FInternetAddr> Sender = SocketSubsystem->CreateInternetAddr();

    while ((FPlatformTime::Seconds() - StartTime) < TimeoutSeconds)
    {
        uint32 PendingDataSize = 0;
        if (UDPSocket->HasPendingData(PendingDataSize))
        {
            int32 BytesRead = 0;
            if (UDPSocket->RecvFrom(ReceivedData.GetData(), ReceivedData.Num(), BytesRead, *Sender))
            {
                // Convert the received bytes to a string.
                FString Message = FString(ANSI_TO_TCHAR(reinterpret_cast<const char*>(ReceivedData.GetData()))).TrimStartAndEnd();
                UE_LOG(LogTemp, Log, TEXT("Received advertisement: %s"), *Message);

                // Parse the message as a port number.
                OutPort = FCString::Atoi(*Message);
                if (OutPort == 0)
                {
                    OutPort = 27036; // Use default port if parsing fails.
                }

                // Use the sender's IP address as the hostname.
                OutHostname = Sender->ToString(false);
                bReceived = true;
                break;
            }
        }
        FPlatformProcess::Sleep(0.1f);
    }

    SocketSubsystem->DestroySocket(UDPSocket);

    if (!bReceived)
    {
        UE_LOG(LogTemp, Error, TEXT("No advertisement received within %f seconds."), TimeoutSeconds);
    }

    return bReceived;
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
}

void UArticyBridgeClientCommands::UnregisterConsoleCommands()
{
    IConsoleManager::Get().UnregisterConsoleObject(TEXT("StartBridgeConnection"), false);
    IConsoleManager::Get().UnregisterConsoleObject(TEXT("StopBridgeConnection"), false);
}

TUniquePtr<FArticyBridgeClientRunnable> ClientRunnable;

FArticyBridgeClientRunnable::FArticyBridgeClientRunnable(FString InHostname, int32 InPort)
    : Hostname(InHostname), Port(InPort), bRun(true), bShouldReconnect(true)
{
    Thread = FRunnableThread::Create(this, TEXT("TCPClientThread"), 0, TPri_Normal);
}

FArticyBridgeClientRunnable::~FArticyBridgeClientRunnable()
{
    if (Thread)
    {
        Thread->Kill(true);
        delete Thread;
    }
    if (Socket)
    {
        Socket->Close();
        if (ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get())
        {
            SocketSubsystem->DestroySocket(Socket.Get());
        }
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
        if (!Socket.IsValid())
        {
            Connect();
        }
        else
        {
            uint32 DataSize;
            while (Socket->HasPendingData(DataSize))
            {
                TArray<uint8> ReceivedData;
                ReceivedData.SetNumUninitialized(DataSize);
                int32 BytesRead;
                if (Socket->Recv(ReceivedData.GetData(), ReceivedData.Num(), BytesRead))
                {
                    AccumulateAndParseReceivedData(ReceivedData);
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
    FArticyPackageDef* CurrentPackageDef = nullptr;

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
                    if (PackageDef.GetName().Equals(Package->Name))
                    {
                        CurrentPackageDef = &PackageDef;
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

    if (MessageType.Equals(TEXT("CreatedObject"))) {
        return;
    }

    if (MessageType.Equals(TEXT("GlobalVariableCreated"))) {
        return;
    }

    if (MessageType.Equals(TEXT("CreatedConnection"))) {
        return;
    }

    if (MessageType.Equals(TEXT("RemovedConnection"))) {
        return;
    }

    if (MessageType.Equals(TEXT("ObjectsDeleted"))) {
        return;
    }

    if (MessageType.Equals(TEXT("ChangedBasicProperty"))) {
        return;
    }

    if (MessageType.Equals(TEXT("ChangedLocalizableText")))
    {
        FString LangId;
        Message->TryGetStringField(TEXT("LanguageIso"), LangId);

        TMap<FString, FArticyTexts>& UpdatedTexts = CurrentPackageDef->GetTexts();

        if (Property.Equals(TEXT("VoAsset"))) {
            UpdatedTexts[TechnicalName.ToString()].Content[LangId].VoAsset = Value;
        }
        else if (Property.Equals(TEXT("Text"))) {
            UpdatedTexts[TechnicalName.ToString()].Content[LangId].Text = Value;
        }

        const FString StringTableFileName = PackageName.Replace(TEXT(" "), TEXT("_"));
        const FArticyLanguageDef LangDef = Languages[LangId];
        TPair<FString, FArticyLanguageDef> Lang(LangId, LangDef);

        StringTableGenerator(StringTableFileName, LangId,
            [&](StringTableGenerator* CsvOutput)
            {
                return UArticyImportData::ProcessStrings(CsvOutput, UpdatedTexts, Lang);
            });

        return;
    }
}

void FArticyBridgeClientRunnable::AccumulateAndParseReceivedData(const TArray<uint8>& Data)
{
    DataBuffer.Append(Data);

    while (DataBuffer.Num() >= 40) // Minimum header size: RTCB + TypeName + Length
    {
        FString HeaderCheck = FString(ANSI_TO_TCHAR(reinterpret_cast<const char*>(DataBuffer.GetData()))).Left(4);
        if (HeaderCheck != "RTCB")
        {
            UE_LOG(LogTemp, Error, TEXT("Invalid message header."));
            DataBuffer.Empty();
            return;
        }

        const FString MessageType = FString(ANSI_TO_TCHAR(reinterpret_cast<const char*>(DataBuffer.GetData() + 4))).Left(32).TrimStartAndEnd();
        int32 MessageLength = *reinterpret_cast<const int32*>(DataBuffer.GetData() + 36);

        if (MessageLength <= 0 || 40 + MessageLength > DataBuffer.Num())
        {
            return; // Wait for more data
        }

        const FString JsonData = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(DataBuffer.GetData() + 40)));
        DataBuffer.RemoveAt(0, 40 + MessageLength); // Remove processed data

        UE_LOG(LogTemp, Log, TEXT("Received Message Type: %s"), *MessageType);
        UE_LOG(LogTemp, Log, TEXT("Received JSON Data: %s"), *JsonData);

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

    // If arguments were provided, use them; otherwise try to discover via UDP advertisement.
    if (Args.Num() > 0)
    {
        Hostname = Args[0];
        if (Args.Num() > 1)
        {
            Port = FCString::Atoi(*Args[1]);
        }
        else
        {
            Port = 27036;  // Default port if not specified.
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("No address specified. Attempting to discover server via UDP advertisement..."));
        if (!DiscoverServerAdvertisement(Hostname, Port))
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to discover server via advertisement."));
            return;
        }
    }

    if (!ClientRunnable.IsValid())
    {
        ClientRunnable = MakeUnique<FArticyBridgeClientRunnable>(Hostname, Port);
        UE_LOG(LogTemp, Log, TEXT("Started Bridge Connection to %s:%d"), *Hostname, Port);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Bridge Connection is already running"));
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
