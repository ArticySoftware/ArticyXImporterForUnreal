//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "Slate/DatabaseView/SArticyDatabaseView.h"
#include "ArticyTestTextModel.h"
#include "Framework/Application/SlateApplication.h"

#if WITH_AUTOMATION_TESTS

namespace
{
	/** An object as the importer leaves it, with an id, a technical name, a display name and a text. */
	UArticyObject* MakeObject(uint64 Id, const TCHAR* TechnicalName, const TCHAR* DisplayName, const TCHAR* Text)
	{
		UArticyTestPlainNamedObject* Object = NewObject<UArticyTestPlainNamedObject>(GetTransientPackage());
		Object->DisplayName = DisplayName;
		Object->Text = Text;

		UArticyBaseObject* Reflectable = Object;
		Reflectable->SetProp<FArticyId>(TEXT("Id"), FArticyId(Id));
		Reflectable->SetProp<FString>(TEXT("TechnicalName"), TechnicalName);
		return Object;
	}

	FString Names(const TArray<FArticyDatabaseViewItemPtr>& Items)
	{
		TArray<FString> Result;
		for (const FArticyDatabaseViewItemPtr& Item : Items)
		{
			Result.Add(Item->TechnicalName);
		}
		return FString::Join(Result, TEXT(","));
	}
}

