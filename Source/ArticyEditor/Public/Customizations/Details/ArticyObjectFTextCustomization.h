//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

/**
 * Renders FText properties on UArticyBaseObject (UArticyObject and UArticyBaseFeature
 * subclasses) using their resolved string-table value instead of the raw localization
 * key, so the editor's details panel matches what runtime code sees via LocalizeString.
 */
class FArticyObjectFTextCustomization final : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
