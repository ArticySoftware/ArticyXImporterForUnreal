//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"

/**
 * @brief Where the plugin was installed from.
 */
enum class EArticyInstallSource : uint8
{
	/** Copied into the project or engine by hand, e.g. from a GitHub release. */
	GitHub,
	/** Installed into the engine through Fab and the Epic Games Launcher. */
	Fab
};

/**
 * @brief The outcome of looking for a newer release of the plugin.
 */
enum class EArticyUpdateState : uint8
{
	/** Not checked. */
	Unknown,
	Checking,
	UpToDate,
	UpdateAvailable,
	/** The latest release could not be determined. */
	Failed
};

/**
 * @brief The installed and the latest version of the plugin, with what to tell the user about it.
 */
struct ARTICYEDITOR_API FArticyPluginUpdateStatus
{
	EArticyUpdateState State = EArticyUpdateState::Unknown;
	EArticyInstallSource InstallSource = EArticyInstallSource::GitHub;
	FString InstalledVersion;
	/** The latest release on GitHub, without a leading "v". */
	FString LatestVersion;
	/** The page of the latest release on GitHub. */
	FString ReleaseUrl;
	/** The page of the plugin on Fab. */
	FString FabUrl;

	/** @brief Sets the latest release and derives the state from it. */
	void SetLatestRelease(const FString& InLatestVersion, const FString& InReleaseUrl);

	/** @brief Gets the text of the version label. */
	FText GetLabel() const;

	/** @brief Gets the explanation shown as the tooltip of the label. */
	FText GetToolTip() const;

	/** @brief Gets the color of the label. */
	FLinearColor GetColor() const;

	/** @brief Gets the page to get the update from; empty if there is no update. */
	FString GetUpdateUrl() const;
};

/**
 * @brief Looks for a newer release of the plugin on GitHub.
 *
 * Fab releases follow the GitHub releases with a delay, so a plugin installed from Fab is told
 * about the release but pointed to Fab for the update.
 */
class ARTICYEDITOR_API FArticyPluginUpdateChecker : public TSharedFromThis<FArticyPluginUpdateChecker>
{
public:
	FArticyPluginUpdateChecker();

	/** @brief Asks GitHub for the latest release, unless that was done already or is turned off in the settings. */
	void CheckForUpdates();

	const FArticyPluginUpdateStatus& GetStatus() const { return Status; }

	/**
	 * @brief Compares two versions like "1.6.1" or "v1.7".
	 * @return A negative value if A is older than B, a positive one if it is newer, 0 if they are equal.
	 */
	static int32 CompareVersions(const FString& A, const FString& B);

	/**
	 * @brief Tells whether the plugin was installed from Fab, which puts plugins into the engine's Marketplace folder.
	 * @param PluginDirectory The directory of the plugin.
	 * @param EngineDirectory The Engine directory.
	 */
	static EArticyInstallSource DetectInstallSource(const FString& PluginDirectory, const FString& EngineDirectory);

	/**
	 * @brief Reads the version and the page of a release from GitHub's reply.
	 * @return True if the reply names a release.
	 */
	static bool ParseRelease(const FString& Json, FString& OutVersion, FString& OutUrl);

private:
	void HandleResponse(bool bSucceeded, int32 ResponseCode, const FString& Content);

	FArticyPluginUpdateStatus Status;
};
