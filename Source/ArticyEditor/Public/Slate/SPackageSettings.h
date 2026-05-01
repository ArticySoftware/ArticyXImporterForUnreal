//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

enum class ECheckBoxState : unsigned char;

/**
 * SPackageSettings is a widget that displays and manages package settings, including the ability to toggle package defaults.
 */
class ARTICYEDITOR_API SPackageSettings : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPackageSettings) {}

		SLATE_ARGUMENT(FName, PackageToDisplay)

	SLATE_END_ARGS()

	/**
	 * Construct this widget
	 *
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct(const FArguments& InArgs);

private:
	/** The name of the package to display and manage settings for */
	FName PackageToDisplay;

	/**
	 * Gets the package name to be displayed.
	 *
	 * @return The text representation of the package name.
	 */
	FText GetPackageName() const;

	/**
	 * Handles the change in the checkbox state.
	 *
	 * @param NewState The new state of the checkbox.
	 */
	void OnCheckStateChanged(ECheckBoxState NewState) const;

	/**
	 * Determines the current checkbox state for the package.
	 *
	 * @return The current state of the checkbox.
	 */
	ECheckBoxState IsChecked() const;
};
