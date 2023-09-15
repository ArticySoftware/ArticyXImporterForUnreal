//  
// Copyright (c) articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyTextExtension.h"
#include "ArticyDatabase.h"
#include "ArticyGlobalVariables.h"
#include "ArticyTypeSystem.h"

UArticyTextExtension* UArticyTextExtension::Get()
{
	static TWeakObjectPtr<UArticyTextExtension> ArticyTextExtension;

	if (!ArticyTextExtension.IsValid())
	{
		ArticyTextExtension = TWeakObjectPtr<UArticyTextExtension>(NewObject<UArticyTextExtension>());
	}

	return ArticyTextExtension.Get();
}

FString UArticyTextExtension::GetSource(const FString& SourceName) const
{
	if (SourceName.IsEmpty())
	{
		return TEXT("");
	}

	// Split the SourceName by dots
	TArray<FString> SourceParts;
	SourceName.ParseIntoArray(SourceParts, TEXT("."));
	if (SourceParts.Num() == 0)
	{
		return TEXT("");
	}

	const FString MethodName = SourceParts[0];
	FString Parameters;
	FString RemValue;

	// Construct Parameters and RemValue
	for (int32 Index = 1; Index < SourceParts.Num(); ++Index)
	{
		if (!Parameters.IsEmpty())
		{
			Parameters += TEXT(",");
			RemValue += TEXT(".");
		}
		Parameters += SourceParts[Index];
		RemValue += SourceParts[Index];
	}

	// Check if Parameters represent a method
	if (Parameters.Contains(TEXT("(")) && Parameters.Contains(TEXT(")")))
	{
		FString Method;
		FString ArgsString;
		Parameters.Split(TEXT("("), &Method, &ArgsString);
		ArgsString.RemoveFromEnd(TEXT(")"));
		TArray<FString> Args;
		ArgsString.ParseIntoArray(Args, TEXT(","), true);

		// Execute the method
		return ExecuteMethod(FText::FromString(Method), Args);
	}

	// Process types
	if (MethodName.StartsWith(TEXT("$Type.")))
	{
		const FString TypeName = MethodName.Mid(6);
		bool bSuccess = false;
		FString Result;
		GetTypeProperty(TypeName, RemValue, Result, bSuccess);
		return bSuccess ? Result : SourceName;
	}

	// Process Global Variables
	const FArticyGvName GvName = FArticyGvName(FName(MethodName), FName(RemValue));
	bool bSuccess = false;
	FString Result;
	GetGlobalVariable(SourceName, GvName, Result, bSuccess);
	if (bSuccess)
	{
		return Result;
	}

	// Type for object
	bool bRequestType = false;
	if (RemValue.EndsWith(TEXT(".$Type")))
	{
		RemValue = RemValue.LeftChop(6);  // Using LeftChop is more efficient than Left + Len
		bRequestType = true;
	}

	// Process Objects & Script Properties
	GetObjectProperty(SourceName, MethodName, RemValue, bRequestType, Result, bSuccess);
	return bSuccess ? Result : SourceName;
}

