// Copyright Michael Galetzka, 2017

#include "SVGImportSettings.h"
#include "SVG_Importer_Plugin.h"


USVGImportSettings* USVGImportSettings::Get()
{
    // This is a singleton, use default object
    USVGImportSettings* DefaultSettings = GetMutableDefault<USVGImportSettings>();
    return DefaultSettings;
}
