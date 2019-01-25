// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SVG_Importer_Plugin.h"
#include "Modules/ModuleManager.h"
#include "ISettingsModule.h"
#include "SVGPluginRuntimeSettings.h"

#define LOCTEXT_NAMESPACE "SVG_Importer_Plugin" 
	
IMPLEMENT_MODULE(FSVG_Importer_PluginModule, SVG_Importer_Plugin)

void FSVG_Importer_PluginModule::StartupModule()
{
    auto SavedSettings = GetMutableDefault<USVGPluginRuntimeSettings>();
    auto importSettings = USVGImportSettings::Get();
    importSettings->ImportType = SavedSettings->ImportType;
    importSettings->ImportResolution = SavedSettings->ImportResolution;
    importSettings->UseSvgResolution = SavedSettings->UseSvgResolution;
    importSettings->angleThresholdRadians = SavedSettings->angleThresholdRadians;
    importSettings->pixelRange = SavedSettings->pixelRange;
    importSettings->TranslationSDF = SavedSettings->TranslationSDF;
    importSettings->edgeThreshold = SavedSettings->edgeThreshold;
    importSettings->ConvertAllElementsToPath = SavedSettings->ConvertAllElementsToPath;

    ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
    if (SettingsModule)
    {
        SettingsModule->RegisterSettings("Project", "Plugins", "SVG Importer",
            LOCTEXT("RuntimeSettingsName", "SVG Importer"),
            LOCTEXT("RuntimeSettingsDescription", "General settings for SVG Importer."),
            SavedSettings);
    }
}

void FSVG_Importer_PluginModule::ShutdownModule()
{
    ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
    if (SettingsModule != nullptr)
    {
        SettingsModule->UnregisterSettings("Project", "Plugins", "SVG Importer");
    }
}

#undef LOCTEXT_NAMESPACE
