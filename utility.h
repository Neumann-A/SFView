/*
 * SF Viewer - A program to visualize MPI system matrices
 * Copyright (C) 2014-2017  Ulrich Heinen <ulrich.heinen@hs-pforzheim.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * $Id: SystemMatrix.h 63 2016-12-11 15:01:06Z uhei $
 */

#ifndef UTILITY_H
#define UTILITY_H

// This file contains definitions that might not be available on all platforms

#ifndef M_PI
// Not predefined on Windows MSVC
#    define M_PI 3.14159265358979323846
#endif

#endif // UTILITY_H
