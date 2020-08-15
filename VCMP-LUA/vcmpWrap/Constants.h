#pragma once
#include "pch.h"

#define MAX_TIMERS		2048
#define	MAX_PLAYERS		100
#define MAX_VEHICLES	1000
#define MAX_OBJECTS		3000
#define MAX_PICKUPS		2000
#define MAX_CHECKPOINTS 2000

typedef enum {
	Normal = 1,
	NormalRelative = 2,
	Turn = 3,
	TurnRelative = 4
} vcmpVehicleSpeed;