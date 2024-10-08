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

// TODO: fix bike gun
// check if machinegun sentry cam and hold L2 manual aim use same values, they feel similar and left stick only does up and down just like machinegun
//	 try freezing fov above 400, the camera moves independent of gun

// OFFSET addresses, requires a base address to use
#define MOHU_camx 0x000F116E - 0x000F0BE0
#define MOHU_camy 0x000F1162 - 0x000F0BE0
#define MOHU_on_sentry_flag 0x000E6D2E - 0x000E6D20 // 2 = on sentry, 0 = not on sentry?
#define MOHU_fov 0x000DEA58 - 0x000DE5B0
#define MOHU_machinegun_camx 0x242
#define MOHU_machinegun_sanity_address 0xC // -C from machinegun base
#define MOHU_machinegun_sanity_value 0xE003F900 // value expected at offset
#define MOHU_bikegun_camx 0x000D63A0 - 0x000D6160
#define MOHU_bikegun_sanity_offset 0x15
#define MOHU_bikegun_sanity_value 0x42
// STATIC addresses
#define MOHU_playerbase 0x000AA1A8
#define MOHU_playerbase_sanity 0x14
#define MOHU_rightstick_x 0x000CB840 // 1 byte, 00 left - 80 middle - FF right
#define MOHU_rightstick_y 0x000CB841 // 1 byte, 00 top - 80 middle - FF bottom
#define MOHU_machinegunbase 0x001FFDB4
#define MOHU_bikegunbase 0x000AA388

static uint8_t PS1_MOHU_Status(void);
static uint8_t PS1_MOHU_DetectPlayer(void);
static void PS1_MOHU_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"PS1 Medal of Honor: Underground",
	PS1_MOHU_Status,
	PS1_MOHU_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS1_MOHUNDERGROUND = &GAMEDRIVER_INTERFACE;

static uint32_t playerbase = 0;
static uint32_t machinegunbase = 0;
static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_MOHU_Status(void)
{
	return (PS1_MEM_ReadWord(0xB8B4) == 0x6D3A5C53U && PS1_MEM_ReadWord(0xB8B8) == 0x4C55535FU &&
			PS1_MEM_ReadWord(0xB8BC) == 0x3031322EU && PS1_MEM_ReadWord(0xB8C0) == 0x37303B31U); // m:\SLUS_012.70;1
}
//==========================================================================
// Purpose: determines if there is a player
//==========================================================================
static uint8_t PS1_MOHU_DetectPlayer(void)
{
	const uint32_t tempplayerbase = PS1_MEM_ReadPointer(MOHU_playerbase);
	if (PS1WITHINMEMRANGE(tempplayerbase))
	{
		const uint32_t tempsanity_1 = PS1_MEM_ReadWord(tempplayerbase);
		const uint32_t tempsanity_2 = PS1_MEM_ReadWord(tempplayerbase + MOHU_playerbase_sanity);
		if (tempsanity_1 == 0x02000300 && tempsanity_2 == 0xD04664C8)
		{
			playerbase = tempplayerbase;
			return 1;
		}
	}

	return 0;
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_MOHU_Inject(void)
{
	// NOTE: if controller type is set to digital, auto-center will be enabled

	if(!PS1_MOHU_DetectPlayer())
		return;
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	const float looksensitivity = (float)sensitivity / 20.f;

	// if (PS1_MEM_ReadByte(playerbase + MOHU_bikegun_sanity_offset) == MOHU_bikegun_sanity_value) { // on bike gun
	// 	uint16_t gunbikebase = PS1_MEM_ReadPointer(MOHU_bikegunbase);
	// 	uint16_t bg_camx = PS1_MEM_ReadHalfword(gunbikebase + MOHU_bikegun_camx);

	// 	bg_camx += (float)xmouse * looksensitivity;

	// 	PS1_MEM_WriteHalfword(gunbikebase + MOHU_bikegun_camx, (uint16_t)bg_camx);
	// }
	if (PS1_MEM_ReadByte(playerbase + MOHU_on_sentry_flag)) { // on mounted machinegun
		if (machinegunbase == 0)
		{
			// return if not a pointer
			if (PS1_MEM_ReadByte(MOHU_machinegunbase + 0x3) != 0x80) 
				return;

			machinegunbase = PS1_MEM_ReadPointer(MOHU_machinegunbase);

			uint32_t sanity_address = machinegunbase - MOHU_machinegun_sanity_address;
			// check that pointer points to machinegun
			if (PS1_MEM_ReadWord(sanity_address) != MOHU_machinegun_sanity_value)
			{
				machinegunbase = 0;
				return;
			}
		}

		uint16_t mg_camx = PS1_MEM_ReadHalfword(machinegunbase - MOHU_machinegun_camx);
		uint8_t stick_y = PS1_MEM_ReadByte(MOHU_rightstick_y);

		mg_camx += (float)xmouse * looksensitivity;

		// simulate right stick movement due to not being able to find a writeable camy value
		if (ymouse < 0)
			stick_y = 0x0;
		else
			stick_y = 0xFF;

		PS1_MEM_WriteHalfword(machinegunbase - MOHU_machinegun_camx, (uint16_t)mg_camx);
		PS1_MEM_WriteByte(MOHU_rightstick_y, stick_y);
	}
	else { // on foot
		machinegunbase = 0;

		// uint16_t fov = PS1_MEM_ReadHalfword(playerbase + MOHU_fov);
		// uint16_t camx = PS1_MEM_ReadHalfword(playerbase + MOHU_camx);
		// uint16_t camy = PS1_MEM_ReadHalfword(playerbase + MOHU_camy);

		// fov /= 400;

		// camx += (float)xmouse * looksensitivity / fov;

		// camy -= (float)ymouse * looksensitivity / fov;
		// // clamp camy
		// if (camy > 60000 && camy < 64754)
		// 	camy = 64754U;
		// if (camy > 682 && camy < 4000)
		// 	camy = 682U;

		// PS1_MEM_WriteHalfword(playerbase + MOHU_camx, (uint16_t)camx);
		// PS1_MEM_WriteHalfword(playerbase + MOHU_camy, (uint16_t)camy);

		uint16_t camX = PS1_MEM_ReadHalfword(playerbase + MOHU_camx);
		uint16_t camY = PS1_MEM_ReadHalfword(playerbase + MOHU_camy);
		float camXF = (float)camX;
		float camYF = (float)camY;

		// const float looksensitivity = (float)sensitivity / 20.f;
		const float scale = 1.f;

		float dx = (float)xmouse * looksensitivity * scale;
		AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);

		float ym = (float)(invertpitch ? -ymouse : ymouse);
		float dy = -ym * looksensitivity * scale;
		AccumulateAddRemainder(&camYF, &yAccumulator, -ym, dy);

		// while (camYF > 4096)
		// 	camYF -= 4096;

		// clamp y-axis
		if (camYF > 60000 && camYF < 64764)
			camYF = 64754;
		if (camYF > 682 && camYF < 4000)
			camYF = 682;

		PS1_MEM_WriteHalfword(playerbase + MOHU_camx, (uint16_t)camXF);
		PS1_MEM_WriteHalfword(playerbase + MOHU_camy, (uint16_t)camYF);
	}
}