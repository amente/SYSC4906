#ifndef __PLAYER_H
#define __PLAYER_H

#include "sta013.h"
#include <stdint.h>

#define PLAYER_SIG_SKIP     0xFF

#define PLAYER_PLAY(x) osMessagePut(PlayerMsgQId, (uint32_t)x, osWaitForever)

#endif
