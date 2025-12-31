#pragma once

#include "CoreMinimal.h"

class FVertigoEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};