// Process SourceValue with NumberFormat according to C# Custom Number Formatting rules
FString UArticyTextExtension::FormatNumber(const FString& SourceValue, const FString& NumberFormat) const
{
	double Value;
	// Handle booleans
	if (SourceValue.Equals(TEXT("true")))
	{
		Value = 1.f;
	}
	else if (SourceValue.Equals(TEXT("false")))
	{
		Value = 0.f;
	}
	else
	{
		Value = FCString::Atof(*SourceValue);
	}
	
	FString FormattedValue;

	int32 FormatIndex = 0;

	while (FormatIndex < NumberFormat.Len())
	{
		const TCHAR CurrentChar = NumberFormat[FormatIndex];
		if (CurrentChar == '0')
		{
			int32 ZeroCount = 1;
			while (FormatIndex + ZeroCount < NumberFormat.Len() && NumberFormat[FormatIndex + ZeroCount] == '0')
				ZeroCount++;

			FormattedValue += FString::Printf(TEXT("%0*lld"), ZeroCount, FMath::RoundToInt(Value));
			FormatIndex += ZeroCount;
		}
		else if (CurrentChar == '#')
		{
			int32 DigitCount = 1;
			while (FormatIndex + DigitCount < NumberFormat.Len() && NumberFormat[FormatIndex + DigitCount] == '#')
				DigitCount++;

			FormattedValue += FString::Printf(TEXT("%.*f"), DigitCount, Value);
			FormatIndex += DigitCount;
		}
		else if (CurrentChar == '.')
		{
			int32 FractionalPartCount = 1;
			while (FormatIndex + FractionalPartCount < NumberFormat.Len() && NumberFormat[FormatIndex + FractionalPartCount] == '#')
				FractionalPartCount++;

			FormattedValue += FString::Printf(TEXT("%.*f"), FractionalPartCount, Value);
			FormatIndex += FractionalPartCount;
		}
		else
		{
			FormattedValue += CurrentChar;
			FormatIndex++;
		}
	}

	return FormattedValue;
}

// Process Global Variables
void UArticyTextExtension::GetGlobalVariable(const FString& SourceName, FArticyGvName GvName, FString& OutString, bool& OutSuccess) const
{
	// Get the GV set
	const auto DB = UArticyDatabase::Get(this);
	const auto Gvs = DB->GetGVs();
	const auto Set = Gvs->GetNamespace(GvName.GetNamespace());
	
	if (!Set)
	{
		OutSuccess = false;
		return;
	}

	// Handle according to type
	switch (UArticyVariable** Variable = Set->GetPropPtr<UArticyVariable*>(GvName.GetVariable()); GetObjectType(Variable))
	{
	case EArticyObjectType::UArticyBool:
		OutString = ResolveBoolean(SourceName, Gvs->GetBoolVariable(GvName, OutSuccess));
		break;
	case EArticyObjectType::UArticyInt:
		OutString = FString::FromInt(Gvs->GetIntVariable(GvName, OutSuccess));
		break;
	case EArticyObjectType::UArticyString:
		OutString = Gvs->GetStringVariable(GvName, OutSuccess);
		break;
	default:
		OutSuccess = false;
		break;
	}
}

void UArticyTextExtension::GetObjectProperty(const FString& SourceName, const FString& NameOrId, const FString& PropertyName, const bool bRequestType, FString& OutString, bool& OutSuccess) const
{
	const auto DB = UArticyDatabase::Get(this);
	UArticyObject* Object = nullptr;

	// Split and get object based on input type
	FString ObjectName, ObjectInstance;
	SplitInstance(NameOrId, ObjectName, ObjectInstance);
	if (NameOrId.StartsWith(TEXT("0x")))
	{
		Object = DB->GetObject<UArticyObject>(FArticyId{ArticyHelpers::HexToUint64(ObjectName)}, FCString::Atod(*ObjectInstance));
	}
	else if (NameOrId.IsNumeric())
	{
		Object = DB->GetObject<UArticyObject>(FArticyId{FCString::Strtoui64(*ObjectName, nullptr, 10)}, FCString::Atod(*ObjectInstance));
	}
	else
	{
		Object = DB->GetObjectByName(*ObjectName, FCString::Atod(*ObjectInstance));
	}

	if (!Object)
	{
		OutSuccess = false;
		return;
	}

	TArray<FString> SourceParts;
	PropertyName.ParseIntoArray(SourceParts, TEXT("."));

	for (int Depth = 1; Depth < SourceParts.Num(); Depth++)
	{
		const auto& Children = Object->GetChildren();
		const auto FoundChild = Children.FindByPredicate([&](const TWeakObjectPtr<UArticyObject>& ChildPtr)
		{
			if (ChildPtr.IsValid())
			{
				if (SourceParts[Depth].StartsWith(TEXT("0x")) && ChildPtr->GetId() == ArticyHelpers::HexToUint64(SourceParts[Depth]))
				{
					return true;
				}
				if (ChildPtr->GetTechnicalName().IsEqual(FName(SourceParts[Depth])))
				{
					return true;
				}
			}
			return false;
		});
		
		Object = FoundChild ? FoundChild->Get() : nullptr;

		if (!Object)
		{
			OutSuccess = false;
			return;
		}
	}

	if (bRequestType)
	{
		OutString = Object->ArticyType.GetProperty(PropertyName).PropertyType;
		OutSuccess = true;
		return;
	}

	ExpressoType PropertyType {Object, PropertyName};
	switch (PropertyType.Type) 
	{
	case ExpressoType::Bool:
		OutString = ResolveBoolean(SourceName, PropertyType.GetBool());
		break;
	case ExpressoType::Int:
		OutString = FString::FromInt(PropertyType.GetInt());
		break;
	case ExpressoType::Float:
		OutString = FString::SanitizeFloat(PropertyType.GetFloat());
		break;
	case ExpressoType::String:
		OutString = PropertyType.GetString();
		break;
	default:
		OutSuccess = false;
		return;
	}
	OutSuccess = true;
}

