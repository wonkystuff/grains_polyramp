/**********************************************************************/
/* This file is autogenerated - DO NOT EDIT! (change calc.rb instead) */
/**********************************************************************/
/*
 * Wavetable calculator for the dr1.a firmware.
 *
 * Copyright (C) 2017-2018  John A. Tuffen
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Questions/queries can be directed to info@wonkystuff.net
 */
#include "arduino.h"
#define SRATE    (25000L)
#define WTSIZE   (512L)
#define FRACBITS (6L)
#define HALF     (0x20)
#define DACRANGE (1024L)
extern const uint16_t octaveLookup[DACRANGE];
extern const uint8_t  sine[WTSIZE];