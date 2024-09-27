//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "Internationalization/Regex.h"
#include "ArticyObject.h"
#include "ArticyDatabase.h"
#include "ArticyScriptInterpreter.h"

#include "ArticyExpressoScripts.generated.h"

class UArticyString;
class UArticyInt;
class UArticyBool;
class UArticyExpressoScripts;
struct ExpressoType;

/**
 * @brief The ExpressoType struct represents a flexible data type used in the Articy runtime.
 *
 * The ExpressoType struct is designed to handle various types of data within the Articy runtime, such as booleans, integers, floats, and strings.
 * It provides implicit conversion operators for different data types, allowing seamless interactions with properties in Articy objects.
 */
struct ARTICYRUNTIME_API ExpressoType
{
    union
    {
        bool BoolValue;   ///< Boolean value of the ExpressoType
        int64 IntValue = 0;   ///< Integer value of the ExpressoType
        double FloatValue;   ///< Float value of the ExpressoType
    };
    FString StringValue;   ///< String value of the ExpressoType

    /**
     * @brief Enumeration for the type of the ExpressoType.
     */
    enum EType
    {
        Undefined, Bool, Int, Float, String
    } Type = Undefined;   ///< The type of the ExpressoType

    /**
     * @brief Retrieves the boolean value of the ExpressoType.
     *
     * @return The boolean value.
     */
    virtual bool& GetBool();

    /**
     * @brief Retrieves the boolean value of the ExpressoType.
     *
     * @return The boolean value.
     */
    virtual const bool& GetBool() const;

    /**
     * @brief Retrieves the int64 value of the ExpressoType.
     *
     * @return The int64 value.
     */
    virtual int64& GetInt();

    /**
     * @brief Retrieves the int64 value of the ExpressoType.
     *
     * @return The int64 value.
     */
    virtual const int64& GetInt() const;

    /**
     * @brief Retrieves the double value of the ExpressoType.
     *
     * @return The double value.
     */
    virtual double& GetFloat();

    /**
     * @brief Retrieves the double value of the ExpressoType.
     *
     * @return The double value.
     */
    virtual const double& GetFloat() const;

    /**
     * @brief Retrieves the string value of the ExpressoType.
     *
     * @return The string value.
     */
    virtual FString& GetString();

    /**
     * @brief Retrieves the string value of the ExpressoType.
     *
     * @return The string value.
     */
    virtual const FString& GetString() const;

    /**
     * @brief Converts the ExpressoType instance to a string representation.
     *
     * @return The string representation of the ExpressoType.
     */
    virtual FString ToString() const;


    //---------------------------------------------------------------------------//

    /**
     * @brief Default constructor for ExpressoType.
     */
    ExpressoType();

    /**
     * @brief Virtual destructor for ExpressoType.
     */
    virtual ~ExpressoType();

    /**
     * @brief Constructs an ExpressoType from an object and a property name.
     *
     * This constructor initializes an ExpressoType instance based on the specified property of a given object.
     *
     * @param Object The object containing the property.
     * @param Property The name of the property.
     */
    ExpressoType(UArticyBaseObject* Object, const FString& Property);

    // ReSharper disable CppNonExplicitConvertingConstructor

    //implicit conversion from value type

    /**
     * @brief Constructs an ExpressoType from a boolean value.
     *
     * @param Value The boolean value to initialize the ExpressoType with.
     */
    ExpressoType(const bool& Value);

    /**
     * @brief Constructs an ExpressoType from an int64 value.
     *
     * @param Value The int64 value to initialize the ExpressoType with.
     */
    ExpressoType(const int64& Value);

    /**
     * @brief Constructs an ExpressoType from an int32 value.
     *
     * @param Value The int32 value to initialize the ExpressoType with.
     */
    ExpressoType(const int32& Value);

    /**
     * @brief Constructs an ExpressoType from an int16 value.
     *
     * @param Value The int16 value to initialize the ExpressoType with.
     */
    ExpressoType(const int16& Value);

    /**
     * @brief Constructs an ExpressoType from an int8 value.
     *
     * @param Value The int8 value to initialize the ExpressoType with.
     */
    ExpressoType(const int8& Value);

    /**
     * @brief Constructs an ExpressoType from a uint64 value.
     *
     * @param Value The uint64 value to initialize the ExpressoType with.
     */
    ExpressoType(const uint64& Value);

    /**
     * @brief Constructs an ExpressoType from a uint32 value.
     *
     * @param Value The uint32 value to initialize the ExpressoType with.
     */
    ExpressoType(const uint32& Value);

    /**
     * @brief Constructs an ExpressoType from a uint16 value.
     *
     * @param Value The uint16 value to initialize the ExpressoType with.
     */
    ExpressoType(const uint16& Value);

    /**
     * @brief Constructs an ExpressoType from a uint8 value.
     *
     * @param Value The uint8 value to initialize the ExpressoType with.
     */
    ExpressoType(const uint8& Value);

    /**
     * @brief Constructs an ExpressoType from a double value.
     *
     * @param Value The double value to initialize the ExpressoType with.
     */
    ExpressoType(const double& Value);

    /**
     * @brief Constructs an ExpressoType from a float value.
     *
     * @param Value The float value to initialize the ExpressoType with.
     */
    ExpressoType(const float& Value);

