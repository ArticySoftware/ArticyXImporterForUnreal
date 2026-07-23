//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "PackagesImport.h"
#include "ArticyArchiveReader.h"
#include "Dom/JsonObject.h"

#if WITH_AUTOMATION_TESTS

namespace
{
	// A model entry as it appears in a package's Objects array.
	TSharedPtr<FJsonObject> MakeModelJson(const FString& TechnicalName, const FString& HexId)
	{
		TSharedPtr<FJsonObject> Properties = MakeShared<FJsonObject>();
		Properties->SetStringField(TEXT("TechnicalName"), TechnicalName);
		Properties->SetStringField(TEXT("Id"), HexId);

		TSharedPtr<FJsonObject> Model = MakeShared<FJsonObject>();
		Model->SetStringField(TEXT("Type"), TEXT("DialogueFragment"));
		Model->SetObjectField(TEXT("Properties"), Properties);
		return Model;
	}

	// A package entry as it appears in the manifest, without the Files node - so nothing
	// is fetched from the archive and the reader below is never actually read from.
	TSharedPtr<FJsonObject> MakePackageJson(const FString& Name, const FString& HexId, bool bIncluded)
	{
		TSharedPtr<FJsonObject> Package = MakeShared<FJsonObject>();
		Package->SetStringField(TEXT("Id"), HexId);
		Package->SetStringField(TEXT("Name"), Name);
		Package->SetStringField(TEXT("Description"), TEXT("A package."));
		Package->SetBoolField(TEXT("IsIncluded"), bIncluded);
		return Package;
	}
}

// Covers the parts of the package import that decide what an object is called and where it
// lands on disk. Asset naming moved from the package name to the package id (so a renamed
// package keeps its assets), and selective import made IsIncluded gate the whole parse -
// both are easy to regress and invisible until a reimport goes wrong.
BEGIN_DEFINE_SPEC(FArticyPackagesImportSpec, "Articy.Editor.PackagesImport",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyPackagesImportSpec)

