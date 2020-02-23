// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class GameLiftTutorialTarget : TargetRules
{
	public GameLiftTutorialTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

		// enable logs and debugging for Shipping builds
		if (Configuration == UnrealTargetConfiguration.Shipping)
		{
			BuildEnvironment = TargetBuildEnvironment.Unique;
			//bUseChecksInShipping = true; <- need to update unreal engine to version 4.24.2 to fix issue caused by this line
			bUseLoggingInShipping = true;
		}

		ExtraModuleNames.Add("GameLiftTutorial");
	}
}
