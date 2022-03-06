# This file defines a series of "global" variables that will be used by each
# of the "DRIVER" components (example: the "output folder" where binaries
# should be placed) *and* also overwrites a series of Make variables used for
# deciding which compiler (and compiler options) to use for each architecture.

#/****************************************************************************************/
#/**                  Copyright (c) 2018, 2021 Skyworks Solution Inc.                   **/
#/****************************************************************************************/
#/** This software is provided 'as-is', without any express or implied warranty.        **/
#/** In no event will the authors be held liable for any damages arising from the use   **/
#/** of this software.                                                                  **/
#/** Permission is granted to anyone to use this software for any purpose, including    **/
#/** commercial applications, and to alter it and redistribute it freely, subject to    **/
#/** the following restrictions:                                                        **/
#/** 1. The origin of this software must not be misrepresented; you must not claim that **/
#/**    you wrote the original software. If you use this software in a product,         **/
#/**    an acknowledgment in the product documentation would be appreciated but is not  **/
#/**    required.                                                                       **/
#/** 2. Altered source versions must be plainly marked as such, and must not be         **/
#/**    misrepresented as being the original software.                                  **/
#/** 3. This notice may not be removed or altered from any source distribution.         **/
#/****************************************************************************************/

DEPFLAGS = -MT $@ -MMD -MP -MF $(OUTPUT)/$*.Td
OUTPUT_DIR=output
LIB_DIR=lib
PKG_LIB_DIR=pkg_lib
DEF=-D __INTERNAL_BUILD__
LIBPOSTFIX=_sync_timing
__PLATFORM__=

export OUTPUT_DIR
export LIB_DIR
export DEF
export LIBPOSTFIX
export __PLATFORM__
export DEPFLAGS


