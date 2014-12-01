#ifndef __PLAYER_H
#define __PLAYER_H

#include "sta013.h"
#include <stdint.h>

extern uint8_t Player_State;

#define PLAYER_STATE_WAITING    0
#define PLAYER_STATE_PLAYING    1

#define PLAYER_SIG_SKIP     0xFF

#define PLAYER_PLAY(x) osMessagePut(PlayerMsgQId, (uint32_t)x, 0)

#endif
