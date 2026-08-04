# ==============================================================================
###
### @file CMakeASM_IARInformation.cmake
###
### @brief Support for AT&T syntax assemblers, e.g. GNU as 
###
### @note Distributed under the OSI-approved BSD License (the "License")
###     see accompanying file Copyright.txt for details. This software is 
###     distributed WITHOUT ANY WARRANTY; without even the implied warranty of 
###     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
###     See the License for more information.
###
### @ingroup CMakeLib
###
### @copyright Copyright 2007-2009 Kitware, Inc.
# ==============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

set(ASM_DIALECT "_IAR")
# *.S files are supposed to be preprocessed, so they should not be passed to
# assembler but should be processed by gcc
set(CMAKE_ASM${ASM_DIALECT}_SOURCE_FILE_EXTENSIONS s;S;asm)

set(CMAKE_ASM${ASM_DIALECT}_COMPILE_OBJECT "<CMAKE_ASM${ASM_DIALECT}_COMPILER> ${CMAKE_ASM${ASM_DIALECT}_FLAGS} <SOURCE> -o <OBJECT>")

include(CMakeASMInformation)
set(ASM_DIALECT)
