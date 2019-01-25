// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/**
 * UI to pick options when importing a data table
 */

#pragma once
#include "SVGImportFactory.h"



class SVG_IMPORTER_PLUGIN_API SSVGImportOptions : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SSVGImportOptions)
        : _WidgetWindow()
        , _FullPath()
        , _ImportSettings(nullptr)
    {}

    SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)
        SLATE_ARGUMENT(FText, FullPath)
        SLATE_ARGUMENT(USVGImportSettings*, ImportSettings)
        SLATE_END_ARGS()

        SSVGImportOptions()
        : bImport(false),
          ImportSettings(nullptr)
    {}

    void Construct(const FArguments& InArgs);

    /** If we should import */
    bool ShouldImport();

    /** Called when 'OK' button is pressed */
    FReply OnImport();

    /** Called when 'Cancel' button is pressed */
    FReply OnCancel();

    USVGImportSettings* GetSettings() const {
        return ImportSettings;
    }

private:
    bool bImport;
    USVGImportSettings*	ImportSettings;

    /** Window that owns us */
    TWeakPtr< SWindow >	WidgetWindow;

};
