// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/**
 * Factory for importing Scalable Vector Graphics (SVG)
 */

#pragma once
#include <vector>
#include "Factories/Factory.h"
#include "Editor.h"
#include "EditorReimportHandler.h"
#include "SVGImportSettings.h"
#include "SVGAssetImportData.h"
#include "SVGImportFactory.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSVGImportFactory, Log, All);

struct SVGRenderResult {
    TArray<FString> Problems;
    int32 svgWidth;
    int32 svgHeight;
};

UCLASS(hidecategories = Object)
class SVG_IMPORTER_PLUGIN_API USVGImportFactory : public UFactory, public FReimportHandler
{
    GENERATED_UCLASS_BODY()

    //~ Begin UFactory Interface
    virtual FText GetDisplayName() const override;
    virtual UObject* FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn) override;
    virtual bool DoesSupportClass(UClass * Class) override;
    virtual bool FactoryCanImport(const FString& Filename) override;
    //~ End UFactory Interface

    //~ Begin FReimportHandler Interface
    virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
    virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
    virtual EReimportResult::Type Reimport(UObject* Obj) override;
    virtual int32 GetPriority() const override;
    //~ End FReimportHandler Interface

private:
    SVGRenderResult ImportSVG(std::vector<uint8>& TargetPixelData, const FString& RawData, FSVGAssetSettings settings);
    bool Reimport(UObject* Obj, const FString& Path);

    static ESVGImportResolution dimensionToEnum(int32 dimension);
    static int32 enumToDimension(ESVGImportResolution value);
    static int32 enumToPixelWidth(ESVGImportType ImportType);
    static ETextureSourceFormat enumToTextureFormat(ESVGImportType ImportType);

    UPROPERTY()
        FSVGAssetSettings AutomatedImportSettings;
};

