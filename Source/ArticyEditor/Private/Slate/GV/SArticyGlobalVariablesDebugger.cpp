//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "Slate/GV/SArticyGlobalVariablesDebugger.h"
#include "Slate/GV/SArticyGlobalVariables.h"
#include "ArticyPluginSettings.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "ArticyGlobalVarSelection"

/**
 * Constructs the Articy Global Variables Runtime Debugger widget.
 *
 * @param Args The construction arguments.
 */
void SArticyGlobalVariablesRuntimeDebugger::Construct(const FArguments& Args)
{
	// the runtime switcher will display a 'disabled' message if no game is running
#if __cplusplus >= 202002L
	RuntimeSwitcher = SNew(SWidgetSwitcher).WidgetIndex_Lambda([=, this]()
#else
	RuntimeSwitcher = SNew(SWidgetSwitcher).WidgetIndex_Lambda([=]()
#endif
		{
			return CurrentGlobalVariables.IsValid() ? 1 : 0;
		});

	GlobalVariablesWidget = SNew(SArticyGlobalVariables, CurrentGlobalVariables).bInitiallyCollapsed(Args._bInitiallyCollapsed);

	FMenuBuilder Builder(true, nullptr);

	TAttribute<FText> LabelAttribute;
	LabelAttribute.BindRaw(this, &SArticyGlobalVariablesRuntimeDebugger::GetCurrentGVLabel);
	Builder.BeginSection(TEXT("Select"));
	Builder.AddSubMenu(
		LabelAttribute,
		LOCTEXT("ChooseGVSubMenuToolTip", "Choose the runtime Global Variables instance to display."),
		FNewMenuDelegate::CreateSP(this, &SArticyGlobalVariablesRuntimeDebugger::BuildGVPickerContent),
		true
	);
	Builder.EndSection();

	RuntimeSwitcher
		->AddSlot(0)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
				.Text(FText::FromString(TEXT("Please start the game to access the runtime Global Variables debugger")))
		];

	RuntimeSwitcher
		->AddSlot(1)
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Left)
				.Padding(5.f)
				[
					Builder.MakeWidget()
				]
				+ SVerticalBox::Slot()
				// if this was autoheight the scrollbar would stop working properly
				.FillHeight(1.f)
				[
					GlobalVariablesWidget.ToSharedRef()
				]
		];
	ChildSlot
		[
			RuntimeSwitcher.ToSharedRef()
		];
}

/**
 * Updates the state of the widget every frame.
 *
 * @param AllottedGeometry The geometry of the widget.
 * @param InCurrentTime The current time.
 * @param InDeltaTime The time since the last tick.
 */
void SArticyGlobalVariablesRuntimeDebugger::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if (!CurrentGlobalVariables.IsValid() && GEditor->GetPIEWorldContext())
	{
		UpdateGVInstance(UArticyGlobalVariables::GetDefault(GEditor->GetPIEWorldContext()->World()));
	}
	else if (CurrentGlobalVariables.IsValid() && !GEditor->GetPIEWorldContext())
	{
		UpdateGVInstance(nullptr);
	}
}

/**
 * Updates the current Global Variables instance.
 *
 * @param InGVs The new Global Variables instance.
 */
void SArticyGlobalVariablesRuntimeDebugger::UpdateGVInstance(TWeakObjectPtr<UArticyGlobalVariables> InGVs)
{
	CurrentGlobalVariables = InGVs;
	GlobalVariablesWidget->UpdateDisplayedGlobalVariables(CurrentGlobalVariables);
}

/**
 * Builds the content for the Global Variables picker menu.
 *
 * @param MenuBuilder The menu builder to populate.
 */
void SArticyGlobalVariablesRuntimeDebugger::BuildGVPickerContent(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("GlobalVariables", LOCTEXT("GVHeading", "Global Variables"));
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (World && (World->IsGameWorld()))
			{
				UArticyGlobalVariables* GV = UArticyGlobalVariables::GetDefault(World);
				MenuBuilder.AddMenuEntry(
					FText::FromString(GV->GetName()),
					LOCTEXT("ChooseGVToolTip", "Display available GV instances."),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateSP(this, &SArticyGlobalVariablesRuntimeDebugger::OnSelectGVs, MakeWeakObjectPtr(GV)),
						FCanExecuteAction(),
						FIsActionChecked::CreateSP(this, &SArticyGlobalVariablesRuntimeDebugger::IsGVChecked, MakeWeakObjectPtr(GV))
					),
					NAME_None,
					EUserInterfaceActionType::RadioButton
				);

				// if we want to keep our global variables, we'll only have a single clone, so break after the first one
				if (GetDefault<UArticyPluginSettings>()->bKeepGlobalVariablesBetweenWorlds)
				{
					break;
				}

			}
		}

	}
	MenuBuilder.EndSection();
}

/**
 * Handles the selection of a Global Variables instance.
 *
 * @param InVars The selected Global Variables instance.
 */
void SArticyGlobalVariablesRuntimeDebugger::OnSelectGVs(TWeakObjectPtr<UArticyGlobalVariables> InVars)
{
	CurrentGlobalVariables = InVars;
}

/**
 * Checks if a Global Variables instance is currently selected.
 *
 * @param InVars The Global Variables instance to check.
 * @return True if the instance is selected, false otherwise.
 */
bool SArticyGlobalVariablesRuntimeDebugger::IsGVChecked(TWeakObjectPtr<UArticyGlobalVariables> InVars) const
{
	return CurrentGlobalVariables == InVars;
}

/**
 * Retrieves the label for the current Global Variables instance.
 *
 * @return The label text.
 */
FText SArticyGlobalVariablesRuntimeDebugger::GetCurrentGVLabel() const
{
	FString OutValue = TEXT("Current GV: ");

	if (CurrentGlobalVariables.IsValid())
	{
		return FText::FromString(OutValue.Append(CurrentGlobalVariables->GetName()));
	}

	return FText::FromString(OutValue.Append(TEXT("None")));
}

#undef LOCTEXT_NAMESPACE