    /**
     * @brief Constructs an ExpressoType from a string value.
     *
     * @param Value The string value to initialize the ExpressoType with.
     */
    ExpressoType(const FString& Value);

    /**
     * @brief Constructs an ExpressoType from an FText value.
     *
     * @param Value The FText value to initialize the ExpressoType with.
     */
    ExpressoType(const FText& Value);

    /**
     * @brief Constructs an ExpressoType from an FName value.
     *
     * @param Value The FName value to initialize the ExpressoType with.
     */
    ExpressoType(const FName& Value);

    /**
     * @brief Constructs an ExpressoType from an ArticyPrimitive object.
     *
     * This constructor initializes an ExpressoType instance based on the ID and clone ID of the specified object.
     *
     * @param Object The ArticyPrimitive object.
     */
    ExpressoType(const UArticyPrimitive* Object);

    /**
     * @brief Constructs an ExpressoType from an ArticyString value.
     *
     * @param Value The ArticyString value to initialize the ExpressoType with.
     */
    ExpressoType(const UArticyString& Value);

    /**
     * @brief Constructs an ExpressoType from an ArticyInt value.
     *
     * @param Value The ArticyInt value to initialize the ExpressoType with.
     */
    ExpressoType(const UArticyInt& Value);

    /**
     * @brief Constructs an ExpressoType from an ArticyBool value.
     *
     * @param Value The ArticyBool value to initialize the ExpressoType with.
     */
    ExpressoType(const UArticyBool& Value);

    /**
     * @brief Constructs an ExpressoType from an ArticyId value.
     *
     * @param Value The ArticyId value to initialize the ExpressoType with.
     */
    ExpressoType(const FArticyId& Value);

    //implicit conversion to value type

    /**
     * @brief Conversion operator to boolean.
     *
     * This operator converts the ExpressoType instance to a boolean value.
     *
     * @return The boolean value of the ExpressoType.
     */
    explicit operator bool() const;

    /**
     * @brief Conversion operator to int64.
     *
     * This operator converts the ExpressoType instance to an int64 value.
     *
     * @return The int64 value of the ExpressoType.
     */
    explicit operator int64() const;

    /**
     * @brief Conversion operator to int8.
     *
     * This operator converts the ExpressoType instance to an int8 value.
     *
     * @return The int8 value of the ExpressoType.
     */
    explicit operator int8() const;

    /**
     * @brief Conversion operator to uint8.
     *
     * This operator converts the ExpressoType instance to a uint8 value.
     *
     * @return The uint8 value of the ExpressoType.
     */
    explicit operator uint8() const;

    /**
     * @brief Conversion operator to int16.
     *
     * This operator converts the ExpressoType instance to an int16 value.
     *
     * @return The int16 value of the ExpressoType.
     */
    explicit operator int16() const;

    /**
     * @brief Conversion operator to uint16.
     *
     * This operator converts the ExpressoType instance to a uint16 value.
     *
     * @return The uint16 value of the ExpressoType.
     */
    explicit operator uint16() const;

    /**
     * @brief Conversion operator to int32.
     *
     * This operator converts the ExpressoType instance to an int32 value.
     *
     * @return The int32 value of the ExpressoType.
     */
    explicit operator int32() const;

    /**
     * @brief Conversion operator to uint32.
     *
     * This operator converts the ExpressoType instance to a uint32 value.
     *
     * @return The uint32 value of the ExpressoType.
     */
    explicit operator uint32() const;

    /**
     * @brief Conversion operator to uint64.
     *
     * This operator converts the ExpressoType instance to a uint64 value.
     *
     * @return The uint64 value of the ExpressoType.
     */
    explicit operator uint64() const;

    /**
     * @brief Conversion operator to double.
     *
     * This operator converts the ExpressoType instance to a double value.
     *
     * @return The double value of the ExpressoType.
     */
    explicit operator double() const;

    /**
     * @brief Conversion operator to float.
     *
     * This operator converts the ExpressoType instance to a float value.
     *
     * @return The float value of the ExpressoType.
     */
    explicit operator float() const;

    /**
     * @brief Conversion operator to FString.
     *
     * This operator converts the ExpressoType instance to an FString.
     *
     * @return The FString value of the ExpressoType.
     */
    explicit operator FString() const;

    /**
     * @brief Conversion operator to FText.
     *
     * This operator converts the ExpressoType instance to an FText.
     *
     * @return The FText value of the ExpressoType.
     */
    explicit operator FText() const;

    /**
     * @brief Conversion operator to FName.
     *
     * This operator converts the ExpressoType instance to an FName.
     *
     * @return The FName value of the ExpressoType.
     */
    explicit operator FName() const;

    /**
     * @brief Conversion operator to FArticyId.
     *
     * This operator converts the ExpressoType instance to an FArticyId.
     *
     * @return The FArticyId value of the ExpressoType.
     */
    explicit operator FArticyId() const;

    // ReSharper restore CppNonExplicitConvertingConstructor

    //---------------------------------------------------------------------------//

    /**
     * @brief Negates the ExpressoType instance.
     *
     * This operator returns a new ExpressoType instance with the negated value of the current instance.
     *
     * @return The negated ExpressoType instance.
     */
    ExpressoType operator-() const;

