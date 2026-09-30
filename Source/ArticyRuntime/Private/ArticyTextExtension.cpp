//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "ArticyTextExtension.h"
#include "ArticyDatabase.h"
#include "ArticyGlobalVariables.h"
#include "ArticyTypeSystem.h"
#include "ArticyHelpers.h"
#include "ArticyObject.h"
#include "ArticyScriptFragment.h"
#include "ArticyExpressoScripts.h"
#include "ArticyPluginSettings.h"
#include "Interfaces/ArticyObjectWithSpeaker.h"
#include "Engine/Engine.h"
#include "UObject/UnrealType.h"
#include "UObject/TextProperty.h"

namespace
{
	// Nested tokens and localized values can refer back to themselves; stop somewhere sane.
	constexpr int32 MaxResolveDepth = 16;

	bool IsEscapable(TCHAR C)
	{
		return C == TEXT('[') || C == TEXT(']') || C == TEXT('\\');
	}

	bool IsDigitString(const FString& S)
	{
		if (S.IsEmpty())
			return false;
		for (const TCHAR C : S)
		{
			if (!FChar::IsDigit(C))
				return false;
		}
		return true;
	}

	// [+-]digits[.digits]
	bool IsNumericLiteral(const FString& S, bool& bOutIsFloat)
	{
		bOutIsFloat = false;
		int32 I = 0;
		if (I < S.Len() && (S[I] == TEXT('+') || S[I] == TEXT('-')))
			++I;
		int32 Digits = 0;
		while (I < S.Len() && FChar::IsDigit(S[I])) { ++I; ++Digits; }
		if (Digits == 0)
			return false;
		if (I < S.Len() && S[I] == TEXT('.'))
		{
			bOutIsFloat = true;
			++I;
			int32 Fraction = 0;
			while (I < S.Len() && FChar::IsDigit(S[I])) { ++I; ++Fraction; }
			if (Fraction == 0)
				return false;
		}
		return I == S.Len();
	}

	// The "<id>_<cloneId>" string an object travels through scripts and string variables as.
	bool IsObjectRepresentation(const FString& S, uint64& OutId, int32& OutCloneId)
	{
		FString IdPart, ClonePart;
		if (!S.Split(TEXT("_"), &IdPart, &ClonePart, ESearchCase::CaseSensitive, ESearchDir::FromEnd))
			return false;
		if (!IsDigitString(IdPart) || !IsDigitString(ClonePart))
			return false;
		OutId = FCString::Strtoui64(*IdPart, nullptr, 10);
		OutCloneId = FCString::Atoi(*ClonePart);
		return true;
	}

	bool IsIdentifier(const FString& S)
	{
		if (S.IsEmpty() || !(FChar::IsAlpha(S[0]) || S[0] == TEXT('_')))
			return false;
		for (const TCHAR C : S)
		{
			if (!FChar::IsAlnum(C) && C != TEXT('_'))
				return false;
		}
		return true;
	}

	FString UnquoteLiteral(const FString& Quoted)
	{
		FString Inner = Quoted.Mid(1, Quoted.Len() - 2);
		FString Out;
		Out.Reserve(Inner.Len());
		for (int32 I = 0; I < Inner.Len(); ++I)
		{
			if (Inner[I] == TEXT('\\') && I + 1 < Inner.Len() && (Inner[I + 1] == TEXT('"') || Inner[I + 1] == TEXT('\\')))
			{
				Out.AppendChar(Inner[++I]);
			}
			else
			{
				Out.AppendChar(Inner[I]);
			}
		}
		return Out;
	}

	bool IsQuoted(const FString& S)
	{
		return S.Len() >= 2 && S[0] == TEXT('"') && S[S.Len() - 1] == TEXT('"');
	}

	/**
	 * Splits S on Separator wherever the separator is not inside quotes, parentheses,
	 * brackets or angle brackets. Keeps empty parts.
	 */
	TArray<FString> SplitTopLevel(const FString& S, TCHAR Separator)
	{
		TArray<FString> Parts;
		FString Current;
		int32 Depth = 0;
		bool bInQuotes = false;
		for (int32 I = 0; I < S.Len(); ++I)
		{
			const TCHAR C = S[I];
			if (bInQuotes)
			{
				Current.AppendChar(C);
				if (C == TEXT('\\') && I + 1 < S.Len())
				{
					Current.AppendChar(S[++I]);
				}
				else if (C == TEXT('"'))
				{
					bInQuotes = false;
				}
				continue;
			}
			if (C == TEXT('"'))
			{
				bInQuotes = true;
			}
			else if (C == TEXT('(') || C == TEXT('[') || C == TEXT('<'))
			{
				++Depth;
			}
			else if (C == TEXT(')') || C == TEXT(']') || C == TEXT('>'))
			{
				Depth = FMath::Max(0, Depth - 1);
			}
			else if (C == Separator && Depth == 0)
			{
				Parts.Add(Current);
				Current.Reset();
				continue;
			}
			Current.AppendChar(C);
		}
		Parts.Add(Current);
		return Parts;
	}

	// One dot-separated part of a token source: Name, optional <accessor>, optional (args).
	struct FPathSegment
	{
		FString Name;
		FString Accessor;
		bool bHasAccessor = false;
		bool bIsCall = false;
		FString Args;
	};