void UArticyTextExtension::GetTypeProperty(const FString& TypeName, const FString& PropertyName, FString& OutString, bool& OutSuccess)
{
	UArticyTypeSystem* TypeSystem = UArticyTypeSystem::Get();
	FArticyType TypeData = TypeSystem->GetArticyType(TypeName);
	FArticyPropertyInfo PropertyInfo{};
	bool bFoundProperty = false;

	// Split the PropertyName to check for feature prefix
	TArray<FString> NameParts;
	PropertyName.ParseIntoArray(NameParts, TEXT("."));

	TArray<FArticyPropertyInfo> RelevantProperties;

	// Handle feature prefix
	if (NameParts.Num() > 1 && TypeData.Features.Contains(NameParts[0]))
	{
		RelevantProperties = TypeData.GetPropertiesInFeature(NameParts[0]);
	}
	else
	{
		RelevantProperties = TypeData.Properties;
	}

	for (const auto& Property : RelevantProperties)
	{
		if (Property.TechnicalName.Equals(NameParts.Last()) || Property.LocaKey_DisplayName.Equals(NameParts.Last()))
		{
			PropertyInfo = Property;
			bFoundProperty = true;
			break;
		}
	}

	if (!bFoundProperty)
	{
		OutSuccess = false;
		return;
	}

	OutString = PropertyInfo.PropertyType;
	OutSuccess = true;
}

FString UArticyTextExtension::ExecuteMethod(const FText& Method, const TArray<FString>& Args) const
{
	const FString MethodStr = Method.ToString();

	if (Args.Num() >= 3)
	{
		// Handle internal methods
		if (MethodStr == TEXT("if") || MethodStr == TEXT("not"))
		{
			const FText ResolveResult = Resolve(FText::FromString(Args[0]), *Args[1], TEXT("0"));

			// Flip args for not vs if
			if (ResolveResult.ToString() == TEXT("1"))
			{
				return (MethodStr == TEXT("if")) ? Args[2] : Args[3];
			}
			return (MethodStr == TEXT("if")) ? Args[3] : Args[2];
		}
	}

	// Handle user methods
	if (UserMethodMap.Contains(MethodStr))
	{
		const FArticyUserMethodCallback Callback = UserMethodMap[MethodStr];
		return Callback(Args);
	}

	return TEXT("");
}

EArticyObjectType UArticyTextExtension::GetObjectType(UArticyVariable** Object) const
{
	if (Cast<UArticyBool>(*Object))
	{
		return EArticyObjectType::UArticyBool;
	}
	if (Cast<UArticyInt>(*Object))
	{
		return EArticyObjectType::UArticyInt;
	}
	if (Cast<UArticyString>(*Object))
	{
		return EArticyObjectType::UArticyString;
	}

	return EArticyObjectType::Other;
}

