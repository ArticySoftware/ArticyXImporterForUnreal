//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class UArticyFlowPlayer;

/**
 * @class FArticyFlowPlayerCustomization
 * @brief Customizes the details panel of the Articy Flow Player component.
 *
 * Reports what UArticyFlowPlayer::ValidateSetup found at the top of the panel, hides the
 * properties that only carry a value while the game runs, and disables the ones that are
 * only read when the component begins play.
 */
class FArticyFlowPlayerCustomization : public IDetailCustomization
{
public:
	/**
	 * @brief Creates an instance of the ArticyFlowPlayerCustomization.
	 *
	 * @return A shared pointer to a new instance of the ArticyFlowPlayerCustomization class.
	 */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/**
	 * @brief Customizes the details panel of a flow player component.
	 *
	 * @param DetailBuilder The detail layout builder used to customize the details panel.
	 */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	/**
	 * @brief Whether the panel is showing a flow player that is currently playing.
	 *
	 * Properties that are only read in BeginPlay cannot be changed with any effect from here on.
	 *
	 * @param Player The flow player being customized.
	 * @return True if the flow player lives in a game world and has begun play.
	 */
	static bool IsPlaying(const UArticyFlowPlayer* Player);

	/** @brief Properties that only hold a meaningful value while the flow player is playing. */
	static const TArray<FName>& GetRuntimeOnlyProperties();

	/** @brief Properties the flow player only reads when it begins play. */
	static const TArray<FName>& GetStartupOnlyProperties();
};
