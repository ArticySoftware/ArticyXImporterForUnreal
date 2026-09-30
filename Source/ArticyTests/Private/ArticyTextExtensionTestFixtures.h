//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "ArticyObject.h"
#include "ArticyBaseObject.h"
#include "ArticyDatabase.h"
#include "ArticyGlobalVariables.h"
#include "ArticyExpressoScripts.h"
#include "ArticyScriptFragment.h"
#include "ArticyTextExtension.h"
#include "ArticyTypeSystem.h"
#include "Interfaces/ArticyObjectWithDisplayName.h"
#include "Interfaces/ArticyObjectWithSpeaker.h"
#include "ArticyTextExtensionTestFixtures.generated.h"

/**
 * Hand-authored stand-ins for generated content, so the text extension can be exercised
 * against a real database, real global variables and real script fragments without a
 * project that has imported articy content.
 */

/** Plays the role of a generated articy enum. */
UENUM()
enum class EArticyTextTestClass : uint8
{
	Warrior = 0,
	Mage = 1,
	Bard = 2
};

/** A script condition whose expression tests can set (Expression is protected on the real class). */
UCLASS()
class UArticyTextTestCondition : public UArticyScriptCondition
{
	GENERATED_BODY()

public:
	void SetExpression(const FString& InExpression) { Expression = InExpression; }
};

/** A script instruction whose expression tests can set. */
UCLASS()
class UArticyTextTestInstruction : public UArticyScriptInstruction
{
	GENERATED_BODY()

public:
	void SetExpression(const FString& InExpression) { Expression = InExpression; }
};

/** Plays the role of a generated template feature ("Character"). */
UCLASS()
class UArticyTextTestFeature : public UArticyBaseFeature
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int32 HP = 0;

	UPROPERTY()
	float HitRating = 0.f;

	UPROPERTY()
	bool IsHero = false;

	UPROPERTY()
	FString Motivation;

	/** An FText property, like every localized template text: holds a loca key. */
	UPROPERTY()
	FText Title;

	/** A reference slot. */
	UPROPERTY()
	FArticyId Companion;

	/** A reference strip. */
	UPROPERTY()
	TArray<FArticyId> Inventory;

	UPROPERTY()
	EArticyTextTestClass ClassEnum = EArticyTextTestClass::Warrior;

	UPROPERTY()
	UArticyTextTestCondition* IsStrong = nullptr;

	UPROPERTY()
	UArticyTextTestInstruction* Heal = nullptr;

	/** A string holding an object representation, as a script would store one. */
	UPROPERTY()
	FString Partner;
};

/** Plays the role of a generated entity / dialogue fragment class. */
UCLASS()
class UArticyTextTestEntity : public UArticyObject, public IArticyObjectWithDisplayName, public IArticyObjectWithSpeaker
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FText DisplayName;

	UPROPERTY()
	FArticyId Speaker;

	UPROPERTY()
	UArticyTextTestFeature* Character = nullptr;

	UPROPERTY()
	int32 ZIndex = 0;

	UPROPERTY()
	FString Nickname;

	/** Id and TechnicalName are protected on the base classes; the fixture needs to assign them. */
	void SetIdentity(uint64 InId, const FString& InTechnicalName)
	{
		Id = FArticyId{ InId };
		TechnicalName = InTechnicalName;
	}
};

/** The global variable namespace "Session". */
UCLASS()
class UArticyTextTestVariableSet : public UArticyBaseVariableSet
{
	GENERATED_BODY()

public:
	UPROPERTY()
	UArticyBool* Flag = nullptr;

	UPROPERTY()
	UArticyInt* Score = nullptr;

	UPROPERTY()
	UArticyString* PlayerName = nullptr;

	UPROPERTY()
	UArticyString* Counterpart = nullptr;

	UPROPERTY()
	UArticyString* Player1 = nullptr;

	UPROPERTY()
	UArticyString* Player2 = nullptr;

	UPROPERTY()
	UArticyInt* CurrentPlayerIndex = nullptr;

	UPROPERTY()
	UArticyInt* Gold = nullptr;

