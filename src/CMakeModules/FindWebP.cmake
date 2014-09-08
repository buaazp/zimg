# - Find WebP library
# Find the native WebP headers and libraries.
#
#  WEBP_INCLUDE_DIRS - where to find webp/decode.h, etc.
#  WEBP_LIBRARIES    - List of libraries when using webp.
#  WEBP_FOUND        - True if webp is found.

#=============================================================================
#Copyright 2000-2009 Kitware, Inc., Insight Software Consortium
#All rights reserved.
#
#Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#* Redistributions of source code must retain the above copyright notice,
#this list of conditions and the following disclaimer.
#
#* Redistributions in binary form must reproduce the above copyright notice,
#this list of conditions and the following disclaimer in the documentation
#and/or other materials provided with the distribution.
#
#* Neither the names of Kitware, Inc., the Insight Software Consortium, nor
#the names of their contributors may be used to endorse or promote products
#derived from this software without specific prior written  permission.
#
#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
#LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

# Look for the header file.
FIND_PATH(WEBP_INCLUDE_DIR NAMES webp/decode.h)
MARK_AS_ADVANCED(WEBP_INCLUDE_DIR)

# Look for the library.
FIND_LIBRARY(WEBP_LIBRARY NAMES webp)
MARK_AS_ADVANCED(WEBP_LIBRARY)

# handle the QUIETLY and REQUIRED arguments and set WEBFOUND_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(WebP DEFAULT_MSG WEBP_LIBRARY WEBP_INCLUDE_DIR)

SET(WEBP_LIBRARIES ${WEBP_LIBRARY})
SET(WEBP_INCLUDE_DIRS ${WEBP_INCLUDE_DIR})

SET(_WEBP_RQ_INCLUDES ${CMAKE_REQUIRED_INCLUDES})

if(NOT DEFINED _WEBP_COMPILATION_TEST)
  INCLUDE (CheckCSourceCompiles)
  SET(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${WEBP_INCLUDE_DIRS})
  CHECK_C_SOURCE_COMPILES("#include <webp/decode.h>
  int main(void) {
  #if WEBP_DECODER_ABI_VERSION < 0x0200
  error; // Deliberate compile-time error
  #endif
  }"
   _WEBP_COMPILATION_TEST)
  SET(CMAKE_REQUIRED_INCLUDES ${_WEBP_RQ_INCLUDES})
endif()

if(NOT _WEBP_COMPILATION_TEST)
  set( USE_INTERNAL_WEBP 1 )
endif()