    /**
     * @brief Equality operator for ExpressoType.
     *
     * This operator compares two ExpressoType instances for equality.
     *
     * @param Other The other ExpressoType instance to compare.
     * @return True if the instances are equal, false otherwise.
     */
    bool operator==(const ExpressoType& Other) const;

    /**
     * @brief Inequality operator for ExpressoType.
     *
     * This operator compares two ExpressoType instances for inequality.
     *
     * @param Other The other ExpressoType instance to compare.
     * @return True if the instances are not equal, false otherwise.
     */
    bool operator!=(const ExpressoType& Other) const;

    /**
     * @brief Less-than operator for ExpressoType.
     *
     * This operator compares two ExpressoType instances for less-than relationship.
     *
     * @param Other The other ExpressoType instance to compare.
     * @return True if the current instance is less than the other instance, false otherwise.
     */
    bool operator<(const ExpressoType& Other) const;

    /**
     * @brief Greater-than operator for ExpressoType.
     *
     * This operator compares two ExpressoType instances for greater-than relationship.
     *
     * @param Other The other ExpressoType instance to compare.
     * @return True if the current instance is greater than the other instance, false otherwise.
     */
    bool operator>(const ExpressoType& Other) const;

    /**
     * @brief Less-than or equal-to operator for ExpressoType.
     *
     * This operator compares two ExpressoType instances for less-than or equal-to relationship.
     *
     * @param Other The other ExpressoType instance to compare.
     * @return True if the current instance is less than or equal to the other instance, false otherwise.
     */
    bool operator<=(const ExpressoType& Other) const;

    /**
     * @brief Greater-than or equal-to operator for ExpressoType.
     *
     * This operator compares two ExpressoType instances for greater-than or equal-to relationship.
     *
     * @param Other The other ExpressoType instance to compare.
     * @return True if the current instance is greater than or equal to the other instance, false otherwise.
     */
    bool operator>=(const ExpressoType& Other) const;

    /**
     * @brief Logical AND operator for ExpressoType.
     *
     * This operator performs logical AND operation on two ExpressoType instances.
     *
     * @param Other The other ExpressoType instance.
     * @return The result of the logical AND operation.
     */
    ExpressoType operator&&(const ExpressoType& Other) const;

    /**
     * @brief Logical OR operator for ExpressoType.
     *
     * This operator performs logical OR operation on two ExpressoType instances.
     *
     * @param Other The other ExpressoType instance.
     * @return The result of the logical OR operation.
     */
    ExpressoType operator||(const ExpressoType& Other) const;

    /**
     * @brief Logical XOR operator for ExpressoType.
     *
     * This operator performs logical XOR operation on two ExpressoType instances.
     *
     * @param Other The other ExpressoType instance.
     * @return The result of the logical XOR operation.
     */
    ExpressoType operator^(const ExpressoType& Other) const;

    /**
     * @brief Addition operator for ExpressoType.
     *
     * This operator performs addition operation on two ExpressoType instances.
     *
     * @param Other The other ExpressoType instance.
     * @return The result of the addition operation.
     */
    ExpressoType operator+(const ExpressoType& Other) const;

    /**
     * @brief Subtraction operator for ExpressoType.
     *
     * This operator performs subtraction operation on two ExpressoType instances.
     *
     * @param Other The other ExpressoType instance.
     * @return The result of the subtraction operation.
     */
    ExpressoType operator-(const ExpressoType& Other) const;

    /**
     * @brief Multiplication operator for ExpressoType.
     *
     * This operator performs multiplication operation on two ExpressoType instances.
     *
     * @param Other The other ExpressoType instance.
     * @return The result of the multiplication operation.
     */
    ExpressoType operator*(const ExpressoType& Other) const;

    /**
     * @brief Division operator for ExpressoType.
     *
     * This operator performs division operation on two ExpressoType instances.
     *
     * @param Other The other ExpressoType instance.
     * @return The result of the division operation.
     */
    ExpressoType operator/(const ExpressoType& Other) const;

    /**
     * @brief Modulo operator for ExpressoType.
     *
     * This operator performs modulo operation on two ExpressoType instances.
     *
     * @param Other The other ExpressoType instance.
     * @return The result of the modulo operation.
     */
    ExpressoType operator%(const ExpressoType& Other) const;

    //NOTE: No assignment/mutation operators are defined, as those are not
    //allowed anywhere where a ExpressoType is used!
    /*ExpressoType& operator++();
    ExpressoType operator++(int);
    ExpressoType& operator--();
    ExpressoType operator--(int);*/

    //---------------------------------------------------------------------------//

    /**
     * @brief The Definition struct represents a type definition for ExpressoType.
     *
     * The Definition struct contains factory and setter functions for creating and setting ExpressoType instances.
     */
    struct Definition;

    /**
     * @brief Map of type definitions for ExpressoType.
     *
     * This map contains a Factory and a Setter lambda for all the types that may exist as properties.
     * This is needed because we need the cpp type (template argument) for reflection.
     */
    static TMap<FName, Definition> Definitions;

