#include "EyeTargetDetails.h"
#include "VertigoEditor.h"
#include "TargetManagerDetails.h"
#include "Interaction/TargetManager.h"
#include "Interaction/EyeTarget.h"

void FVertigoEditorModule::StartupModule()
{
	UE_LOG(LogTemp, Warning, TEXT("Startup VertigoEditor Module"));

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// Register our custom detail panel
	PropertyModule.RegisterCustomClassLayout(
		ATargetManager::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FTargetManagerDetails::MakeInstance)
	);

	PropertyModule.RegisterCustomClassLayout(
		AEyeTarget::StaticClass()->GetFName(),
	FOnGetDetailCustomizationInstance::CreateStatic(&FEyeTargetDetails::MakeInstance)
);
}

void FVertigoEditorModule::ShutdownModule()
{
	UE_LOG(LogTemp, Warning, TEXT("Shutdown VertigoEditor Module"));

	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor")){
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

		// Unregister our custom detail panel to be clean
		PropertyModule.UnregisterCustomClassLayout(ATargetManager::StaticClass()->GetFName());
		PropertyModule.UnregisterCustomClassLayout(AEyeTarget::StaticClass()->GetFName());
	}
}

IMPLEMENT_MODULE(FVertigoEditorModule, VertigoEditor);