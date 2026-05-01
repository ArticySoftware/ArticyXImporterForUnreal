//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CodeGenerator.h"

/**
 * @class CodeFileGenerator
 * @brief A class to generate and manage code files with convenience methods for generating code lines, comments, and more.
 *
 * Holds a content string that can be written to a file and includes methods for generating code structures like classes and structs.
 */
class CodeFileGenerator
{
public:

	/**
	 * @brief Executes a lambda function safely.
	 *
	 * If the Lambda is nullptr (type nullptr_t), nothing happens.
	 *
	 * @tparam Lambda The type of the lambda function to execute.
	 * @param Lamb The lambda function to execute.
	 */
	template<typename Lambda>
	void SafeExecute(Lambda Lamb);

	/**
	 * @brief Creates a new code file generator with default lines and executes a content generator.
	 *
	 * Adds default content such as copyright information and then calls WriteToFile.
	 *
	 * @tparam Lambda The type of the content generator function.
	 * @param Path The path to the file to be generated.
	 * @param bHeader Whether the file is a header file.
	 * @param ContentGenerator The function generating the file content.
	 */
	template<typename Lambda>
	CodeFileGenerator(const FString& Path, const bool bHeader, Lambda ContentGenerator);

	/**
	 * @brief Adds a line to the file content.
	 *
	 * @param Line The line of code to add.
	 * @param bSemicolon Whether to append a semicolon at the end of the line.
	 * @param bIndent Whether to indent the line.
	 * @param IndentOffset The number of additional indentations to apply.
	 */
	void Line(const FString& Line = "", const bool bSemicolon = false, const bool bIndent = true, const int IndentOffset = 0);

	/**
	 * @brief Adds a comment to the file content.
	 *
	 * @param Text The comment text to add.
	 */
	void Comment(const FString& Text);

	/**
	 * @brief Adds an access modifier with optional colon.
	 *
	 * @param Text The access modifier text.
	 */
	void AccessModifier(const FString& Text);

	/**
	 * @brief Adds a UPROPERTY macro to the file content.
	 *
	 * @param Specifiers The specifiers for the UPROPERTY macro.
	 */
	void UPropertyMacro(const FString& Specifiers);

	/**
	 * @brief Adds a UFUNCTION macro to the file content.
	 *
	 * @param Specifiers The specifiers for the UFUNCTION macro.
	 */
	void UFunctionMacro(const FString& Specifiers);

	//========================================//

	/**
	 * @brief Adds a block of code with optional indentation and semicolon.
	 *
	 * @tparam Lambda The type of the content function.
	 * @param bIndent Whether to indent the block.
	 * @param Content The function generating the block content.
	 * @param bSemicolonAtEnd Whether to append a semicolon after the block.
	 */
	template<typename Lambda>
	void Block(const bool bIndent, Lambda Content, const bool bSemicolonAtEnd = false);

	/**
	 * @brief Adds a class definition with optional UCLASS macro.
	 *
	 * @tparam Lambda The type of the content function.
	 * @param Classname The name of the class.
	 * @param Comment A comment describing the class.
	 * @param bUClass Whether the class is a UCLASS.
	 * @param Content The function generating the class content.
	 * @param UClassSpecifiers The specifiers for the UCLASS macro.
	 */
	template<typename Lambda>
	void Class(const FString& Classname, const FString& Comment, const bool bUClass, Lambda Content, const FString& UClassSpecifiers = "BlueprintType");

	/**
	 * @brief Adds a struct definition with optional USTRUCT macro.
	 *
	 * @tparam Lambda The type of the content function.
	 * @param Structname The name of the struct.
	 * @param Comment A comment describing the struct.
	 * @param bUStruct Whether the struct is a USTRUCT.
	 * @param Content The function generating the struct content.
	 * @param InlineDeclaration An optional inline declaration for the struct.
	 */
	template<typename Lambda>
	void Struct(const FString& Structname, const FString& Comment, const bool bUStruct, Lambda Content, const FString& InlineDeclaration = "");