    /**
     * @brief Retrieves the definition of the specified C++ type.
     *
     * This method returns the definition of the specified C++ type from the ExpressoType definitions map.
     *
     * @param CppType The name of the C++ type.
     * @return The definition of the specified type.
     */
    const Definition& GetDefinition(const FName& CppType) const;

    /**
     * @brief Adds a type definition for ExpressoType.
     *
     * This method adds a type definition for the specified C++ type to the ExpressoType definitions map.
     *
     * @tparam T The C++ type.
     * @param CppType The name of the C++ type.
     */
    template<typename T>
    static void AddDefinition(const FName& CppType);

    //---------------------------------------------------------------------------//

    /**
     * @brief Sets the value of a property on an object.
     *
     * This method sets the value of the specified property on the given object using the ExpressoType value.
     *
     * @param Object The object containing the property.
     * @param Property The name of the property.
     */
    void SetValue(UArticyBaseObject* Object, FString Property) const;

    /**
     * @brief Attempts to reroute a property to a feature object.
     *
     * This method checks if the specified property contains a dot, indicating that it is a feature property,
     * and reroutes it to the appropriate feature object if necessary.
     *
     * @param Object The object containing the property.
     * @param Property The name of the property.
     * @return The feature object, if rerouted, or the original object if not rerouted.
     */
    static UArticyBaseObject* TryFeatureReroute(UArticyBaseObject* Object, FString& Property);
};

/**
 * @brief The Definition struct represents a type definition for ExpressoType.
 *
 * The Definition struct contains factory and setter functions for creating and setting ExpressoType instances.
 */
struct ExpressoType::Definition
{
    /**
     * @brief Factory function to create an ExpressoType from a property.
     */
    TFunction<ExpressoType(UArticyBaseObject*, FProperty*)> Factory;

    /**
     * @brief Setter function to set a property from an ExpressoType value.
     */
    TFunction<void(UArticyBaseObject*, FProperty*, const ExpressoType&)> Setter;
};

/**
 * @brief Adds a type definition for ExpressoType.
 *
 * This method adds a type definition for the specified C++ type to the ExpressoType definitions map.
 *
 * @tparam T The C++ type.
 * @param CppType The name of the C++ type.
 */
template<typename T>
void ExpressoType::AddDefinition(const FName& CppType)
{
    Definition def;

    def.Factory = [](UArticyBaseObject* Object, FProperty* Property)
        {
            if (Object && Property)
            {
                T* ptr = Property->ContainerPtrToValuePtr<T>(Object);
                if (ptr)
                    return ExpressoType(*ptr);
            }

            return ExpressoType{};
        };

    def.Setter = [](UArticyBaseObject* Object, FProperty* Property, const ExpressoType& Value)
        {
            if (!Object || !Property)
                return;

            T* ptr = Property->ContainerPtrToValuePtr<T>(Object);
            if (ptr)
                (*ptr) = (T)Value;
        };

    Definitions.Add(CppType, def);
}

/**
 * @brief Addition operator for int and ExpressoType.
 *
 * This operator performs addition operation between an int and an ExpressoType instance.
 *
 * @param Lhs The int value.
 * @param Rhs The ExpressoType instance.
 * @return The result of the addition operation.
 */
FORCEINLINE int operator+(const int& Lhs, const ExpressoType& Rhs)
{
    return Lhs + (int)Rhs;
}

/**
 * @brief Subtraction operator for int and ExpressoType.
 *
 * This operator performs subtraction operation between an int and an ExpressoType instance.
 *
 * @param Lhs The int value.
 * @param Rhs The ExpressoType instance.
 * @return The result of the subtraction operation.
 */
FORCEINLINE int operator-(const int& Lhs, const ExpressoType& Rhs)
{
    return Lhs - (int)Rhs;
}

/**
 * @brief Multiplication operator for int and ExpressoType.
 *
 * This operator performs multiplication operation between an int and an ExpressoType instance.
 *
 * @param Lhs The int value.
 * @param Rhs The ExpressoType instance.
 * @return The result of the multiplication operation.
 */
FORCEINLINE int operator*(const int& Lhs, const ExpressoType& Rhs)
{
    return Lhs * (int)Rhs;
}

/**
 * @brief Division operator for int and ExpressoType.
 *
 * This operator performs division operation between an int and an ExpressoType instance.
 *
 * @param Lhs The int value.
 * @param Rhs The ExpressoType instance.
 * @return The result of the division operation.
 */
FORCEINLINE int operator/(const int& Lhs, const ExpressoType& Rhs)
{
    return Lhs / (int)Rhs;
}

/**
 * @brief Modulo operator for int and ExpressoType.
 *
 * This operator performs modulo operation between an int and an ExpressoType instance.
 *
 * @param Lhs The int value.
 * @param Rhs The ExpressoType instance.
 * @return The result of the modulo operation.
 */
FORCEINLINE int operator%(const int& Lhs, const ExpressoType& Rhs)
{
    return Lhs % (int)Rhs;
}

/**
 * @brief Addition operator for float and ExpressoType.
 *
 * This operator performs addition operation between a float and an ExpressoType instance.
 *
 * @param Lhs The float value.
 * @param Rhs The ExpressoType instance.
 * @return The result of the addition operation.
 */
FORCEINLINE float operator+(const float& Lhs, const ExpressoType& Rhs)
{
    return Lhs + (float)Rhs;
}