	UPROPERTY()
	UArticyString* Title = nullptr;
};

/** Plays the role of the generated global variables class. */
UCLASS()
class UArticyTextTestGlobalVariables : public UArticyGlobalVariables
{
	GENERATED_BODY()

public:
	UPROPERTY()
	UArticyTextTestVariableSet* Session = nullptr;

	template<typename TVariable>
	TVariable* MakeVariable(const TCHAR* Name, const typename TVariable::UnderlyingType& Value)
	{
		TVariable* Variable = NewObject<TVariable>(Session);
		Variable->template Init<TVariable>(Session, this, FName(Name), Value);
		return Variable;
	}

	void Populate(const FString& CounterpartRepresentation)
	{
		Session = NewObject<UArticyTextTestVariableSet>(this);
		Session->Flag = MakeVariable<UArticyBool>(TEXT("Session.Flag"), true);
		Session->Score = MakeVariable<UArticyInt>(TEXT("Session.Score"), 42);
		Session->PlayerName = MakeVariable<UArticyString>(TEXT("Session.PlayerName"), TEXT("Bob"));
		Session->Counterpart = MakeVariable<UArticyString>(TEXT("Session.Counterpart"), CounterpartRepresentation);
		Session->Player1 = MakeVariable<UArticyString>(TEXT("Session.Player1"), TEXT("Bob"));
		Session->Player2 = MakeVariable<UArticyString>(TEXT("Session.Player2"), TEXT("Anna"));
		Session->CurrentPlayerIndex = MakeVariable<UArticyInt>(TEXT("Session.CurrentPlayerIndex"), 2);
		Session->Gold = MakeVariable<UArticyInt>(TEXT("Session.Gold"), 1234567);
		Session->Title = MakeVariable<UArticyString>(TEXT("Session.Title"), TEXT("Loca.Title"));
	}
};

/**
 * Stand-in for the generated ExpressoScripts class: registers the conditions and
 * instructions the fixture's script properties refer to.
 */
UCLASS()
class UArticyTextTestExpresso : public UArticyExpressoScripts
{
	GENERATED_BODY()

public:
	static int32 Hash(const TCHAR* Expression) { return static_cast<int32>(GetTypeHash(FString(Expression))); }

	UArticyTextTestExpresso()
	{
		Conditions.Add(Hash(TEXT("strong")), [&] { return true; });
		Conditions.Add(Hash(TEXT("weak")), [&] { return false; });
		Conditions.Add(Hash(TEXT("hasSelf")), [&] { return self != nullptr; });
		Instructions.Add(Hash(TEXT("heal")), [&] { ++HealCount; });
	}

	void SetGV(UArticyGlobalVariables* GV) const override { ActiveGV = GV; }
	UArticyGlobalVariables* GetGV() override { return ActiveGV.Get(); }

	/** How often the "heal" instruction ran. */
	int32 HealCount = 0;

private:
	mutable TWeakObjectPtr<UArticyGlobalVariables> ActiveGV = nullptr;
};

/** A text extension whose database, variables, type system and settings tests can inject. */
UCLASS()
class UArticyTestTextExtension : public UArticyTextExtension
{
	GENERATED_BODY()

public:
	UPROPERTY()
	UArticyDatabase* Database = nullptr;

	UPROPERTY()
	UArticyGlobalVariables* GlobalVariables = nullptr;

	UPROPERTY()
	UArticyTypeSystem* TypeSystem = nullptr;

	TOptional<bool> AllowInvalidTokensOverride;

protected:
	UArticyDatabase* GetDatabase(UObject* Outer) const override { return Database; }
	UArticyGlobalVariables* GetGlobalVariables(UObject* Outer) const override { return GlobalVariables; }
	UArticyTypeSystem* GetTypeSystem() const override { return TypeSystem ? TypeSystem : Super::GetTypeSystem(); }
	bool AllowInvalidTokens() const override
	{
		return AllowInvalidTokensOverride.IsSet() ? AllowInvalidTokensOverride.GetValue() : Super::AllowInvalidTokens();
	}
};
