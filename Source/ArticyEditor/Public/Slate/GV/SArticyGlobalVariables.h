//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "ArticyGlobalVariables.h"
#include "Misc/TextFilterExpressionEvaluator.h"
#include "Slate/ArticyFilterHelpers.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"

/** ref: detailcategorygroupnode.cpp */
struct FGlobalVariablesSizeData
{
	TAttribute<float> LeftColumnWidth;
	TAttribute<float> RightColumnWidth;
	SSplitter::FOnSlotResized OnWidthChanged;

	void SetColumnWidth(float InWidth) { OnWidthChanged.ExecuteIfBound(InWidth); }
};

/**
 * A widget representing a set of Articy variables within an expandable area.
 */
class SArticyVariableSet : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SArticyVariableSet) :
		_bInitiallyCollapsed(true),
		_SizeData(nullptr)
		{}

		/** Whether the variable set should be initially collapsed. */
		SLATE_ARGUMENT(bool, bInitiallyCollapsed)

		/** Pointer to size data for controlling column widths. */
		SLATE_ARGUMENT(const FGlobalVariablesSizeData*, SizeData)

	SLATE_END_ARGS()

	/**
	 * Constructs the variable set widget.
	 *
	 * @param Args The widget arguments.
	 * @param VariableSet The variable set to represent.
	 */
	void Construct(const FArguments& Args, TWeakObjectPtr<UArticyBaseVariableSet> VariableSet);

	/**
	 * Sets the expanded state of the variable set.
	 *
	 * @param bInExpanded Whether to expand or collapse the variable set.
	 */
	void SetExpanded(bool bInExpanded) const;

	/**
	 * Checks if the variable set is expanded.
	 *
	 * @return True if expanded, false otherwise.
	 */
	bool IsExpanded() const;

	/**
	 * Updates the visibility of a specific variable.
	 *
	 * @param Variable The variable to update visibility for.
	 * @param InVisibility The desired visibility.
	 */
	void UpdateVisibility(const UArticyVariable* Variable, EVisibility InVisibility);

	/**
	 * Retrieves the variables in the set.
	 *
	 * @return An array of variables.
	 */
	TArray<UArticyVariable*> GetVariables();

private:
	/** Builds the widgets for the variables in the set. */
	void BuildVariableWidgets();

private:
	/** Flag indicating whether the variable widgets have been built. */
	bool bVariableWidgetsBuilt = false;

	/** Flag indicating whether a slider is currently being moved. */
	bool bSliderMoving = false;

	/** The variable set being represented. */
	TWeakObjectPtr<UArticyBaseVariableSet> VariableSet;

	/** Mapping of variables to their corresponding widgets. */
	TMap<UArticyVariable*, TWeakPtr<SWidget>> VariableWidgetMapping;

	/** Container for the variable widgets. */
	TSharedPtr<SVerticalBox> VariableContainer;

	/** Expandable area for the namespace. */
	TSharedPtr<SExpandableArea> NamespaceExpansion;

	/** Pointer to size data for controlling column widths. */
	const FGlobalVariablesSizeData* SizeData = nullptr;

	/**
	 * Handles value changes for variables.
	 *
	 * @tparam T The type of the variable value.
	 * @tparam T2 The type of the variable.
	 * @param Value The new value.
	 * @param Var The variable being changed.
	 */
	template<typename T, typename T2>
	void OnValueChanged(T Value, T2* Var);
};

template <typename T, typename T2>
void SArticyVariableSet::OnValueChanged(T Value, T2* Var)
{
	if (bSliderMoving)
	{
		if (Var->Get() == Value)
		{
			return;
		}
		Var->Modify();
		*Var = Value;
	}
}

/**
 * A widget for displaying and managing Articy global variables.
 */
class SArticyGlobalVariables : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SArticyGlobalVariables) :
		_bInitiallyCollapsed(true)
		{}
		/** Whether the global variables should be initially collapsed. */
		SLATE_ARGUMENT(bool, bInitiallyCollapsed)
	SLATE_END_ARGS()

	/**
	 * Constructs the global variables widget.
	 *
	 * @param Args The widget arguments.
	 * @param GV The global variables object.
	 */
	void Construct(const FArguments& Args, TWeakObjectPtr<UArticyGlobalVariables> GV);

	/**
	 * Updates the displayed global variables.
	 *
	 * @param InGV The global variables object.
	 */
	void UpdateDisplayedGlobalVariables(TWeakObjectPtr<UArticyGlobalVariables> InGV);

private:
	/** Pointer to the global variables object. */
	TWeakObjectPtr<UArticyGlobalVariables> GlobalVariables;

private:
	/** Size data for controlling column widths. */
	FGlobalVariablesSizeData SizeData;

	/** Column width for the right column. */
	float ColumnWidth = 0.65f;

	/** Whether the global variables should be initially collapsed. */
	bool bInitiallyCollapsed = true;

private:
	/** Retrieves the left column width. */
	float OnGetLeftColumnWidth() const { return 1.0f - ColumnWidth; }

	/** Retrieves the right column width. */
	float OnGetRightColumnWidth() const { return ColumnWidth; }

	/** Sets the column width. */
	void OnSetColumnWidth(float InWidth) { ColumnWidth = InWidth; }

private:
	/** Handles changes in the search box text. */
	void OnSearchBoxChanged(const FText& InSearchText);

	/** Handles search box text commit events. */
	void OnSearchBoxCommitted(const FText& InSearchText, ETextCommit::Type CommitInfo);

	/** Sets the search box text. */
	void SetSearchBoxText(const FText& InSearchText);

	/** Handles changes in the frontend filters. */
	void OnFrontendFiltersChanged();

	/** Tests a variable against the frontend filters. */
	bool TestAgainstFrontendFilters(const UArticyVariable* Item) const;

	/** Caches the expansion states of variable sets. */
	void CacheExpansionStates();

	/** Restores the expansion states of variable sets. */
	void RestoreExpansionStates();

private:
	/** Container for the variable set widgets. */
	TSharedPtr<SVerticalBox> SetContainer;

	/** Array of variable set widgets. */
	TArray<TSharedPtr<SArticyVariableSet>> VariableSetWidgets;

	/** Flag indicating whether sets should be force expanded. */
	bool bShouldForceExpand = false;

	/** Cache for the expansion states of variable sets. */
	TMap<TSharedPtr<SArticyVariableSet>, bool> ExpansionCache;

	/** Filter for variable names. */
	TSharedPtr<FFrontendFilter_ArticyVariable> VariableFilter;

	/** Collection of frontend filters. */
	TSharedPtr<FArticyVariableFilterCollectionType> FrontendFilters;
};
