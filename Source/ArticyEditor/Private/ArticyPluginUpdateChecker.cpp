//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "ArticyPluginUpdateChecker.h"
#include "ArticyEditorModule.h"
#include "ArticyPluginSettings.h"
#include "Dom/JsonObject.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#define LOCTEXT_NAMESPACE "ArticyPluginUpdateChecker"

namespace
{
	const TCHAR* LatestReleaseUrl = TEXT("https://api.github.com/repos/ArticySoftware/ArticyXImporterForUnreal/releases/latest");

	/** Splits "v1.6.1" into its numbers. */
	TArray<int32> ParseVersion(const FString& Version)
	{
		FString Trimmed = Version.TrimStartAndEnd();
		Trimmed.RemoveFromStart(TEXT("v"), ESearchCase::IgnoreCase);

		TArray<FString> Parts;
		Trimmed.ParseIntoArray(Parts, TEXT("."));

		TArray<int32> Numbers;
		for (const FString& Part : Parts)
		{
			Numbers.Add(FCString::Atoi(*Part));
		}
		return Numbers;
	}
}

void FArticyPluginUpdateStatus::SetLatestRelease(const FString& InLatestVersion, const FString& InReleaseUrl)
{
	LatestVersion = InLatestVersion;
	LatestVersion.RemoveFromStart(TEXT("v"), ESearchCase::IgnoreCase);
	ReleaseUrl = InReleaseUrl;

	State = FArticyPluginUpdateChecker::CompareVersions(InstalledVersion, LatestVersion) < 0
		? EArticyUpdateState::UpdateAvailable
		: EArticyUpdateState::UpToDate;
}

FText FArticyPluginUpdateStatus::GetLabel() const
{
	const FText Installed = FText::FromString(InstalledVersion);

	switch (State)
	{
	case EArticyUpdateState::UpdateAvailable:
		return InstallSource == EArticyInstallSource::Fab
			? FText::Format(LOCTEXT("UpdateOnGitHub", "Version {0} - {1} released on GitHub"), Installed, FText::FromString(LatestVersion))
			: FText::Format(LOCTEXT("UpdateAvailable", "Version {0} - update to {1} available"), Installed, FText::FromString(LatestVersion));
	case EArticyUpdateState::UpToDate:
		return FText::Format(LOCTEXT("UpToDate", "Version {0} - up to date"), Installed);
	case EArticyUpdateState::Checking:
		return FText::Format(LOCTEXT("Checking", "Version {0} - checking for updates..."), Installed);
	default:
		return FText::Format(LOCTEXT("Version", "Version {0}"), Installed);
	}
}

FText FArticyPluginUpdateStatus::GetToolTip() const
{
	switch (State)
	{
	case EArticyUpdateState::UpdateAvailable:
		return InstallSource == EArticyInstallSource::Fab
			? LOCTEXT("UpdateOnGitHub_ToolTip", "A newer version was released on GitHub. This plugin was installed from Fab, where releases arrive later: update it through the Epic Games Launcher once it is offered there.")
			: LOCTEXT("UpdateAvailable_ToolTip", "A newer version was released on GitHub. Click to open the release page.");
	case EArticyUpdateState::UpToDate:
		return LOCTEXT("UpToDate_ToolTip", "This is the latest release of the importer.");
	case EArticyUpdateState::Failed:
		return LOCTEXT("Failed_ToolTip", "The latest release could not be determined.");
	case EArticyUpdateState::Unknown:
		return LOCTEXT("Unknown_ToolTip", "The check for plugin updates is turned off in the project settings.");
	default:
		return FText::GetEmpty();
	}
}

FLinearColor FArticyPluginUpdateStatus::GetColor() const
{
	switch (State)
	{
	case EArticyUpdateState::UpdateAvailable:
		// An update that has to arrive on Fab first is less urgent
		return InstallSource == EArticyInstallSource::Fab ? FLinearColor(0.35f, 0.65f, 1.f) : FLinearColor(1.f, 0.6f, 0.1f);
	case EArticyUpdateState::UpToDate:
		return FLinearColor(0.3f, 0.8f, 0.3f);
	default:
		return FLinearColor(0.6f, 0.6f, 0.6f);
	}
}

