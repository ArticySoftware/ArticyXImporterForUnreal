//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CodeGenerator.h"

/**
 * Holds a content string which can be written to a file, specified in the constructor.
 * Contains convenience methods for generating code (line, comments ecc.)
 */
class CodeFileGenerator
{
public:

	/**
	 * Executes a lambda. If the Lambda is nullptr (type nullptr_t), nothing bad happens.
	 */
	template<typename Lambda>
	void SafeExecute(Lambda Lamb);

	/**
	 * Creates a new code file generator with some default lines (copyright ecc.),
	 * then executes the ContentGenerator. At last, WriteToFile is called.
	 */
	template<typename Lambda>
	CodeFileGenerator(const FString& Path, const bool bHeader, Lambda ContentGenerator);

	/** Add a line to the content. */
	void Line(const FString& Line = "", const bool bSemicolon = false, const bool bIndent = true, const int IndentOffset = 0);

	/** Adds a comment to the content. */
	void Comment(const FString& Text);
	/** 
	 * Adds a (less-indented) access modifier.
	 * The colon can be ommitted.
	 */
	void AccessModifier(const FString& Text);

	void UPropertyMacro(const FString& Specifiers);
	void UFunctionMacro(const FString& Specifiers);

	//========================================//

	/** Adds a block to the file, with optional semicolon at the end. */
	template<typename Lambda>
	void Block(const bool bIndent, Lambda Content, const bool bSemicolonAtEnd = false);
	/** Adds a class (or UCLASS) definition. */
	template<typename Lambda>
	void Class(const FString& Classname, const FString& Comment, const bool bUClass, Lambda Content, const FString& UClassSpecifiers = "BlueprintType");
	/** Adds a struct (or USTRUCT) definition. */
	template<typename Lambda>
	void Struct(const FString& Structname, const FString& Comment, const bool bUStruct, Lambda Content, const FString& InlineDeclaration = "");
	/** Adds a UINTERFACE (UMyInterface and IMyInterface) definition. */
	template<typename Lambda>
	void UInterface(const FString& Classname, const FString& UInterfaceSpecifiers, const FString& Comment, Lambda Content);
	
	/** Adds an enum (or UENUM) definition. */
	template<typename Type>
	void Enum(const FString& Enumname, const FString& Comment, const bool bUEnum, TArray<Type> Values);

	//========================================//
	
	/** Adds a variable, optionally as UPROPERTY. */
	void Variable(const FString& Type, const FString& Name, const FString& Value = "", const FString& Comment = "",
				  const bool bUProperty = false, const FString& UPropertySpecifiers = "VisibleAnywhere, BlueprintReadOnly");

	/** Adds a method, optionally as UFUNCTION. */
	template<typename Lambda>
	void Method(const FString& ReturnType, const FString& Name, const FString& Parameters = "", Lambda Definition = nullptr, const FString& Comment = "",
				const bool bUFunction = false, const FString& UFunctionSpecifiers ="", const FString& MethodSpecifiers = "");

	
private:

	const FString Path;
	FString FileContent = "";

	int IndentCount = 0;
	uint8 BlockCount = 0;

	void PushIndent() { ++IndentCount; }
	void PopIndent() { IndentCount = FMath::Max(0, IndentCount-1); }

	/** Starts a block and pushes an indent. */
	void StartBlock(const bool bIndent = true);
	/** Ends a block and pops an indent. */
	void EndBlock(const bool bUnindent = true, const bool bSemicolon = false);

	/** Starts a class (with optional comment), and pushes an indent. */
	void StartClass(const FString& Classname, const FString& Comment, const bool bUClass, const FString& UClassSpecifiers = "BlueprintType");
	void EndClass() { EndBlock(true, true); }

	/** Starts a struct (with optional comment), and pushes an indent. */
	void StartStruct(const FString& Structname, const FString& Comment, const bool bUStruct);
	void EndStruct(const FString& InlineDeclaration);

