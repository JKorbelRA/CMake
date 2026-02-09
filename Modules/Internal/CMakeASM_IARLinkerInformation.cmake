# ==============================================================================
###
### @file CMakeASM_IARLinkerInformation.cmake
###
### @brief Support for IAR syntax assemblers, e.g. GNU as 
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
include(Internal/CMakeASMLinkerInformation)
set(ASM_DIALECT)