	/**
	 * @brief Adds a UINTERFACE definition with corresponding interface class.
	 *
	 * @tparam Lambda The type of the content function.
	 * @param Classname The name of the interface class.
	 * @param UInterfaceSpecifiers The specifiers for the UINTERFACE macro.
	 * @param Comment A comment describing the interface.
	 * @param Content The function generating the interface content.
	 */
	template<typename Lambda>
	void UInterface(const FString& Classname, const FString& UInterfaceSpecifiers, const FString& Comment, Lambda Content);

	/**
	 * @brief Adds an enum definition with optional UENUM macro.
	 *
	 * @tparam Type The type of the enum values.
	 * @param Enumname The name of the enum.
	 * @param Comment A comment describing the enum.
	 * @param bUEnum Whether the enum is a UENUM.
	 * @param Values The values of the enum.
	 */
	template<typename Type>
	void Enum(const FString& Enumname, const FString& Comment, const bool bUEnum, TArray<Type> Values);

	//========================================//

	/**
	 * @brief Adds a variable definition, optionally as UPROPERTY.
	 *
	 * @param Type The type of the variable.
	 * @param Name The name of the variable.
	 * @param Value The initial value of the variable.
	 * @param Comment A comment describing the variable.
	 * @param bUProperty Whether to add a UPROPERTY macro.
	 * @param UPropertySpecifiers The specifiers for the UPROPERTY macro.
	 */
	void Variable(const FString& Type, const FString& Name, const FString& Value = "", const FString& Comment = "",
		const bool bUProperty = false, const FString& UPropertySpecifiers = "VisibleAnywhere, BlueprintReadOnly");

	/**
	 * @brief Adds a method definition, optionally as UFUNCTION.
	 *
	 * @tparam Lambda The type of the method definition function.
	 * @param ReturnType The return type of the method.
	 * @param Name The name of the method.
	 * @param Parameters The parameters of the method.
	 * @param Definition The function defining the method.
	 * @param Comment A comment describing the method.
	 * @param bUFunction Whether to add a UFUNCTION macro.
	 * @param UFunctionSpecifiers The specifiers for the UFUNCTION macro.
	 * @param MethodSpecifiers Additional specifiers for the method.
	 */
	template<typename Lambda>
	void Method(const FString& ReturnType, const FString& Name, const FString& Parameters = "", Lambda Definition = nullptr, const FString& Comment = "",
		const bool bUFunction = false, const FString& UFunctionSpecifiers = "", const FString& MethodSpecifiers = "");


private:

	const FString Path; ///< The path to the generated file.
	FString FileContent = ""; ///< The content of the generated file.

	int IndentCount = 0; ///< The current indentation level.
	uint8 BlockCount = 0; ///< The current block count.

	/**
	 * @brief Increases the indentation level.
	 */
	void PushIndent() { ++IndentCount; }

	/**
	 * @brief Decreases the indentation level.
	 */
	void PopIndent() { IndentCount = FMath::Max(0, IndentCount - 1); }

	/**
	 * @brief Starts a block and optionally indents it.
	 *
	 * @param bIndent Whether to indent the block.
	 */
	void StartBlock(const bool bIndent = true);

	/**
	 * @brief Ends a block and optionally unindents it.
	 *
	 * @param bUnindent Whether to unindent the block.
	 * @param bSemicolon Whether to append a semicolon after the block.
	 */
	void EndBlock(const bool bUnindent = true, const bool bSemicolon = false);

	/**
	 * @brief Starts a class definition and optionally indents it.
	 *
	 * @param Classname The name of the class.
	 * @param Comment A comment describing the class.
	 * @param bUClass Whether the class is a UCLASS.
	 * @param UClassSpecifiers The specifiers for the UCLASS macro.
	 */
	void StartClass(const FString& Classname, const FString& Comment, const bool bUClass, const FString& UClassSpecifiers = "BlueprintType");

	/**
	 * @brief Ends the current class definition.
	 */
	void EndClass() { EndBlock(true, true); }

	/**
	 * @brief Starts a struct definition and optionally indents it.
	 *
	 * @param Structname The name of the struct.
	 * @param Comment A comment describing the struct.
	 * @param bUStruct Whether the struct is a USTRUCT.
	 */
	void StartStruct(const FString& Structname, const FString& Comment, const bool bUStruct);

