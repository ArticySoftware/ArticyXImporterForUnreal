//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "ArticyGlobalVariables.h"
#include "FrontendFilterBase.h"
#include "Misc/TextFilterExpressionEvaluator.h"
#include "ArticyObject.h"

#define LOCTEXT_NAMESPACE "ArticyFilterHelpers"

// Define the filter types used for Articy objects and variables.
typedef const FAssetData& FArticyObjectFilterType;
typedef TFilterCollection<FArticyObjectFilterType> FArticyObjectFilterCollectionType;

typedef const UArticyVariable*& FArticyVariableFilterType;
typedef TFilterCollection<const UArticyVariable*&> FArticyVariableFilterCollectionType;

/**
 * A filter for testing Articy objects for various traits such as display name, speaker name, text, etc.
 */
class FFrontendFilter_ArticyObject : public IFilter<FArticyObjectFilterType>
{
public:
	/** Constructor for FFrontendFilter_ArticyObject. */
	FFrontendFilter_ArticyObject();
	/** Destructor for FFrontendFilter_ArticyObject. */
	~FFrontendFilter_ArticyObject();

	// FFrontendFilter implementation
	/** Gets the name of the filter.
	 *
	 * @return The name of the filter as a string.
	 */
	virtual FString GetName() const { return TEXT("ArticyObjectFilter"); }
	/** Gets the display name of the filter.
	 *
	 * @return The display name of the filter as FText.
	 */
	virtual FText GetDisplayName() const { return LOCTEXT("FrontendFilter_ArticyObjectFilter", "Articy Object Filter"); }
	/** Gets the tooltip text for the filter.
	 *
	 * @return The tooltip text of the filter as FText.
	 */
	virtual FText GetToolTipText() const { return LOCTEXT("FrontendFilter_ArticyObjectTooltip", "Show only articy objects that match the criteria"); }

	// IFilter implementation
	/** Determines if the given item passes the filter.
	 *
	 * @param InItem The item to test against the filter.
	 * @return True if the item passes the filter, false otherwise.
	 */
	virtual bool PassesFilter(FArticyObjectFilterType InItem) const override;

	DECLARE_DERIVED_EVENT(FFrontendFilter_ArticyObject, IFilter<FArticyObjectFilterType>::FChangedEvent, FChangedEvent);
	virtual FChangedEvent& OnChanged() override { return ChangedEvent; }

	FChangedEvent ChangedEvent;
protected:
	/** Broadcasts the changed event to notify listeners. */
	void BroadcastChangedEvent() const { ChangedEvent.Broadcast(); }
public:
	/** Returns the unsanitized and unsplit filter terms.
	 *
	 * @return The raw filter text.
	 */
	FText GetRawFilterText() const;

	/** Set the Text to be used as the Filter's restrictions.
	 *
	 * @param InFilterText The filter text to set.
	 */
	void SetRawFilterText(const FText& InFilterText);

	/** Get the last error returned from lexing or compiling the current filter text.
	 *
	 * @return The filter error text.
	 */
	FText GetFilterErrorText() const;

	/** If bIncludeClassName is true, the text filter will include an asset's class name in the search.
	 *
	 * @param InIncludeClassName Boolean value indicating inclusion of class name.
	 */
	void SetIncludeClassName(const bool InIncludeClassName);

private:

	/** Transient context data, used when calling PassesFilter. Kept around to minimize re-allocations between multiple calls to PassesFilter */
	TSharedRef<class FFrontendFilter_ArticyObjectFilterExpressionContext> TextFilterExpressionContext;

	/** Expression evaluator that can be used to perform complex text filter queries */
	FTextFilterExpressionEvaluator TextFilterExpressionEvaluator;
};

/**
 * A filter to restrict Articy objects to a specific class.
 */
class FArticyClassRestrictionFilter : public IFilter<FArticyObjectFilterType>
{
public:
	/** Constructor for FArticyClassRestrictionFilter.
	 *
	 * @param AllowedClass The class to be allowed by the filter.
	 * @param bExactClass Whether the class restriction is exact.
	 */
	FArticyClassRestrictionFilter(TSubclassOf<UArticyObject> AllowedClass, bool bExactClass);

