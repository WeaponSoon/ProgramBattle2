// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProgramBattle2GameMode.h"
#include "ProgramBattle2Character.h"
#include "UObject/ConstructorHelpers.h"

AProgramBattle2GameMode::AProgramBattle2GameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
