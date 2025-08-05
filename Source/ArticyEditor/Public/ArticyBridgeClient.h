//  
// Copyright (c) 2025 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/Console.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "ArticyBridgeClient.generated.h"

UCLASS()
class UArticyBridgeClientCommands : public UObject
{
    GENERATED_BODY()

public:
    static void RegisterConsoleCommands();
    static void UnregisterConsoleCommands();
    static void StartBridgeConnection(const TArray<FString>& Args);
    static void StopBridgeConnection();
    static bool DiscoverServerAdvertisement(FString& OutHostname, int32& OutPort);
    static void ShowBridgeDialog(const TArray<FString>&);
};

class FArticyBridgeClientRunnable : public FRunnable
{
public:
    FString Hostname;
    FString Host;
    int32 Port;
    bool bRun;
    bool bShouldReconnect;
    TSharedPtr<FSocket, ESPMode::ThreadSafe> Socket;
    FRunnableThread* Thread;

    FArticyBridgeClientRunnable(FString InHostname, int32 InPort);
    virtual ~FArticyBridgeClientRunnable();

    virtual bool Init() override;
    virtual uint32 Run() override;

    void SendHandshake();
    void UpdateAssets(class UArticyImportData* ImportData);
    void ProcessMessage(const FString& MessageType, const TSharedPtr<FJsonObject> Message, class UArticyImportData* ImportData);
    void ParseReceivedData(const TArray<uint8>& Data);
    void HandleServerMessage(const TSharedPtr<FJsonObject>& Msg);
    void Connect();
    void StopRunning();

    bool bSessionEstablished = false;
};