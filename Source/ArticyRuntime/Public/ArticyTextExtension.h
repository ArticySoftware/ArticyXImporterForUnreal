//  
// Copyright (c) articy Software GmbH & Co. KG. All rights reserved.  
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
	
	template<typename ... Args>
	FText Resolve(const FText& Format, Args... args) const
	{
		TArray<FString> ArgumentValues = {FString::Printf(TEXT("%s"), args)...};

		FString FormattedString = ReplacePlaceholders(Format.ToString(), ArgumentValues);

		FormattedString = ProcessTokens(FormattedString, [&](const FString& SourceName, const FString& Formatting) -> FString {
			FString SourceValue = GetSource(SourceName);
			if (!Formatting.IsEmpty())
			{
				return FormatNumber(SourceValue, Formatting);
			}
			return SourceValue.IsEmpty() ? TEXT("") : SourceValue;
		});

		return FText::FromString(FormattedString);
	}

	template<typename ... Args>
	FText ResolveAdvance(const FText& Format, TMap<FString, TFunction<FString(Args...)>> CallbackMap, Args... args) const
	{
		TArray<FString> ArgumentValues = {FString::Printf(TEXT("%s"), args)...};

		FString FormattedString = ReplacePlaceholders(Format.ToString(), ArgumentValues);

		FormattedString = ProcessTokens(FormattedString, [&](const FString& SourceName, const FString&) -> FString {
			TFunction<FString(Args...)> Callback;
			if (CallbackMap.Contains(SourceName))
			{
				Callback = CallbackMap[SourceName];
				return Callback(args...);
			}
			return TEXT("");
		});

		return FText::FromString(FormattedString);
	}

	void AddUserMethod(const FString& MethodName, FArticyUserMethodCallback Callback);

protected:
	FString GetSource(const FString &SourceName) const;
	FString FormatNumber(const FString &SourceValue, const FString &NumberFormat) const;
	void GetGlobalVariable(const FString& SourceName, const FArticyGvName GvName, FString& OutString, bool& OutSuccess) const;
	void GetObjectProperty(const FString& SourceName, const FString& NameOrId, const FString& PropertyName, const bool bRequestType, FString& OutString, bool& OutSuccess) const;
	static void GetTypeProperty(const FString& TypeName, const FString& PropertyName, FString& OutString, bool& OutSuccess);
	FString ExecuteMethod(const FText& Method, const TArray<FString>& Args) const;
	EArticyObjectType GetObjectType(UArticyVariable** Object) const;
	FString ResolveBoolean(const FString &SourceName, const bool Value) const;
	FString LocalizeString(const FString &Input) const;
	static void SplitInstance(const FString& InString, FString& OutName, FString& OutInstanceNumber);
	static FString ReplacePlaceholders(const FString& Input, const TArray<FString>& ArgumentValues);
	static FString ProcessTokens(const FString& Input, TFunction<FString(const FString&, const FString&)> TokenHandler);

	TMap<FString, FArticyUserMethodCallback> UserMethodMap;
};