	/**
	 * @brief Ends the current struct definition with an optional inline declaration.
	 *
	 * @param InlineDeclaration An optional inline declaration for the struct.
	 */
	void EndStruct(const FString& InlineDeclaration);

	/**
	 * @brief Adds an entry to an enum without specifying the underlying value.
	 *
	 * @param Name The name of the enum entry.
	 */
	void AddEnumEntry(FString Name);

	/**
	 * @brief Adds an entry with an underlying value to an enum.
	 *
	 * @tparam TNameValuePair The type of the name-value pair.
	 * @param Pair The name-value pair for the enum entry.
	 */
	template <typename TNameValuePair>
	void AddEnumEntry(TNameValuePair Pair);

	/**
	 * @brief Retrieves the export macro for the current module.
	 *
	 * @return The export macro string.
	 */
	static FString GetExportMacro();

	/**
	 * @brief Writes the content to the specified file path.
	 */
	void WriteToFile() const;

	/**
	 * @brief Splits a camel-case property name into a human-readable format.
	 *
	 * @param Name The camel-case name to split.
	 * @return The split name as a string.
	 */
	FString SplitName(const FString& Name);
};

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

template<typename Lambda>
void CodeFileGenerator::SafeExecute(Lambda Lamb)
{
	Lamb();
}

template<>
inline void CodeFileGenerator::SafeExecute(nullptr_t Lamb)
{
}

/**
 * @brief Template constructor for CodeFileGenerator.
 *
 * Adds default content, executes the content generator, and writes to file.
 *
 * @tparam Lambda The type of the content generator function.
 * @param Path The path to the file to be generated.
 * @param bHeader Whether the file is a header file.
 * @param ContentGenerator The function generating the file content.
 */
template <typename Lambda>
CodeFileGenerator::CodeFileGenerator(const FString& Path, const bool bHeader, Lambda ContentGenerator) : Path(CodeGenerator::GetSourceFolder() / Path)
{
	Line("// articy Software GmbH & Co. KG");
	Comment("This code file was generated by ArticyImporter. Changes to this file will get lost once the code is regenerated.");

	if (bHeader)
	{
		Line();
		Line("#pragma once");
	}

	Line();

	if (ensure(!std::is_null_pointer<Lambda>::value))
		ContentGenerator(this);

	WriteToFile();
}

/**
 * @brief Adds a block of code with optional indentation and semicolon.
 *
 * @tparam Lambda The type of the content function.
 * @param bIndent Whether to indent the block.
 * @param Content The function generating the block content.
 * @param bSemicolonAtEnd Whether to append a semicolon after the block.
 */
template <typename Lambda>
void CodeFileGenerator::Block(const bool bIndent, Lambda Content, const bool bSemicolonAtEnd)
{
	StartBlock(bIndent);
	{
		// Add the content
		SafeExecute(Content);
	}
	EndBlock(bIndent, bSemicolonAtEnd);
}

/**
 * @brief Adds a class definition with optional UCLASS macro.
 *
 * @tparam Lambda The type of the content function.
 * @param Classname The name of the class.
 * @param Comment A comment describing the class.
 * @param bUClass Whether the class is a UCLASS.
 * @param Content The function generating the class content.
 * @param UClassSpecifiers The specifiers for the UCLASS macro.
 */
template <typename Lambda>
void CodeFileGenerator::Class(const FString& Classname, const FString& Comment, const bool bUClass, Lambda Content, const FString& UClassSpecifiers)
{
	StartClass(Classname, Comment, bUClass, UClassSpecifiers);
	{
		// Add the content
		SafeExecute(Content);
	}
	EndClass();
}

/**
 * @brief Adds a struct definition with optional USTRUCT macro.
 *
 * @tparam Lambda The type of the content function.
 * @param Structname The name of the struct.
 * @param Comment A comment describing the struct.
 * @param bUStruct Whether the struct is a USTRUCT.
 * @param Content The function generating the struct content.
 * @param InlineDeclaration An optional inline declaration for the struct.
 */
template <typename Lambda>
void CodeFileGenerator::Struct(const FString& Structname, const FString& Comment, const bool bUStruct, Lambda Content, const FString& InlineDeclaration)
{
	StartStruct(Structname, Comment, bUStruct);
	{
		// Add the content
		SafeExecute(Content);
	}
	EndStruct(InlineDeclaration);
}

