//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Slate/DatabaseView/SArticyDatabaseView.h"
#include "ArticyDatabase.h"
#include "ArticyEditorFunctionLibrary.h"
#include "ArticyEditorModule.h"
#include "ArticyEditorStyle.h"
#include "ArticyHelpers.h"
#include "ArticyLocalizerSystem.h"
#include "ArticyObject.h"
#include "ArticyPackage.h"
#include "ArticyPluginSettings.h"
#include "EditorStyleSet.h"
#include "IDetailsView.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Slate/UserInterfaceHelperFunctions.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >0
#include "AssetRegistry/AssetRegistryModule.h"
#include "Styling/AppStyle.h"
#else
#include "AssetRegistryModule.h"
#endif

#define LOCTEXT_NAMESPACE "ArticyDatabaseView"

const FName SArticyDatabaseView::NameColumn(TEXT("Name"));
const FName SArticyDatabaseView::TechnicalNameColumn(TEXT("TechnicalName"));
const FName SArticyDatabaseView::TypeColumn(TEXT("Type"));
const FName SArticyDatabaseView::IdColumn(TEXT("Id"));

namespace
{
	const ISlateStyle& GetAppStyle()
	{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >0
		return FAppStyle::Get();
#else
		return FEditorStyle::Get();
#endif
	}

	const FLinearColor ErrorBoxColor(0.42f, 0.08f, 0.08f);
	const FLinearColor WarningBoxColor(0.42f, 0.30f, 0.04f);
	const FLinearColor WarningTextColor(1.f, 0.6f, 0.1f);

	/** A row of the object list. */
	class SArticyDatabaseViewRow : public SMultiColumnTableRow<FArticyDatabaseViewItemPtr>
	{
	public:
		SLATE_BEGIN_ARGS(SArticyDatabaseViewRow) {}
			SLATE_ARGUMENT(FArticyDatabaseViewItemPtr, Item)
			SLATE_ATTRIBUTE(FText, HighlightText)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable)
		{
			Item = InArgs._Item;
			HighlightText = InArgs._HighlightText;
			SMultiColumnTableRow<FArticyDatabaseViewItemPtr>::Construct(FSuperRowType::FArguments(), OwnerTable);
		}

		virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Column) override
		{
			const TSharedRef<SWidget> Text = SNew(STextBlock)
				.Text(FText::FromString(Item->GetColumnText(Column)))
				.HighlightText(HighlightText);

			if (Column != SArticyDatabaseView::NameColumn)
			{
				return SNew(SBox).Padding(FMargin(4.f, 2.f)).VAlign(VAlign_Center)[Text];
			}

			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(4.f, 2.f)
				[
					SNew(SBox)
					.WidthOverride(16.f)
					.HeightOverride(16.f)
					[
						SNew(SImage).Image(UserInterfaceHelperFunctions::GetArticyTypeImage(Item->Object.Get(), UserInterfaceHelperFunctions::Small))
					]
				]
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				[
					Text
				];
		}

	private:
		FArticyDatabaseViewItemPtr Item;
		TAttribute<FText> HighlightText;
	};
}

//---------------------------------------------------------------------------//

FArticyDatabaseViewItem::FArticyDatabaseViewItem(UArticyObject* InObject, const FString& ClassPrefix)
	: Object(InObject)
{
	if (!InObject)
	{
		return;
	}

	DisplayName = UserInterfaceHelperFunctions::GetDisplayName(InObject);
	TechnicalName = InObject->GetTechnicalName().ToString();

	// Generated classes are named <Project><Type>
	Type = InObject->ArticyTypeName;
	if (Type.IsEmpty())
	{
		Type = InObject->GetClass()->GetName();
		if (!ClassPrefix.IsEmpty())
		{
			Type.RemoveFromStart(ClassPrefix);
		}
	}

	// As articy:draft shows ids: 16 hex digits
	Id = FString::Printf(TEXT("0x%016llX"), InObject->GetId().Get());
}

bool FArticyDatabaseViewItem::Matches(const FString& Filter) const
{
	if (Filter.IsEmpty() || Type.Contains(Filter) || DisplayName.Contains(Filter))
	{
		return true;
	}

	return UArticyDatabase::MatchesFilter(Object.Get(), Filter);
}

