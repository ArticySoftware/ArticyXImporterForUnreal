//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "Slate/Pins/SArticyIdPin.h"
#include "ArticyRef.h"
#include "EdGraph/EdGraphSchema.h"
#include "EdGraph/EdGraphNode.h"
#include "Slate/SArticyIdProperty.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Slate/SArticyRefProperty.h"

/**
 * Constructs the SArticyIdPin widget.
 *
 * @param InArgs The arguments for construction.
 * @param GraphPin The graph pin associated with this widget.
 */
void SArticyIdPin::Construct(const FArguments& InArgs, UEdGraphPin* GraphPin)
{
	SGraphPin::Construct(SGraphPin::FArguments(), GraphPin);
}

/**
 * Gets the default value widget for the pin.
 *
 * @return A shared reference to the default value widget.
 */
TSharedRef<SWidget> SArticyIdPin::GetDefaultValueWidget()
{
	return SNew(SArticyIdProperty)
		.ArticyIdToDisplay(this, &SArticyIdPin::GetArticyId)
		.OnArticyIdChanged(this, &SArticyIdPin::OnArticyIdChanged)
		.Visibility(this, &SArticyIdPin::GetDefaultValueVisibility);
}

/**
 * Determines the visibility of the default value widget.
 *
 * @return The visibility state of the default value widget.
 */
EVisibility SArticyIdPin::GetDefaultValueVisibility() const
{
	if (bOnlyShowDefaultValue)
	{
		return EVisibility::Visible;
	}

	const UEdGraphSchema* Schema = !GraphPinObj->IsPendingKill() ? GraphPinObj->GetSchema() : nullptr;
	if (Schema == nullptr || Schema->ShouldHidePinDefaultValue(GraphPinObj))
	{
		return EVisibility::Collapsed;
	}

	// If it's not connectable we want to always show it
	if (!GraphPinObj->bOrphanedPin && GraphPinObj->bNotConnectable)
	{
		return EVisibility::Visible;
	}

	// don't show on outputs
	if (GraphPinObj->Direction == EGPD_Output)
	{
		return EVisibility::Collapsed;
	}

	// show only when not connected
	return IsConnected() ? EVisibility::Collapsed : EVisibility::Visible;
}

/**
 * Retrieves the Articy ID from the pin's default value string.
 *
 * @return The Articy ID associated with this pin.
 */
FArticyId SArticyIdPin::GetArticyId() const
{
	FString IdString = GraphPinObj->GetDefaultAsString();
	FArticyId Id;

	if (!Id.InitFromString(IdString))
	{
		Id = FArticyId();
	}

	return Id;
}

/**
 * Callback for when the Articy ID changes.
 *
 * @param NewArticyId The new Articy ID value.
 */
void SArticyIdPin::OnArticyIdChanged(const FArticyId& NewArticyId)
{
	FString FormattedValueString;
	FArticyRef::StaticStruct()->ExportText(FormattedValueString, &NewArticyId, nullptr, nullptr, (PPF_Copy), nullptr);

	ArticyId = NewArticyId;

	if (GraphPinObj->GetDefaultAsString() != FormattedValueString)
	{
		const FScopedTransaction Transaction(NSLOCTEXT("GraphEditor", "ChangeArticyIdValue", "Change Articy Id Value"));
		GraphPinObj->Modify();

		GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, NewArticyId.ToString());

		if (OwnerNodePtr.IsValid())
		{
			//OwnerNodePtr.Pin()->UpdateGraphNode();
		}
	}
}