/**
 * @brief Subtraction operator for float and ExpressoType.
 *
 * This operator performs subtraction operation between a float and an ExpressoType instance.
 *
 * @param Lhs The float value.
 * @param Rhs The ExpressoType instance.
 * @return The result of the subtraction operation.
 */
FORCEINLINE float operator-(const float& Lhs, const ExpressoType& Rhs)
{
    return Lhs - (float)Rhs;
}

/**
 * @brief Multiplication operator for float and ExpressoType.
 *
 * This operator performs multiplication operation between a float and an ExpressoType instance.
 *
 * @param Lhs The float value.
 * @param Rhs The ExpressoType instance.
 * @return The result of the multiplication operation.
 */
FORCEINLINE float operator*(const float& Lhs, const ExpressoType& Rhs)
{
    return Lhs * (float)Rhs;
}

/**
 * @brief Division operator for float and ExpressoType.
 *
 * This operator performs division operation between a float and an ExpressoType instance.
 *
 * @param Lhs The float value.
 * @param Rhs The ExpressoType instance.
 * @return The result of the division operation.
 */
FORCEINLINE float operator/(const float& Lhs, const ExpressoType& Rhs)
{
    return Lhs / (float)Rhs;
}

/**
 * @brief The UArticyExpressoScripts class manages script conditions and instructions.
 *
 * The UArticyExpressoScripts class is responsible for evaluating conditions and executing instructions
 * within the Articy runtime. It provides functionality to interact with Articy objects, properties,
 * and global variables, and supports various script-related operations.
 */
UCLASS(abstract)
class ARTICYRUNTIME_API UArticyExpressoScripts : public UObject
{
    GENERATED_BODY()

public:

    /**
     * @brief Default constructor for UArticyExpressoScripts.
     */
    UArticyExpressoScripts()
    {
        //add empty condition and instruction
        Conditions.Add(GetTypeHash(FString{ "" }), [&] { return true; });
        Instructions.Add(GetTypeHash(FString{ "" }), [&] { return; });
    }

    /**
     * @brief Retrieves the user methods provider interface.
     *
     * This method returns the user methods provider interface for script evaluation and execution.
     *
     * @return The user methods provider interface.
     */
    virtual UClass* GetUserMethodsProviderInterface() { return nullptr; }

    /**
     * @brief Initializes the expresso scripts with the specified database.
     *
     * This method initializes the expresso scripts with the specified Articy database.
     *
     * @param DB The Articy database.
     */
    virtual void Init(UArticyDatabase* DB) { OwningDatabase = DB; }

    /**
     * @brief Retrieves the Articy database.
     *
     * This method returns the Articy database associated with the expresso scripts.
     *
     * @return The Articy database.
     */
    UArticyDatabase* GetDb() const { return OwningDatabase; }


    /**
     * @brief Sets the current object for script evaluation.
     *
     * This method sets the current object used for script evaluation.
     *
     * @param Object The current object.
     */
    void SetCurrentObject(UArticyPrimitive* Object) { self = Object; }

    /**
     * @brief Sets the speaker for dialog fragments.
     *
     * This method sets the speaker for dialog fragments within the expresso scripts.
     *
     * @param Speaker The speaker object.
     */
    void SetSpeaker(UArticyObject* Speaker) { speaker = Speaker; }

    /**
     * @brief Evaluates the condition and returns the result.
     *
     * This method evaluates the specified condition fragment using the given global variables
     * and method provider, and returns the result of the evaluation.
     *
     * @param ConditionFragmentHash The hash of the condition fragment.
     * @param GV The global variables used in the evaluation.
     * @param MethodProvider The method provider used in the evaluation.
     * @return The result of the evaluation.
     */
    bool Evaluate(const int& ConditionFragmentHash, UArticyGlobalVariables* GV, UObject* MethodProvider) const;

    /**
     * @brief Executes the instruction and returns true if successful.
     *
     * This method executes the specified instruction fragment using the given global variables
     * and method provider, and returns true if the execution was successful.
     *
     * @param InstructionFragmentHash The hash of the instruction fragment.
     * @param GV The global variables used in the execution.
     * @param MethodProvider The method provider used in the execution.
     * @return True if the execution was successful, false otherwise.
     */
    bool Execute(const int& InstructionFragmentHash, UArticyGlobalVariables* GV, UObject* MethodProvider) const;

    /**
     * @brief Sets a default method provider for script evaluation and execution.
     *
     * This method sets a default method provider that will be used whenever scripts are evaluated
     * or executed without a valid method provider.
     *
     * @param MethodProvider The default method provider.
     */
    void SetDefaultUserMethodsProvider(UObject* MethodProvider);

    /**
     * @brief Retrieves the default user methods provider.
     *
     * This method returns the default user methods provider used for script evaluation and execution.
     *
     * @return The default user methods provider.
     */
    UObject* GetDefaultUserMethodsProvider() const;

    /**
     * @brief Retrieves the user methods provider object.
     *
     * This method returns the current user methods provider object used for script evaluation and execution.
     *
     * @return The user methods provider object.
     */
    UObject* GetUserMethodsProviderObject() const;

