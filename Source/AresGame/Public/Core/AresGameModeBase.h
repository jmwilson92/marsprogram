// The default game mode for every Ares map.
//
// Its only job is wiring the pawn and controller classes in C++, so that any
// map — including a stock empty one — plays as MARS without a Blueprint asset
// or a per-map override.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "AresGameModeBase.generated.h"

UCLASS()
class ARESGAME_API AAresGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AAresGameModeBase();
};