	/** Updates the filtered class.
	 *
	 * @param NewAllowedClass The new class to be allowed by the filter.
	 */
	void UpdateFilteredClass(TSubclassOf<UArticyObject> NewAllowedClass) { AllowedClass = NewAllowedClass;	OnChanged().Broadcast(); }
	/** Updates the exact class restriction.
	 *
	 * @param bNewExactClass Whether the class restriction is exact.
	 */
	void UpdateExactClass(bool bNewExactClass) { bExactClass = bNewExactClass; OnChanged().Broadcast(); }
	// IFilter implementation
	/** Determines if the given item passes the class restriction filter.
	 *
	 * @param InItem The item to test against the filter.
	 * @return True if the item passes the filter, false otherwise.
	 */
	virtual bool PassesFilter(FArticyObjectFilterType InItem) const override;

	// IFilter implementation
	DECLARE_DERIVED_EVENT(FArticyClassRestrictionFilter, IFilter<FArticyObjectFilterType>::FChangedEvent, FChangedEvent);
	virtual FChangedEvent& OnChanged() override { return ChangedEvent; }

	FChangedEvent ChangedEvent;
private:
	/** The class allowed by the filter. */
	TSubclassOf<UArticyObject> AllowedClass = UArticyObject::StaticClass();
	/** Whether the class restriction is exact. */
	bool bExactClass = false;
};

/**
 * A filter for testing Articy variables for various traits such as name, set name, etc.
 */
class FFrontendFilter_ArticyVariable : public IFilter<FArticyVariableFilterType>
{
public:
	/** Constructor for FFrontendFilter_ArticyVariable. */
	FFrontendFilter_ArticyVariable();
	/** Destructor for FFrontendFilter_ArticyVariable. */
	~FFrontendFilter_ArticyVariable();

	// FFrontendFilter implementation
	/** Gets the name of the filter.
	 *
	 * @return The name of the filter as a string.
	 */
	virtual FString GetName() const { return TEXT("ArticyVariableFilter"); }
	/** Gets the display name of the filter.
	 *
	 * @return The display name of the filter as FText.
	 */
	virtual FText GetDisplayName() const { return LOCTEXT("FrontendFilter_ArticyVariable", "Articy Variable Filter"); }
	/** Gets the tooltip text for the filter.
	 *
	 * @return The tooltip text of the filter as FText.
	 */
	virtual FText GetToolTipText() const { return LOCTEXT("FrontendFilter_ArticyVariableTooltip", "Show only articy variables that match the criteria"); }

	// IFilter implementation
	/** Determines if the given variable passes the filter.
	 *
	 * @param InItem The variable to test against the filter.
	 * @return True if the variable passes the filter, false otherwise.
	 */
	virtual bool PassesFilter(FArticyVariableFilterType InItem) const override;

public:
	/** Returns the unsanitized and unsplit filter terms.
	 *
	 * @return The raw filter text.
	 */
	FText GetRawFilterText() const;

	/** Set the Text to be used as the Filter's restrictions.
	 *
	 * @param InFilterText The filter text to set.
	 */
	void SetRawFilterText(const FText& InFilterText);

	/** Get the last error returned from lexing or compiling the current filter text.
	 *
	 * @return The filter error text.
	 */
	FText GetFilterErrorText() const;

private:
	/** We only need one expression context for all comparisons, so reuse it */
	TSharedRef<class FFrontendFilter_ArticyGVFilterExpressionContext> TextFilterExpressionContext;
	/** Expression evaluator to test against text expressions */
	FTextFilterExpressionEvaluator TextFilterExpressionEvaluator;

	DECLARE_DERIVED_EVENT(FFrontendFilter, IFilter<FArticyVariableFilterType>::FChangedEvent, FChangedEvent);
	virtual FChangedEvent& OnChanged() override { return ChangedEvent; }

private:
	FChangedEvent ChangedEvent;

};

#undef LOCTEXT_NAMESPACE
