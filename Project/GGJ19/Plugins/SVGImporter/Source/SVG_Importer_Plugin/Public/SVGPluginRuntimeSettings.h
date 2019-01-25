// Copyright Michael Galetzka, 2017

#pragma once

#include "SVGImportSettings.h"
#include "SVGPluginRuntimeSettings.generated.h"

/**
 * Implements the settings for the SVG Importer plugin.
 */
UCLASS(config = Engine, defaultconfig)
class SVG_IMPORTER_PLUGIN_API USVGPluginRuntimeSettings : public UObject
{
	GENERATED_BODY()

public:
        // Hide the options dialog on reimport and use the settings from the assets creation
        UPROPERTY(EditAnywhere, config, Category = Settings)
        bool bSilentReimport = true;
        
        /** This setting defines how the SVG is rendered and imported */
        UPROPERTY(EditAnywhere, config, Category = DefaultImportSettings, meta = (ConfigRestartRequired = true))
            ESVGImportType ImportType = ESVGImportType::ESVG_ColorTexture;

        /** The resolution of the resulting texture */
        UPROPERTY(EditAnywhere, config, Category = DefaultImportSettings, meta = (ConfigRestartRequired = true, EditCondition = "!UseSvgResolution"))
            ESVGImportResolution ImportResolution = ESVGImportResolution::ESVG_1024;

        /** If true then the texture size is determined by the SVG size. */
        UPROPERTY(EditAnywhere, config, Category = DefaultImportSettings, meta = (ConfigRestartRequired = true))
            bool UseSvgResolution = false;

        /** specifies the maximum angle to be considered a corner */
        UPROPERTY(EditAnywhere, config, Category = DefaultImportSettings, meta = (ConfigRestartRequired = true))
            float angleThresholdRadians = 1;

        /** Specifies the width of the range around the shape between the minimum and maximum representable signed distance in pixels */
        UPROPERTY(EditAnywhere, config, Category = DefaultImportSettings, meta = (ConfigRestartRequired = true))
            float pixelRange = 4;

        UPROPERTY(EditAnywhere, config, Category = DefaultImportSettings, AdvancedDisplay, meta = (ConfigRestartRequired = true))
            FVector2D TranslationSDF;

        UPROPERTY(EditAnywhere, config, Category = DefaultImportSettings, AdvancedDisplay, meta = (ConfigRestartRequired = true))
            float edgeThreshold = 1;

        /** (Only for distance fields) Tries to convert all SVG elements to bezier curves before importing. This option should only be used if the default creates render artifacts. */
        UPROPERTY(EditAnywhere, config, Category = DefaultImportSettings, AdvancedDisplay, meta = (ConfigRestartRequired = true))
            bool ConvertAllElementsToPath = false;
};
