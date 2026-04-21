//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "ArticyGlobalVariables.h"
#include "Slate/GV/SArticyGlobalVariables.h"
#include "Misc/TextFilterExpressionEvaluator.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

/**
 * A widget for debugging Articy Global Variables at runtime.
 */
class SArticyGlobalVariablesRuntimeDebugger : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SArticyGlobalVariablesRuntimeDebugger) :
		_bInitiallyCollapsed(true)
		{}
		/** Whether the global variables should be initially collapsed. */
		SLATE_ARGUMENT(bool, bInitiallyCollapsed)
	SLATE_END_ARGS()

	/**
	 * Constructs the Articy Global Variables Runtime Debugger widget.
	 *
	 * @param Args The construction arguments.
	 */
	void Construct(const FArguments& Args);

	/**
	 * Updates the state of the widget every frame.
	 *
	 * @param AllottedGeometry The geometry of the widget.
	 * @param InCurrentTime The current time.
	 * @param InDeltaTime The time since the last tick.
	 */
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	/**
	 * Updates the current Global Variables instance.
	 *
	 * @param InGVs The new Global Variables instance.
	 */
	void UpdateGVInstance(TWeakObjectPtr<UArticyGlobalVariables> InGVs);

	/**
	 * Builds the content for the Global Variables picker menu.
	 *
	 * @param Builder The menu builder to populate.
	 */
	void BuildGVPickerContent(FMenuBuilder& Builder);

	/**
	 * Handles the selection of a Global Variables instance.
	 *
	 * @param InVars The selected Global Variables instance.
	 */
	void OnSelectGVs(TWeakObjectPtr<UArticyGlobalVariables> InVars);

	/**
	 * Checks if a Global Variables instance is currently selected.
	 *
	 * @param InVars The Global Variables instance to check.
	 * @return True if the instance is selected, false otherwise.
	 */
	bool IsGVChecked(TWeakObjectPtr<UArticyGlobalVariables> InVars) const;

	/**
	 * Retrieves the label for the current Global Variables instance.
	 *
	 * @return The label text.
	 */
	FText GetCurrentGVLabel() const;

private:
	/** Pointer to the current Global Variables instance. */
	TWeakObjectPtr<UArticyGlobalVariables> CurrentGlobalVariables;

	/** Widget for displaying the Global Variables. */
	TSharedPtr<SArticyGlobalVariables> GlobalVariablesWidget;

	/** Widget for selecting the World context. */
	TSharedPtr<SWidget> WorldSelector;

	/** Switcher for displaying runtime content or messages. */
	TSharedPtr<SWidgetSwitcher> RuntimeSwitcher;

	/** Flag indicating whether the debugger is active. */
	bool bIsActive = false;

	/** Handles for the PIE (Play In Editor) events. */
	TArray<FDelegateHandle> RuntimeSwitchHandles;
};

#undef LOCTEXT_NAMESPACE
