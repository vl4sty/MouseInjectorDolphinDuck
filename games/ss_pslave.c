//===========================================================
// Mouse Injector for Dolphin
//==========================================================================
// Copyright (C) 2019-2020 Carnivorous
// All rights reserved.
//
// Mouse Injector is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, visit http://www.gnu.org/licenses/gpl-2.0.html
//==========================================================================
#include <stdint.h>
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"

#define PS_CAMY 0x632AC
#define PS_CAMX_INT 0x632A8
#define PS_CAMX_FRAC 0x632AA

static uint8_t SS_PS_Status(void);
static void SS_PS_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"PowerSlave",
	SS_PS_Status,
	SS_PS_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_SS_POWERSLAVE = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t SS_PS_Status(void)
{
	return (PS1_MEM_ReadWord(0x0) == 0x00064A09U && PS1_MEM_ReadWord(0x4) == 0x00064A09U);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void SS_PS_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float looksensitivity = (float)sensitivity / 20.f;
	float scale = 10000.f;
	// float scale = 1.f;
	
	int16_t camXInt = PS1_MEM_ReadInt16(PS_CAMX_INT);
	uint16_t camXFrac = PS1_MEM_ReadHalfword(PS_CAMX_FRAC);
	float camXFracF = (float)camXFrac;
	float camXF = (float)camXInt;

	camXFracF -= (float)xmouse * looksensitivity * scale;
	// float dx = -(float)xmouse * looksensitivity * scale;
	// AccumulateAddRemainder(&camXFracF, &xAccumulator, -xmouse, dx);

	while (camXFracF > 65536)
	{
		camXFracF -= 65536;
		camXF += 1;
	}
	while (camXFracF < 0)
	{
		camXFracF += 65536;
		camXF -= 1;
	}

	while (camXF >= 180)
		camXF -= 360;
	while (camXF <= -180)
		camXF += 360;

	PS1_MEM_WriteInt16(PS_CAMX_INT, (int16_t)camXF);
	PS1_MEM_WriteHalfword(PS_CAMX_FRAC, (uint16_t)camXFracF);

}