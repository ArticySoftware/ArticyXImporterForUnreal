//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyGlobalVariables.h"

#if WITH_AUTOMATION_TESTS

// Exercises the runtime global-variable value mechanics at shadow level 0.
// (Shadow push/pop is driven privately by UArticyFlowPlayer and can't be
// invoked in isolation, so it is out of scope for these unit tests.)
BEGIN_DEFINE_SPEC(FArticyGlobalVariablesSpec, "Articy.Runtime.GlobalVariables",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	UArticyGlobalVariables* Store = nullptr;
	UArticyBaseVariableSet* Set = nullptr;
END_DEFINE_SPEC(FArticyGlobalVariablesSpec)

void FArticyGlobalVariablesSpec::Define()
{
	BeforeEach([this]()
	{
		Store = NewObject<UArticyGlobalVariables>();
		Set = NewObject<UArticyBaseVariableSet>();
	});

	Describe("UArticyInt", [this]()
	{
		It("initializes, reads, sets and supports arithmetic", [this]()
		{
			UArticyInt* Var = NewObject<UArticyInt>();
			Var->Init<UArticyInt>(Set, Store, TEXT("GameState.score"), 10);

			TestEqual(TEXT("initial"), Var->Get(), 10);
			TestEqual(TEXT("gv name"), Var->GetGVName().ToString(), FString(TEXT("GameState.score")));

			Var->Set(25);
			TestEqual(TEXT("set"), Var->Get(), 25);

			*Var += 5;
			TestEqual(TEXT("add-assign"), Var->Get(), 30);
		});
	});

	Describe("UArticyBool", [this]()
	{
		It("initializes, reads and sets a boolean", [this]()
		{
			UArticyBool* Var = NewObject<UArticyBool>();
			Var->Init<UArticyBool>(Set, Store, TEXT("GameState.flag"), false);

			TestFalse(TEXT("initial"), Var->Get());
			*Var = true;
			TestTrue(TEXT("set"), Var->Get());
		});
	});

	Describe("UArticyString", [this]()
	{
		It("initializes, reads and sets a string", [this]()
		{
			UArticyString* Var = NewObject<UArticyString>();
			Var->Init<UArticyString>(Set, Store, TEXT("GameState.name"), TEXT("Bob"));

			TestEqual(TEXT("initial"), Var->Get(), FString(TEXT("Bob")));
			*Var = FString(TEXT("Alice"));
			TestEqual(TEXT("set"), Var->Get(), FString(TEXT("Alice")));
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