    /**
     * @brief Retrieves the active global variables instance.
     *
     * This method returns the active global variables instance in use during script evaluation and execution,
     * or nullptr if no instance is active.
     *
     * @return The active global variables instance, or nullptr.
     */
    virtual UArticyGlobalVariables* GetGV() { return nullptr; }

    TArray<FString> SerializedScripts;

protected:

    /**
     * @brief Sets the global variables instance for script execution.
     *
     * This method sets the global variables instance to be used during the execution of expresso script fragments.
     *
     * @param GV The global variables instance.
     */
    virtual void SetGV(UArticyGlobalVariables* GV) const { }

    //========================================//

    /**
     * @brief The current object for script evaluation.
     *
     * The current object where the script is evaluated on. Do not change the name, as it's called like this in script fragments!
     */
    UArticyPrimitive* self = nullptr;

    /**
     * @brief The speaker for dialog fragments.
     *
     * Inside a DialogFragment, this is a reference to the current speaker. Do not change the name, as it's called like this in script fragments!
     */
    UArticyObject* speaker = nullptr;

    /**
     * @brief Map of condition fragments.
     *
     * This map contains condition fragments identified by their hash values, represented as lambda functions.
     */
    TMap<uint32, TFunction<bool()>> Conditions;

    /**
     * @brief Map of instruction fragments.
     *
     * This map contains instruction fragments identified by their hash values, represented as lambda functions.
     */
    TMap<uint32, TFunction<void()>> Instructions;

    /**
     * @brief Retrieves an Articy object by name or ID.
     *
     * This method returns an Articy object based on the specified name or ID and clone ID.
     *
     * @param NameOrId The name or ID of the object.
     * @param CloneId The clone ID of the object.
     * @return The Articy object.
     */
    UArticyObject* getObj(const FString& NameOrId, const uint32& CloneId = 0) const;

    /**
     * @brief Sets the value of a property on an Articy object.
     *
     * This method sets the value of the specified property on the given Articy object using the ExpressoType value.
     *
     * @param Object The Articy object containing the property.
     * @param Property The name of the property.
     * @param Value The value to set.
     */
    static void setProp(UArticyBaseObject* Object, const FString& Property, const ExpressoType& Value);

    /**
     * @brief Sets the value of a property on an Articy object.
     *
     * This method sets the value of the specified property on the given Articy object using the ExpressoType value.
     *
     * @param Id_CloneId The compound ID of the object.
     * @param Property The name of the property.
     * @param Value The value to set.
     */
    void setProp(const ExpressoType& Id_CloneId, const FString& Property, const ExpressoType& Value) const;

    /**
     * @brief Retrieves the value of a property on an Articy object.
     *
     * This method returns the value of the specified property on the given Articy object as an ExpressoType.
     *
     * @param Object The Articy object containing the property.
     * @param Property The name of the property.
     * @return The value of the property.
     */
    static ExpressoType getProp(UArticyBaseObject* Object, const FString& Property);

    /**
     * @brief Retrieves the value of a property on an Articy object.
     *
     * This method returns the value of the specified property on the given Articy object as an ExpressoType.
     *
     * @param Id_CloneId The compound ID of the object.
     * @param Property The name of the property.
     * @return The value of the property.
     */
    ExpressoType getProp(const ExpressoType& Id_CloneId, const FString& Property) const;

    /**
     * @brief Generates a random integer between Min and Max.
     *
     * This method returns a random integer between the specified minimum and maximum values.
     *
     * @param Min The minimum value.
     * @param Max The maximum value.
     * @return The random integer.
     */
    int random(int Min, int Max);

    /**
     * @brief Generates a random integer between 0 and Max.
     *
     * This method returns a random integer between 0 and the specified maximum value.
     *
     * @param Max The maximum value.
     * @return The random integer.
     */
    int random(int Max);

    /**
     * @brief Generates a random float between Min and Max.
     *
     * This method returns a random float between the specified minimum and maximum values.
     *
     * @param Min The minimum value.
     * @param Max The maximum value.
     * @return The random float.
     */
    float random(float Min, float Max);

    /**
     * @brief Generates a random float between 0 and Max.
     *
     * This method returns a random float between 0 and the specified maximum value.
     *
     * @param Max The maximum value.
     * @return The random float.
     */
    float random(float Max);

    /**
     * @brief Generates a random ExpressoType between Min and Max.
     *
     * This method returns a random ExpressoType between the specified minimum and maximum values.
     *
     * @param Min The minimum value.
     * @param Max The maximum value.
     * @return The random ExpressoType.
     */
    ExpressoType random(const ExpressoType& Min, const ExpressoType& Max);

    /**
     * @brief Generates a random ExpressoType between 0 and Max.
     *
     * This method returns a random ExpressoType between 0 and the specified maximum value.
     *
     * @param Max The maximum value.
     * @return The random ExpressoType.
     */
    ExpressoType random(const ExpressoType& Max);

    /**
     * @brief Increments the value of a property on an Articy object.
     *
     * This method increments the value of the specified property on the given Articy object by the specified value.
     *
     * @param Object The Articy object containing the property.
     * @param Property The name of the property.
     * @param Value The increment value (default is 1).
     */
    static void incrementProp(UArticyBaseObject* Object, const FString& Property, const float Value = 1);

