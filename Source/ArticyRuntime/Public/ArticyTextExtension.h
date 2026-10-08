//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonValue.h"
#include "Misc/AutomationTest.h"
#include "Misc/Optional.h"
#include <type_traits>
#include "ArticyTextExtension.generated.h"

class UArticyDatabase;
class UArticyGlobalVariables;
class UArticyTypeSystem;
class UArticyVariable;
struct FArticyGvName;

/**
 * @brief Describes one token while a string is being resolved.
 *
 * Handed to user methods and to the ResolveAdvance callback so custom code can see where
 * a token came from. Mirrors the ArticyTextToken structure of the Unity importer.
 */
struct ARTICYRUNTIME_API FArticyTextToken
{
	/** The context the resolve was started with (needed for $Self and $Speaker). */
	UObject* ContextObject = nullptr;

	/** The whole input string this token is part of (after positional parameters were inserted). */
	FString Expression;

	/** The positional parameters passed to the resolve call, already converted to strings. */
	TArray<FString> Params;

	/** The unresolved token source without brackets; nested tokens have already been resolved. */
	FString Token;

	/** Zero-based index of this token within the resolve call, in the order tokens are resolved. */
	int32 TokenIndex = 0;
};

/** Callback for a user method registered with AddUserMethod: receives the token and the parsed arguments. */
using FArticyUserMethodCallback = TFunction<FString(const FArticyTextToken&, const TArray<FString>&)>;

/** Legacy user method callback that only receives the arguments. Still accepted by AddUserMethod. */
using FArticyLegacyUserMethodCallback = TFunction<FString(const TArray<FString>&)>;

/**
 * Callback for ResolveAdvance. Return a value to resolve the token yourself, or an unset
 * TOptional to fall back to the default resolving (see ResolveToken).
 */
using FArticyTokenResolverCallback = TFunction<TOptional<FString>(const FArticyTextToken&)>;

UENUM()
enum class EArticyObjectType : uint8
{
	UArticyBool = 0,
	UArticyInt = 1,
	UArticyString = 2,
	Other
};

namespace ArticyTextExtensionPrivate
{
	/** Converts a positional parameter to its string form (strings, texts, names, numbers, bools). */
	template<typename T>
	FString ArgToString(const T& Value)
	{
		using TDecayed = std::decay_t<T>;
		if constexpr (std::is_same_v<TDecayed, bool>)
		{
			return Value ? TEXT("true") : TEXT("false");
		}
		else if constexpr (std::is_same_v<TDecayed, FString>)
		{
			return Value;
		}
		else if constexpr (std::is_same_v<TDecayed, FText>)
		{
			return Value.ToString();
		}
		else if constexpr (std::is_same_v<TDecayed, FName>)
		{
			return Value.ToString();
		}
		else if constexpr (std::is_floating_point_v<TDecayed>)
		{
			return FString::SanitizeFloat(Value);
		}
		else if constexpr (std::is_arithmetic_v<TDecayed>)
		{
			return LexToString(Value);
		}
		else
		{
			// TCHAR arrays and pointers
			return FString(Value);
		}
	}
}

/**
 * @brief Resolves articy text extension tokens inside strings.
 *
 * A token is written as [(Source)(:Formatting)]. Sources are global variables, objects and
 * their (template) properties, reference slots and strips, $Self / $Speaker / $Type, literal
 * values, scripts and methods. Tokens may be nested, brackets can be escaped with a backslash,
 * and positional {0} parameters are inserted before resolving starts. See the text extension
 * documentation for the full syntax.
 */
UCLASS(BlueprintType)
class ARTICYRUNTIME_API UArticyTextExtension : public UObject
{
	GENERATED_BODY()

public:
	/** Returns the shared text extension instance. */
	static UArticyTextExtension* Get();

	/**
	 * Resolves all tokens inside Format, with respect to the context object Outer.
	 * Positional parameters are inserted into {0}, {1}, ... before resolving. If Format is a
	 * localization key, it is localized first.
	 */
	template<typename... Types>
	FText Resolve(UObject* Outer, const FText* Format, Types... Args) const;

	/** Same as the FText overload, for plain strings. */
	template<typename... Types>
	FText Resolve(UObject* Outer, const FString& Format, Types... Args) const;

	/**
	 * Traverses all tokens inside Format and calls Callback for each one. A callback that
	 * returns an unset TOptional falls back to the default resolving of that token.
	 */
	template<typename... Types>
	FText ResolveAdvance(UObject* Outer, const FText& Format, FArticyTokenResolverCallback Callback, Types... Args) const;

	/**
	 * Resolves tokens through a map of callbacks keyed by token source. Tokens whose source
	 * is not in the map are resolved with the default logic.
	 */
	template<typename... Types>
	FText ResolveAdvance(const FText& Format, TMap<FString, TFunction<FString(Types...)>> CallbackMap, Types... Args) const;

	/**
	 * Resolves a single token (without brackets) using the default logic. Useful inside a
	 * ResolveAdvance callback or a user method to fall back to the built-in resolving.
	 * Returns an empty string for a token that cannot be resolved.
	 */
	FString ResolveToken(UObject* Outer, const FString& Token) const;

