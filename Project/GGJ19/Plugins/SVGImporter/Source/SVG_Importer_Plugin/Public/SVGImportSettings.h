// Copyright Michael Galetzka, 2017

#pragma once

#include "SVGImportSettings.generated.h"

/** Enum to indicate what to import CSV as */
UENUM()
enum class ESVGImportType : uint8
{
    /** Render the SVG and import the result as texture */
    ESVG_ColorTexture UMETA(DisplayName = "RGBA texture rendering"),
    /** Import as multichannel signed distance field */
    ESVG_MSDF         UMETA(DisplayName = "Multichannel signed distance field"),
    /** Import as signed distance field */
    ESVG_SDF          UMETA(DisplayName = "Conventional single-channel signed distance field"),
    /** Import as a pseudo SDF */
    ESVG_PSDF         UMETA(DisplayName = "Single-channel signed pseudo-distance field")
};

/** Enum to indicate what resolution to import CSV with */
UENUM()
enum class ESVGImportResolution : uint8
{
    /** Import as 32x32 */
    ESVG_32     UMETA(DisplayName = "32x32px"),
    /** Import as 64x64 */
    ESVG_64     UMETA(DisplayName = "64x64px"),
    /** Import as 128x128 */
    ESVG_128    UMETA(DisplayName = "128x128px"), 
    /** Import as 256x256 */
    ESVG_256    UMETA(DisplayName = "256x256px"),
    /** Import as 512x512 */
    ESVG_512    UMETA(DisplayName = "512x512px"),
    /** Import as 1024x1024 */
    ESVG_1024   UMETA(DisplayName = "1024x1024px"),
    /** Import as a 2048x2048 */
    ESVG_2048   UMETA(DisplayName = "2048x2048px"),
    /** Import as a 4096x4096 */
    ESVG_4096   UMETA(DisplayName = "4096x4096px")
};

/**
 * 
 */
UCLASS()
class SVG_IMPORTER_PLUGIN_API USVGImportSettings : public UObject
{
	GENERATED_BODY()
public:

    /** Accessor and initializer **/
    static USVGImportSettings* Get();

    /** This setting defines how the SVG is rendered and imported */
    UPROPERTY(EditAnywhere, Category = GeneralImportSettings)
        ESVGImportType ImportType = ESVGImportType::ESVG_ColorTexture;

    /** The resolution of the resulting texture */
    UPROPERTY(EditAnywhere, Category = GeneralImportSettings, meta = (EditCondition = "!UseSvgResolution"))
        ESVGImportResolution ImportResolution = ESVGImportResolution::ESVG_1024;

    /** If true then the texture size is determined by the SVG size. */
    UPROPERTY(EditAnywhere, Category = GeneralImportSettings)
        bool UseSvgResolution = false;
	
    /** specifies the maximum angle to be considered a corner */
    UPROPERTY(EditAnywhere, Category = DistanceFieldSettings)
        float angleThresholdRadians = 1;

    /** Specifies the width of the range around the shape between the minimum and maximum representable signed distance in pixels */
    UPROPERTY(EditAnywhere, Category = DistanceFieldSettings)
        float pixelRange = 4;

    UPROPERTY(EditAnywhere, Category = DistanceFieldSettings, AdvancedDisplay)
        FVector2D TranslationSDF;

    UPROPERTY(EditAnywhere, Category = DistanceFieldSettings, AdvancedDisplay)
        float edgeThreshold = 1;
    
    /** (Only for distance fields) Tries to convert all SVG elements to bezier curves before importing. This option should only be used if the default creates render artifacts. */
    UPROPERTY(EditAnywhere, Category = DistanceFieldSettings, AdvancedDisplay)
        bool ConvertAllElementsToPath = false;
};
