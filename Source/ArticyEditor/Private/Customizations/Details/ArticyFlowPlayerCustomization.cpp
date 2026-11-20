//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Customizations/Details/ArticyFlowPlayerCustomization.h"
#include "ArticyFlowPlayer.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
#include "PropertyHandle.h"
#include "Engine/World.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ArticyFlowPlayerCustomization"

namespace
{
	/** Seconds between two re-validations while the details panel is open. */
	const double ValidationInterval = 0.25;

	/** Holds the setup issues of one flow player, re-validated while the panel is painted. */
	struct FValidationCache
	{
		TWeakObjectPtr<UArticyFlowPlayer> Player;
		TArray<FArticyFlowPlayerSetupIssue> Issues;
		double LastValidation = -ValidationInterval;

		const TArray<FArticyFlowPlayerSetupIssue>& Get()
		{
			const double Now = FPlatformTime::Seconds();
			if (Now - LastValidation >= ValidationInterval)
			{
				LastValidation = Now;
				Issues = Player.IsValid() ? Player->ValidateSetup() : TArray<FArticyFlowPlayerSetupIssue>();
			}

			return Issues;
		}
	};
}

/**
 * @brief Customizes the details panel of a flow player component.
 *
 * @param DetailBuilder The detail layout builder used to customize the details panel.
 */
void FArticyFlowPlayerCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> WeakObjects;
	DetailBuilder.GetObjectsBeingCustomized(WeakObjects);

	// Nothing useful to say about a mixed or empty selection
	if (WeakObjects.Num() != 1)
		return;

	UArticyFlowPlayer* Player = Cast<UArticyFlowPlayer>(WeakObjects[0].Get());
	if (!Player)
		return;

	const bool bPlaying = IsPlaying(Player);

	// The branches only hold a value while the flow is being traversed
	if (!bPlaying)
	{
		for (const FName& PropertyName : GetRuntimeOnlyProperties())
		{
			DetailBuilder.HideProperty(DetailBuilder.GetProperty(PropertyName));
		}
	}
	else
	{
		// Editing these from here does not re-run the setup they feed, so the panel would lie
		// about the running flow player. Hiding first keeps the re-added row from duplicating.
		IDetailCategoryBuilder& SetupCategory = DetailBuilder.EditCategory(TEXT("Setup"));
		for (const FName& PropertyName : GetStartupOnlyProperties())
		{
			TSharedRef<IPropertyHandle> Handle = DetailBuilder.GetProperty(PropertyName);
			DetailBuilder.HideProperty(Handle);
			SetupCategory.AddProperty(Handle).IsEnabled(false);
		}
	}

	// An "Important" category sorts in front of the component's own ones
	IDetailCategoryBuilder& IssueCategory = DetailBuilder.EditCategory(
		TEXT("ArticySetupIssues"), LOCTEXT("SetupCategory", "Articy Flow Player"), ECategoryPriority::Important);

	TSharedRef<FValidationCache> Validation = MakeShared<FValidationCache>();
	Validation->Player = Player;

	const auto GetMessage = [Validation]() -> FText
	{
		TArray<FText> Lines;
		for (const FArticyFlowPlayerSetupIssue& Issue : Validation->Get())
		{
			Lines.Add(Issue.Message);
		}

		return FText::Join(FText::FromString(TEXT("\n\n")), Lines);
	};

	const auto GetColor = [Validation]() -> FSlateColor
	{
		for (const FArticyFlowPlayerSetupIssue& Issue : Validation->Get())
		{
			if (Issue.Severity == EArticyFlowPlayerIssueSeverity::Error)
				return FSlateColor(FLinearColor(1.f, 0.33f, 0.25f));
		}

		return FSlateColor(FLinearColor(1.f, 0.8f, 0.25f));
	};

	const auto GetVisibility = [Validation]() -> EVisibility
	{
		return Validation->Get().Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
	};

	IssueCategory.AddCustomRow(LOCTEXT("SetupIssuesFilter", "Articy Flow Player Setup"))
		.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateLambda(GetVisibility)))
		.WholeRowContent()
		[
			SNew(SBorder)
			.Padding(FMargin(8.f, 6.f))
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda(GetMessage)))
				.ColorAndOpacity(TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateLambda(GetColor)))
			]
		];
}

/**
 * @brief Whether the panel is showing a flow player that is currently playing.
 *
 * @param Player The flow player being customized.
 * @return True if the flow player lives in a game world and has begun play.
 */
bool FArticyFlowPlayerCustomization::IsPlaying(const UArticyFlowPlayer* Player)
{
	if (!Player)
		return false;

	const UWorld* World = Player->GetWorld();
	return World && World->IsGameWorld() && Player->HasBegunPlay();
}

/**
 * @brief Properties that only hold a meaningful value while the flow player is playing.
 *
 * @return The names of the runtime-only properties.
 */
const TArray<FName>& FArticyFlowPlayerCustomization::GetRuntimeOnlyProperties()
{
	// Protected on the flow player, so they are named rather than looked up; Articy.Editor.FlowPlayerDetails pins the names
	static const TArray<FName> Properties = {
		FName(TEXT("AvailableBranches"))
	};

	return Properties;
}

/**
 * @brief Properties the flow player only reads when it begins play.
 *
 * @return The names of the startup-only properties.
 */
const TArray<FName>& FArticyFlowPlayerCustomization::GetStartupOnlyProperties()
{
	static const TArray<FName> Properties = {
		FName(TEXT("StartOn")),
		FName(TEXT("OverrideGV")),
		FName(TEXT("UserMethodsProvider"))
	};

	return Properties;
}

/**
 * @brief Creates an instance of the ArticyFlowPlayerCustomization.
 *
 * @return A shared pointer to a new instance of the ArticyFlowPlayerCustomization class.
 */
TSharedRef<IDetailCustomization> FArticyFlowPlayerCustomization::MakeInstance()
{
	return MakeShareable(new FArticyFlowPlayerCustomization());
}

#undef LOCTEXT_NAMESPACE