	/** Adds an entry to the enum, without specifying the underlying value. */
	void AddEnumEntry(FString Name);
	/** Adds an entry with underlying value to the enum. */
	template <typename TNameValuePair>
	void AddEnumEntry(TNameValuePair Pair);

	/** Returns the export macro of the current module */
	static FString GetExportMacro();
	
	/** Write the content to file. */
	void WriteToFile() const;

	/** Splits a property name string for BP formatting */
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

template <typename Lambda>
CodeFileGenerator::CodeFileGenerator(const FString& Path, const bool bHeader, Lambda ContentGenerator) : Path(CodeGenerator::GetSourceFolder() / Path)
{
	Line("// articy Software GmbH & Co. KG");
	Comment("This code file was generated by ArticyImporter. Changes to this file will get lost once the code is regenerated.");
	
	if(bHeader)
	{
		Line();
		Line("#pragma once");
	}

	Line();

	if(ensure(!std::is_null_pointer<Lambda>::value))
		ContentGenerator(this);

	WriteToFile();
}

template <typename Lambda>
void CodeFileGenerator::Block(const bool bIndent, Lambda Content, const bool bSemicolonAtEnd)
{
	StartBlock(bIndent);
	{
		//add the content
		SafeExecute(Content);
	}
	EndBlock(bIndent, bSemicolonAtEnd);
}

template <typename Lambda>
void CodeFileGenerator::Class(const FString& Classname, const FString& Comment, const bool bUClass, Lambda Content, const FString& UClassSpecifiers)
{
	StartClass(Classname, Comment, bUClass, UClassSpecifiers);
	{
		//add the content
		SafeExecute(Content);
	}
	EndClass();
}

template <typename Lambda>
void CodeFileGenerator::Struct(const FString& Structname, const FString& Comment, const bool bUStruct, Lambda Content, const FString& InlineDeclaration)
{
	StartStruct(Structname, Comment, bUStruct);
	{
		//add the content
		SafeExecute(Content);
	}
	EndStruct(InlineDeclaration);
}

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

template <typename Type>
void CodeFileGenerator::Enum(const FString& Enumname, const FString& Comment, const bool bUEnum, TArray<Type> Values)
{
	if(bUEnum)
		Line("UENUM(BlueprintType)");

	Line("enum");
	StartClass(Enumname + (bUEnum ? " : uint8" : ""), Comment, false);
	{
		//add the values
		for(auto val : Values)
			AddEnumEntry(val);
	}
	EndClass();
}

inline void CodeFileGenerator::AddEnumEntry(FString Name)
{
	Line(FString::Printf(TEXT("%s,"), *Name));
}

template <typename TNameValuePair>
void CodeFileGenerator::AddEnumEntry(TNameValuePair Pair)
{
	Line(FString::Printf(TEXT("%s = %d,"), *Pair.Name, Pair.Value));
}

//---------------------------------------------------------------------------//

template<typename Lambda>
void CodeFileGenerator::Method(const FString& ReturnType, const FString& Name, const FString& Parameters, Lambda Definition, const FString& Comment, const bool bUFunction, const FString& UFunctionSpecifiers, const FString& MethodSpecifiers)
{
	if(!ensure(!Name.IsEmpty()))
	   return;

	if(!Comment.IsEmpty())
		this->Comment(Comment);
	
	if(bUFunction)
		this->UFunctionMacro(UFunctionSpecifiers);

	//check if nullptr was passed as Definition
	constexpr auto hasDefinition = !std::is_null_pointer<Lambda>::value;

	//ReturnType Name(Parameters..)
	//only add the semicolon if there is no definition
	Line(FString::Printf(TEXT("%s %s(%s) %s"), *ReturnType, *Name, *Parameters, *MethodSpecifiers), !hasDefinition);

	//add definition, if any
	if(hasDefinition)
		Block(true, Definition, false);
}