const FString& FArticyDatabaseViewItem::GetColumnText(const FName& Column) const
{
	if (Column == SArticyDatabaseView::TechnicalNameColumn)
	{
		return TechnicalName;
	}
	if (Column == SArticyDatabaseView::TypeColumn)
	{
		return Type;
	}
	if (Column == SArticyDatabaseView::IdColumn)
	{
		return Id;
	}

	return DisplayName;
}

//---------------------------------------------------------------------------//

SArticyDatabaseView::~SArticyDatabaseView()
{
	if (FModuleManager::Get().IsModuleLoaded(TEXT("ArticyEditor")))
	{
		FArticyEditorModule& Module = FArticyEditorModule::Get();
		Module.OnAssetsGenerated.Remove(AssetsGeneratedHandle);
		Module.OnImportFinished.Remove(ImportFinishedHandle);
	}
}

void SArticyDatabaseView::Construct(const FArguments& InArgs)
{
	bLiveData = InArgs._bLiveData;
	SortColumn = NameColumn;

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			CreateHeader()
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			CreateHelpBox()
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SNew(SSplitter)
			+ SSplitter::Slot()
			.Value(0.55f)
			[
				CreateObjectList()
			]
			+ SSplitter::Slot()
			.Value(0.45f)
			[
				CreateInspector()
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			CreateLocalizationBar()
		]
	];

	if (bLiveData)
	{
		FArticyEditorModule& Module = FArticyEditorModule::Get();
		AssetsGeneratedHandle = Module.OnAssetsGenerated.AddSP(this, &SArticyDatabaseView::Refresh);
		ImportFinishedHandle = Module.OnImportFinished.AddSP(this, &SArticyDatabaseView::Refresh);
		Module.GetUpdateChecker()->CheckForUpdates();

		Refresh();
	}
}

void SArticyDatabaseView::Refresh()
{
	SetImportStatus(FArticyImportStatus::Query());

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> PackageAssets;
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >0
	AssetRegistryModule.Get().GetAssetsByClass(UArticyPackage::StaticClass()->GetClassPathName(), PackageAssets);
#else
	AssetRegistryModule.Get().GetAssetsByClass(UArticyPackage::StaticClass()->GetFName(), PackageAssets);
#endif

	// An object exported with several packages is listed once
	TSet<FArticyId> SeenIds;
	TArray<UArticyObject*> Objects;
	for (const FAssetData& PackageAsset : PackageAssets)
	{
		UArticyPackage* Package = Cast<UArticyPackage>(PackageAsset.GetAsset());
		if (!Package)
		{
			continue;
		}

		for (UArticyObject* Object : Package->GetAssets())
		{
			if (Object && !SeenIds.Contains(Object->GetId()))
			{
				SeenIds.Add(Object->GetId());
				Objects.Add(Object);
			}
		}
	}

	SetObjects(Objects);
}

void SArticyDatabaseView::SetImportStatus(const FArticyImportStatus& InStatus)
{
	ImportStatus = InStatus;
}

void SArticyDatabaseView::SetUpdateStatus(const FArticyPluginUpdateStatus& InStatus)
{
	UpdateStatusOverride = InStatus;
}

void SArticyDatabaseView::SetObjects(const TArray<UArticyObject*>& Objects)
{
	// The selection is carried over by id, as an import replaces the objects
	const TArray<FArticyDatabaseViewItemPtr> Selected = ListView->GetSelectedItems();
	const FString SelectedId = Selected.Num() > 0 ? Selected[0]->Id : FString();

	ListView->ClearSelection();
	DetailsView->SetObject(nullptr);

	AllItems.Reset(Objects.Num());
	for (UArticyObject* Object : Objects)
	{
		AllItems.Add(MakeShared<FArticyDatabaseViewItem>(Object, ImportStatus.Facts.ProjectTechnicalName));
	}

	UpdateVisibleItems();

	for (const FArticyDatabaseViewItemPtr& Item : VisibleItems)
	{
		if (!SelectedId.IsEmpty() && Item->Id == SelectedId)
		{
			ListView->SetSelection(Item);
			break;
		}
	}
}

