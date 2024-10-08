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
#include <stdio.h>
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"

#define WINE_CAMBASE_POINTER 0x80102CD0
#define WINE_CAMBASE_SANITY_VALUE_1 0x0007D000
#define WINE_CAMBASE_SANITY_VALUE_2 0xFFFFFFFF
// -- offsets from cambase --
#define WINE_CAMBASE_SANITY_1 0x0
#define WINE_CAMBASE_SANITY_2 0xC
// offsets from camValueBasePointer
#define WINE_CAMX -0x44
#define WINE_CAMY 0x34
#define WINE_CAM_ZOOM 0x3C
#define WINE_AUTO_LEVEL 0x17D

#define WINE_STATUS 0x8022CFE8
#define WINE_STATUS_NOT_BUSY 0xDE000000

static uint8_t N64_WINE_Status(void);
static uint8_t N64_WINE_DetectCam(void);
static void N64_WINE_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"007: The World is Not Enough",
	N64_WINE_Status,
	N64_WINE_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_N64_007WINE = &GAMEDRIVER_INTERFACE;

static uint32_t camBase = 0;
static uint32_t camValueBase = 0;

static uint8_t N64_WINE_Status(void)
{
	return (N64_MEM_ReadUInt(0x80000000) == 0x3C1A800A && N64_MEM_ReadUInt(0x80000004) == 0x275A71D0); // unique header in RDRAM
}

static uint8_t N64_WINE_DetectCam(void)
{
	uint32_t tempCamBase = N64_MEM_ReadUInt(WINE_CAMBASE_POINTER);
	if (tempCamBase &&
		N64_MEM_ReadUInt(tempCamBase + WINE_CAMBASE_SANITY_1) == WINE_CAMBASE_SANITY_VALUE_1)// &&
		// N64_MEM_ReadUInt(tempCamBase + WINE_CAMBASE_SANITY_2) == WINE_CAMBASE_SANITY_VALUE_2)
	{
		camBase = tempCamBase;
		return 1;
	}
	return 0;
}

static void N64_WINE_Inject(void)
{
	if (N64_MEM_ReadUInt(WINE_STATUS) != WINE_STATUS_NOT_BUSY)
		return;

	if (!N64_WINE_DetectCam())
		return;
	// camBase = N64_MEM_ReadUInt(WINE_CAMBASE);
	
	N64_MEM_WriteByte(camBase + WINE_AUTO_LEVEL - 0x1, 0x1);
	N64_MEM_WriteByte(camBase + WINE_AUTO_LEVEL, 0x1);

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	const float looksensitivity = (float)sensitivity;
	const float scale = 20000.f;
	const float zoom = 1 / N64_MEM_ReadFloat(camBase + WINE_CAM_ZOOM);

	float camX = N64_MEM_ReadFloat(camBase + WINE_CAMX);
	float camY = N64_MEM_ReadFloat(camBase + WINE_CAMY);

	camX -= (float)xmouse * looksensitivity / scale * 1024.f * zoom;
	camY -= (float)ymouse * looksensitivity / scale * zoom;
	
	camY = ClampFloat(camY, -1.f, 1.f);

	N64_MEM_WriteFloat(camBase + WINE_CAMX, camX);
	N64_MEM_WriteFloat(camBase + WINE_CAMY, camY);
}