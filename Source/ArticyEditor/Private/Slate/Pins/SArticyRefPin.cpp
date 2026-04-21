//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "Slate/Pins/SArticyRefPin.h"
#include "ArticyRef.h"
#include "EdGraph/EdGraphSchema.h"
#include "EdGraph/EdGraphNode.h"
#include "Slate/SArticyIdProperty.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Slate/SArticyRefProperty.h"

/**
 * Constructs the SArticyRefPin widget.
 *
 * @param InArgs The arguments for construction.
 * @param GraphPin The graph pin associated with this widget.
 */
void SArticyRefPin::Construct(const FArguments& InArgs, UEdGraphPin* GraphPin)
{
	SGraphPin::Construct(SGraphPin::FArguments(), GraphPin);
}

/**
 * Gets the default value widget for the pin.
 *
 * @return A shared reference to the default value widget.
 */
TSharedRef<SWidget> SArticyRefPin::GetDefaultValueWidget()
{
	return SNew(SArticyRefProperty)
		.ArticyRefToDisplay(this, &SArticyRefPin::GetArticyRef)
		.OnArticyRefChanged(this, &SArticyRefPin::OnArticyRefChanged)
		.Visibility(this, &SArticyRefPin::GetDefaultValueVisibility);
}

/**
 * Determines the visibility of the default value widget.
 *
 * @return The visibility state of the default value widget.
 */
EVisibility SArticyRefPin::GetDefaultValueVisibility() const
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
 * Retrieves the Articy reference from the pin's default value string.
 *
 * @return The Articy reference associated with this pin.
 */
FArticyRef SArticyRefPin::GetArticyRef() const
{
	FString RefString = GraphPinObj->GetDefaultAsString();
	FArticyRef Ref;

	if (!Ref.InitFromString(RefString))
	{
		Ref = FArticyRef();
	}

	return Ref;
}

/**
 * Callback for when the Articy reference changes.
 *
 * @param NewArticyRef The new Articy reference value.
 */
void SArticyRefPin::OnArticyRefChanged(const FArticyRef& NewArticyRef)
{
	FString FormattedValueString;
	FArticyRef::StaticStruct()->ExportText(FormattedValueString, &ArticyRef, nullptr, nullptr, (PPF_Copy), nullptr);

	ArticyRef = NewArticyRef;

	if (GraphPinObj->GetDefaultAsString() != FormattedValueString)
	{
		const FScopedTransaction Transaction(NSLOCTEXT("GraphEditor", "ChangeArticyRefValue", "Change Articy Ref Value"));
		GraphPinObj->Modify();

		GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, ArticyRef.ToString());

		if (OwnerNodePtr.IsValid())
		{
			//OwnerNodePtr.Pin()->UpdateGraphNode();
		}
	}
}
