//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyTextExtension.h"
#include "ArticyDatabase.h"
#include "ArticyGlobalVariables.h"
#include "ArticyTypeSystem.h"
#include "ArticyHelpers.h"

UArticyTextExtension* UArticyTextExtension::Get()
{
	static TWeakObjectPtr<UArticyTextExtension> ArticyTextExtension;

	if (!ArticyTextExtension.IsValid())
	{
		ArticyTextExtension = TWeakObjectPtr<UArticyTextExtension>(NewObject<UArticyTextExtension>());
	}

	return ArticyTextExtension.Get();
}

// Retrieve string from specified source
FString UArticyTextExtension::GetSource(UObject* Outer, const FString& SourceName) const
{
	// Split the SourceName by dots
	TArray<FString> SourceParts;
	SourceName.ParseIntoArray(SourceParts, TEXT("."));
	FString RemValue = TEXT("");
	FString Result = TEXT("");
	bool bSuccess = false;

	if (SourceParts.Num() > 0)
	{
		FString MethodName = SourceParts[0];
        
		FString Parameters;
		for (int32 Index = 1; Index < SourceParts.Num(); ++Index)
		{
			if (!Parameters.IsEmpty())
			{
				Parameters += TEXT(",");
			}
			Parameters += SourceParts[Index];
		}
        
		if (Parameters.Contains(TEXT("(")) && Parameters.Contains(TEXT(")")))
		{
			FString Method;
			FString ArgsString;
			Parameters.Split(TEXT("("), &Method, &ArgsString);

			ArgsString.RemoveFromEnd(TEXT(")"));
			TArray<FString> Args;
			ArgsString.ParseIntoArray(Args, TEXT(","), true);

			// Execute the method
			return ExecuteMethod(Outer, FText::FromString(Method), Args);
		}
	}
	else
	{
		// No source
		return Result;
	}

	if (SourceParts.Num() > 1)
	{
		for (int32 Index = 1; Index < SourceParts.Num(); ++Index)
		{
			if (!RemValue.IsEmpty())
			{
				RemValue += TEXT(".");
			}
			RemValue += SourceParts[Index];
		}
	}

	// Process types
	if (SourceParts[0].StartsWith(TEXT("$Type.")))
	{
		const FString TypeName = SourceParts[0].Mid(6);
		GetTypeProperty(TypeName, RemValue, Result, bSuccess);
		
		if (bSuccess)
		{
			return Result;
		}

		return SourceName;
	}

	// Process Global Variables
	const FArticyGvName GvName = FArticyGvName(FName(SourceParts[0]), FName(RemValue));
	GetGlobalVariable(Outer, SourceName, GvName, Result, bSuccess);
	if (bSuccess)
	{
		return Result;
	}

	// Type for object
	bool bRequestType = false;
	if (RemValue.EndsWith(TEXT(".$Type")))
	{
		RemValue = RemValue.Left(RemValue.Len() - 6);
		bRequestType = true;
	}

	// Process Objects & Script Properties
	GetObjectProperty(Outer, SourceName, SourceParts[0], RemValue, bRequestType, Result, bSuccess);
	if (bSuccess)
	{
		return Result;
	}
	
	return SourceName;
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
void UArticyTextExtension::GetGlobalVariable(UObject* Outer, const FString& SourceName, FArticyGvName GvName, FString& OutString, bool& OutSuccess) const
{
	const auto& WorldContext = GEngine->GetWorldFromContextObject(Outer, EGetWorldErrorMode::ReturnNull);
	if (!WorldContext)
	{
		// Cannot use this context
		OutString = SourceName;
		OutSuccess = false;
		return;
	}

	const auto& DB = UArticyDatabase::Get(Outer);
	const auto& GlobalVariables = DB->GetGVs();
	const auto& Set = GlobalVariables->GetNamespace(GvName.GetNamespace());
	if (!Set)
	{
		OutSuccess = false;
		return;
	}
	
	UArticyVariable** Variable = Set->GetPropPtr<UArticyVariable*>(GvName.GetVariable());
	switch (GetObjectType(Variable))
	{
	case EArticyObjectType::UArticyBool:
		{
			const auto boolValue = GlobalVariables->GetBoolVariable(GvName, OutSuccess);
			if (OutSuccess)
			{
				OutString = ResolveBoolean(Outer, SourceName, boolValue);
			}
			break;
		}
	case EArticyObjectType::UArticyInt:
		{
			OutString = FString::FromInt(GlobalVariables->GetIntVariable(GvName, OutSuccess));
			break;
		}
	case EArticyObjectType::UArticyString:
		{
			OutString = GlobalVariables->GetStringVariable(GvName, OutSuccess);
			break;
		}
	default:
		{
			OutSuccess = false;
			break;
		}
	}
}

void UArticyTextExtension::GetObjectProperty(UObject* Outer, const FString& SourceName, const FString& NameOrId, const FString& PropertyName, const bool bRequestType, FString& OutString, bool& OutSuccess) const
{
	const auto& WorldContext = GEngine->GetWorldFromContextObject(Outer, EGetWorldErrorMode::ReturnNull);
	if (!WorldContext)
	{
		// Cannot use this context
		OutString = SourceName;
		OutSuccess = false;
		return;
	}

	// Get the object
	const auto& DB = UArticyDatabase::Get(this);
	UArticyObject* Object;

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

	if (bRequestType)
	{
		OutString = Object->ArticyType.GetProperty(PropertyName).PropertyType;
		OutSuccess = true;
		return;
	}

	// Handle based on data type
	ExpressoType PropertyType {Object, PropertyName};
	switch (PropertyType.Type) {
	case ExpressoType::Bool:
		{
			OutString = ResolveBoolean(Outer, SourceName, PropertyType.GetBool());
			OutSuccess = true;
			break;
		}
	case ExpressoType::Int:
		{
			OutString = FString::FromInt(PropertyType.GetInt());
			OutSuccess = true;
			break;
		}
	case ExpressoType::Float:
		{
			OutString = FString::SanitizeFloat(PropertyType.GetFloat());
			OutSuccess = true;
			break;
		}
	case ExpressoType::String:
		{
			OutString = PropertyType.GetString();
			OutSuccess = true;
			break;
		}
	default:
		{
			OutSuccess = false;
		}
	}
}

void UArticyTextExtension::GetTypeProperty(const FString& TypeName, const FString& PropertyName, FString& OutString,
	bool& OutSuccess)
{
	UArticyTypeSystem* TypeSystem = UArticyTypeSystem::Get();
	FArticyType TypeData = TypeSystem->GetArticyType(TypeName);
	FArticyPropertyInfo PropertyInfo{};
	bool bFoundProperty = false;
	
	for (const auto& Property : TypeData.Properties)
	{
		if (Property.TechnicalName.Equals(PropertyName))
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

FString UArticyTextExtension::ExecuteMethod(UObject* Outer, const FText& Method, const TArray<FString>& Args) const
{
	if (Method.ToString() == TEXT("if"))
	{
		if (Args.Num() >= 3)
		{
			const FText& ResolveString = FText::FromString(Args[0]);
			const FText& ResolveResult = Resolve(Outer, &ResolveString, *Args[1], TEXT("0"));
            
			if (ResolveResult.ToString() == TEXT("1"))
			{
				return Args[2];
			}
			return Args[3];
		}
	}
	else if (Method.ToString() == TEXT("not"))
	{
		if (Args.Num() >= 3)
		{
			const FText& ResolveString = FText::FromString(Args[0]);
			const FText& ResolveResult = Resolve(Outer, &ResolveString, *Args[1], TEXT("0"));
            
			if (ResolveResult.ToString() == TEXT("1"))
			{
				return Args[3];
			}
			return Args[2];
		}
	}
	else
	{
		if (UserMethodMap.Contains(Method.ToString()))
		{
			const FArticyUserMethodCallback Callback = UserMethodMap[Method.ToString()];
			return Callback(Args);
		}
	}
    
	return TEXT("");
}

EArticyObjectType UArticyTextExtension::GetObjectType(UArticyVariable** Object) const
{
	// TODO: Use type system
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

FString UArticyTextExtension::ResolveBoolean(UObject* Outer, const FString &SourceName, const bool Value) const
{
	FString SourceValue;
	FString SourceInput[2];
	SourceInput[0] = SourceName;
	if (Value)
	{
		SourceInput[1] = TEXT("True");
		SourceValue = LocalizeString(Outer, FString::Join(SourceInput, TEXT(".")));		
	}
	else
	{
		SourceInput[1] = TEXT("False");
		SourceValue = LocalizeString(Outer, FString::Join(SourceInput, TEXT(".")));		
	}
	if (!SourceValue.IsEmpty())
	{
		return SourceValue;
	}
	
	FString VariableConstants;
	if (Value)
	{
		VariableConstants = LocalizeString(Outer, TEXT("VariableConstants.Boolean.True"));		
	}
	else
	{
		VariableConstants = LocalizeString(Outer, TEXT("VariableConstants.Boolean.False"));
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

void UArticyTextExtension::AddUserMethod(const FString& MethodName, const FArticyUserMethodCallback Callback)
{
	// Add the callback to the user method map
	UserMethodMap.Add(MethodName, Callback);
}
