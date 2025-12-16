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

    // Returns true if a client runnable exists
    static bool IsBridgeRunning();

    // Fetch current target for UI
    static void GetCurrentBridgeTarget(FString& OutHost, int32& OutPort);
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
    void UpdateAssetsAndCode(UArticyImportData* ImportData);
    void Connect();
    void StopRunning();

    void RequestSwitchServer(const FString& NewHost, int32 NewPort)
    {
        FScopeLock Lock(&TargetMutex);
        Hostname = NewHost;
        Port = NewPort;
        bReconnectRequested = true;
    }

    void GetCurrentTarget(FString& OutHost, int32& OutPort) const
    {
        OutHost = Hostname;
        OutPort = Port;
    }

    bool bSessionEstablished = false;

private:
    FCriticalSection TargetMutex;
    TAtomic<bool> bReconnectRequested{ false };

    void CloseSocket_NoLock()
    {
        if (Socket.IsValid())
        {
            Socket->Close();
            if (ISocketSubsystem* SS = ISocketSubsystem::Get())
            {
                SS->DestroySocket(Socket.Get());
            }
            Socket.Reset();
        }
        bSessionEstablished = false;
    }
};