void SArticyDatabaseView::SetFilterText(const FText& InFilterText)
{
	FilterText = InFilterText;
	UpdateVisibleItems();

	// Shows a filter that was not typed into the search box
	if (!SearchBox->GetText().EqualTo(InFilterText))
	{
		SearchBox->SetText(InFilterText);
	}
}

void SArticyDatabaseView::SelectObject(const UArticyObject* Object)
{
	for (const FArticyDatabaseViewItemPtr& Item : VisibleItems)
	{
		if (Object && Item->Object.Get() == Object)
		{
			ListView->SetSelection(Item);
			ListView->RequestScrollIntoView(Item);
			return;
		}
	}

	ListView->ClearSelection();
}

UArticyObject* SArticyDatabaseView::GetSelectedObject() const
{
	if (!ListView.IsValid())
	{
		return nullptr;
	}

	const TArray<FArticyDatabaseViewItemPtr> Selected = ListView->GetSelectedItems();
	return Selected.Num() > 0 ? Selected[0]->Object.Get() : nullptr;
}

TArray<FArticyDatabaseViewItemPtr> SArticyDatabaseView::FilterAndSort(const TArray<FArticyDatabaseViewItemPtr>& Items, const FString& Filter,
	const FName& SortColumn, bool bAscending)
{
	TArray<FArticyDatabaseViewItemPtr> Result;
	for (const FArticyDatabaseViewItemPtr& Item : Items)
	{
		if (Item->Matches(Filter))
		{
			Result.Add(Item);
		}
	}

	Result.StableSort([&SortColumn, bAscending](const FArticyDatabaseViewItemPtr& A, const FArticyDatabaseViewItemPtr& B)
	{
		const int32 Order = A->GetColumnText(SortColumn).Compare(B->GetColumnText(SortColumn), ESearchCase::IgnoreCase);
		return bAscending ? Order < 0 : Order > 0;
	});

	return Result;
}

void SArticyDatabaseView::UpdateVisibleItems()
{
	const FArticyDatabaseViewItemPtr Selected = ListView->GetNumItemsSelected() > 0 ? ListView->GetSelectedItems()[0] : nullptr;

	VisibleItems = FilterAndSort(AllItems, FilterText.ToString(), SortColumn, bSortAscending);
	ListView->RequestListRefresh();

	// A selected object that is filtered out is no longer inspected
	if (Selected.IsValid() && !VisibleItems.Contains(Selected))
	{
		ListView->ClearSelection();
	}
}

//---------------------------------------------------------------------------//

