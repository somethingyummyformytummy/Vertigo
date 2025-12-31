// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class VertigoEditor : ModuleRules
{
	public VertigoEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicIncludePaths.Add("VertigoEditor/Public");
		PrivateIncludePaths.Add("VertigoEditor/Private");
		
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "DetailCustomizations", "PropertyEditor", "EditorStyle", "Vertigo", "UMG" });
		PublicDependencyModuleNames.AddRange(new string[] { "UnrealEd", "DesktopPlatform", "Slate", "SlateCore" });
	}
}
