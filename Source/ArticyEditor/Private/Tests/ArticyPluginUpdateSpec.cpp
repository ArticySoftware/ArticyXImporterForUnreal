//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyPluginUpdateChecker.h"

#if WITH_AUTOMATION_TESTS

namespace
{
	FArticyPluginUpdateStatus MakeStatus(const TCHAR* Installed, EArticyInstallSource Source)
	{
		FArticyPluginUpdateStatus Status;
		Status.InstalledVersion = Installed;
		Status.InstallSource = Source;
		Status.FabUrl = TEXT("https://fab.example/articy");
		return Status;
	}
}

// Pins when an update is announced, and where a GitHub and a Fab install are sent for it.
BEGIN_DEFINE_SPEC(FArticyPluginUpdateSpec, "Articy.Editor.PluginUpdate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyPluginUpdateSpec)

void FArticyPluginUpdateSpec::Define()
{
	Describe("CompareVersions", [this]()
	{
		It("orders versions by their numbers, not as text", [this]()
		{
			TestTrue(TEXT("1.6.1 < 1.7.0"), FArticyPluginUpdateChecker::CompareVersions(TEXT("1.6.1"), TEXT("1.7.0")) < 0);
			TestTrue(TEXT("1.10.0 > 1.9.0"), FArticyPluginUpdateChecker::CompareVersions(TEXT("1.10.0"), TEXT("1.9.0")) > 0);
			TestEqual(TEXT("1.6.1 == 1.6.1"), FArticyPluginUpdateChecker::CompareVersions(TEXT("1.6.1"), TEXT("1.6.1")), 0);
		});

		It("ignores the v of a release tag", [this]()
		{
			TestEqual(TEXT("v1.6.1 == 1.6.1"), FArticyPluginUpdateChecker::CompareVersions(TEXT("v1.6.1"), TEXT("1.6.1")), 0);
		});

		It("counts a missing number as zero", [this]()
		{
			TestEqual(TEXT("1.7 == 1.7.0"), FArticyPluginUpdateChecker::CompareVersions(TEXT("1.7"), TEXT("1.7.0")), 0);
			TestTrue(TEXT("1.7 < 1.7.1"), FArticyPluginUpdateChecker::CompareVersions(TEXT("1.7"), TEXT("1.7.1")) < 0);
		});
	});

	Describe("DetectInstallSource", [this]()
	{
		It("recognizes the engine's Marketplace folder as an install from Fab", [this]()
		{
			TestTrue(TEXT("Fab"), FArticyPluginUpdateChecker::DetectInstallSource(
				TEXT("C:/Epic/UE_5.6/Engine/Plugins/Marketplace/ArticyXImporter"), TEXT("C:/Epic/UE_5.6/Engine"))
				== EArticyInstallSource::Fab);
		});

		It("takes a plugin in the project for a manual install", [this]()
		{
			TestTrue(TEXT("GitHub"), FArticyPluginUpdateChecker::DetectInstallSource(
				TEXT("D:/Game/Plugins/ArticyXImporter"), TEXT("C:/Epic/UE_5.6/Engine"))
				== EArticyInstallSource::GitHub);
		});

		It("takes a plugin copied elsewhere into the engine for a manual install", [this]()
		{
			TestTrue(TEXT("GitHub"), FArticyPluginUpdateChecker::DetectInstallSource(
				TEXT("C:/Epic/UE_5.6/Engine/Plugins/ArticyXImporter"), TEXT("C:/Epic/UE_5.6/Engine"))
				== EArticyInstallSource::GitHub);
		});
	});

	Describe("ParseRelease", [this]()
	{
		It("reads the tag and the page of a release", [this]()
		{
			FString Version, Url;
			const bool bParsed = FArticyPluginUpdateChecker::ParseRelease(
				TEXT("{\"tag_name\":\"v1.7.0\",\"html_url\":\"https://github.com/x/releases/tag/v1.7.0\",\"prerelease\":false}"), Version, Url);

			TestTrue(TEXT("parsed"), bParsed);
			TestEqual(TEXT("version"), Version, FString(TEXT("v1.7.0")));
			TestEqual(TEXT("url"), Url, FString(TEXT("https://github.com/x/releases/tag/v1.7.0")));
		});

		It("rejects a reply without a release", [this]()
		{
			FString Version, Url;

			TestFalse(TEXT("error reply"), FArticyPluginUpdateChecker::ParseRelease(TEXT("{\"message\":\"Not Found\"}"), Version, Url));
			TestFalse(TEXT("no json"), FArticyPluginUpdateChecker::ParseRelease(TEXT("<html>"), Version, Url));
		});
	});

	Describe("The update status", [this]()
	{
		It("announces a newer release to a GitHub install and links its page", [this]()
		{
			FArticyPluginUpdateStatus Status = MakeStatus(TEXT("1.6.1"), EArticyInstallSource::GitHub);
			Status.SetLatestRelease(TEXT("v1.7.0"), TEXT("https://github.example/v1.7.0"));

			TestTrue(TEXT("state"), Status.State == EArticyUpdateState::UpdateAvailable);
			TestEqual(TEXT("label"), Status.GetLabel().ToString(), FString(TEXT("Version 1.6.1 - update to 1.7.0 available")));
			TestEqual(TEXT("url"), Status.GetUpdateUrl(), FString(TEXT("https://github.example/v1.7.0")));
		});

		It("tells a Fab install about the release, but sends it to Fab", [this]()
		{
			FArticyPluginUpdateStatus Status = MakeStatus(TEXT("1.6.1"), EArticyInstallSource::Fab);
			Status.SetLatestRelease(TEXT("v1.7.0"), TEXT("https://github.example/v1.7.0"));

			TestTrue(TEXT("state"), Status.State == EArticyUpdateState::UpdateAvailable);
			TestEqual(TEXT("label"), Status.GetLabel().ToString(), FString(TEXT("Version 1.6.1 - 1.7.0 released on GitHub")));
			TestEqual(TEXT("url"), Status.GetUpdateUrl(), FString(TEXT("https://fab.example/articy")));

			FArticyPluginUpdateStatus GitHubStatus = MakeStatus(TEXT("1.6.1"), EArticyInstallSource::GitHub);
			GitHubStatus.SetLatestRelease(TEXT("v1.7.0"), FString());
			TestTrue(TEXT("colored differently"), Status.GetColor() != GitHubStatus.GetColor());
		});

		It("reports the latest release as up to date, without a link", [this]()
		{
			FArticyPluginUpdateStatus Status = MakeStatus(TEXT("1.7.0"), EArticyInstallSource::GitHub);
			Status.SetLatestRelease(TEXT("v1.7.0"), TEXT("https://github.example/v1.7.0"));

			TestTrue(TEXT("state"), Status.State == EArticyUpdateState::UpToDate);
			TestEqual(TEXT("label"), Status.GetLabel().ToString(), FString(TEXT("Version 1.7.0 - up to date")));
			TestTrue(TEXT("no url"), Status.GetUpdateUrl().IsEmpty());
		});

		It("does not announce an older release to a newer install", [this]()
		{
			FArticyPluginUpdateStatus Status = MakeStatus(TEXT("1.8.0"), EArticyInstallSource::GitHub);
			Status.SetLatestRelease(TEXT("v1.7.0"), FString());

			TestTrue(TEXT("state"), Status.State == EArticyUpdateState::UpToDate);
		});

		It("shows just the version when nothing is known", [this]()
		{
			const FArticyPluginUpdateStatus Status = MakeStatus(TEXT("1.6.1"), EArticyInstallSource::GitHub);

			TestEqual(TEXT("label"), Status.GetLabel().ToString(), FString(TEXT("Version 1.6.1")));
			TestTrue(TEXT("no url"), Status.GetUpdateUrl().IsEmpty());
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