/**
 * @brief Adds a UINTERFACE definition with corresponding interface class.
 *
 * @tparam Lambda The type of the content function.
 * @param Classname The name of the interface class.
 * @param UInterfaceSpecifiers The specifiers for the UINTERFACE macro.
 * @param Comment A comment describing the interface.
 * @param Content The function generating the interface content.
 */
template <typename Lambda>
void CodeFileGenerator::UInterface(const FString& Classname, const FString& UInterfaceSpecifiers, const FString& Comment, Lambda Content)
{
	if (!Comment.IsEmpty())
		this->Comment(Comment);
	Line(FString::Printf(TEXT("UINTERFACE(%s)"), *UInterfaceSpecifiers));
	Line(FString::Printf(TEXT("class U%s : public UInterface { GENERATED_BODY() };"), *Classname));
	Class("I" + Classname, "", false, [&]
		{
			Line("GENERATED_BODY()");
			Line();
			SafeExecute(Content);
		});
}

//---------------------------------------------------------------------------//

/**
 * @brief Adds an enum definition with optional UENUM macro.
 *
 * @tparam Type The type of the enum values.
 * @param Enumname The name of the enum.
 * @param Comment A comment describing the enum.
 * @param bUEnum Whether the enum is a UENUM.
 * @param Values The values of the enum.
 */
template <typename Type>
void CodeFileGenerator::Enum(const FString& Enumname, const FString& Comment, const bool bUEnum, TArray<Type> Values)
{
	if (bUEnum)
		Line("UENUM(BlueprintType)");

	Line("enum");
	StartClass(Enumname + (bUEnum ? " : uint8" : ""), Comment, false);
	{
		// Add the values
		for (auto val : Values)
			AddEnumEntry(val);
	}
	EndClass();
}

/**
 * @brief Adds an entry to an enum without specifying the underlying value.
 *
 * @param Name The name of the enum entry.
 */
inline void CodeFileGenerator::AddEnumEntry(FString Name)
{
	Line(FString::Printf(TEXT("%s,"), *Name));
}

/**
 * @brief Adds an entry with an underlying value to an enum.
 *
 * @tparam TNameValuePair The type of the name-value pair.
 * @param Pair The name-value pair for the enum entry.
 */
template <typename TNameValuePair>
void CodeFileGenerator::AddEnumEntry(TNameValuePair Pair)
{
	Line(FString::Printf(TEXT("%s = %d,"), *Pair.Name, Pair.Value));
}

//---------------------------------------------------------------------------//

/**
 * @brief Adds a method definition, optionally as UFUNCTION.
 *
 * @tparam Lambda The type of the method definition function.
 * @param ReturnType The return type of the method.
 * @param Name The name of the method.
 * @param Parameters The parameters of the method.
 * @param Definition The function defining the method.
 * @param Comment A comment describing the method.
 * @param bUFunction Whether to add a UFUNCTION macro.
 * @param UFunctionSpecifiers The specifiers for the UFUNCTION macro.
 * @param MethodSpecifiers Additional specifiers for the method.
 */
template<typename Lambda>
void CodeFileGenerator::Method(const FString& ReturnType, const FString& Name, const FString& Parameters, Lambda Definition, const FString& Comment, const bool bUFunction, const FString& UFunctionSpecifiers, const FString& MethodSpecifiers)
{
	if (!ensure(!Name.IsEmpty()))
		return;

	if (!Comment.IsEmpty())
		this->Comment(Comment);

	if (bUFunction)
		this->UFunctionMacro(UFunctionSpecifiers);

	// Check if nullptr was passed as Definition
	constexpr auto hasDefinition = !std::is_null_pointer<Lambda>::value;

	// ReturnType Name(Parameters..)
	// Only add the semicolon if there is no definition
	Line(FString::Printf(TEXT("%s %s(%s) %s"), *ReturnType, *Name, *Parameters, *MethodSpecifiers), !hasDefinition);

	// Add definition, if any
	if (hasDefinition)
		Block(true, Definition, false);
}
