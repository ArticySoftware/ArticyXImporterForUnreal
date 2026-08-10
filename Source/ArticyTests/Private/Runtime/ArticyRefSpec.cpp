//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyRef.h"

#if WITH_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FArticyRefSpec, "Articy.Runtime.Ref",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyRefSpec)

void FArticyRefSpec::Define()
{
	Describe("SetId / GetId", [this]()
	{
		It("stores and returns the id", [this]()
		{
			FArticyRef Ref;
			Ref.SetId(FArticyId(static_cast<uint64>(0x0000000100000002ull)));
			TestEqual(TEXT("low"), Ref.GetId().Low, 2);
			TestEqual(TEXT("high"), Ref.GetId().High, 1);
		});
	});

	Describe("GetEffectiveCloneId", [this]()
	{
		It("is 0 when referencing the base object regardless of CloneId", [this]()
		{
			FArticyRef Ref;
			Ref.bReferenceBaseObject = true;
			Ref.CloneId = 7;
			TestEqual(TEXT("base -> 0"), static_cast<int32>(Ref.GetEffectiveCloneId()), 0);
		});

		It("is the CloneId when not referencing the base object", [this]()
		{
			FArticyRef Ref;
			Ref.bReferenceBaseObject = false;
			Ref.CloneId = 7;
			TestEqual(TEXT("clone"), static_cast<int32>(Ref.GetEffectiveCloneId()), 7);
		});
	});

	Describe("InitFromString", [this]()
	{
		It("round-trips through ToString", [this]()
		{
			FArticyRef Ref;
			Ref.bReferenceBaseObject = false;
			Ref.CloneId = 3;
			Ref.SetId(FArticyId(static_cast<uint64>(0x0000000100000002ull)));

			FArticyRef Parsed;
			const FArticyRef::EStringInitResult Result = Parsed.InitFromString(Ref.ToString());

			TestTrue(TEXT("all set"), Result == FArticyRef::AllSet);
			TestEqual(TEXT("low"), Parsed.GetId().Low, 2);
			TestEqual(TEXT("high"), Parsed.GetId().High, 1);
			TestEqual(TEXT("clone"), Parsed.CloneId, 3);
			TestFalse(TEXT("not base"), Parsed.bReferenceBaseObject);
		});

		It("returns NoneSet for an unparseable string", [this]()
		{
			FArticyRef Ref;
			TestTrue(TEXT("none set"), Ref.InitFromString(TEXT("nonsense")) == FArticyRef::NoneSet);
		});
	});

	Describe("equality", [this]()
	{
		It("compares effective clone ids (base-object clones are equal)", [this]()
		{
			FArticyRef A;
			A.bReferenceBaseObject = true;
			A.CloneId = 1;
			A.SetId(FArticyId(static_cast<uint64>(5)));

			FArticyRef B;
			B.bReferenceBaseObject = true;
			B.CloneId = 2;
			B.SetId(FArticyId(static_cast<uint64>(5)));

			TestTrue(TEXT("effective equal"), A == B);
			TestFalse(TEXT("raw differs"), A.MatchesRaw(B));
			TestTrue(TEXT("hash equal"), GetTypeHash(A) == GetTypeHash(B));
		});

		It("distinguishes different effective clone ids", [this]()
		{
			FArticyRef C;
			C.bReferenceBaseObject = false;
			C.CloneId = 1;
			C.SetId(FArticyId(static_cast<uint64>(5)));

			FArticyRef D;
			D.bReferenceBaseObject = false;
			D.CloneId = 2;
			D.SetId(FArticyId(static_cast<uint64>(5)));

			TestTrue(TEXT("effective differ"), C != D);
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
