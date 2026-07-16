//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyHelpers.h"

#if WITH_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FArticyHelpersSpec, "Articy.Runtime.Helpers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyHelpersSpec)

void FArticyHelpersSpec::Define()
{
	Describe("HexToUint64", [this]()
	{
		It("parses a 0x-prefixed hex string", [this]()
		{
			TestEqual(TEXT("0x1234"), ArticyHelpers::HexToUint64(TEXT("0x1234")), static_cast<uint64>(0x1234));
		});

		It("returns 0 for an empty string", [this]()
		{
			TestEqual(TEXT("empty"), ArticyHelpers::HexToUint64(TEXT("")), static_cast<uint64>(0));
		});
	});

	Describe("Uint64ToHex", [this]()
	{
		It("formats a value with a 0x prefix", [this]()
		{
			TestEqual(TEXT("0x1234"), ArticyHelpers::Uint64ToHex(0x1234), FString(TEXT("0x1234")));
		});

		It("round-trips through HexToUint64", [this]()
		{
			const uint64 Id = 0x0123456789ABCDEFull;
			TestEqual(TEXT("round-trip"), ArticyHelpers::HexToUint64(ArticyHelpers::Uint64ToHex(Id)), Id);
		});
	});

	Describe("Uint64ToObjectString", [this]()
	{
		It("appends the _0 suffix used for generated asset names", [this]()
		{
			TestEqual(TEXT("decimal + suffix"), ArticyHelpers::Uint64ToObjectString(4096), FString(TEXT("4096_0")));
		});
	});

	Describe("ParseFVector2DFromJson", [this]()
	{
		It("reads the x and y fields", [this]()
		{
			TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
			Obj->SetNumberField(TEXT("x"), 3.0);
			Obj->SetNumberField(TEXT("y"), 4.0);

			const FVector2D Result = ArticyHelpers::ParseFVector2DFromJson(MakeShared<FJsonValueObject>(Obj));
			// FVector2D components are float on UE4 and double on UE5; compare as double.
			TestEqual(TEXT("x"), static_cast<double>(Result.X), 3.0);
			TestEqual(TEXT("y"), static_cast<double>(Result.Y), 4.0);
		});

		It("returns a zero vector for invalid json", [this]()
		{
			const FVector2D Result = ArticyHelpers::ParseFVector2DFromJson(nullptr);
			TestEqual(TEXT("zero"), Result, FVector2D::ZeroVector);
		});
	});

	Describe("ParseColorFromJson", [this]()
	{
		It("defaults alpha to 1.0 when not provided", [this]()
		{
			TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
			Obj->SetNumberField(TEXT("r"), 0.5);
			Obj->SetNumberField(TEXT("g"), 0.25);
			Obj->SetNumberField(TEXT("b"), 0.1);

			const FLinearColor Result = ArticyHelpers::ParseColorFromJson(MakeShared<FJsonValueObject>(Obj));
			TestEqual(TEXT("r"), Result.R, 0.5f);
			TestEqual(TEXT("alpha default"), Result.A, 1.0f);
		});
	});

	Describe("ParseFMatrixFromJson", [this]()
	{
		It("maps a 3x3 articy matrix into a 4x4 with the translation in the last row", [this]()
		{
			// Articy exports a 9-element row-major 3x3 matrix.
			TArray<TSharedPtr<FJsonValue>> Values;
			for (int32 i = 1; i <= 9; ++i)
			{
				Values.Add(MakeShared<FJsonValueNumber>(i));
			}

			const FMatrix M = ArticyHelpers::ParseFMatrixFromJson(MakeShared<FJsonValueArray>(Values));
			// Rotation/scale rows.
			TestEqual(TEXT("m00"), static_cast<double>(M.M[0][0]), 1.0);
			TestEqual(TEXT("m11"), static_cast<double>(M.M[1][1]), 5.0);
			TestEqual(TEXT("m22"), static_cast<double>(M.M[2][2]), 9.0);
			// Translation (articy elements 7 & 8) moved to the last row.
			TestEqual(TEXT("tx"), static_cast<double>(M.M[3][0]), 7.0);
			TestEqual(TEXT("ty"), static_cast<double>(M.M[3][1]), 8.0);
			TestEqual(TEXT("m33"), static_cast<double>(M.M[3][3]), 1.0);
		});

		It("returns identity for invalid json", [this]()
		{
			const FMatrix M = ArticyHelpers::ParseFMatrixFromJson(nullptr);
			TestEqual(TEXT("m00"), static_cast<double>(M.M[0][0]), 1.0);
			TestEqual(TEXT("m01"), static_cast<double>(M.M[0][1]), 0.0);
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