	/** Resolves a single token using the default logic. */
	FString ResolveToken(const FArticyTextToken& Token) const;

	/** Registers a method that can be called inside texts as [MethodName(arg, ...)]. */
	void AddUserMethod(const FString& MethodName, FArticyUserMethodCallback Callback);

	/** Registers a method with the legacy argument-only signature. */
	void AddUserMethod(const FString& MethodName, FArticyLegacyUserMethodCallback Callback);

	/** Removes a user method again. Returns true if a method of that name was registered. */
	bool RemoveUserMethod(const FString& MethodName);

	/** Whether a user method of that name is registered. */
	bool HasUserMethod(const FString& MethodName) const;

	/** The indices picked by <?> random strip accessors during the last top-level resolve. */
	const TArray<int32>& GetLastRandomResults() const { return LastRandomResults; }

protected:
	/** The database used for object lookups. Overridable so tests can inject one. */
	virtual UArticyDatabase* GetDatabase(UObject* Outer) const;
	/** The global variables used for variable lookups. Overridable so tests can inject them. */
	virtual UArticyGlobalVariables* GetGlobalVariables(UObject* Outer) const;
	/** The type system used for $Type and enum lookups. Overridable so tests can inject one. */
	virtual UArticyTypeSystem* GetTypeSystem() const;
	/** Whether invalid tokens resolve to an empty string (true) or make the whole string resolve to its input (false). */
	virtual bool AllowInvalidTokens() const;

	FString FormatNumber(const FString& SourceValue, const FString& NumberFormat) const;
	FString ResolveBoolean(UObject* Outer, const FString& SourceName, const bool Value) const;
	FString LocalizeString(UObject* Outer, const FString& Input) const;
	static void SplitInstance(const FString& InString, FString& OutName, FString& OutInstanceNumber);

	/** Core of all Resolve overloads: resolves every token in Input. */
	FString ResolveInternal(UObject* Outer, const FString& Input, const TArray<FString>& Params, const FArticyTokenResolverCallback* Callback) const;

	TMap<FString, FArticyUserMethodCallback> UserMethodMap;
	mutable TArray<int32> LastRandomResults;

private:
	friend struct FArticyTextResolver;

	/** Localizes Input if it is a key; returns the input otherwise. */
	FString LocalizeInput(UObject* Outer, const FString& Input) const;

#if WITH_AUTOMATION_TESTS
public:
	// Test-only accessors for the protected helpers.
	FString Test_FormatNumber(const FString& SourceValue, const FString& NumberFormat) const { return FormatNumber(SourceValue, NumberFormat); }
	static void Test_SplitInstance(const FString& InString, FString& OutName, FString& OutInstanceNumber) { SplitInstance(InString, OutName, OutInstanceNumber); }
	FString Test_ResolveBoolean(UObject* Outer, const FString& SourceName, const bool Value) const { return ResolveBoolean(Outer, SourceName, Value); }
	void Test_ClearUserMethods() { UserMethodMap.Reset(); }
#endif
};

template<typename... Types>
FText UArticyTextExtension::Resolve(UObject* Outer, const FText* Format, Types... Args) const
{
	// Do not try to process null values
	if (Format == nullptr)
	{
		return FText::GetEmpty();
	}

	const TArray<FString> ArgumentValues = { ArticyTextExtensionPrivate::ArgToString(Args)... };
	return FText::FromString(ResolveInternal(Outer, Format->ToString(), ArgumentValues, nullptr));
}

template<typename... Types>
FText UArticyTextExtension::Resolve(UObject* Outer, const FString& Format, Types... Args) const
{
	const TArray<FString> ArgumentValues = { ArticyTextExtensionPrivate::ArgToString(Args)... };
	return FText::FromString(ResolveInternal(Outer, Format, ArgumentValues, nullptr));
}

template<typename... Types>
FText UArticyTextExtension::ResolveAdvance(UObject* Outer, const FText& Format, FArticyTokenResolverCallback Callback, Types... Args) const
{
	const TArray<FString> ArgumentValues = { ArticyTextExtensionPrivate::ArgToString(Args)... };
	return FText::FromString(ResolveInternal(Outer, Format.ToString(), ArgumentValues, Callback ? &Callback : nullptr));
}

template<typename... Types>
FText UArticyTextExtension::ResolveAdvance(const FText& Format, TMap<FString, TFunction<FString(Types...)>> CallbackMap, Types... Args) const
{
	// The map is keyed by the token source, i.e. the part before the optional ":format".
	const FArticyTokenResolverCallback Callback = [&CallbackMap, &Args...](const FArticyTextToken& Token) -> TOptional<FString>
	{
		FString SourceName = Token.Token;
		FString Formatting;
		if (Token.Token.Split(TEXT(":"), &SourceName, &Formatting))
		{
			SourceName.TrimStartAndEndInline();
		}
		if (const TFunction<FString(Types...)>* Found = CallbackMap.Find(SourceName))
		{
			return (*Found)(Args...);
		}
		return TOptional<FString>();
	};

	const TArray<FString> ArgumentValues = { ArticyTextExtensionPrivate::ArgToString(Args)... };
	return FText::FromString(ResolveInternal(nullptr, Format.ToString(), ArgumentValues, &Callback));
}
