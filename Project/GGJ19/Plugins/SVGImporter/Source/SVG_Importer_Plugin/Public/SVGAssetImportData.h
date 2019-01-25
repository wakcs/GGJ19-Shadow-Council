// Copyright Michael Galetzka, 2017

#pragma once

#include "Editor.h"
#include "EditorFramework/AssetImportData.h"
#include "SVGImportSettings.h"
#include "SVGAssetImportData.generated.h"


USTRUCT()
struct FSVGAssetSettings
{
    GENERATED_BODY()
        FSVGAssetSettings();

    UPROPERTY()
        ESVGImportResolution ImportResolution = ESVGImportResolution::ESVG_1024;

    UPROPERTY()
        bool UseSvgResolution = false;

    UPROPERTY()
        ESVGImportType ImportType = ESVGImportType::ESVG_ColorTexture;

    UPROPERTY()
        float AngleThreshold = 1;

    UPROPERTY()
        float Range = 4;

    UPROPERTY()
        FVector2D TranslationSDF;

    UPROPERTY()
        float EdgeThreshold = 1;

    UPROPERTY()
        bool ConvertAllElementsToPath = false;
};

/**
 * 
 */
UCLASS()
class SVG_IMPORTER_PLUGIN_API USVGAssetImportData : public UAssetImportData
{
	GENERATED_BODY()
public:
        UPROPERTY()
        FSVGAssetSettings ImportSettings;
};