TSharedRef<SWidget> SArticyDatabaseView::CreateHeader()
{
	return SNew(SBorder)
		.BorderImage(GetAppStyle().GetBrush("ToolPanel.GroupBorder"))
		.Padding(FMargin(8.f, 6.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(SImage).Image(FArticyEditorStyle::Get().GetBrush("ArticyImporter.ArticyDraft.32"))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
				.Text_Lambda([this]()
				{
					return ImportStatus.Facts.ProjectName.IsEmpty()
						? LOCTEXT("NoProject", "Articy Database")
						: FText::FromString(ImportStatus.Facts.ProjectName);
				})
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(12.f, 0.f)
			[
				SNew(SButton)
				.ButtonStyle(GetAppStyle(), "NoBorder")
				.ContentPadding(0.f)
				.ToolTipText_Lambda([this]() { return GetUpdateStatus().GetToolTip(); })
				.OnClicked(this, &SArticyDatabaseView::OnUpdateLabelClicked)
				[
					// Filled when an update is available, so it is noticed
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.Padding(FMargin(6.f, 2.f))
					.BorderBackgroundColor_Lambda([this]()
					{
						const FArticyPluginUpdateStatus& Status = GetUpdateStatus();
						return FSlateColor(Status.State == EArticyUpdateState::UpdateAvailable ? Status.GetColor() : FLinearColor::Transparent);
					})
					[
						SNew(STextBlock)
						.Text_Lambda([this]() { return GetUpdateStatus().GetLabel(); })
						.ColorAndOpacity_Lambda([this]()
						{
							const FArticyPluginUpdateStatus& Status = GetUpdateStatus();
							return FSlateColor(Status.State == EArticyUpdateState::UpdateAvailable ? FLinearColor::Black : Status.GetColor());
						})
					]
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			[
				SNew(SSpacer)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(2.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("ImportChanges", "Import Changes"))
				.ToolTipText(LOCTEXT("ImportChanges_ToolTip", "Imports only the changes from last import."))
				.OnClicked_Lambda([this]() { RunImportAction(EArticyImportAction::ImportChanges); return FReply::Handled(); })
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(2.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("FullReimport", "Full Reimport"))
				.ToolTipText(LOCTEXT("FullReimport_ToolTip", "Forces a complete reimport including code and asset generation."))
				.OnClicked_Lambda([this]() { RunImportAction(EArticyImportAction::FullReimport); return FReply::Handled(); })
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(2.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Refresh", "Refresh"))
				.ToolTipText(LOCTEXT("Refresh_ToolTip", "Reads the import state and the objects again."))
				.OnClicked_Lambda([this]() { Refresh(); return FReply::Handled(); })
			]
		];
}

TSharedRef<SWidget> SArticyDatabaseView::CreateHelpBox()
{
	return SNew(SBorder)
		.Visibility_Lambda([this]() { return IsHelpBoxVisible() ? EVisibility::Visible : EVisibility::Collapsed; })
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor_Lambda([this]() { return FSlateColor(ImportStatus.IsError() ? ErrorBoxColor : WarningBoxColor); })
		.Padding(FMargin(8.f, 6.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(SImage)
				.Image_Lambda([this]() { return GetAppStyle().GetBrush(ImportStatus.IsError() ? "Icons.Error" : "Icons.Warning"); })
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.ColorAndOpacity(FLinearColor::White)
				.Text_Lambda([this]() { return ImportStatus.GetMessage(); })
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(8.f, 0.f, 0.f, 0.f)
			[
				SNew(SButton)
				.Visibility_Lambda([this]() { return ImportStatus.GetAction() != EArticyImportAction::None ? EVisibility::Visible : EVisibility::Collapsed; })
				.Text_Lambda([this]() { return ImportStatus.GetActionLabel(); })
				.OnClicked_Lambda([this]() { RunImportAction(ImportStatus.GetAction()); return FReply::Handled(); })
			]
		];
}

TSharedRef<SWidget> SArticyDatabaseView::CreateObjectList()
{
	auto MakeColumn = [this](const FName& Column, const FText& Label, float Width)
	{
		return SHeaderRow::Column(Column)
			.DefaultLabel(Label)
			.FillWidth(Width)
			.SortMode(TAttribute<EColumnSortMode::Type>::Create(TAttribute<EColumnSortMode::Type>::FGetter::CreateSP(this, &SArticyDatabaseView::GetColumnSortMode, Column)))
			.OnSort(this, &SArticyDatabaseView::OnSortModeChanged);
	};

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.f)
		[
			SAssignNew(SearchBox, SSearchBox)
			.HintText(LOCTEXT("SearchHint", "Filter by name, type, id or text"))
			.OnTextChanged(this, &SArticyDatabaseView::SetFilterText)
			.DelayChangeNotificationsWhileTyping(true)
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SAssignNew(ListView, SListView<FArticyDatabaseViewItemPtr>)
			.ListItemsSource(&VisibleItems)
			.SelectionMode(ESelectionMode::Single)
			.OnGenerateRow(this, &SArticyDatabaseView::OnGenerateRow)
			.OnSelectionChanged(this, &SArticyDatabaseView::OnSelectionChanged)
			.HeaderRow
			(
				SNew(SHeaderRow)
				+ MakeColumn(NameColumn, LOCTEXT("NameColumn", "Name"), 0.34f)
				+ MakeColumn(TechnicalNameColumn, LOCTEXT("TechnicalNameColumn", "Technical Name"), 0.26f)
				+ MakeColumn(TypeColumn, LOCTEXT("TypeColumn", "Type"), 0.2f)
				+ MakeColumn(IdColumn, LOCTEXT("IdColumn", "Id"), 0.2f)
			)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(6.f, 3.f)
		[
			SNew(STextBlock)
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.Text_Lambda([this]()
			{
				return FText::Format(LOCTEXT("ObjectCount", "{0} of {1} objects"), FText::AsNumber(VisibleItems.Num()), FText::AsNumber(AllItems.Num()));
			})
		];
}

TSharedRef<SWidget> SArticyDatabaseView::CreateInspector()
{
	FDetailsViewArgs Args;
	Args.bAllowSearch = true;
	Args.bHideSelectionTip = true;
	Args.bLockable = false;
	Args.bUpdatesFromSelection = false;
	Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;

	DetailsView = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(Args);

	// The objects are regenerated by every import, so they are shown but not edited
	DetailsView->SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateLambda([]() { return false; }));

	return SNew(SWidgetSwitcher)
		.WidgetIndex_Lambda([this]() { return GetSelectedObject() ? 1 : 0; })
		+ SWidgetSwitcher::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.Text(LOCTEXT("NoSelection", "Select an object to see its properties."))
		]
		+ SWidgetSwitcher::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(6.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SBox)
					.WidthOverride(32.f)
					.HeightOverride(32.f)
					[
						SNew(SImage)
						.Image_Lambda([this]() { return UserInterfaceHelperFunctions::GetArticyTypeImage(GetSelectedObject(), UserInterfaceHelperFunctions::Medium); })
					]
				]
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
						.Text_Lambda([this]()
						{
							const TArray<FArticyDatabaseViewItemPtr> Selected = ListView->GetSelectedItems();
							return Selected.Num() > 0 ? FText::FromString(Selected[0]->DisplayName) : FText::GetEmpty();
						})
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						.Text_Lambda([this]()
						{
							const TArray<FArticyDatabaseViewItemPtr> Selected = ListView->GetSelectedItems();
							return Selected.Num() > 0
								? FText::FromString(FString::Printf(TEXT("%s  -  %s  -  %s"), *Selected[0]->Type, *Selected[0]->TechnicalName, *Selected[0]->Id))
								: FText::GetEmpty();
						})
					]
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				DetailsView.ToSharedRef()
			]
		];
}