// Pins what the object list of the database view shows for a filter and a sort column.
BEGIN_DEFINE_SPEC(FArticyDatabaseViewSpec, "Articy.Editor.DatabaseView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	TArray<UArticyObject*> Objects;
	TArray<FArticyDatabaseViewItemPtr> Items;
END_DEFINE_SPEC(FArticyDatabaseViewSpec)

void FArticyDatabaseViewSpec::Define()
{
	BeforeEach([this]()
	{
		Objects = {
			MakeObject(0x0100000000000C01, TEXT("Chr_Manfred"), TEXT("Manfred"), TEXT("A patient")),
			MakeObject(0x0100000000000A02, TEXT("Chr_Hamster"), TEXT("Hamster"), TEXT("Talks back")),
			MakeObject(0x0100000000000B03, TEXT("Loc_Cell"), TEXT("Padded Cell"), TEXT("Soft walls")),
		};

		Items.Reset();
		for (UArticyObject* Object : Objects)
		{
			Items.Add(MakeShared<FArticyDatabaseViewItem>(Object));
		}
	});

	Describe("An item", [this]()
	{
		It("shows the names, the type and the id of its object", [this]()
		{
			const FArticyDatabaseViewItem& Item = *Items[0];

			TestEqual(TEXT("display name"), Item.DisplayName, FString(TEXT("Manfred")));
			TestEqual(TEXT("technical name"), Item.TechnicalName, FString(TEXT("Chr_Manfred")));
			TestEqual(TEXT("type"), Item.Type, UArticyTestPlainNamedObject::StaticClass()->GetName());
			TestEqual(TEXT("id"), Item.Id, FString(TEXT("0x0100000000000C01")));
		});

		It("prefers the articy type of the object over its class", [this]()
		{
			Objects[0]->ArticyTypeName = TEXT("Character");

			TestEqual(TEXT("type"), FArticyDatabaseViewItem(Objects[0], TEXT("ArticyTest")).Type, FString(TEXT("Character")));
		});

		It("drops the project prefix from the class of an object without an articy type", [this]()
		{
			TestEqual(TEXT("type"), FArticyDatabaseViewItem(Objects[0], TEXT("ArticyTest")).Type, FString(TEXT("PlainNamedObject")));
		});
	});

	Describe("FilterAndSort", [this]()
	{
		It("keeps every object for an empty filter", [this]()
		{
			TestEqual(TEXT("objects"), SArticyDatabaseView::FilterAndSort(Items, FString(), SArticyDatabaseView::NameColumn, true).Num(), 3);
		});

		It("finds objects by a part of the technical name", [this]()
		{
			TestEqual(TEXT("objects"), Names(SArticyDatabaseView::FilterAndSort(Items, TEXT("Chr_"), SArticyDatabaseView::TechnicalNameColumn, true)),
				FString(TEXT("Chr_Hamster,Chr_Manfred")));
		});

		It("finds objects by display name, text, type and id", [this]()
		{
			TestEqual(TEXT("by display name"), Names(SArticyDatabaseView::FilterAndSort(Items, TEXT("Padded"), SArticyDatabaseView::NameColumn, true)),
				FString(TEXT("Loc_Cell")));
			TestEqual(TEXT("by text"), Names(SArticyDatabaseView::FilterAndSort(Items, TEXT("Talks"), SArticyDatabaseView::NameColumn, true)),
				FString(TEXT("Chr_Hamster")));
			TestEqual(TEXT("by type"), SArticyDatabaseView::FilterAndSort(Items, TEXT("PlainNamedObject"), SArticyDatabaseView::NameColumn, true).Num(), 3);
			TestEqual(TEXT("by id"), Names(SArticyDatabaseView::FilterAndSort(Items, TEXT("0x0100000000000C01"), SArticyDatabaseView::NameColumn, true)),
				FString(TEXT("Chr_Manfred")));
		});

		It("finds nothing for a filter no object matches", [this]()
		{
			TestEqual(TEXT("objects"), SArticyDatabaseView::FilterAndSort(Items, TEXT("Nobody"), SArticyDatabaseView::NameColumn, true).Num(), 0);
		});

		It("sorts by the chosen column in both directions", [this]()
		{
			TestEqual(TEXT("by name"), Names(SArticyDatabaseView::FilterAndSort(Items, FString(), SArticyDatabaseView::NameColumn, true)),
				FString(TEXT("Chr_Hamster,Chr_Manfred,Loc_Cell")));
			TestEqual(TEXT("by name, descending"), Names(SArticyDatabaseView::FilterAndSort(Items, FString(), SArticyDatabaseView::NameColumn, false)),
				FString(TEXT("Loc_Cell,Chr_Manfred,Chr_Hamster")));
			TestEqual(TEXT("by id"), Names(SArticyDatabaseView::FilterAndSort(Items, FString(), SArticyDatabaseView::IdColumn, true)),
				FString(TEXT("Chr_Hamster,Loc_Cell,Chr_Manfred")));
		});
	});

	Describe("The view", [this]()
	{
		It("lists the objects it is given and narrows them down with the filter", [this]()
		{
			if (!FSlateApplication::IsInitialized())
			{
				return;
			}

			const TSharedRef<SArticyDatabaseView> View = SNew(SArticyDatabaseView).bLiveData(false);
			View->SetObjects(Objects);
			TestEqual(TEXT("objects"), View->GetNumObjects(), 3);
			TestEqual(TEXT("visible objects"), View->GetNumVisibleObjects(), 3);

			View->SetFilterText(FText::FromString(TEXT("Chr_")));
			TestEqual(TEXT("objects"), View->GetNumObjects(), 3);
			TestEqual(TEXT("visible objects"), View->GetNumVisibleObjects(), 2);
		});

		It("inspects the selected object until it is filtered out", [this]()
		{
			if (!FSlateApplication::IsInitialized())
			{
				return;
			}

			const TSharedRef<SArticyDatabaseView> View = SNew(SArticyDatabaseView).bLiveData(false);
			View->SetObjects(Objects);
			TestNull(TEXT("selection at first"), View->GetSelectedObject());

			View->SelectObject(Objects[2]);
			TestTrue(TEXT("selected"), View->GetSelectedObject() == Objects[2]);

			View->SetFilterText(FText::FromString(TEXT("Chr_")));
			TestNull(TEXT("selection after filtering it out"), View->GetSelectedObject());
		});

		It("keeps the selection when the objects are replaced by an import", [this]()
		{
			if (!FSlateApplication::IsInitialized())
			{
				return;
			}

			const TSharedRef<SArticyDatabaseView> View = SNew(SArticyDatabaseView).bLiveData(false);
			View->SetObjects(Objects);
			View->SelectObject(Objects[0]);

			UArticyObject* Reimported = MakeObject(0x0100000000000C01, TEXT("Chr_Manfred"), TEXT("Manfred"), TEXT("A patient"));
			View->SetObjects({ Reimported, Objects[1] });

			TestTrue(TEXT("selected"), View->GetSelectedObject() == Reimported);
		});

		It("shows the help box only while the import needs attention", [this]()
		{
			if (!FSlateApplication::IsInitialized())
			{
				return;
			}

			const TSharedRef<SArticyDatabaseView> View = SNew(SArticyDatabaseView).bLiveData(false);
			TestFalse(TEXT("help box when up to date"), View->IsHelpBoxVisible());

			FArticyImportFacts Facts;
			Facts.NumExportFiles = 1;
			View->SetImportStatus(FArticyImportStatus::Evaluate(Facts));
			TestTrue(TEXT("help box when not imported"), View->IsHelpBoxVisible());
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
