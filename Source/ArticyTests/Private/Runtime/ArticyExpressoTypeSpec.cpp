//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyExpressoScripts.h"

#if WITH_AUTOMATION_TESTS

// ExpressoType is the value type every generated Expresso condition/instruction runs on,
// so its conversions and operators define what an authored script actually evaluates to.
// Several operators deliberately reinterpret their operand (a bool + bool is an OR, a
// string minus anything is a no-op); those are pinned here as the contract, since the
// generated code relies on them.
//
// Note on the operand types used below: the conversion operators and comparisons ensure()
// on the type they accept, and a failed ensure is reported as a test error. Mixed
// combinations that trip an ensure are therefore left to the source, not exercised here.
BEGIN_DEFINE_SPEC(FArticyExpressoTypeSpec, "Articy.Runtime.ExpressoType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyExpressoTypeSpec)

void FArticyExpressoTypeSpec::Define()
{
	Describe("construction", [this]()
	{
		It("tags a bool, int, float and string with its type", [this]()
		{
			TestEqual(TEXT("bool"), (int32)ExpressoType(true).Type, (int32)ExpressoType::Bool);
			TestEqual(TEXT("int"), (int32)ExpressoType(int64(7)).Type, (int32)ExpressoType::Int);
			TestEqual(TEXT("float"), (int32)ExpressoType(1.5).Type, (int32)ExpressoType::Float);
			TestEqual(TEXT("string"), (int32)ExpressoType(FString(TEXT("x"))).Type, (int32)ExpressoType::String);
		});

		It("defaults to Undefined", [this]()
		{
			TestEqual(TEXT("default type"), (int32)ExpressoType().Type, (int32)ExpressoType::Undefined);
		});

		It("widens every narrower integer to Int", [this]()
		{
			TestEqual(TEXT("int8"), ExpressoType(int8(-5)).GetInt(), int64(-5));
			TestEqual(TEXT("int16"), ExpressoType(int16(-300)).GetInt(), int64(-300));
			TestEqual(TEXT("int32"), ExpressoType(int32(-70000)).GetInt(), int64(-70000));
			TestEqual(TEXT("uint8"), ExpressoType(uint8(200)).GetInt(), int64(200));
			TestEqual(TEXT("uint16"), ExpressoType(uint16(40000)).GetInt(), int64(40000));
			TestEqual(TEXT("uint32"), ExpressoType(uint32(3000000000u)).GetInt(), int64(3000000000ll));
		});

		It("promotes a float to Float and FText/FName to String", [this]()
		{
			const ExpressoType FromFloat{ 2.5f };
			TestEqual(TEXT("float type"), (int32)FromFloat.Type, (int32)ExpressoType::Float);
			TestEqual(TEXT("float value"), FromFloat.GetFloat(), 2.5);

			TestEqual(TEXT("from FText"), ExpressoType(FText::FromString(TEXT("hi"))).GetString(), FString(TEXT("hi")));
			TestEqual(TEXT("from FName"), ExpressoType(FName(TEXT("hi"))).GetString(), FString(TEXT("hi")));
		});
	});

	// The compound "<id>_<cloneId>" string is how an object travels through a script:
	// getObj() hands one out, a slot/variable stores it, and getObj()/operator int64
	// have to read it back. Both halves are decimal, not hex.
	Describe("object handles", [this]()
	{
		It("encodes an id as the decimal <id>_<cloneId> string", [this]()
		{
			const ExpressoType Handle{ FArticyId{ uint64(0x0100000000000123) } };
			TestEqual(TEXT("type"), (int32)Handle.Type, (int32)ExpressoType::String);
			TestEqual(TEXT("handle"), Handle.GetString(),
				FString::Printf(TEXT("%llu_0"), uint64(0x0100000000000123)));
		});

		It("encodes a null id and a null object as the placeholder 0_0", [this]()
		{
			TestEqual(TEXT("null id"), ExpressoType(FArticyId{}).GetString(), FString(TEXT("0_0")));
			TestEqual(TEXT("null object"), ExpressoType((const UArticyPrimitive*)nullptr).GetString(),
				FString(TEXT("0_0")));
		});

		It("reads the id half back out through int64", [this]()
		{
			const uint64 Raw = 0x0100000000000123;
			const ExpressoType Handle{ FArticyId{ Raw } };
			TestEqual(TEXT("int64 of handle"), int64(Handle), int64(Raw));
		});

		It("round-trips an FArticyId through a handle", [this]()
		{
			// The conversion back is explicit, so a handle can never silently decay into
			// an id where an overload expected something else.
			const FArticyId Original{ uint64(0x0100000000004567) };
			const FArticyId RoundTripped = static_cast<FArticyId>(ExpressoType{ Original });
			TestEqual(TEXT("id survives"), (int64)RoundTripped.Get(), (int64)Original.Get());
		});

		It("reads the placeholder handle back as a null id", [this]()
		{
			const FArticyId FromNull = static_cast<FArticyId>(ExpressoType{ FArticyId{} });
			TestTrue(TEXT("null id"), FromNull.IsNull());
		});
	});

	Describe("conversions", [this]()
	{
		It("truncates a float to int64 and widens an int to double", [this]()
		{
			TestEqual(TEXT("float -> int"), int64(ExpressoType(2.9)), int64(2));
			TestEqual(TEXT("negative float -> int"), int64(ExpressoType(-2.9)), int64(-2));
			TestEqual(TEXT("int -> double"), double(ExpressoType(int64(3))), 3.0);
		});

		It("returns the raw value through bool and FString", [this]()
		{
			TestTrue(TEXT("bool"), bool(ExpressoType(true)));
			TestFalse(TEXT("bool"), bool(ExpressoType(false)));
			TestEqual(TEXT("string"), FString(ExpressoType(FString(TEXT("abc")))), FString(TEXT("abc")));
		});
	});

	Describe("ToString", [this]()
	{
		It("prints a string, an int and a float", [this]()
		{
			TestEqual(TEXT("string"), ExpressoType(FString(TEXT("abc"))).ToString(), FString(TEXT("abc")));
			TestEqual(TEXT("int"), ExpressoType(int64(42)).ToString(), FString(TEXT("42")));
			TestEqual(TEXT("float"), ExpressoType(2.5).ToString(), FString::SanitizeFloat(2.5));
		});
	});

	Describe("negation", [this]()
	{
		It("negates a number, inverts a bool and empties a string", [this]()
		{
			TestEqual(TEXT("int"), (-ExpressoType(int64(5))).GetInt(), int64(-5));
			TestEqual(TEXT("float"), (-ExpressoType(1.5)).GetFloat(), -1.5);
			// A bool has no additive inverse, so unary minus is defined as a logical NOT.
			TestFalse(TEXT("bool"), (-ExpressoType(true)).GetBool());
			// ...and a string collapses to empty, which is what makes "a" - "b" == "a".
			TestEqual(TEXT("string"), (-ExpressoType(FString(TEXT("abc")))).GetString(), FString());
		});
	});

	Describe("comparison", [this]()
	{
		It("compares two ints", [this]()
		{
			TestTrue(TEXT("=="), ExpressoType(int64(3)) == ExpressoType(int64(3)));
			TestTrue(TEXT("!="), ExpressoType(int64(3)) != ExpressoType(int64(4)));
			TestTrue(TEXT("<"), ExpressoType(int64(3)) < ExpressoType(int64(4)));
			TestTrue(TEXT(">"), ExpressoType(int64(4)) > ExpressoType(int64(3)));
			TestTrue(TEXT("<="), ExpressoType(int64(3)) <= ExpressoType(int64(3)));
			TestTrue(TEXT(">="), ExpressoType(int64(3)) >= ExpressoType(int64(3)));
		});

		It("compares an int against a float in both directions", [this]()
		{
			TestTrue(TEXT("int < float"), ExpressoType(int64(3)) < ExpressoType(3.5));
			TestTrue(TEXT("float > int"), ExpressoType(3.5) > ExpressoType(int64(3)));
			TestTrue(TEXT("int == float"), ExpressoType(int64(3)) == ExpressoType(3.0));
			TestTrue(TEXT("float == int"), ExpressoType(3.0) == ExpressoType(int64(3)));
		});

		It("compares strings lexically", [this]()
		{
			TestTrue(TEXT("=="), ExpressoType(FString(TEXT("a"))) == ExpressoType(FString(TEXT("a"))));
			TestTrue(TEXT("<"), ExpressoType(FString(TEXT("a"))) < ExpressoType(FString(TEXT("b"))));
			TestTrue(TEXT(">"), ExpressoType(FString(TEXT("b"))) > ExpressoType(FString(TEXT("a"))));
		});

		It("compares bools with false ordered before true", [this]()
		{
			TestTrue(TEXT("=="), ExpressoType(true) == ExpressoType(true));
			TestTrue(TEXT("!="), ExpressoType(true) != ExpressoType(false));
			TestTrue(TEXT("false < true"), ExpressoType(false) < ExpressoType(true));
			TestFalse(TEXT("true not < false"), ExpressoType(true) < ExpressoType(false));
		});

		It("lets two object handles be compared for identity", [this]()
		{
			// How a script's "obj == someSlot" ends up being evaluated.
			const ExpressoType A{ FArticyId{ uint64(0x0100000000000001) } };
			const ExpressoType B{ FArticyId{ uint64(0x0100000000000001) } };
			const ExpressoType C{ FArticyId{ uint64(0x0100000000000002) } };
			TestTrue(TEXT("same id"), A == B);
			TestTrue(TEXT("different id"), A != C);
		});
	});

	Describe("logical operators", [this]()
	{
		It("ands, ors and xors two bools", [this]()
		{
			TestTrue(TEXT("&&"), (ExpressoType(true) && ExpressoType(true)).GetBool());
			TestFalse(TEXT("&&"), (ExpressoType(true) && ExpressoType(false)).GetBool());
			TestTrue(TEXT("||"), (ExpressoType(false) || ExpressoType(true)).GetBool());
			TestFalse(TEXT("||"), (ExpressoType(false) || ExpressoType(false)).GetBool());
			TestTrue(TEXT("^"), (ExpressoType(true) ^ ExpressoType(false)).GetBool());
			TestFalse(TEXT("^"), (ExpressoType(true) ^ ExpressoType(true)).GetBool());
		});

		It("treats ints as truthy and returns a bool result", [this]()
		{
			const ExpressoType AndResult = ExpressoType(int64(2)) && ExpressoType(int64(3));
			TestEqual(TEXT("result type"), (int32)AndResult.Type, (int32)ExpressoType::Bool);
			TestTrue(TEXT("2 && 3"), AndResult.GetBool());
			TestFalse(TEXT("2 && 0"), (ExpressoType(int64(2)) && ExpressoType(int64(0))).GetBool());
			TestTrue(TEXT("0 || 3"), (ExpressoType(int64(0)) || ExpressoType(int64(3))).GetBool());
		});

		It("xors ints bitwise, not logically", [this]()
		{
			// ^ on Int is the C++ bitwise xor, so it yields an Int, not a Bool.
			const ExpressoType Result = ExpressoType(int64(6)) ^ ExpressoType(int64(3));
			TestEqual(TEXT("result type"), (int32)Result.Type, (int32)ExpressoType::Int);
			TestEqual(TEXT("6 ^ 3"), Result.GetInt(), int64(5));
		});

		It("yields Undefined for operands it cannot combine", [this]()
		{
			TestEqual(TEXT("string &&"),
				(int32)(ExpressoType(FString(TEXT("a"))) && ExpressoType(FString(TEXT("b")))).Type,
				(int32)ExpressoType::Undefined);
			TestEqual(TEXT("float ^"),
				(int32)(ExpressoType(1.5) ^ ExpressoType(2.5)).Type,
				(int32)ExpressoType::Undefined);
		});
	});

	Describe("arithmetic", [this]()
	{
		It("adds, subtracts, multiplies, divides and mods ints", [this]()
		{
			TestEqual(TEXT("+"), (ExpressoType(int64(5)) + ExpressoType(int64(3))).GetInt(), int64(8));
			TestEqual(TEXT("-"), (ExpressoType(int64(5)) - ExpressoType(int64(3))).GetInt(), int64(2));
			TestEqual(TEXT("*"), (ExpressoType(int64(5)) * ExpressoType(int64(3))).GetInt(), int64(15));
			TestEqual(TEXT("/"), (ExpressoType(int64(7)) / ExpressoType(int64(2))).GetInt(), int64(3));
			TestEqual(TEXT("%"), (ExpressoType(int64(7)) % ExpressoType(int64(3))).GetInt(), int64(1));
		});

		It("adds, subtracts, multiplies and divides floats", [this]()
		{
			TestEqual(TEXT("+"), (ExpressoType(1.5) + ExpressoType(2.25)).GetFloat(), 3.75);
			TestEqual(TEXT("-"), (ExpressoType(3.0) - ExpressoType(1.25)).GetFloat(), 1.75);
			TestEqual(TEXT("*"), (ExpressoType(1.5) * ExpressoType(2.0)).GetFloat(), 3.0);
			TestEqual(TEXT("/"), (ExpressoType(3.0) / ExpressoType(2.0)).GetFloat(), 1.5);
		});

		It("concatenates strings with +", [this]()
		{
			TestEqual(TEXT("concat"),
				(ExpressoType(FString(TEXT("ab"))) + ExpressoType(FString(TEXT("cd")))).GetString(),
				FString(TEXT("abcd")));
		});

		It("leaves a string unchanged when subtracting from it", [this]()
		{
			// Subtraction is defined as a + (-b), and -string is the empty string, so
			// string subtraction is a no-op rather than an error.
			TestEqual(TEXT("subtract"),
				(ExpressoType(FString(TEXT("abc"))) - ExpressoType(FString(TEXT("b")))).GetString(),
				FString(TEXT("abc")));
		});

		It("folds bool arithmetic into logic", [this]()
		{
			// + is OR and * is AND for bools; this is what "flagA + flagB" compiles to.
			TestTrue(TEXT("false + true"), (ExpressoType(false) + ExpressoType(true)).GetBool());
			TestFalse(TEXT("false + false"), (ExpressoType(false) + ExpressoType(false)).GetBool());
			TestTrue(TEXT("true * true"), (ExpressoType(true) * ExpressoType(true)).GetBool());
			TestFalse(TEXT("true * false"), (ExpressoType(true) * ExpressoType(false)).GetBool());
		});

		It("yields Undefined for arithmetic it does not define", [this]()
		{
			TestEqual(TEXT("string *"),
				(int32)(ExpressoType(FString(TEXT("a"))) * ExpressoType(FString(TEXT("b")))).Type,
				(int32)ExpressoType::Undefined);
			TestEqual(TEXT("bool /"),
				(int32)(ExpressoType(true) / ExpressoType(true)).Type,
				(int32)ExpressoType::Undefined);
			TestEqual(TEXT("undefined +"),
				(int32)(ExpressoType() + ExpressoType(int64(1))).Type,
				(int32)ExpressoType::Undefined);
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