FString FArticyPluginUpdateStatus::GetUpdateUrl() const
{
	if (State != EArticyUpdateState::UpdateAvailable)
	{
		return FString();
	}

	return InstallSource == EArticyInstallSource::Fab && !FabUrl.IsEmpty() ? FabUrl : ReleaseUrl;
}

FArticyPluginUpdateChecker::FArticyPluginUpdateChecker()
{
	if (const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("ArticyXImporter")))
	{
		Status.InstalledVersion = Plugin->GetDescriptor().VersionName;
		Status.FabUrl = Plugin->GetDescriptor().MarketplaceURL;
		Status.InstallSource = DetectInstallSource(Plugin->GetBaseDir(), FPaths::EngineDir());
	}
}

void FArticyPluginUpdateChecker::CheckForUpdates()
{
	if (Status.State != EArticyUpdateState::Unknown || !GetDefault<UArticyPluginSettings>()->bCheckForPluginUpdates)
	{
		return;
	}
	Status.State = EArticyUpdateState::Checking;

	auto Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(LatestReleaseUrl);
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("Accept"), TEXT("application/vnd.github+json"));

	const TWeakPtr<FArticyPluginUpdateChecker> WeakChecker = AsShared();
	Request->OnProcessRequestComplete().BindLambda([WeakChecker](FHttpRequestPtr, FHttpResponsePtr Response, bool bSucceeded)
	{
		if (const TSharedPtr<FArticyPluginUpdateChecker> This = WeakChecker.Pin())
		{
			const bool bHasResponse = bSucceeded && Response.IsValid();
			This->HandleResponse(bHasResponse, bHasResponse ? Response->GetResponseCode() : 0, bHasResponse ? Response->GetContentAsString() : FString());
		}
	});
	Request->ProcessRequest();
}

void FArticyPluginUpdateChecker::HandleResponse(bool bSucceeded, int32 ResponseCode, const FString& Content)
{
	FString Version, Url;
	if (bSucceeded && ResponseCode == 200 && ParseRelease(Content, Version, Url))
	{
		Status.SetLatestRelease(Version, Url);
	}
	else
	{
		UE_LOG(LogArticyEditor, Log, TEXT("Could not determine the latest release of the importer (response code %d)."), ResponseCode);
		Status.State = EArticyUpdateState::Failed;
	}
}

int32 FArticyPluginUpdateChecker::CompareVersions(const FString& A, const FString& B)
{
	const TArray<int32> NumbersA = ParseVersion(A);
	const TArray<int32> NumbersB = ParseVersion(B);

	// A missing number counts as 0, so 1.7 equals 1.7.0
	for (int32 Index = 0; Index < FMath::Max(NumbersA.Num(), NumbersB.Num()); ++Index)
	{
		const int32 NumberA = NumbersA.IsValidIndex(Index) ? NumbersA[Index] : 0;
		const int32 NumberB = NumbersB.IsValidIndex(Index) ? NumbersB[Index] : 0;
		if (NumberA != NumberB)
		{
			return NumberA < NumberB ? -1 : 1;
		}
	}

	return 0;
}

EArticyInstallSource FArticyPluginUpdateChecker::DetectInstallSource(const FString& PluginDirectory, const FString& EngineDirectory)
{
	const FString MarketplaceDirectory = FPaths::ConvertRelativePathToFull(EngineDirectory / TEXT("Plugins") / TEXT("Marketplace"));
	const FString FullPluginDirectory = FPaths::ConvertRelativePathToFull(PluginDirectory);

	return FPaths::IsUnderDirectory(FullPluginDirectory, MarketplaceDirectory) ? EArticyInstallSource::Fab : EArticyInstallSource::GitHub;
}

bool FArticyPluginUpdateChecker::ParseRelease(const FString& Json, FString& OutVersion, FString& OutUrl)
{
	TSharedPtr<FJsonObject> Release;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, Release) || !Release.IsValid())
	{
		return false;
	}

	if (!Release->TryGetStringField(TEXT("tag_name"), OutVersion) || OutVersion.IsEmpty())
	{
		return false;
	}

	Release->TryGetStringField(TEXT("html_url"), OutUrl);
	return true;
}

#undef LOCTEXT_NAMESPACE
