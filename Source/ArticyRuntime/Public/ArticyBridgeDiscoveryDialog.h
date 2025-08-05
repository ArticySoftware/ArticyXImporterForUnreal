//  
// Copyright (c) 2025 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "ArticyBridgeClient.h"

/**
 * Simple struct to hold discovered bridge endpoint info
 */
struct FBridgeEndpoint
{
    FString Hostname;
    int32 Port;

    FBridgeEndpoint(const FString& InHost, int32 InPort)
        : Hostname(InHost), Port(InPort)
    {
    }
};

/**
 * Slate dialog for discovering and connecting to Articy Bridge instances.
 */
class SBridgeDiscoveryDialog : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SBridgeDiscoveryDialog)
        : _DialogWindow()
        {
        }
        SLATE_ARGUMENT(TWeakPtr<SWindow>, DialogWindow)
    SLATE_END_ARGS()

    /** Constructs the dialog */
    void Construct(const FArguments& InArgs)
    {
        DialogWindow = InArgs._DialogWindow;

        // Kick off initial scan
        RefreshEndpoints();

        ChildSlot
            [
                SNew(SVerticalBox)

                    // Title
                    + SVerticalBox::Slot().AutoHeight().Padding(4)
                    [
                        SNew(STextBlock)
                            .Text(FText::FromString(TEXT("Select or Enter Bridge Instance")))
                            .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 14))
                    ]

                    // List of discovered endpoints
                    + SVerticalBox::Slot().FillHeight(1.0f).Padding(4)
                    [
                        SAssignNew(EndpointListView, SListView<TSharedPtr<FBridgeEndpoint>>)
                            .ListItemsSource(&Endpoints)
                            .OnGenerateRow(this, &SBridgeDiscoveryDialog::OnGenerateRow)
                            .OnSelectionChanged(this, &SBridgeDiscoveryDialog::OnEndpointSelected)
                            .SelectionMode(ESelectionMode::Single)
                    ]

                    // Manual entry
                    + SVerticalBox::Slot().AutoHeight().Padding(4)
                    [
                        SNew(SHorizontalBox)

                            + SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
                            [
                                SAssignNew(HostTextBox, SEditableTextBox)
                                    .HintText(FText::FromString(TEXT("Hostname or IP")))
                            ]

                            + SHorizontalBox::Slot().AutoWidth().Padding(2)
                            [
                                SAssignNew(PortTextBox, SEditableTextBox)
                                    .HintText(FText::FromString(TEXT("Port")))
                            ]
                    ]

                    // Buttons
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(4)
                    [
                        SNew(SUniformGridPanel).SlotPadding(2)

                            + SUniformGridPanel::Slot(0, 0)
                            [
                                SNew(SButton)
                                    .Text(FText::FromString(TEXT("Scan")))
                                    .OnClicked(this, &SBridgeDiscoveryDialog::OnScanClicked)
                            ]

                            + SUniformGridPanel::Slot(1, 0)
                            [
                                SNew(SButton)
                                    .Text(FText::FromString(TEXT("Connect")))
                                    .IsEnabled(this, &SBridgeDiscoveryDialog::CanConnect)
                                    .OnClicked(this, &SBridgeDiscoveryDialog::OnConnectClicked)
                            ]

                            + SUniformGridPanel::Slot(2, 0)
                            [
                                SNew(SButton)
                                    .Text(FText::FromString(TEXT("Cancel")))
                                    .OnClicked(this, &SBridgeDiscoveryDialog::OnCancelClicked)
                            ]
                    ]
            ];
    }

private:
    /** Refresh the list by probing a single advertisement */
    void RefreshEndpoints()
    {
        FString Host;
        int32 Port = 0;
        if (UArticyBridgeClientCommands::DiscoverServerAdvertisement(Host, Port))
        {
            TSharedPtr<FBridgeEndpoint> NewEntry = MakeShared<FBridgeEndpoint>(Host, Port);
            // avoid duplicates
            bool bExists = false;
            for (auto& E : Endpoints)
            {
                if (E->Hostname == Host && E->Port == Port)
                {
                    bExists = true;
                    break;
                }
            }
            if (!bExists)
            {
                Endpoints.Add(NewEntry);
                if (EndpointListView.IsValid())
                {
                    EndpointListView->RequestListRefresh();
                }
            }
        }
    }

    /** Generate a row for each endpoint */
    TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FBridgeEndpoint> Item, const TSharedRef<STableViewBase>& Owner)
    {
        return SNew(STableRow<TSharedPtr<FBridgeEndpoint>>, Owner)
            [
                SNew(STextBlock)
                    .Text(FText::FromString(Item->Hostname + TEXT(":") + FString::FromInt(Item->Port)))
            ];
    }

    /** Handler for list selection */
    void OnEndpointSelected(TSharedPtr<FBridgeEndpoint> Item, ESelectInfo::Type)
    {
        if (Item.IsValid())
        {
            SelectedEndpoint = Item;
            HostTextBox->SetText(FText::FromString(Item->Hostname));
            PortTextBox->SetText(FText::FromString(FString::FromInt(Item->Port)));
        }
    }

    /** Whether the Connect button should be enabled */
    bool CanConnect() const
    {
        return (!HostTextBox->GetText().IsEmpty());
    }

    /** Scan button clicked */
    FReply OnScanClicked()
    {
        RefreshEndpoints();
        return FReply::Handled();
    }

    /** Connect using selected or manual entry */
    FReply OnConnectClicked()
    {
        const FString Host = HostTextBox->GetText().ToString();
        const int32 Port = FCString::Atoi(*PortTextBox->GetText().ToString());

        // invoke existing command
        UArticyBridgeClientCommands::StartBridgeConnection({ Host, FString::FromInt(Port) });

        // close window
        if (DialogWindow.IsValid())
        {
            DialogWindow.Pin()->RequestDestroyWindow();
        }
        return FReply::Handled();
    }

    /** Cancel button clicked */
    FReply OnCancelClicked()
    {
        if (DialogWindow.IsValid())
        {
            DialogWindow.Pin()->RequestDestroyWindow();
        }
        return FReply::Handled();
    }

    // List of discovered endpoints
    TArray<TSharedPtr<FBridgeEndpoint>> Endpoints;
    TSharedPtr<SListView<TSharedPtr<FBridgeEndpoint>>> EndpointListView;
    TSharedPtr<SEditableTextBox> HostTextBox;
    TSharedPtr<SEditableTextBox> PortTextBox;
    TSharedPtr<FBridgeEndpoint> SelectedEndpoint;
    TWeakPtr<SWindow> DialogWindow;
};
