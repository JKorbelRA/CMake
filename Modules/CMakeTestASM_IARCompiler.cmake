# ==============================================================================
###
### @file CMakeTestASM_IARCompiler.cmake
###
### @brief This file is used by EnableLanguage in cmGlobalGenerator to
###     determine that the selected ASM-ATT "compiler" works.
###
### @note For assembler this can only check whether the compiler has been found,
###     because otherwise there would have to be a separate assembler source file
###     for each assembler on every architecture. 
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
include(CMakeTestASMCompiler)
set(ASM_DIALECT)
