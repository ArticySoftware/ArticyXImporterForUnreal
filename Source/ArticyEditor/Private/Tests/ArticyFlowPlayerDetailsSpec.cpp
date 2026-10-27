//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyEditorStyle.h"
#include "ArticyFlowPlayer.h"
#include "Customizations/Details/ArticyFlowPlayerCustomization.h"
#include "Styling/ISlateStyle.h"

#if WITH_AUTOMATION_TESTS

// The flow player details customization reaches the properties it hides or disables by name,
// because they are protected on the component. A rename would silently stop doing anything,
// so the names are pinned here, together with the class icons the component is drawn with.
BEGIN_DEFINE_SPEC(FArticyFlowPlayerDetailsSpec, "Articy.Editor.FlowPlayerDetails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyFlowPlayerDetailsSpec)

void FArticyFlowPlayerDetailsSpec::Define()
{
	Describe("Customized properties", [this]()
	{
		It("names runtime-only properties that exist on the flow player", [this]()
		{
			const TArray<FName>& Properties = FArticyFlowPlayerCustomization::GetRuntimeOnlyProperties();
			TestTrue(TEXT("there are runtime-only properties"), Properties.Num() > 0);

			for (const FName& Name : Properties)
			{
				TestNotNull(*FString::Printf(TEXT("UArticyFlowPlayer::%s"), *Name.ToString()),
					UArticyFlowPlayer::StaticClass()->FindPropertyByName(Name));
			}
		});

		It("names startup-only properties that exist on the flow player", [this]()
		{
			const TArray<FName>& Properties = FArticyFlowPlayerCustomization::GetStartupOnlyProperties();
			TestTrue(TEXT("there are startup-only properties"), Properties.Num() > 0);

			for (const FName& Name : Properties)
			{
				TestNotNull(*FString::Printf(TEXT("UArticyFlowPlayer::%s"), *Name.ToString()),
					UArticyFlowPlayer::StaticClass()->FindPropertyByName(Name));
			}
		});
	});

	Describe("IsPlaying", [this]()
	{
		It("is false without a flow player", [this]()
		{
			TestFalse(TEXT("no player"), FArticyFlowPlayerCustomization::IsPlaying(nullptr));
		});

		It("is false for a flow player that is not in a game world", [this]()
		{
			UArticyFlowPlayer* Player = NewObject<UArticyFlowPlayer>(GetTransientPackage());
			TestFalse(TEXT("not playing"), FArticyFlowPlayerCustomization::IsPlaying(Player));
		});
	});

	Describe("Class icons", [this]()
	{
		It("are registered for the flow player and the flow debugger", [this]()
		{
			const ISlateStyle& Style = FArticyEditorStyle::Get();

			TestNotNull(TEXT("ClassIcon.ArticyFlowPlayer"), Style.GetOptionalBrush(TEXT("ClassIcon.ArticyFlowPlayer"), nullptr, nullptr));
			TestNotNull(TEXT("ClassThumbnail.ArticyFlowPlayer"), Style.GetOptionalBrush(TEXT("ClassThumbnail.ArticyFlowPlayer"), nullptr, nullptr));
			TestNotNull(TEXT("ClassIcon.ArticyFlowDebugger"), Style.GetOptionalBrush(TEXT("ClassIcon.ArticyFlowDebugger"), nullptr, nullptr));
			TestNotNull(TEXT("ClassThumbnail.ArticyFlowDebugger"), Style.GetOptionalBrush(TEXT("ClassThumbnail.ArticyFlowDebugger"), nullptr, nullptr));
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
