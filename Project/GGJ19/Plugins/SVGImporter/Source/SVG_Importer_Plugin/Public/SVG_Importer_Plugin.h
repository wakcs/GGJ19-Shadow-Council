// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleInterface.h"

class FSVG_Importer_PluginModule : public IModuleInterface
{
public:
    // Begin IModuleInterface
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    // End IModuleInterface
};