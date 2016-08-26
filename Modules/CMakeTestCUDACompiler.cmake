# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

if(CMAKE_CUDA_COMPILER_FORCED)
  # The compiler configuration was forced by the user.
  # Assume the user has configured all compiler information.
  set(CMAKE_CUDA_COMPILER_WORKS TRUE)
  return()
endif()

include(CMakeTestCompilerCommon)

# Remove any cached result from an older CMake version.
# We now store this in CMakeCUDACompiler.cmake.
unset(CMAKE_CUDA_COMPILER_WORKS CACHE)

# This file is used by EnableLanguage in cmGlobalGenerator to
# determine that that selected cuda compiler can actually compile
# and link the most basic of programs.   If not, a fatal error
# is set and cmake stops processing commands and will not generate
# any makefiles or projects.
if(NOT CMAKE_CUDA_COMPILER_WORKS)
  PrintTestCompilerStatus("CUDA" "")
  file(WRITE ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/main.cu
    "#ifndef __CUDACC__\n"
    "# error \"The CMAKE_CUDA_COMPILER is set to an invalid CUDA compiler\"\n"
    "#endif\n"
    "int main(){return 0;}\n")

  try_compile(CMAKE_CUDA_COMPILER_WORKS ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/main.cu
    OUTPUT_VARIABLE __CMAKE_CUDA_COMPILER_OUTPUT)

  # Move result from cache to normal variable.
  set(CMAKE_CUDA_COMPILER_WORKS ${CMAKE_CUDA_COMPILER_WORKS})
  unset(CMAKE_CUDA_COMPILER_WORKS CACHE)
  set(CUDA_TEST_WAS_RUN 1)
endif()

if(NOT CMAKE_CUDA_COMPILER_WORKS)
  PrintTestCompilerStatus("CUDA" " -- broken")
  file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
    "Determining if the CUDA compiler works failed with "
    "the following output:\n${__CMAKE_CUDA_COMPILER_OUTPUT}\n\n")
  message(FATAL_ERROR "The CUDA compiler \"${CMAKE_CUDA_COMPILER}\" "
    "is not able to compile a simple test program.\nIt fails "
    "with the following output:\n ${__CMAKE_CUDA_COMPILER_OUTPUT}\n\n"
    "CMake will not be able to correctly generate this project.")
else()
  if(CUDA_TEST_WAS_RUN)
    PrintTestCompilerStatus("CUDA" " -- works")
    file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
      "Determining if the CUDA compiler works passed with "
      "the following output:\n${__CMAKE_CUDA_COMPILER_OUTPUT}\n\n")
  endif()
endif()

unset(__CMAKE_CUDA_COMPILER_OUTPUT)