    /**
     * @brief Increments the value of a property on an Articy object.
     *
     * This method increments the value of the specified property on the given Articy object by the specified value.
     *
     * @param Id_CloneId The compound ID of the object.
     * @param Property The name of the property.
     * @param Value The increment value (default is 1).
     */
    void incrementProp(const ExpressoType& Id_CloneId, const FString& Property, const float Value = 1) const;

    /**
     * @brief Decrements the value of a property on an Articy object.
     *
     * This method decrements the value of the specified property on the given Articy object by the specified value.
     *
     * @param Object The Articy object containing the property.
     * @param Property The name of the property.
     * @param Value The decrement value (default is 1).
     */
    static void decrementProp(UArticyBaseObject* Object, const FString& Property, const float Value = 1);

    /**
     * @brief Decrements the value of a property on an Articy object.
     *
     * This method decrements the value of the specified property on the given Articy object by the specified value.
     *
     * @param Id_CloneId The compound ID of the object.
     * @param Property The name of the property.
     * @param Value The decrement value (default is 1).
     */
    void decrementProp(const ExpressoType& Id_CloneId, const FString& Property, const float Value = 1) const;

    /**
     * @brief Checks if a value is within a range.
     *
     * This method checks if the specified value is within the given lower and upper bounds.
     *
     * @param valueToTest The value to test.
     * @param lowerBound The lower bound.
     * @param upperBound The upper bound.
     * @return True if the value is within the range, false otherwise.
     */
    static bool isInRange(float valueToTest, float lowerBound, float upperBound);

    /**
     * @brief Checks if a property value is within a range.
     *
     * This method checks if the value of the specified property on the given Articy object is within the given lower and upper bounds.
     *
     * @param Object The Articy object containing the property.
     * @param Property The name of the property.
     * @param lowerBound The lower bound.
     * @param upperBound The upper bound.
     * @return True if the property value is within the range, false otherwise.
     */
    static bool isPropInRange(UArticyBaseObject* Object, const FString& Property, float lowerBound, float upperBound);

    /**
     * @brief Checks if a property value is within a range.
     *
     * This method checks if the value of the specified property on the given Articy object is within the given lower and upper bounds.
     *
     * @param Id_CloneId The compound ID of the object.
     * @param Property The name of the property.
     * @param lowerBound The lower bound.
     * @param upperBound The upper bound.
     * @return True if the property value is within the range, false otherwise.
     */
    bool isPropInRange(const ExpressoType& Id_CloneId, const FString& Property, float lowerBound, float upperBound) const;

    /**
     * @brief Checks if a string value is within a range.
     *
     * This method checks if the specified string value is within the given lower and upper bounds.
     *
     * @param valueToTest The string value to test.
     * @param lowerBound The lower bound.
     * @param upperBound The upper bound.
     * @return True if the string value is within the range, false otherwise.
     */
    static bool isInRange(const FString& valueToTest, const FString& lowerBound, const FString& upperBound);

    /**
     * @brief Checks if a property value is within a range.
     *
     * This method checks if the value of the specified property on the given Articy object is within the given lower and upper bounds.
     *
     * @param Object The Articy object containing the property.
     * @param Property The name of the property.
     * @param lowerBound The lower bound.
     * @param upperBound The upper bound.
     * @return True if the property value is within the range, false otherwise.
     */
    static bool isPropInRange(UArticyBaseObject* Object, const FString& Property, const FString& lowerBound, const FString& upperBound);

    /**
     * @brief Checks if a property value is within a range.
     *
     * This method checks if the value of the specified property on the given Articy object is within the given lower and upper bounds.
     *
     * @param Id_CloneId The compound ID of the object.
     * @param Property The name of the property.
     * @param lowerBound The lower bound.
     * @param upperBound The upper bound.
     * @return True if the property value is within the range, false otherwise.
     */
    bool isPropInRange(const ExpressoType& Id_CloneId, const FString& Property, const FString& lowerBound, const FString& upperBound) const;

    /**
     * @brief Resets all seen counters in the global variables.
     *
     * This method resets all seen counters in the global variables.
     */
    void resetAllSeenCounters();

    /**
     * @brief Retrieves the seen counter for an Articy object.
     *
     * This method returns the seen counter for the specified Articy object, or the current object if none is specified.
     *
     * @param Object The Articy object (default is nullptr, which means the current object).
     * @return The seen counter value.
     */
    int getSeenCounter(UArticyBaseObject* Object = nullptr);

    /**
     * @brief Retrieves the seen counter for an Articy object by name or ID.
     *
     * This method returns the seen counter for the specified Articy object based on its name or ID.
     *
     * @param NameOrId The name or ID of the object.
     * @return The seen counter value.
     */
    int getSeenCounter(const FString& NameOrId);

    /**
     * @brief Sets the seen counter for the current object.
     *
     * This method sets the seen counter for the current object to the specified value.
     *
     * @param Value The value to set (default is 1).
     * @return The updated seen counter value.
     */
    int setSeenCounter(const int Value = 1);

    /**
     * @brief Sets the seen counter for an Articy object.
     *
     * This method sets the seen counter for the specified Articy object to the specified value.
     *
     * @param Object The Articy object.
     * @param Value The value to set.
     * @return The updated seen counter value.
     */
    int setSeenCounter(UArticyBaseObject* Object, const int Value = 1);