FString UArticyTextExtension::ResolveBoolean(const FString &SourceName, const bool Value) const
{
	FString SourceValue;
	FString SourceInput[2];
	SourceInput[0] = SourceName;
	if (Value)
	{
		SourceInput[1] = TEXT("True");
		SourceValue = LocalizeString(FString::Join(SourceInput, TEXT(".")));		
	}
	else
	{
		SourceInput[1] = TEXT("False");
		SourceValue = LocalizeString(FString::Join(SourceInput, TEXT(".")));		
	}
	if (!SourceValue.IsEmpty())
	{
		return SourceValue;
	}
	
	FString VariableConstants;
	if (Value)
	{
		VariableConstants = LocalizeString(TEXT("VariableConstants.Boolean.True"));		
	}
	else
	{
		VariableConstants = LocalizeString(TEXT("VariableConstants.Boolean.False"));
	}
	if (!VariableConstants.IsEmpty())
	{
		return VariableConstants;
	}

	if (Value)
	{
		return TEXT("true");
	}

	return TEXT("false");
}

FString UArticyTextExtension::LocalizeString(const FString& Input) const
{
	const FText MissingEntry = FText::FromString("<MISSING STRING TABLE ENTRY>");

	// Look up entry in specified string table
	TOptional<FString> TableName = FTextInspector::GetNamespace(FText::FromString(Input));
	if (!TableName.IsSet())
	{
		TableName = TEXT("ARTICY");
	}
	const FText SourceString = FText::FromStringTable(
		FName(TableName.GetValue()),
		Input,
		EStringTableLoadingPolicy::FindOrFullyLoad);
	const FString Decoded = SourceString.ToString();
	if (!SourceString.IsEmpty() && !SourceString.EqualTo(MissingEntry))
	{
		return SourceString.ToString();
	}

	// By default, return empty
	return TEXT("");
}

void UArticyTextExtension::SplitInstance(const FString& InString, FString& OutName, FString& OutInstanceNumber)
{
	const FString SearchSubstr = TEXT("<");
	const int32 StartIdx = InString.Find(SearchSubstr);

	if (StartIdx != INDEX_NONE)
	{
		const int32 EndIdx = InString.Find(TEXT(">"), ESearchCase::CaseSensitive, ESearchDir::FromEnd, StartIdx);

		if (EndIdx != INDEX_NONE)
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

// Helper function to replace placeholders with given arguments
FString UArticyTextExtension::ReplacePlaceholders(const FString& Input, const TArray<FString>& ArgumentValues)
{
	FString Output = Input;
	for (int32 ArgIndex = 0; ArgIndex < ArgumentValues.Num(); ++ArgIndex)
	{
		const FString Placeholder = FString::Printf(TEXT("{%d}"), ArgIndex);
		Output = Output.Replace(*Placeholder, *ArgumentValues[ArgIndex]);
	}
	return Output;
}

// Helper function to process tokens
FString UArticyTextExtension::ProcessTokens(const FString& Input, TFunction<FString(const FString&, const FString&)> TokenHandler)
{
	FString Output = Input;

	while (true)
	{
		const int32 TokenStartIndex = Output.Find(TEXT("["), ESearchCase::CaseSensitive);
		if (TokenStartIndex == INDEX_NONE)
			break;

		const int32 TokenEndIndex = Output.Find(TEXT("]"), ESearchCase::CaseSensitive, ESearchDir::FromStart, TokenStartIndex);
		if (TokenEndIndex == INDEX_NONE)
			break;

		FString Token = Output.Mid(TokenStartIndex + 1, TokenEndIndex - TokenStartIndex - 1);
		FString FullToken = Output.Mid(TokenStartIndex, TokenEndIndex - TokenStartIndex + 1);
		FString SourceName, Formatting;

		Token.Split(TEXT(":"), &SourceName, &Formatting);
		if (SourceName.IsEmpty())
		{
			SourceName = Token;
		}

		FString Replacement = TokenHandler(SourceName, Formatting);
		Output = Output.Replace(*FullToken, *Replacement);
	}
	return Output;
}

void UArticyTextExtension::AddUserMethod(const FString& MethodName, const FArticyUserMethodCallback Callback)
{
	// Add the callback to the user method map
	UserMethodMap.Add(MethodName, Callback);
}