void FArticyPackagesImportSpec::Define()
{
	Describe("FArticyModelDef::ImportFromJson", [this]()
	{
		It("parses the type, technical name and hex ids", [this]()
		{
			TSharedPtr<FJsonObject> Json = MakeModelJson(TEXT("Frag_Hello"), TEXT("0x0100000000000123"));
			Json->GetObjectField(TEXT("Properties"))->SetStringField(TEXT("Parent"), TEXT("0x0100000000000001"));

			FArticyModelDef Model;
			Model.ImportFromJson(Json);

			TestEqual(TEXT("type"), Model.GetType().ToString(), FString(TEXT("DialogueFragment")));
			TestEqual(TEXT("technical name"), Model.GetTechnicalName(), FString(TEXT("Frag_Hello")));
			TestEqual(TEXT("id"), (int64)Model.GetId().Get(), (int64)0x0100000000000123);
			TestEqual(TEXT("parent"), (int64)Model.GetParent().Get(), (int64)0x0100000000000001);
		});

		It("builds NameAndId from the technical name and the raw id string", [this]()
		{
			// NameAndId keys the generated asset, so it pairs the name with the *string*
			// form of the id exactly as exported, not the parsed 64-bit value.
			FArticyModelDef Model;
			Model.ImportFromJson(MakeModelJson(TEXT("Frag_Hello"), TEXT("0x0100000000000123")));

			TestEqual(TEXT("name and id"), Model.GetNameAndId(),
				FString(TEXT("Frag_Hello_0x0100000000000123")));
		});

		It("maps the asset category string onto the enum", [this]()
		{
			auto CategoryOf = [](const FString& Category)
			{
				TSharedPtr<FJsonObject> Json = MakeModelJson(TEXT("Asset"), TEXT("0x01"));
				Json->SetStringField(TEXT("Category"), Category);
				Json->SetStringField(TEXT("AssetRef"), TEXT("Assets/voice.wav"));
				FArticyModelDef Model;
				Model.ImportFromJson(Json);
				return Model.GetAssetCat();
			};

			TestEqual(TEXT("Image"), (int32)CategoryOf(TEXT("Image")), (int32)EArticyAssetCategory::Image);
			TestEqual(TEXT("Audio"), (int32)CategoryOf(TEXT("Audio")), (int32)EArticyAssetCategory::Audio);
			TestEqual(TEXT("Document"), (int32)CategoryOf(TEXT("Document")), (int32)EArticyAssetCategory::Document);
			TestEqual(TEXT("All"), (int32)CategoryOf(TEXT("All")), (int32)EArticyAssetCategory::All);
			// Anything unrecognised - including a missing Category - is None, not a guess.
			TestEqual(TEXT("unknown"), (int32)CategoryOf(TEXT("Hologram")), (int32)EArticyAssetCategory::None);
		});

		It("keeps the asset reference for an asset model", [this]()
		{
			TSharedPtr<FJsonObject> Json = MakeModelJson(TEXT("Asset"), TEXT("0x01"));
			Json->SetStringField(TEXT("AssetRef"), TEXT("Assets/voice.wav"));

			FArticyModelDef Model;
			Model.ImportFromJson(Json);
			TestEqual(TEXT("asset ref"), Model.GetAssetRef(), FString(TEXT("Assets/voice.wav")));
		});

		It("round-trips the properties through the serialized json string", [this]()
		{
			// Properties are kept as condensed json and re-parsed on demand; a model that
			// loses them generates an asset with default values.
			TSharedPtr<FJsonObject> Json = MakeModelJson(TEXT("Frag_Hello"), TEXT("0x01"));
			Json->GetObjectField(TEXT("Properties"))->SetStringField(TEXT("DisplayName"), TEXT("Hello"));

			FArticyModelDef Model;
			Model.ImportFromJson(Json);

			TSharedPtr<FJsonObject> Properties = Model.GetPropertiesJson();
			if (!TestTrue(TEXT("properties parsed back"), Properties.IsValid())) return;
			TestEqual(TEXT("display name"), Properties->GetStringField(TEXT("DisplayName")), FString(TEXT("Hello")));
		});

		It("round-trips a template through the serialized json string", [this]()
		{
			TSharedPtr<FJsonObject> Feature = MakeShared<FJsonObject>();
			Feature->SetNumberField(TEXT("Volume"), 7);
			TSharedPtr<FJsonObject> Template = MakeShared<FJsonObject>();
			Template->SetObjectField(TEXT("Audio"), Feature);

			TSharedPtr<FJsonObject> Json = MakeModelJson(TEXT("Frag_Hello"), TEXT("0x01"));
			Json->SetObjectField(TEXT("Template"), Template);

			FArticyModelDef Model;
			Model.ImportFromJson(Json);

			TSharedPtr<FJsonObject> Parsed = Model.GetTemplatesJson();
			if (!TestTrue(TEXT("template parsed back"), Parsed.IsValid())) return;
			TestTrue(TEXT("feature survived"), Parsed->HasField(TEXT("Audio")));
		});
	});

	Describe("FArticyPackageDef::ImportFromJson", [this]()
	{
		It("parses the id, name, description and flags", [this]()
		{
			UArticyArchiveReader* Archive = NewObject<UArticyArchiveReader>();
			TSharedPtr<FJsonObject> Json = MakePackageJson(TEXT("Chapter 1"), TEXT("0x0100000000000ABC"), true);
			Json->SetBoolField(TEXT("IsDefaultPackage"), true);
			Json->SetStringField(TEXT("ScriptFragmentHash"), TEXT("deadbeef"));

			FArticyPackageDef Package;
			Package.ImportFromJson(*Archive, Json);

			TestEqual(TEXT("id"), (int64)Package.GetId().Get(), (int64)0x0100000000000ABC);
			TestEqual(TEXT("name"), Package.GetName(), FString(TEXT("Chapter 1")));
			TestTrue(TEXT("included"), Package.GetIsIncluded());
			TestTrue(TEXT("default package"), Package.GetIsDefaultPackage());
			TestEqual(TEXT("script hash"), Package.GetScriptFragmentHash(), FString(TEXT("deadbeef")));
		});

		It("defaults to excluded when the manifest omits IsIncluded", [this]()
		{
			// Selective import treats absence as "not selected", so an older manifest
			// without the flag imports no objects at all rather than everything.
			UArticyArchiveReader* Archive = NewObject<UArticyArchiveReader>();
			TSharedPtr<FJsonObject> Json = MakePackageJson(TEXT("Chapter 1"), TEXT("0x01"), true);
			Json->RemoveField(TEXT("IsIncluded"));

			FArticyPackageDef Package;
			Package.ImportFromJson(*Archive, Json);
			TestFalse(TEXT("excluded"), Package.GetIsIncluded());
		});

		It("still records the metadata of an excluded package", [this]()
		{
			// The include check happens after the properties are read, so an excluded
			// package is still known by name and id - the UI needs that to offer it.
			UArticyArchiveReader* Archive = NewObject<UArticyArchiveReader>();

			FArticyPackageDef Package;
			Package.ImportFromJson(*Archive, MakePackageJson(TEXT("Chapter 2"), TEXT("0x02"), false));

			TestEqual(TEXT("name"), Package.GetName(), FString(TEXT("Chapter 2")));
			TestEqual(TEXT("id"), (int64)Package.GetId().Get(), (int64)0x02);
			TestFalse(TEXT("excluded"), Package.GetIsIncluded());
			TestEqual(TEXT("no models"), Package.GetModels().Num(), 0);
		});

		It("ignores an invalid json object", [this]()
		{
			UArticyArchiveReader* Archive = NewObject<UArticyArchiveReader>();
			FArticyPackageDef Package;
			Package.ImportFromJson(*Archive, nullptr);
			TestTrue(TEXT("name untouched"), Package.GetName().IsEmpty());
		});
	});

	Describe("FArticyPackageDef naming", [this]()
	{
		It("derives the on-disk asset name from the id, not the package name", [this]()
		{
			// Assets used to be named after the package, which broke every reference when
			// a package was renamed in articy. The id is stable, so it names the asset.
			UArticyArchiveReader* Archive = NewObject<UArticyArchiveReader>();
			FArticyPackageDef Package;
			Package.ImportFromJson(*Archive, MakePackageJson(TEXT("Chapter 1"), TEXT("0x0100000000000ABC"), true));

			TestEqual(TEXT("asset file name"), Package.GetAssetFileName(),
				FString(TEXT("Pkg_0100000000000ABC")));
		});

		It("zero-pads the id to 16 hex digits", [this]()
		{
			UArticyArchiveReader* Archive = NewObject<UArticyArchiveReader>();
			FArticyPackageDef Package;
			Package.ImportFromJson(*Archive, MakePackageJson(TEXT("Small"), TEXT("0x2A"), true));

			TestEqual(TEXT("asset file name"), Package.GetAssetFileName(),
				FString(TEXT("Pkg_000000000000002A")));
		});

		It("puts the package under Packages/<asset name>", [this]()
		{
			UArticyArchiveReader* Archive = NewObject<UArticyArchiveReader>();
			FArticyPackageDef Package;
			Package.ImportFromJson(*Archive, MakePackageJson(TEXT("Chapter 1"), TEXT("0x2A"), true));

			TestEqual(TEXT("folder"), Package.GetFolder(),
				FString(TEXT("Packages/Pkg_000000000000002A")));
		});

		It("keeps the name and asset name independent of each other", [this]()
		{
			UArticyArchiveReader* Archive = NewObject<UArticyArchiveReader>();
			FArticyPackageDef Package;
			Package.ImportFromJson(*Archive, MakePackageJson(TEXT("Chapter 1"), TEXT("0x2A"), true));
			const FString AssetNameBefore = Package.GetAssetFileName();

			Package.SetName(TEXT("Chapter One"));

			TestEqual(TEXT("new name"), Package.GetName(), FString(TEXT("Chapter One")));
			TestEqual(TEXT("previous name remembered"), Package.GetPreviousName(), FString(TEXT("Chapter 1")));
			TestEqual(TEXT("asset name unchanged"), Package.GetAssetFileName(), AssetNameBefore);
		});

		It("reports the current name as the previous one until it is renamed", [this]()
		{
			UArticyArchiveReader* Archive = NewObject<UArticyArchiveReader>();
			FArticyPackageDef Package;
			Package.ImportFromJson(*Archive, MakePackageJson(TEXT("Chapter 1"), TEXT("0x2A"), true));

			// A never-renamed package must not look like it was renamed from "".
			TestEqual(TEXT("previous name"), Package.GetPreviousName(), FString(TEXT("Chapter 1")));
		});

		It("compares packages by id, so a rename does not make two packages differ", [this]()
		{
			UArticyArchiveReader* Archive = NewObject<UArticyArchiveReader>();
			FArticyPackageDef A, B;
			A.ImportFromJson(*Archive, MakePackageJson(TEXT("Chapter 1"), TEXT("0x2A"), true));
			B.ImportFromJson(*Archive, MakePackageJson(TEXT("Chapter One"), TEXT("0x2A"), true));
			TestTrue(TEXT("same id"), A == B);

			FArticyPackageDef C;
			C.ImportFromJson(*Archive, MakePackageJson(TEXT("Chapter 1"), TEXT("0x2B"), true));
			TestFalse(TEXT("different id"), A == C);
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