    /**
     * @brief Sets the seen counter for an Articy object by name or ID.
     *
     * This method sets the seen counter for the specified Articy object based on its name or ID to the specified value.
     *
     * @param NameOrId The name or ID of the object.
     * @param Value The value to set.
     * @return The updated seen counter value.
     */
    int setSeenCounter(const FString& NameOrId, const int Value = 1);

    /**
     * @brief Performs a fallback operation for an Articy object.
     *
     * This method performs a fallback operation for the specified Articy object.
     *
     * @param Object The Articy object.
     * @return True if the fallback operation was successful, false otherwise.
     */
    bool fallback(UArticyBaseObject* Object);

    /**
     * @brief Performs a fallback operation for an Articy object by name or ID.
     *
     * This method performs a fallback operation for the specified Articy object based on its name or ID.
     *
     * @param NameOrId The name or ID of the object.
     * @return True if the fallback operation was successful, false otherwise.
     */
    bool fallback(const FString& NameOrId);

    /**
     * @brief Performs a fallback operation for the current object.
     *
     * This method performs a fallback operation for the current object.
     *
     * @return True if the fallback operation was successful, false otherwise.
     */
    bool fallback();

    /**
     * @brief Prints a formatted message to the log.
     *
     * This method prints a formatted message to the log, allowing placeholders in the message.
     *
     * @tparam ArgTypes The types of the arguments for formatting.
     * @param Msg The message to print.
     * @param Args The arguments for formatting the message.
     */
    template<typename ...ArgTypes>
    static void print(const FString& Msg, ArgTypes... Args);

    /**
     * @brief Prints a formatted message to the log.
     *
     * This method prints a formatted message to the log, allowing placeholders in the message.
     *
     * @tparam ArgTypes The types of the arguments for formatting.
     * @param Msg The message to print.
     * @param Args The arguments for formatting the message.
     */
    template<typename ...ArgTypes>
    static void print(const ExpressoType& Msg, ArgTypes... Args)
    {
        print(Msg.ToString(), Args...);
    }

    /**
     * @brief Script conditions that are not empty return the condition value.
     *
     * This method returns the condition value for script conditions that are not empty.
     *
     * @param Condition The condition value.
     * @return The condition value.
     */
    static const bool& ConditionOrTrue(const bool& Condition) { return Condition; }

    /**
     * @brief Script conditions that should evaluate to bool, but conditions evaluates to int.
     *
     * This method returns true if the integer condition is greater than zero, false otherwise.
     *
     * @param Condition The integer condition value.
     * @return True if the condition is greater than zero, false otherwise.
     */
    static const bool ConditionOrTrue(const int& Condition) { return Condition > 0; }

    /**
     * @brief Script conditions that are empty or only contain a comment always return true.
     *
     * This method returns true for script conditions that are empty or only contain a comment.
     *
     * @return True for empty or comment-only conditions.
     */
    static bool ConditionOrTrue(void /*JustAComment*/) { return true; }

    static bool ConditionOrTrue(const FString& Condition, const UArticyGlobalVariables* Globals)
    {
        return UArticyScriptInterpreter::EvaluateCondition(Condition, Globals);
    }

    /**
     * @brief Cache of the current methods provider set during evaluation.
     *
     * This member variable stores the current methods provider set during the evaluation of scripts.
     */
    mutable UObject* UserMethodsProvider = nullptr;

    /**
     * @brief Default methods provider for script evaluation.
     *
     * This member variable stores the default methods provider used for script evaluation and execution.
     * It is a weak pointer in case the provider is deleted.
     */
    TWeakObjectPtr<UObject> DefaultUserMethodsProvider = nullptr;

private:

    /**
     * @brief Articy database associated with the expresso scripts.
     *
     * This member variable stores the Articy database associated with the expresso scripts.
     */
    UArticyDatabase* OwningDatabase = nullptr;

    /**
     * @brief Retrieves an Articy object by a compound ID.
     *
     * This method returns an Articy object based on a compound ID, which is a string representation of ID and clone ID.
     *
     * @param Id_CloneId The compound ID.
     * @return The Articy object.
     */
    UArticyObject* getObjInternal(const ExpressoType& Id_CloneId) const;

    /**
     * @brief Prints a message to the log.
     *
     * This method prints the specified message to the log.
     *
     * @param msg The message to print.
     */
    static void PrintInternal(const FString& msg);
};

/**
 * @brief Prints a formatted message to the log.
 *
 * This method prints a formatted message to the log, allowing placeholders in the message.
 *
 * @tparam ArgTypes The types of the arguments for formatting.
 * @param Msg The message to print.
 * @param Args The arguments for formatting the message.
 */
template <typename ... ArgTypes>
void UArticyExpressoScripts::print(const FString& Msg, ArgTypes... Args)
{
    auto msg = Msg;

    auto arr = TArray<ExpressoType>{ Args... };
    for (int i = 0; i < arr.Num(); ++i)
        msg = msg.Replace(*FString::Printf(TEXT("{%d}"), i), *FString{ arr[i] });

    PrintInternal(msg);
}
