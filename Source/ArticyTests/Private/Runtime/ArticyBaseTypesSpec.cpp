//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyBaseTypes.h"

#if WITH_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FArticyIdSpec, "Articy.Runtime.Id",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyIdSpec)

void FArticyIdSpec::Define()
{
	Describe("uint64 conversion", [this]()
	{
		It("splits a value into Low/High and reconstructs it", [this]()
		{
			const FArticyId Id = static_cast<uint64>(0x0000000100000002ull);
			TestEqual(TEXT("low"), Id.Low, 2);
			TestEqual(TEXT("high"), Id.High, 1);
			TestTrue(TEXT("round-trip"), Id.Get() == 0x0000000100000002ull);
		});

		It("parses a hex string via assignment", [this]()
		{
			const FArticyId Id = FString(TEXT("0x100000002"));
			TestTrue(TEXT("hex value"), Id.Get() == 0x100000002ull);
		});
	});

	Describe("IsNull", [this]()
	{
		It("is true for a default id and false otherwise", [this]()
		{
			TestTrue(TEXT("default is null"), FArticyId().IsNull());
			TestFalse(TEXT("non-zero is not null"), FArticyId(static_cast<uint64>(5)).IsNull());
		});
	});

	Describe("string conversion", [this]()
	{
		It("formats ToString and ToAssetFriendlyString", [this]()
		{
			const FArticyId Id = static_cast<uint64>(0x0000000100000002ull);
			TestEqual(TEXT("ToString"), Id.ToString(), FString(TEXT("(Low=2, High=1)")));
			TestEqual(TEXT("asset friendly"), Id.ToAssetFriendlyString(), FString(TEXT("2_1")));
		});

		It("round-trips through InitFromString", [this]()
		{
			const FArticyId Original = static_cast<uint64>(0x0000000100000002ull);
			FArticyId Parsed;
			TestTrue(TEXT("parse ok"), Parsed.InitFromString(Original.ToString()));
			TestEqual(TEXT("low"), Parsed.Low, 2);
			TestEqual(TEXT("high"), Parsed.High, 1);
		});

		It("fails InitFromString on an unparseable string", [this]()
		{
			FArticyId Parsed;
			TestFalse(TEXT("garbage"), Parsed.InitFromString(TEXT("not an id")));
		});
	});

	Describe("GetTypeHash", [this]()
	{
		It("hashes as Low xor High", [this]()
		{
			const FArticyId Id = static_cast<uint64>(0x0000000100000002ull);
			TestEqual(TEXT("hash"), GetTypeHash(Id), static_cast<uint32>(2 ^ 1));
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