	FPathSegment ParseSegment(const FString& Text)
	{
		FPathSegment Seg;
		const int32 ParenIndex = Text.Find(TEXT("("), ESearchCase::CaseSensitive);
		const int32 AngleIndex = Text.Find(TEXT("<"), ESearchCase::CaseSensitive);

		if (ParenIndex != INDEX_NONE && (AngleIndex == INDEX_NONE || ParenIndex < AngleIndex))
		{
			const int32 CloseIndex = Text.Find(TEXT(")"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
			if (CloseIndex > ParenIndex)
			{
				Seg.bIsCall = true;
				Seg.Name = Text.Left(ParenIndex).TrimStartAndEnd();
				Seg.Args = Text.Mid(ParenIndex + 1, CloseIndex - ParenIndex - 1);
				return Seg;
			}
		}

		if (AngleIndex != INDEX_NONE)
		{
			const int32 CloseIndex = Text.Find(TEXT(">"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
			if (CloseIndex > AngleIndex)
			{
				Seg.bHasAccessor = true;
				Seg.Name = Text.Left(AngleIndex).TrimStartAndEnd();
				Seg.Accessor = Text.Mid(AngleIndex + 1, CloseIndex - AngleIndex - 1).TrimStartAndEnd();
				return Seg;
			}
		}

		Seg.Name = Text.TrimStartAndEnd();
		return Seg;
	}

	TArray<FPathSegment> ParsePath(const FString& Source)
	{
		TArray<FPathSegment> Segments;
		for (const FString& Part : SplitTopLevel(Source, TEXT('.')))
		{
			Segments.Add(ParseSegment(Part));
		}
		return Segments;
	}

	// Splits "source:format" at the first colon that is not inside quotes or brackets.
	void SplitSourceAndFormat(const FString& Token, FString& OutSource, FString& OutFormat)
	{
		const TArray<FString> Parts = SplitTopLevel(Token, TEXT(':'));
		OutSource = Parts[0].TrimStartAndEnd();
		OutFormat.Reset();
		if (Parts.Num() > 1)
		{
			// a format may itself contain colons (e.g. a time mask), so re-join the rest
			for (int32 I = 1; I < Parts.Num(); ++I)
			{
				if (I > 1) OutFormat += TEXT(":");
				OutFormat += Parts[I];
			}
			OutFormat.TrimStartAndEndInline();
		}
	}

	FString GroupThousands(const FString& Digits)
	{
		FString Out;
		const int32 Len = Digits.Len();
		for (int32 I = 0; I < Len; ++I)
		{
			if (I > 0 && (Len - I) % 3 == 0)
				Out.AppendChar(TEXT(','));
			Out.AppendChar(Digits[I]);
		}
		return Out;
	}

	bool HasNonZeroDigit(const FString& S)
	{
		for (const TCHAR C : S)
		{
			if (C >= TEXT('1') && C <= TEXT('9'))
				return true;
		}
		return false;
	}

	// C# custom numeric format: 0 # . , % literals 'quoted' and ; sections.
	FString FormatCustom(double Value, const FString& Format)
	{
		const TArray<FString> Sections = SplitTopLevel(Format, TEXT(';'));
		bool bNegative = Value < 0;
		FString Pattern = Sections[0];
		if (bNegative && Sections.Num() >= 2 && !Sections[1].IsEmpty())
		{
			Pattern = Sections[1];
			bNegative = false; // the section carries its own sign literal
		}
		else if (Value == 0 && Sections.Num() >= 3 && !Sections[2].IsEmpty())
		{
			Pattern = Sections[2];
		}

		struct FItem
		{
			bool bSlot = false;
			FString Literal;
		};
		TArray<FItem> IntItems;
		int32 IntZeros = 0, IntSlots = 0;
		int32 FracZeros = 0, FracSlots = 0;
		FString FracSuffix;
		bool bGrouping = false;
		bool bInFraction = false;
		double Scale = 1.0;

		const auto AddLiteral = [&](const FString& Lit)
		{
			if (bInFraction)
				FracSuffix += Lit;
			else
				IntItems.Add(FItem{ false, Lit });
		};

		for (int32 I = 0; I < Pattern.Len(); ++I)
		{
			const TCHAR C = Pattern[I];
			if (C == TEXT('\'') || C == TEXT('"'))
			{
				const int32 Close = Pattern.Find(FString::Chr(C), ESearchCase::CaseSensitive, ESearchDir::FromStart, I + 1);
				const int32 End = Close == INDEX_NONE ? Pattern.Len() : Close;
				AddLiteral(Pattern.Mid(I + 1, End - I - 1));
				I = End;
			}
			else if (C == TEXT('\\') && I + 1 < Pattern.Len())
			{
				AddLiteral(FString::Chr(Pattern[++I]));
			}
			else if (C == TEXT('0') || C == TEXT('#'))
			{
				if (bInFraction)
				{
					++FracSlots;
					if (C == TEXT('0')) FracZeros = FracSlots; // a required digit makes every digit before it required too
				}
				else
				{
					IntItems.Add(FItem{ true, FString() });
					++IntSlots;
					if (C == TEXT('0')) ++IntZeros;
				}
			}
			else if (C == TEXT('.') && !bInFraction)
			{
				bInFraction = true;
			}
			else if (C == TEXT(',') && !bInFraction)
			{
				bGrouping = true;
			}
			else if (C == TEXT('%'))
			{
				Scale *= 100.0;
				AddLiteral(TEXT("%"));
			}
			else
			{
				AddLiteral(FString::Chr(C));
			}
		}

		// round half away from zero, as C# does, at the number of fraction slots
		const double Multiplier = FMath::Pow(10.0, static_cast<double>(FracSlots));
		const double Rounded = FMath::RoundHalfFromZero(FMath::Abs(Value) * Scale * Multiplier) / Multiplier;
		const FString Fixed = FString::Printf(TEXT("%.*f"), FracSlots, Rounded);
		FString IntDigits, FracDigits;
		if (!Fixed.Split(TEXT("."), &IntDigits, &FracDigits))
		{
			IntDigits = Fixed;
		}

		// optional fraction digits (#) drop trailing zeros
		while (FracDigits.Len() > FracZeros && FracDigits.EndsWith(TEXT("0")))
		{
			FracDigits.LeftChopInline(1);
		}

		// integer digits: no leading zeros unless the pattern asks for them
		while (IntDigits.Len() > 1 && IntDigits[0] == TEXT('0'))
		{
			IntDigits.RemoveAt(0);
		}
		if (IntDigits == TEXT("0") && IntZeros == 0)
		{
			IntDigits.Reset();
		}
		while (IntDigits.Len() < IntZeros)
		{
			IntDigits.InsertAt(0, TEXT('0'));
		}
		if (bGrouping)
		{
			IntDigits = GroupThousands(IntDigits);
		}

		// hand out the digits right-to-left into the slots; whatever is left goes in front of the first slot
		TArray<FString> SlotDigits;
		SlotDigits.SetNum(IntSlots);
		int32 Cursor = IntDigits.Len() - 1;
		for (int32 S = IntSlots - 1; S >= 0 && Cursor >= 0; --S)
		{
			SlotDigits[S] = FString::Chr(IntDigits[Cursor--]);
		}
		const FString Leftover = IntDigits.Left(Cursor + 1);

		FString Result;
		int32 SlotIndex = 0;
		bool bLeftoverPlaced = IntSlots == 0;
		for (const FItem& Item : IntItems)
		{
			if (!Item.bSlot)
			{
				Result += Item.Literal;
				continue;
			}
			if (!bLeftoverPlaced)
			{
				Result += Leftover;
				bLeftoverPlaced = true;
			}
			Result += SlotDigits[SlotIndex++];
		}
		if (!bLeftoverPlaced)
		{
			Result += Leftover;
		}

		if (!FracDigits.IsEmpty())
		{
			Result += TEXT(".") + FracDigits;
		}
		Result += FracSuffix;

		if (bNegative && HasNonZeroDigit(Result))
		{
			Result = TEXT("-") + Result;
		}
		return Result;
	}

	// C# standard numeric format: one letter plus optional precision.
	bool TryFormatStandard(double Value, const FString& Format, FString& Out)
	{
		if (Format.IsEmpty() || !FChar::IsAlpha(Format[0]) || (Format.Len() > 1 && !IsDigitString(Format.Mid(1))))
			return false;

		const TCHAR Spec = FChar::ToUpper(Format[0]);
		const int32 Precision = Format.Len() > 1 ? FCString::Atoi(*Format.Mid(1)) : -1;
		const int64 Integer = static_cast<int64>(FMath::RoundHalfFromZero(Value));

		switch (Spec)
		{
		case TEXT('D'):
		{
			FString Digits = FString::Printf(TEXT("%lld"), Integer < 0 ? -Integer : Integer);
			while (Digits.Len() < Precision)
				Digits.InsertAt(0, TEXT('0'));
			Out = (Integer < 0 ? TEXT("-") : TEXT("")) + Digits;
			return true;
		}
		case TEXT('F'):
			Out = FString::Printf(TEXT("%.*f"), Precision < 0 ? 2 : Precision, Value);
			return true;
		case TEXT('N'):
		{
			const FString Fixed = FString::Printf(TEXT("%.*f"), Precision < 0 ? 2 : Precision, FMath::Abs(Value));
			FString IntDigits, FracDigits;
			if (!Fixed.Split(TEXT("."), &IntDigits, &FracDigits))
				IntDigits = Fixed;
			Out = GroupThousands(IntDigits);
			if (!FracDigits.IsEmpty())
				Out += TEXT(".") + FracDigits;
			if (Value < 0 && HasNonZeroDigit(Out))
				Out = TEXT("-") + Out;
			return true;
		}
		case TEXT('P'):
			Out = FString::Printf(TEXT("%.*f%%"), Precision < 0 ? 0 : Precision, Value * 100.0);
			return true;
		case TEXT('E'):
			Out = FString::Printf(TEXT("%.*e"), Precision < 0 ? 6 : Precision, Value);
			if (Format[0] == TEXT('E'))
			{
				Out = Out.ToUpper();
			}
			return true;
		case TEXT('X'):
		{
			FString Hex = Format[0] == TEXT('X') ? FString::Printf(TEXT("%llX"), Integer) : FString::Printf(TEXT("%llx"), Integer);
			while (Hex.Len() < Precision)
				Hex.InsertAt(0, TEXT('0'));
			Out = Hex;
			return true;
		}
		case TEXT('C'):
			Out = FString::Printf(TEXT("%.*f"), Precision < 0 ? 2 : Precision, Value);
			return true;
		case TEXT('G'):
		case TEXT('R'):
			Out = FString::SanitizeFloat(Value);
			return true;
		default:
			return false;
		}
	}
}

/**
 * Resolves the tokens of one Resolve call. Keeps the per-call state (token counter,
 * invalid flag, random picks) and does the actual source walking.
 */
struct FArticyTextResolver
{
	// What a token source resolved to, before formatting.
	struct FValue
	{
		enum class EKind { Invalid, Bool, Int, Float, String, Object, Enum, TypeMeta, PropertyMeta };
		EKind Kind = EKind::Invalid;
		bool Bool = false;
		int64 Int = 0;
		double Float = 0.0;
		FString String;
		UArticyBaseObject* Object = nullptr;
		UArticyPrimitive* RootObject = nullptr; // the articy object a feature belongs to, used as 'self' for scripts
		int64 EnumValue = 0;
		FString EnumName;
		FString EnumDisplayKey;
		FArticyType Type;
		FArticyPropertyInfo Property;
		bool bRawString = false; // a technical name or type: never localized, never an object representation

		bool IsValid() const { return Kind != EKind::Invalid; }

		static FValue Invalid() { return FValue(); }
		static FValue FromBool(bool B) { FValue V; V.Kind = EKind::Bool; V.Bool = B; return V; }
		static FValue FromInt(int64 I) { FValue V; V.Kind = EKind::Int; V.Int = I; return V; }
		static FValue FromFloat(double F) { FValue V; V.Kind = EKind::Float; V.Float = F; return V; }
		static FValue FromString(const FString& S) { FValue V; V.Kind = EKind::String; V.String = S; return V; }
		static FValue FromRawString(const FString& S) { FValue V = FromString(S); V.bRawString = true; return V; }
		static FValue FromObject(UArticyBaseObject* O, UArticyPrimitive* Root)
		{
			FValue V;
			if (!O) return V;
			V.Kind = EKind::Object;
			V.Object = O;
			V.RootObject = Root ? Root : Cast<UArticyPrimitive>(O);
			return V;
		}
		static FValue FromType(const FArticyType& T) { FValue V; V.Kind = EKind::TypeMeta; V.Type = T; return V; }
		static FValue FromProperty(const FArticyPropertyInfo& P) { FValue V; V.Kind = EKind::PropertyMeta; V.Property = P; return V; }
	};

	const UArticyTextExtension& Ext;
	UObject* Outer;
	const TArray<FString>& Params;
	const FArticyTokenResolverCallback* Callback;
	FString Expression;
	int32 TokenCounter = 0;
	bool bAnyInvalid = false;

	FArticyTextResolver(const UArticyTextExtension& InExt, UObject* InOuter, const TArray<FString>& InParams, const FArticyTokenResolverCallback* InCallback)
		: Ext(InExt), Outer(InOuter), Params(InParams), Callback(InCallback)
	{
	}

	//---------------------------------------------------------------------------//
	// Scanning

	static int32 FindMatchingBracket(const FString& In, int32 Start)
	{
		int32 Depth = 0;
		for (int32 J = Start; J < In.Len(); ++J)
		{
			const TCHAR C = In[J];
			if (C == TEXT('\\') && J + 1 < In.Len() && IsEscapable(In[J + 1]))
			{
				++J;
				continue;
			}
			if (C == TEXT('['))
			{
				++Depth;
			}
			else if (C == TEXT(']'))
			{
				if (--Depth == 0)
					return J;
			}
		}
		return INDEX_NONE;
	}

	/**
	 * Resolves every token in In. Tokens nested inside another token (bInsideToken) yield raw
	 * values - an object becomes its representation, not its display name - so the outer
	 * token can use them as a source.
	 */
	FString ResolveText(const FString& In, int32 Depth, bool bInsideToken)
	{
		if (Depth > MaxResolveDepth)
		{
			return In;
		}

		FString Out;
		Out.Reserve(In.Len());
		int32 I = 0;
		while (I < In.Len())
		{
			const TCHAR C = In[I];
			if (C == TEXT('\\') && I + 1 < In.Len() && IsEscapable(In[I + 1]))
			{
				Out.AppendChar(In[I + 1]);
				I += 2;
				continue;
			}
			if (C == TEXT('['))
			{
				const int32 End = FindMatchingBracket(In, I);
				if (End == INDEX_NONE)
				{
					// unterminated token: leave the rest alone
					Out += In.Mid(I);
					break;
				}

				// innermost first: nested tokens are resolved before the token that contains them
				const FString Inner = ResolveText(In.Mid(I + 1, End - I - 1), Depth + 1, true);
				FString Value;
				if (ResolveTokenText(Inner, Depth, bInsideToken, Value))
				{
					Out += Value;
				}
				else
				{
					bAnyInvalid = true;
				}
				I = End + 1;
				continue;
			}
			Out.AppendChar(C);
			++I;
		}
		return Out;
	}

	bool ResolveTokenText(const FString& TokenText, int32 Depth, bool bInsideToken, FString& OutValue)
	{
		FArticyTextToken Token;
		Token.ContextObject = Outer;
		Token.Expression = Expression;
		Token.Params = Params;
		Token.Token = TokenText;
		Token.TokenIndex = TokenCounter++;

		if (Callback)
		{
			const TOptional<FString> Custom = (*Callback)(Token);
			if (Custom.IsSet())
			{
				OutValue = Custom.GetValue();
				return true;
			}
		}
		return ResolveTokenDefault(Token, Depth, bInsideToken, OutValue);
	}

	bool ResolveTokenDefault(const FArticyTextToken& Token, int32 Depth, bool bInsideToken, FString& OutValue)
	{
		FString Source, Format;
		SplitSourceAndFormat(Token.Token, Source, Format);
		if (Source.IsEmpty())
		{
			return false;
		}
		const FValue Value = ResolveSource(Source, Token, Depth);
		return FormatValue(Value, Source, Format, Depth, bInsideToken, OutValue);
	}

	//---------------------------------------------------------------------------//
	// Sources

	FValue ResolveSource(const FString& Source, const FArticyTextToken& Token, int32 Depth)
	{
		if (IsQuoted(Source))
		{
			return FValue::FromString(UnquoteLiteral(Source));
		}

		bool bIsFloat = false;
		if (IsNumericLiteral(Source, bIsFloat))
		{
			return bIsFloat ? FValue::FromFloat(FCString::Atod(*Source)) : FValue::FromInt(FCString::Atoi64(*Source));
		}

		const TArray<FPathSegment> Segments = ParsePath(Source);
		if (Segments.Num() == 0 || Segments[0].Name.IsEmpty())
		{
			return FValue::Invalid();
		}

		// [method(args)] - and the legacy [anything.method(args)] form for registered methods
		const FPathSegment& Last = Segments.Last();
		if (Last.bIsCall && (Segments.Num() == 1 || IsMethod(Last.Name)))
		{
			const TOptional<FString> Result = ExecuteMethod(Last.Name, Last.Args, Token, Depth);
			return Result.IsSet() ? FValue::FromString(Result.GetValue()) : FValue::Invalid();
		}

		int32 Index = 0;
		FValue Value = ResolveRoot(Segments, Index, Depth);
		while (Value.IsValid() && Index < Segments.Num())
		{
			Value = Walk(Value, Segments, Index, Depth);
		}
		return Value;
	}

	FValue ResolveRoot(const TArray<FPathSegment>& Segments, int32& Index, int32 Depth)
	{
		const FPathSegment& Root = Segments[0];
		const FString& Name = Root.Name;
		Index = 1;

		if (Name == TEXT("$Type"))
		{
			if (Segments.Num() < 2)
				return FValue::Invalid();
			UArticyTypeSystem* TypeSystem = Ext.GetTypeSystem();
			const FArticyType* Type = TypeSystem ? TypeSystem->Types.Find(Segments[1].Name) : nullptr;
			Index = 2;
			return Type ? FValue::FromType(*Type) : FValue::Invalid();
		}

		if (Name == TEXT("$Self"))
		{
			return FValue::FromObject(Cast<UArticyBaseObject>(Outer), nullptr);
		}

		if (Name == TEXT("$Speaker"))
		{
			const IArticyObjectWithSpeaker* WithSpeaker = Cast<IArticyObjectWithSpeaker>(Outer);
			if (!WithSpeaker)
				return FValue::Invalid();
			return FValue::FromObject(GetObjectById(WithSpeaker->GetSpeakerId().Get(), 0), nullptr);
		}

		const int32 CloneId = Root.bHasAccessor ? FCString::Atoi(*Root.Accessor) : 0;

		if (Name.StartsWith(TEXT("0x")))
		{
			return FValue::FromObject(GetObjectById(ArticyHelpers::HexToUint64(Name), CloneId), nullptr);
		}

		uint64 RepId = 0;
		int32 RepClone = 0;
		if (IsObjectRepresentation(Name, RepId, RepClone))
		{
			return FValue::FromObject(GetObjectById(RepId, Root.bHasAccessor ? CloneId : RepClone), nullptr);
		}

		if (IsDigitString(Name))
		{
			return FValue::FromObject(GetObjectById(FCString::Strtoui64(*Name, nullptr, 10), CloneId), nullptr);
		}

		if (Segments.Num() >= 2)
		{
			// [EnumType.Value]
			if (UArticyTypeSystem* TypeSystem = Ext.GetTypeSystem())
			{
				const FArticyType* Type = TypeSystem->Types.Find(Name);
				if (Type && Type->IsEnum)
				{
					const FArticyEnumValueInfo Info = Type->GetEnumValue(Segments[1].Name);
					if (!Info.LocaKey_DisplayName.IsEmpty())
					{
						Index = 2;
						return MakeEnum(Info.Value, Info.TechnicalName.IsEmpty() ? Info.LocaKey_DisplayName : Info.TechnicalName, Info.LocaKey_DisplayName);
					}
				}
			}

			// [Namespace.Variable]
			FValue Variable;
			if (TryGetGlobalVariable(Name, Segments[1].Name, Variable))
			{
				Index = 2;
				return Variable;
			}
		}

		if (UArticyDatabase* DB = Ext.GetDatabase(Outer))
		{
			return FValue::FromObject(DB->GetObjectByName(FName(*Name), CloneId), nullptr);
		}
		return FValue::Invalid();
	}

	UArticyObject* GetObjectById(uint64 Id, int32 CloneId) const
	{
		UArticyDatabase* DB = Ext.GetDatabase(Outer);
		return DB ? DB->GetObject(FArticyId{ Id }, CloneId) : nullptr;
	}

	bool TryGetGlobalVariable(const FString& Namespace, const FString& VariableName, FValue& OutValue) const
	{
		UArticyGlobalVariables* GVs = Ext.GetGlobalVariables(Outer);
		if (!GVs)
			return false;

		// look the set up through reflection: GetNamespace() logs an error for a miss,
		// and a miss is the normal case for every object token
		UArticyBaseVariableSet* const* SetPtr = GVs->GetPropPtr<UArticyBaseVariableSet*>(FName(*Namespace));
		if (!SetPtr || !*SetPtr)
			return false;

		UArticyVariable* const* VariablePtr = (*SetPtr)->GetPropPtr<UArticyVariable*>(FName(*VariableName));
		if (!VariablePtr || !*VariablePtr)
			return false;

		if (const UArticyBool* BoolVar = Cast<UArticyBool>(*VariablePtr))
		{
			OutValue = FValue::FromBool(BoolVar->Get());
			return true;
		}
		if (const UArticyInt* IntVar = Cast<UArticyInt>(*VariablePtr))
		{
			OutValue = FValue::FromInt(IntVar->Get());
			return true;
		}
		if (const UArticyString* StringVar = Cast<UArticyString>(*VariablePtr))
		{
			OutValue = FValue::FromString(StringVar->Get());
			return true;
		}
		return false;
	}

	//---------------------------------------------------------------------------//
	// Walking a path

	FValue Walk(const FValue& Value, const TArray<FPathSegment>& Segments, int32& Index, int32 Depth)
	{
		const FPathSegment& Seg = Segments[Index];
		++Index;

		switch (Value.Kind)
		{
		case FValue::EKind::Object:
		{
			if (Seg.Name == TEXT("$Type"))
			{
				return FValue::FromType(GetObjectType(Value.Object));
			}
			FProperty* Property = Value.Object->GetProperty(FName(*Seg.Name));
			if (!Property)
			{
				return FValue::Invalid();
			}
			return ReadProperty(Value, Property, Seg, Depth);
		}

		case FValue::EKind::String:
		{
			// a string holding an object representation can be walked like the object
			uint64 RepId = 0;
			int32 RepClone = 0;
			if (IsObjectRepresentation(Value.String, RepId, RepClone))
			{
				const FValue AsObject = FValue::FromObject(GetObjectById(RepId, RepClone), nullptr);
				--Index;
				return AsObject.IsValid() ? Walk(AsObject, Segments, Index, Depth) : FValue::Invalid();
			}
			return FValue::Invalid();
		}

		case FValue::EKind::Enum:
			if (Seg.Name == TEXT("DisplayName"))
				return FValue::FromRawString(LocalizeOrSelf(Value.EnumDisplayKey.IsEmpty() ? Value.EnumName : Value.EnumDisplayKey, Depth));
			if (Seg.Name == TEXT("TechnicalName"))
				return FValue::FromRawString(Value.EnumName);
			if (Seg.Name == TEXT("Value"))
				return FValue::FromInt(Value.EnumValue);
			return FValue::Invalid();

		case FValue::EKind::TypeMeta:
			return WalkType(Value.Type, Segments, Index, Seg, Depth);

		case FValue::EKind::PropertyMeta:
			if (Seg.Name == TEXT("DisplayName"))
				return FValue::FromRawString(LocalizeOrSelf(Value.Property.LocaKey_DisplayName, Depth));
			if (Seg.Name == TEXT("TechnicalName"))
				return FValue::FromRawString(Value.Property.TechnicalName.IsEmpty() ? Value.Property.LocaKey_DisplayName : Value.Property.TechnicalName);
			if (Seg.Name == TEXT("PropertyType") || Seg.Name == TEXT("Type"))
				return FValue::FromRawString(Value.Property.PropertyType);
			if (Seg.Name == TEXT("IsTemplateProperty"))
				return FValue::FromBool(Value.Property.IsTemplateProperty);
			return FValue::Invalid();

		default:
			return FValue::Invalid();
		}
	}

	FValue WalkType(const FArticyType& Type, const TArray<FPathSegment>& Segments, int32& Index, const FPathSegment& Seg, int32 Depth)
	{
		// A property that shares its name with a type member (every object has a DisplayName)
		// wins when the path goes on, since only a property has members of its own.
		const bool bMoreSegments = Index < Segments.Num();
		if (bMoreSegments)
		{
			if (const FArticyPropertyInfo* Direct = FindTypeProperty(Type, Seg.Name))
			{
				return FValue::FromProperty(*Direct);
			}
		}

		if (Seg.Name == TEXT("DisplayName"))
			return FValue::FromRawString(LocalizeOrSelf(Type.LocaKey_DisplayName, Depth));
		if (Seg.Name == TEXT("TechnicalName"))
			return FValue::FromRawString(Type.TechnicalName);
		if (Seg.Name == TEXT("CPPType"))
			return FValue::FromRawString(Type.CPPType);
		if (Seg.Name == TEXT("HasTemplate"))
			return FValue::FromBool(Type.HasTemplate);
		if (Seg.Name == TEXT("IsEnum"))
			return FValue::FromBool(Type.IsEnum);

		if (Type.IsEnum)
		{
			const FArticyEnumValueInfo Info = Type.GetEnumValue(Seg.Name);
			if (!Info.LocaKey_DisplayName.IsEmpty())
			{
				return MakeEnum(Info.Value, Info.TechnicalName.IsEmpty() ? Info.LocaKey_DisplayName : Info.TechnicalName, Info.LocaKey_DisplayName);
			}
		}

		// a property, addressed directly or as Feature.Property
		if (const FArticyPropertyInfo* Direct = FindTypeProperty(Type, Seg.Name))
		{
			return FValue::FromProperty(*Direct);
		}
		if (Index < Segments.Num())
		{
			if (const FArticyPropertyInfo* Nested = FindTypeProperty(Type, Seg.Name + TEXT(".") + Segments[Index].Name))
			{
				++Index;
				return FValue::FromProperty(*Nested);
			}
		}
		return FValue::Invalid();
	}

	/** The articy type of an object: what the object reports, or the type system's entry for its generated class. */
	FArticyType GetObjectType(const UArticyBaseObject* Object) const
	{
		const FArticyType Reported = Object->GetArticyType();
		if (!Reported.TechnicalName.IsEmpty() || !Reported.LocaKey_DisplayName.IsEmpty() || !Reported.CPPType.IsEmpty())
		{
			return Reported;
		}
		if (UArticyTypeSystem* TypeSystem = Ext.GetTypeSystem())
		{
			// UClass names drop the U prefix the generated CPPType carries
			const FString ClassName = Object->GetClass()->GetName();
			const FString PrefixedClassName = TEXT("U") + ClassName;
			for (const auto& Pair : TypeSystem->Types)
			{
				if (Pair.Value.CPPType == ClassName || Pair.Value.CPPType == PrefixedClassName)
				{
					return Pair.Value;
				}
			}
		}
		return Reported;
	}

	static const FArticyPropertyInfo* FindTypeProperty(const FArticyType& Type, const FString& Name)
	{
		for (const FArticyPropertyInfo& Info : Type.Properties)
		{
			if (Info.TechnicalName.Equals(Name) || Info.LocaKey_DisplayName.Equals(Name))
				return &Info;
		}
		return nullptr;
	}

	FValue ReadProperty(const FValue& Owner, FProperty* Property, const FPathSegment& Seg, int32 Depth)
	{
		if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			FScriptArrayHelper Helper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(Owner.Object));
			if (!Seg.bHasAccessor)
			{
				return FValue::Invalid();
			}
			const int32 ElementIndex = ResolveArrayIndex(Seg.Accessor, Helper.Num());
			if (ElementIndex == INDEX_NONE)
			{
				return FValue::Invalid();
			}
			return ReadValue(Owner, ArrayProperty->Inner, Helper.GetRawPtr(ElementIndex), Seg, Depth);
		}
		return ReadValue(Owner, Property, Property->ContainerPtrToValuePtr<void>(Owner.Object), Seg, Depth);
	}

	int32 ResolveArrayIndex(const FString& Accessor, int32 Num) const
	{
		if (Num <= 0)
			return INDEX_NONE;
		if (Accessor == TEXT("?"))
		{
			const int32 Pick = FMath::RandRange(0, Num - 1);
			Ext.LastRandomResults.Add(Pick);
			return Pick;
		}
		if (!Accessor.IsNumeric())
			return INDEX_NONE;
		int32 ElementIndex = FCString::Atoi(*Accessor);
		if (ElementIndex < 0)
			ElementIndex += Num; // -1 is the last element
		return ElementIndex >= 0 && ElementIndex < Num ? ElementIndex : INDEX_NONE;
	}

	FValue ReadValue(const FValue& Owner, FProperty* Property, const void* ValuePtr, const FPathSegment& Seg, int32 Depth)
	{
		if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
		{
			return FValue::FromBool(BoolProperty->GetPropertyValue(ValuePtr));
		}
		if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
		{
			return MakeEnum(EnumProperty->GetEnum(), EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(ValuePtr));
		}
		if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
		{
			const int64 Raw = ByteProperty->GetPropertyValue(ValuePtr);
			return ByteProperty->Enum ? MakeEnum(ByteProperty->Enum, Raw) : FValue::FromInt(Raw);
		}
		if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
		{
			if (NumericProperty->IsFloatingPoint())
				return FValue::FromFloat(NumericProperty->GetFloatingPointPropertyValue(ValuePtr));
			return FValue::FromInt(NumericProperty->GetSignedIntPropertyValue(ValuePtr));
		}
		if (const FStrProperty* StrProperty = CastField<FStrProperty>(Property))
		{
			return FValue::FromString(StrProperty->GetPropertyValue(ValuePtr));
		}
		if (const FTextProperty* TextProperty = CastField<FTextProperty>(Property))
		{
			return FValue::FromString(TextProperty->GetPropertyValue(ValuePtr).ToString());
		}
		if (const FNameProperty* NameProperty = CastField<FNameProperty>(Property))
		{
			return FValue::FromString(NameProperty->GetPropertyValue(ValuePtr).ToString());
		}
		if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			if (StructProperty->Struct == FArticyId::StaticStruct())
			{
				const FArticyId& Id = *static_cast<const FArticyId*>(ValuePtr);
				return Id.IsNull() ? FValue::Invalid() : FValue::FromObject(GetObjectById(Id.Get(), 0), nullptr);
			}
			return FValue::Invalid();
		}
		if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
		{
			UObject* Referenced = ObjectProperty->GetObjectPropertyValue(ValuePtr);
			if (!Referenced)
				return FValue::Invalid();
			if (UArticyScriptFragment* Script = Cast<UArticyScriptFragment>(Referenced))
			{
				return Seg.bIsCall ? EvaluateScript(Script, Owner.RootObject) : FValue::FromString(Script->GetExpression());
			}
			if (UArticyBaseObject* Articy = Cast<UArticyBaseObject>(Referenced))
			{
				// features stay attached to their owning object, so scripts on them know 'self'
				return FValue::FromObject(Articy, Articy->IsA<UArticyPrimitive>() ? nullptr : Owner.RootObject);
			}
			return FValue::FromString(Referenced->GetName());
		}
		return FValue::Invalid();
	}

	FValue EvaluateScript(UArticyScriptFragment* Script, UArticyPrimitive* Self) const
	{
		UArticyDatabase* DB = Ext.GetDatabase(Outer);
		UArticyExpressoScripts* Expresso = DB ? DB->GetExpressoInstance() : nullptr;
		if (!Expresso)
			return FValue::Invalid();

		UArticyGlobalVariables* GVs = Ext.GetGlobalVariables(Outer);
		const int32 Hash = static_cast<int32>(GetTypeHash(Script->GetExpression()));

		UArticyPrimitive* PreviousSelf = Expresso->self;
		Expresso->SetCurrentObject(Self);
		FValue Result;
		if (Script->IsA<UArticyScriptCondition>())
		{
			Result = FValue::FromBool(Expresso->Evaluate(Hash, GVs, nullptr));
		}
		else
		{
			Expresso->Execute(Hash, GVs, nullptr);
			Result = FValue::FromString(FString());
		}
		Expresso->SetCurrentObject(PreviousSelf);
		return Result;
	}

	//---------------------------------------------------------------------------//
	// Enums

	FValue MakeEnum(int64 Value, const FString& TechnicalName, const FString& DisplayKey) const
	{
		FValue V;
		V.Kind = FValue::EKind::Enum;
		V.EnumValue = Value;
		V.EnumName = TechnicalName;
		V.EnumDisplayKey = DisplayKey;
		return V;
	}

	FValue MakeEnum(const UEnum* Enum, int64 Value) const
	{
		FString Name = Enum ? Enum->GetNameStringByValue(Value) : FString();
		FString DisplayKey;

		// the type system knows the articy side of the enum: display names and loca keys
		if (Enum)
		{
			if (UArticyTypeSystem* TypeSystem = Ext.GetTypeSystem())
			{
				const FString EnumClassName = Enum->GetName();
				for (const auto& Pair : TypeSystem->Types)
				{
					if (!Pair.Value.IsEnum || !(Pair.Value.CPPType == EnumClassName || Pair.Key == EnumClassName))
						continue;
					const FArticyEnumValueInfo Info = Pair.Value.GetEnumValue(static_cast<int>(Value));
					if (!Info.LocaKey_DisplayName.IsEmpty())
					{
						DisplayKey = Info.LocaKey_DisplayName;
						if (!Info.TechnicalName.IsEmpty())
							Name = Info.TechnicalName;
						else if (Name.IsEmpty())
							Name = Info.LocaKey_DisplayName;
					}
					break;
				}
			}
		}

		if (Name.IsEmpty())
		{
			Name = FString::Printf(TEXT("%lld"), Value);
		}
		return MakeEnum(Value, Name, DisplayKey);
	}

	//---------------------------------------------------------------------------//
	// Methods

	bool IsMethod(const FString& Name) const
	{
		return Name == TEXT("if") || Name == TEXT("not") || Ext.HasUserMethod(Name);
	}

	TOptional<FString> ExecuteMethod(const FString& Name, const FString& ArgsString, const FArticyTextToken& Token, int32 Depth)
	{
		const TArray<FString> Args = ParseArguments(ArgsString, Token, Depth);

		if (Name == TEXT("if") || Name == TEXT("not"))
		{
			if (Args.Num() < 2)
				return TOptional<FString>();
			const bool bCondition = IsTruthy(Args[0]) != (Name == TEXT("not"));
			if (bCondition)
				return Args[1];
			return Args.Num() > 2 ? Args[2] : FString();
		}

		if (const FArticyUserMethodCallback* UserMethod = Ext.UserMethodMap.Find(Name))
		{
			return (*UserMethod)(Token, Args);
		}
		return TOptional<FString>();
	}

	TArray<FString> ParseArguments(const FString& ArgsString, const FArticyTextToken& Token, int32 Depth)
	{
		TArray<FString> Args;
		if (ArgsString.TrimStartAndEnd().IsEmpty())
			return Args;

		for (const FString& RawArg : SplitTopLevel(ArgsString, TEXT(',')))
		{
			const FString Arg = RawArg.TrimStartAndEnd();
			if (IsQuoted(Arg))
			{
				Args.Add(UnquoteLiteral(Arg));
				continue;
			}

			// a nested method call as an argument
			const int32 ParenIndex = Arg.Find(TEXT("("), ESearchCase::CaseSensitive);
			if (ParenIndex > 0 && Arg.EndsWith(TEXT(")")) && IsIdentifier(Arg.Left(ParenIndex)))
			{
				const TOptional<FString> Nested = ExecuteMethod(Arg.Left(ParenIndex), Arg.Mid(ParenIndex + 1, Arg.Len() - ParenIndex - 2), Token, Depth);
				Args.Add(Nested.IsSet() ? Nested.GetValue() : FString());
				continue;
			}

			Args.Add(Arg);
		}
		return Args;
	}

	bool IsTruthy(const FString& Value) const
	{
		const FString Trimmed = Value.TrimStartAndEnd();
		const FString Lower = Trimmed.ToLower();
		if (Lower == TEXT("true") || Lower == TEXT("1") || Lower == TEXT("yes"))
			return true;
		if (Lower.IsEmpty() || Lower == TEXT("false") || Lower == TEXT("0") || Lower == TEXT("no"))
			return false;

		bool bIsFloat = false;
		if (IsNumericLiteral(Trimmed, bIsFloat))
			return FCString::Atod(*Trimmed) != 0.0;

		// a localized boolean, as produced by ResolveBoolean
		const FString LocalizedTrue = Ext.LocalizeString(Outer, TEXT("VariableConstants.Boolean.True"));
		if (!LocalizedTrue.IsEmpty() && Trimmed.Equals(LocalizedTrue, ESearchCase::IgnoreCase))
			return true;
		const FString LocalizedFalse = Ext.LocalizeString(Outer, TEXT("VariableConstants.Boolean.False"));
		if (!LocalizedFalse.IsEmpty() && Trimmed.Equals(LocalizedFalse, ESearchCase::IgnoreCase))
			return false;

		// any other non-empty text counts as present
		return true;
	}

	//---------------------------------------------------------------------------//
	// Output

	FString LocalizeOrSelf(const FString& Key, int32 Depth)
	{
		if (Key.IsEmpty())
			return Key;
		const FString Localized = Ext.LocalizeString(Outer, Key);
		return Localized.IsEmpty() ? Key : ResolveText(Localized, Depth + 1, false);
	}

	FString ObjectToString(UArticyBaseObject* Object, int32 Depth)
	{
		if (const FText* DisplayName = Object->GetPropPtr<FText>(FName(TEXT("DisplayName"))))
		{
			return LocalizeOrSelf(DisplayName->ToString(), Depth);
		}
		if (const UArticyObject* ArticyObject = Cast<UArticyObject>(Object))
		{
			return ArticyObject->GetTechnicalName().ToString();
		}
		return FString();
	}

	bool FormatValue(const FValue& Value, const FString& SourceName, const FString& Format, int32 Depth, bool bInsideToken, FString& Out)
	{
		switch (Value.Kind)
		{
		case FValue::EKind::Invalid:
			return false;

		case FValue::EKind::Object:
		{
			// inside another token the object stays addressable: hand out its representation
			const UArticyPrimitive* Primitive = Cast<UArticyPrimitive>(Value.Object);
			if (bInsideToken && Primitive)
			{
				Out = FString::Printf(TEXT("%llu_%d"), Primitive->GetId().Get(), Primitive->GetCloneId());
				return true;
			}
			Out = ObjectToString(Value.Object, Depth);
			return true;
		}

		case FValue::EKind::TypeMeta:
			Out = LocalizeOrSelf(Value.Type.LocaKey_DisplayName, Depth);
			return true;

		case FValue::EKind::PropertyMeta:
			Out = LocalizeOrSelf(Value.Property.LocaKey_DisplayName, Depth);
			return true;

		case FValue::EKind::Enum:
			// the display name by default; any numeric format (e.g. "D") formats the value
			Out = Format.IsEmpty()
				? LocalizeOrSelf(Value.EnumDisplayKey.IsEmpty() ? Value.EnumName : Value.EnumDisplayKey, Depth)
				: Ext.FormatNumber(FString::Printf(TEXT("%lld"), Value.EnumValue), Format);
			return true;

		case FValue::EKind::Bool:
			Out = Format.IsEmpty()
				? Ext.ResolveBoolean(Outer, SourceName, Value.Bool)
				: Ext.FormatNumber(Value.Bool ? TEXT("true") : TEXT("false"), Format);
			return true;

		case FValue::EKind::Int:
			Out = Format.IsEmpty() ? FString::Printf(TEXT("%lld"), Value.Int) : Ext.FormatNumber(FString::Printf(TEXT("%lld"), Value.Int), Format);
			return true;

		case FValue::EKind::Float:
			Out = Format.IsEmpty() ? FString::SanitizeFloat(Value.Float) : Ext.FormatNumber(FString::SanitizeFloat(Value.Float), Format);
			return true;

		case FValue::EKind::String:
		{
			// technical names and types are inserted verbatim
			if (Value.bRawString)
			{
				Out = Value.String;
				return true;
			}
			// an object representation shows the object (unless an outer token still needs
			// to address it), anything else is localized if it is a key
			uint64 RepId = 0;
			int32 RepClone = 0;
			if (IsObjectRepresentation(Value.String, RepId, RepClone))
			{
				if (bInsideToken)
				{
					Out = Value.String;
					return true;
				}
				if (UArticyObject* Object = GetObjectById(RepId, RepClone))
				{
					Out = ObjectToString(Object, Depth);
					return true;
				}
			}
			const FString Text = LocalizeOrSelf(Value.String, Depth);
			bool bIsFloat = false;
			Out = !Format.IsEmpty() && IsNumericLiteral(Text.TrimStartAndEnd(), bIsFloat) ? Ext.FormatNumber(Text, Format) : Text;
			return true;
		}

		default:
			return false;
		}
	}
};

//---------------------------------------------------------------------------//

UArticyTextExtension* UArticyTextExtension::Get()
{
	static TWeakObjectPtr<UArticyTextExtension> ArticyTextExtension;

	if (!ArticyTextExtension.IsValid())
	{
		ArticyTextExtension = TWeakObjectPtr<UArticyTextExtension>(NewObject<UArticyTextExtension>());
	}

	return ArticyTextExtension.Get();
}

FString UArticyTextExtension::ResolveInternal(UObject* Outer, const FString& Input, const TArray<FString>& Params, const FArticyTokenResolverCallback* Callback) const
{
	LastRandomResults.Reset();

	// A loca key as input is localized before anything else happens.
	FString Text = LocalizeInput(Outer, Input);

	// Positional parameters go in first, so they can even form part of a token.
	for (int32 ArgIndex = 0; ArgIndex < Params.Num(); ++ArgIndex)
	{
		const FString Placeholder = FString::Printf(TEXT("{%d}"), ArgIndex);
		Text = Text.Replace(*Placeholder, *Params[ArgIndex]);
	}

	FArticyTextResolver Resolver(*this, Outer, Params, Callback);
	Resolver.Expression = Text;
	const FString Result = Resolver.ResolveText(Text, 0, false);

	if (Resolver.bAnyInvalid && !AllowInvalidTokens())
	{
		// an error in any token hands the input back, instead of a partially resolved text
		return Text;
	}
	return Result;
}

FString UArticyTextExtension::LocalizeInput(UObject* Outer, const FString& Input) const
{
	if (Input.IsEmpty())
		return Input;
	const FString Localized = LocalizeString(Outer, Input);
	return Localized.IsEmpty() ? Input : Localized;
}

FString UArticyTextExtension::ResolveToken(UObject* Outer, const FString& Token) const
{
	FArticyTextToken TextToken;
	TextToken.ContextObject = Outer;
	TextToken.Expression = Token;
	TextToken.Token = Token;
	return ResolveToken(TextToken);
}

FString UArticyTextExtension::ResolveToken(const FArticyTextToken& Token) const
{
	FArticyTextResolver Resolver(*this, Token.ContextObject, Token.Params, nullptr);
	Resolver.Expression = Token.Expression;
	Resolver.TokenCounter = Token.TokenIndex;

	FString Value;
	if (Resolver.ResolveTokenDefault(Token, 0, false, Value))
	{
		return Value;
	}
	return FString();
}

//---------------------------------------------------------------------------//

UArticyDatabase* UArticyTextExtension::GetDatabase(UObject* Outer) const
{
	if (!Outer)
		return nullptr;

	// Objects handed out by a database are outered to it (clones to their original), so an
	// articy object as context knows its database even without a world - the usual case when
	// a generated getter resolves its own text in the editor.
	if (UArticyDatabase* OwningDatabase = Outer->GetTypedOuter<UArticyDatabase>())
		return OwningDatabase;
	if (UArticyDatabase* Database = Cast<UArticyDatabase>(Outer))
		return Database;

	// Anything else needs a world to find the database clone; a plain UObject has none.
	if (!GEngine || !GEngine->GetWorldFromContextObject(Outer, EGetWorldErrorMode::ReturnNull))
		return nullptr;

	return UArticyDatabase::Get(Outer);
}

UArticyGlobalVariables* UArticyTextExtension::GetGlobalVariables(UObject* Outer) const
{
	UArticyDatabase* DB = GetDatabase(Outer);
	return DB ? DB->GetGVs() : nullptr;
}

UArticyTypeSystem* UArticyTextExtension::GetTypeSystem() const
{
	return UArticyTypeSystem::Get();
}

bool UArticyTextExtension::AllowInvalidTokens() const
{
	const UArticyPluginSettings* Settings = UArticyPluginSettings::Get();
	return Settings ? Settings->bAllowInvalidTokens : true;
}

// Process SourceValue with NumberFormat according to C#'s standard and custom numeric format strings
FString UArticyTextExtension::FormatNumber(const FString& SourceValue, const FString& NumberFormat) const
{
	double Value;
	// Handle booleans
	if (SourceValue.Equals(TEXT("true"), ESearchCase::IgnoreCase))
	{
		Value = 1.0;
	}
	else if (SourceValue.Equals(TEXT("false"), ESearchCase::IgnoreCase))
	{
		Value = 0.0;
	}
	else
	{
		Value = FCString::Atod(*SourceValue.TrimStartAndEnd());
	}

	const FString Format = NumberFormat.TrimStartAndEnd();
	if (Format.IsEmpty())
	{
		return FString::SanitizeFloat(Value);
	}

	FString Formatted;
	if (TryFormatStandard(Value, Format, Formatted))
	{
		return Formatted;
	}
	return FormatCustom(Value, Format);
}

FString UArticyTextExtension::ResolveBoolean(UObject* Outer, const FString& SourceName, const bool Value) const
{
	const TCHAR* Suffix = Value ? TEXT("True") : TEXT("False");

	// A boolean may carry its own display texts as <Source>.True / <Source>.False...
	if (!SourceName.IsEmpty())
	{
		const FString SourceValue = LocalizeString(Outer, SourceName + TEXT(".") + Suffix);
		if (!SourceValue.IsEmpty())
		{
			return SourceValue;
		}
	}

	// ...or fall back to the project-wide constants...
	const FString VariableConstants = LocalizeString(Outer, FString(TEXT("VariableConstants.Boolean.")) + Suffix);
	if (!VariableConstants.IsEmpty())
	{
		return VariableConstants;
	}

	// ...or to the plain words.
	return Value ? TEXT("true") : TEXT("false");
}

FString UArticyTextExtension::LocalizeString(UObject* Outer, const FString& Input) const
{
	const FText Placeholder = FText::FromString(TEXT(""));
	return ArticyHelpers::LocalizeString(Outer, FText::FromString(Input), false, &Placeholder).ToString();
}

void UArticyTextExtension::SplitInstance(const FString& InString, FString& OutName, FString& OutInstanceNumber)
{
	const FString SearchSubstr = TEXT("<");
	const int32 StartIdx = InString.Find(SearchSubstr);

	if (StartIdx != INDEX_NONE)
	{
		const int32 EndIdx = InString.Find(TEXT(">"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);

		if (EndIdx != INDEX_NONE && EndIdx > StartIdx)
		{
			OutName = InString.Left(StartIdx);
			OutInstanceNumber = InString.Mid(StartIdx + 1, EndIdx - StartIdx - 1);
		}
		else
		{
			OutName = InString;
			OutInstanceNumber = TEXT("0");
		}
	}
	else
	{
		OutName = InString;
		OutInstanceNumber = TEXT("0");
	}
}

//---------------------------------------------------------------------------//

void UArticyTextExtension::AddUserMethod(const FString& MethodName, FArticyUserMethodCallback Callback)
{
	UserMethodMap.Add(MethodName, MoveTemp(Callback));
}

void UArticyTextExtension::AddUserMethod(const FString& MethodName, FArticyLegacyUserMethodCallback Callback)
{
	UserMethodMap.Add(MethodName, [Callback](const FArticyTextToken&, const TArray<FString>& Args)
	{
		return Callback(Args);
	});
}

bool UArticyTextExtension::RemoveUserMethod(const FString& MethodName)
{
	return UserMethodMap.Remove(MethodName) > 0;
}

bool UArticyTextExtension::HasUserMethod(const FString& MethodName) const
{
	return UserMethodMap.Contains(MethodName);
}