TSharedRef<SWidget> SArticyDatabaseView::CreateLocalizationBar()
{
	auto MakeSetting = [](const FText& Label, const FText& ToolTip, bool UArticyPluginSettings::* Setting)
	{
		return SNew(SCheckBox)
			.ToolTipText(ToolTip)
			.IsChecked_Lambda([Setting]()
			{
				return GetDefault<UArticyPluginSettings>()->*Setting ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
			.OnCheckStateChanged_Lambda([Setting](ECheckBoxState NewState)
			{
				UArticyPluginSettings* Settings = GetMutableDefault<UArticyPluginSettings>();
				Settings->*Setting = NewState == ECheckBoxState::Checked;
#if ENGINE_MAJOR_VERSION >= 5
				Settings->TryUpdateDefaultConfigFile();
#else
				Settings->UpdateDefaultConfigFile();
#endif
			})
			[
				SNew(STextBlock).Text(Label)
			];
	};

	return SNew(SBorder)
		.BorderImage(GetAppStyle().GetBrush("ToolPanel.GroupBorder"))
		.Padding(FMargin(8.f, 4.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.f, 0.f, 12.f, 0.f)
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				.Text(LOCTEXT("Localization", "Localization"))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.f, 0.f, 16.f, 0.f)
			[
				SNew(STextBlock)
				.Text(this, &SArticyDatabaseView::GetLanguageText)
				.ToolTipText(LOCTEXT("Language_ToolTip", "The language of the editor, which the articy texts are loaded for."))
				.ColorAndOpacity_Lambda([this]() { return IsCurrentLanguageExported() ? FSlateColor::UseForeground() : FSlateColor(WarningTextColor); })
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.f, 0.f, 12.f, 0.f)
			[
				MakeSetting(LOCTEXT("ClearOnSwitch", "Clear on language switch"),
					LOCTEXT("ClearOnSwitch_ToolTip", "Forgets the texts of the previous language when the language changes. If off, they are kept for keys the new language has no text for."),
					&UArticyPluginSettings::bClearLocalizationOnLanguageSwitch)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.f, 0.f, 12.f, 0.f)
			[
				MakeSetting(LOCTEXT("ErrorOnMissingText", "Error on missing text (PIE)"),
					LOCTEXT("ErrorOnMissingText_ToolTip", "Reports a text without a string table entry as an error while playing in the editor."),
					&UArticyPluginSettings::bErrorOnMissingText)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			[
				SNew(SSpacer)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.Text(LOCTEXT("ClearLocalization", "Clear Database"))
				.ToolTipText(LOCTEXT("ClearLocalization_ToolTip", "Reloads the articy string tables and forgets the texts kept from other languages."))
				.OnClicked_Lambda([]()
				{
					if (UArticyLocalizerSystem* Localizer = UArticyLocalizerSystem::Get())
					{
						Localizer->ClearLocalizationDatabase();
					}
					return FReply::Handled();
				})
			]
		];
}

//---------------------------------------------------------------------------//

TSharedRef<ITableRow> SArticyDatabaseView::OnGenerateRow(FArticyDatabaseViewItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SArticyDatabaseViewRow, OwnerTable)
		.Item(Item)
		.HighlightText_Lambda([this]() { return FilterText; });
}

void SArticyDatabaseView::OnSelectionChanged(FArticyDatabaseViewItemPtr Item, ESelectInfo::Type SelectInfo)
{
	DetailsView->SetObject(Item.IsValid() ? Item->Object.Get() : nullptr);
}

void SArticyDatabaseView::OnSortModeChanged(EColumnSortPriority::Type Priority, const FName& Column, EColumnSortMode::Type Mode)
{
	SortColumn = Column;
	bSortAscending = Mode != EColumnSortMode::Descending;
	UpdateVisibleItems();
}

EColumnSortMode::Type SArticyDatabaseView::GetColumnSortMode(FName Column) const
{
	if (Column != SortColumn)
	{
		return EColumnSortMode::None;
	}

	return bSortAscending ? EColumnSortMode::Ascending : EColumnSortMode::Descending;
}

void SArticyDatabaseView::RunImportAction(EArticyImportAction Action)
{
	if (Action == EArticyImportAction::ImportChanges)
	{
		FArticyEditorFunctionLibrary::ReimportChanges();
	}
	else if (Action == EArticyImportAction::FullReimport)
	{
		FArticyEditorFunctionLibrary::ForceCompleteReimport();
	}

	// The import refreshes the view again when it finishes compiling
	if (bLiveData)
	{
		Refresh();
	}
}

FReply SArticyDatabaseView::OnUpdateLabelClicked() const
{
	const FString Url = GetUpdateStatus().GetUpdateUrl();
	if (!Url.IsEmpty())
	{
		FPlatformProcess::LaunchURL(*Url, nullptr, nullptr);
	}

	return FReply::Handled();
}

const FArticyPluginUpdateStatus& SArticyDatabaseView::GetUpdateStatus() const
{
	return UpdateStatusOverride.IsSet() ? UpdateStatusOverride.GetValue() : FArticyEditorModule::Get().GetUpdateChecker()->GetStatus();
}

FText SArticyDatabaseView::GetLanguageText() const
{
	const FCultureRef Culture = FInternationalization::Get().GetCurrentCulture();
	const FText Language = FText::Format(LOCTEXT("Language", "Language: {0} ({1})"),
		FText::FromString(Culture->GetDisplayName()), FText::FromString(Culture->GetName()));

	return IsCurrentLanguageExported()
		? Language
		: FText::Format(LOCTEXT("LanguageNotExported", "{0} - not exported, the default texts are used"), Language);
}

bool SArticyDatabaseView::IsCurrentLanguageExported() const
{
	return ImportStatus.Facts.HasLanguage(UArticyLocalizerSystem::GetCurrentLanguage());
}

#undef LOCTEXT_NAMESPACE
