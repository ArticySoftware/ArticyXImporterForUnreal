//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyGlobalVariables.h"

#if WITH_AUTOMATION_TESTS

// FArticyGvName is what turns an authored "Namespace.Variable" into the two halves the
// global-variable store is keyed by. Every [Namespace.Variable] text token and every
// generated GV access funnels through it, and both of its setters silently do nothing on
// input they cannot split - which is the shape a lookup failure takes further downstream.
BEGIN_DEFINE_SPEC(FArticyGvNameSpec, "Articy.Runtime.GvName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyGvNameSpec)

void FArticyGvNameSpec::Define()
{
	Describe("SetByFullName", [this]()
	{
		It("splits a dotted name into namespace and variable", [this]()
		{
			FArticyGvName Name{ FName(TEXT("GameState.IsDead")) };
			TestEqual(TEXT("namespace"), Name.GetNamespace().ToString(), FString(TEXT("GameState")));
			TestEqual(TEXT("variable"), Name.GetVariable().ToString(), FString(TEXT("IsDead")));
			TestEqual(TEXT("full name"), Name.GetFullName().ToString(), FString(TEXT("GameState.IsDead")));
		});

		It("splits at the first dot, so extra dots stay in the variable half", [this]()
		{
			FArticyGvName Name{ FName(TEXT("A.B.C")) };
			TestEqual(TEXT("namespace"), Name.GetNamespace().ToString(), FString(TEXT("A")));
			TestEqual(TEXT("variable"), Name.GetVariable().ToString(), FString(TEXT("B.C")));
		});

		It("leaves everything unset for a name without a dot", [this]()
		{
			// No dot means no split, and the setter then stores nothing at all - not even
			// FullName. A bare variable name is therefore never resolvable.
			FArticyGvName Name{ FName(TEXT("IsDead")) };
			TestTrue(TEXT("namespace unset"), Name.Namespace.IsNone());
			TestTrue(TEXT("variable unset"), Name.Variable.IsNone());
			TestTrue(TEXT("full name unset"), Name.FullName.IsNone());
		});
	});

	Describe("SetByNamespaceAndVariable", [this]()
	{
		It("joins the two halves into a dotted full name", [this]()
		{
			FArticyGvName Name{ FName(TEXT("GameState")), FName(TEXT("IsDead")) };
			TestEqual(TEXT("full name"), Name.GetFullName().ToString(), FString(TEXT("GameState.IsDead")));
			TestEqual(TEXT("namespace"), Name.GetNamespace().ToString(), FString(TEXT("GameState")));
			TestEqual(TEXT("variable"), Name.GetVariable().ToString(), FString(TEXT("IsDead")));
		});

		It("stores nothing when either half is None", [this]()
		{
			FArticyGvName NoNamespace{ FName(), FName(TEXT("IsDead")) };
			TestTrue(TEXT("no namespace -> nothing set"),
				NoNamespace.FullName.IsNone() && NoNamespace.Variable.IsNone());

			FArticyGvName NoVariable{ FName(TEXT("GameState")), FName() };
			TestTrue(TEXT("no variable -> nothing set"),
				NoVariable.FullName.IsNone() && NoVariable.Namespace.IsNone());
		});
	});

	Describe("lazy getters", [this]()
	{
		It("derives the missing halves from FullName on first access", [this]()
		{
			// The struct is a USTRUCT that can arrive from a Blueprint pin with only
			// FullName filled in; the getters have to back-fill the rest.
			FArticyGvName Name;
			Name.FullName = FName(TEXT("GameState.Score"));
			TestEqual(TEXT("namespace"), Name.GetNamespace().ToString(), FString(TEXT("GameState")));
			TestEqual(TEXT("variable"), Name.GetVariable().ToString(), FString(TEXT("Score")));
		});

		It("derives FullName from the two halves on first access", [this]()
		{
			FArticyGvName Name;
			Name.Namespace = FName(TEXT("GameState"));
			Name.Variable = FName(TEXT("Score"));
			TestEqual(TEXT("full name"), Name.GetFullName().ToString(), FString(TEXT("GameState.Score")));
		});

		It("keeps an already-set value instead of re-deriving it", [this]()
		{
			FArticyGvName Name{ FName(TEXT("GameState")), FName(TEXT("Score")) };
			Name.FullName = FName(TEXT("Stale.Value"));
			// GetNamespace/GetVariable short-circuit while they hold a value, so a
			// mismatching FullName does not overwrite them.
			TestEqual(TEXT("namespace"), Name.GetNamespace().ToString(), FString(TEXT("GameState")));
			TestEqual(TEXT("variable"), Name.GetVariable().ToString(), FString(TEXT("Score")));
		});

		It("reports None for a default-constructed name", [this]()
		{
			FArticyGvName Name;
			TestTrue(TEXT("namespace"), Name.GetNamespace().IsNone());
			TestTrue(TEXT("variable"), Name.GetVariable().IsNone());
			TestTrue(TEXT("full name"), Name.GetFullName().IsNone());
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
