//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Customizations/Details/ArticyObjectFTextCustomization.h"

#include "ArticyBaseObject.h"
#include "ArticyLocalizationSubsystem.h"
#include "ArticyLocalizerSystem.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
#include "PropertyHandle.h"
#include "Widgets/Text/STextBlock.h"
#include "UObject/UnrealType.h"

namespace
{
	FText ResolveArticyText(TWeakObjectPtr<UObject> WeakOuter, TSharedPtr<IPropertyHandle> Handle)
	{
		FText Value;
		if (!Handle.IsValid() || Handle->GetValue(Value) != FPropertyAccess::Success)
		{
			return FText::GetEmpty();
		}

		UObject* Outer = WeakOuter.Get();
		if (!Outer)
		{
			return Value;
		}

		UArticyLocalizationSubsystem* Sub = UArticyLocalizationSubsystem::Get();
		if (!Sub)
		{
			return Value;
		}

		UArticyLocalizerSystem* Localizer = Sub->GetLocalizer();
		if (!Localizer)
		{
			return Value;
		}

		return Localizer->LocalizeString(Outer, Value, true, &Value);
	}

	void CustomizeFTextRow(
		IDetailLayoutBuilder& DetailBuilder,
		TWeakObjectPtr<UObject> WeakOuter,
		TSharedRef<IPropertyHandle> Handle)
	{
		IDetailPropertyRow* Row = DetailBuilder.EditDefaultProperty(Handle);
		if (!Row)
		{
			return;
		}

		TSharedPtr<SWidget> NameWidget;
		TSharedPtr<SWidget> ValueWidget;
		FDetailWidgetRow DefaultRow;
		Row->GetDefaultWidgets(NameWidget, ValueWidget, DefaultRow);

		const TSharedPtr<IPropertyHandle> HandlePtr = Handle;

		Row->CustomWidget()
			.NameContent()
			[
				NameWidget.ToSharedRef()
			]
			.ValueContent()
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text_Lambda([WeakOuter, HandlePtr]()
				{
					return ResolveArticyText(WeakOuter, HandlePtr);
				})
				.ToolTipText_Lambda([HandlePtr]()
				{
					FText Raw;
					if (HandlePtr.IsValid())
					{
						HandlePtr->GetValue(Raw);
					}
					return Raw;
				})
			];
	}

	void CustomizeFTextPropertiesOn(
		IDetailLayoutBuilder& DetailBuilder,
		TWeakObjectPtr<UObject> WeakOuter,
		TSharedPtr<IPropertyHandle> ParentHandle,
		UStruct* Struct)
	{
		if (!Struct)
		{
			return;
		}

		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			FProperty* Prop = *It;
			if (!Prop)
			{
				continue;
			}

			if (CastField<FTextProperty>(Prop))
			{
				TSharedPtr<IPropertyHandle> Handle = ParentHandle.IsValid()
					? ParentHandle->GetChildHandle(Prop->GetFName())
					: DetailBuilder.GetProperty(Prop->GetFName());
				if (Handle.IsValid() && Handle->IsValidHandle())
				{
					CustomizeFTextRow(DetailBuilder, WeakOuter, Handle.ToSharedRef());
				}
				continue;
			}

			// Recurse into UArticyBaseFeature sub-objects (features are inline UObject pointers
			// on the parent, but the details panel renders their FText properties as nested rows).
			if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Prop))
			{
				if (ObjProp->PropertyClass && ObjProp->PropertyClass->IsChildOf(UArticyBaseFeature::StaticClass()))
				{
					TSharedPtr<IPropertyHandle> FeatureHandle = ParentHandle.IsValid()
						? ParentHandle->GetChildHandle(Prop->GetFName())
						: DetailBuilder.GetProperty(Prop->GetFName());
					if (!FeatureHandle.IsValid() || !FeatureHandle->IsValidHandle())
					{
						continue;
					}

					UObject* FeatureObj = nullptr;
					FeatureHandle->GetValue(FeatureObj);
					if (!FeatureObj)
					{
						continue;
					}

					CustomizeFTextPropertiesOn(DetailBuilder, FeatureObj, FeatureHandle, FeatureObj->GetClass());
				}
			}
		}
	}
}

TSharedRef<IDetailCustomization> FArticyObjectFTextCustomization::MakeInstance()
{
	return MakeShareable(new FArticyObjectFTextCustomization());
}

void FArticyObjectFTextCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	if (Objects.Num() != 1)
	{
		// Multi-edit: leave the default widgets alone; LocalizeString needs a single outer.
		return;
	}

	TWeakObjectPtr<UObject> WeakOuter = Objects[0];
	UObject* Outer = WeakOuter.Get();
	if (!Outer)
	{
		return;
	}

	CustomizeFTextPropertiesOn(DetailBuilder, WeakOuter, nullptr, Outer->GetClass());
}
