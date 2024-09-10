//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonValue.h"
#include "ArticyTextExtension.generated.h"

using FArticyUserMethodCallback = TFunction<FString(const TArray<FString>&)>;

struct FArticyGvName;
class UArticyVariable;

UENUM()
enum class EArticyObjectType : uint8
{
	UArticyBool = 0,
	UArticyInt = 1,
	UArticyString = 2,
	Other
};

UCLASS(BlueprintType)
class ARTICYRUNTIME_API UArticyTextExtension : public UObject
{
	GENERATED_BODY()

public:
	static UArticyTextExtension* Get();
	
	template<typename... Types>
	FText Resolve
	(
		UObject* Outer,
		const FText* Format,
		Types... Args
	) const;

	template<typename ... Types>
	FText ResolveAdvance
	(
		const FText& Format,
		TMap<FString, TFunction<FString(Types...)>> CallbackMap,
		Types... Args
	) const;

	void AddUserMethod(const FString& MethodName, FArticyUserMethodCallback Callback);

protected:
	FString GetSource(UObject* Outer, const FString &SourceName) const;
	FString FormatNumber(const FString &SourceValue, const FString &NumberFormat) const;
	void GetGlobalVariable(UObject* Outer, const FString& SourceName, const FArticyGvName GvName, FString& OutString, bool& OutSuccess) const;
	void GetObjectProperty(UObject* Outer, const FString& SourceName, const FString& NameOrId, const FString& PropertyName, const bool bRequestType, FString& OutString, bool& OutSuccess) const;
	static void GetTypeProperty(const FString& TypeName, const FString& PropertyName, FString& OutString, bool& OutSuccess);
	FString ExecuteMethod(UObject* Outer, const FText& Method, const TArray<FString>& Args) const;
	EArticyObjectType GetObjectType(UArticyVariable** Object) const;
	FString ResolveBoolean(UObject* Outer, const FString &SourceName, const bool Value) const;
	FString LocalizeString(UObject* Outer, const FString &Input) const;
	static void SplitInstance(const FString& InString, FString& OutName, FString& OutInstanceNumber);

	TMap<FString, FArticyUserMethodCallback> UserMethodMap;
};

template<typename ... Types>
FText UArticyTextExtension::Resolve(UObject* Outer, const FText* Format, Types... Args) const
{
	// Do not try to process null values
	if (Format == nullptr)
	{
		return FText::GetEmpty();
	}

	TArray<FString> ArgumentValues = {FString::Printf(TEXT("%s"), Args)...};
    
	FString FormattedString = Format->ToString();

	// Regular placeholder replacement
	for (int32 ArgIndex = 0; ArgIndex < ArgumentValues.Num(); ++ArgIndex)
	{
		const FString Placeholder = FString::Printf(TEXT("{%d}"), ArgIndex);
		FormattedString = FormattedString.Replace(*Placeholder, *ArgumentValues[ArgIndex]);
	}

	// Token replacement
	while (true)
	{
		const int32 TokenStartIndex = FormattedString.Find(TEXT("["), ESearchCase::CaseSensitive);
		if (TokenStartIndex == INDEX_NONE)
			break;

		const int32 TokenEndIndex = FormattedString.Find(TEXT("]"), ESearchCase::CaseSensitive, ESearchDir::FromStart, TokenStartIndex);
		if (TokenEndIndex == INDEX_NONE)
			break;

		FString Token = FormattedString.Mid(TokenStartIndex + 1, TokenEndIndex - TokenStartIndex - 1);
		FString FullToken = FormattedString.Mid(TokenStartIndex, TokenEndIndex - TokenStartIndex + 1);
		FString SourceName, Formatting;
		
		Token.Split(TEXT(":"), &SourceName, &Formatting);
		if (SourceName.IsEmpty())
		{
			SourceName = Token;
		}
		
		if (!SourceName.IsEmpty())
		{
			// Get value from source
			FString SourceValue = GetSource(Outer, SourceName);
            
			if (!Formatting.IsEmpty())
			{
				// Custom format the SourceValue based on the rules of C#'s custom numeric format strings
				FString FormattedValue = FormatNumber(SourceValue, Formatting);
				FormattedString = FormattedString.Replace(*FullToken, *FormattedValue);
			}
			else
			{
				FormattedString = FormattedString.Replace(*FullToken, *SourceValue);
			}
		}
		else
		{
			// Remove invalid token if source name is empty
			FormattedString = FormattedString.Replace(*FullToken, TEXT(""));
		}
	}

	return FText::FromString(FormattedString);
}

template<typename ... Types>
FText UArticyTextExtension::ResolveAdvance(const FText& Format, TMap<FString, TFunction<FString(Types...)>> CallbackMap, Types... Args) const
{
	TArray<FString> ArgumentValues = {FString::Printf(TEXT("%s"), Args)...};
    
	FString FormattedString = Format.ToString();

	// Regular placeholder replacement
	for (int32 ArgIndex = 0; ArgIndex < ArgumentValues.Num(); ++ArgIndex)
	{
		const FString Placeholder = FString::Printf(TEXT("{%d}"), ArgIndex);
		FormattedString = FormattedString.Replace(*Placeholder, *ArgumentValues[ArgIndex]);
	}

	// Token replacement
	while (true)
	{
		const int32 TokenStartIndex = FormattedString.Find(TEXT("["), ESearchCase::CaseSensitive);
		if (TokenStartIndex == INDEX_NONE)
			break;

		const int32 TokenEndIndex = FormattedString.Find(TEXT("]"), ESearchCase::CaseSensitive, ESearchDir::FromStart, TokenStartIndex);
		if (TokenEndIndex == INDEX_NONE)
			break;

		FString Token = FormattedString.Mid(TokenStartIndex + 1, TokenEndIndex - TokenStartIndex - 1);
		FString FullToken = FormattedString.Mid(TokenStartIndex, TokenEndIndex - TokenStartIndex + 1);
		FString SourceName, Formatting;
		
		Token.Split(TEXT(":"), &SourceName, &Formatting);
		if (SourceName.IsEmpty())
		{
			SourceName = Token;
		}

		if (!SourceName.IsEmpty())
		{
			TFunction<FString(Types...)> Callback;
			if (CallbackMap.Contains(SourceName))
			{
				Callback = CallbackMap[SourceName];
				FString ReplacementValue = Callback(Args...);
				FormattedString = FormattedString.Replace(*FullToken, *ReplacementValue);
			}
			else
			{
				FormattedString = FormattedString.Replace(*FullToken, TEXT(""));
			}
		}
		else
		{
			FormattedString = FormattedString.Replace(*FullToken, TEXT(""));
		}
	}

	return FText::FromString(FormattedString);
}
