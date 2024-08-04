#
# Try to find FFTW3  library (see www.fftw.org) Once run this will define:
#
# FFTW3_FOUND FFTW3_INCLUDE_DIR FFTW3_LIBRARIES FFTW3_LINK_DIRECTORIES
#
# You may set one of these options before including this file: FFTW3_USE_SSE2
#
# TODO: _F_ versions.
#
# Jan Woetzel 05/2004 www.mip.informatik.uni-kiel.de
# --------------------------------

find_path(
  FFTW3_INCLUDE_DIR
  fftw3.h
  ${FFTW3_DIR}/include
  ${FFTW3_HOME}/include
  ${FFTW3_DIR}
  ${FFTW3_HOME}
  $ENV{FFTW3_DIR}/include
  $ENV{FFTW3_HOME}/include
  $ENV{FFTW3_DIR}
  $ENV{FFTW3_HOME}
  /usr/include
  /usr/local/include
  $ENV{SOURCE_DIR}/fftw3
  $ENV{SOURCE_DIR}/fftw3/include
  $ENV{SOURCE_DIR}/fftw
  $ENV{SOURCE_DIR}/fftw/include)
# MESSAGE("DBG FFTW3_INCLUDE_DIR=${FFTW3_INCLUDE_DIR}")

set(FFTW3_POSSIBLE_LIBRARY_PATH
    ${FFTW3_DIR}/lib
    ${FFTW3_HOME}/lib
    ${FFTW3_DIR}
    ${FFTW3_HOME}
    $ENV{FFTW3_DIR}/lib
    $ENV{FFTW3_HOME}/lib
    $ENV{FFTW3_DIR}
    $ENV{FFTW3_HOME}
    /usr/lib
    /usr/local/lib
    $ENV{SOURCE_DIR}/fftw3
    $ENV{SOURCE_DIR}/fftw3/lib
    $ENV{SOURCE_DIR}/fftw
    $ENV{SOURCE_DIR}/fftw/lib)

# the lib prefix is containe din filename onf W32, unfortuantely. JW teh
# "general" lib:
find_library(
  FFTW3_FFTW_LIBRARY
  NAMES fftw3 libfftw libfftw3 libfftw3-3
  PATHS ${FFTW3_POSSIBLE_LIBRARY_PATH})
# MESSAGE("DBG FFTW3_FFTW_LIBRARY=${FFTW3_FFTW_LIBRARY}")

find_library(
  FFTW3_FFTWF_LIBRARY
  NAMES fftwf3 fftw3f fftwf libfftwf libfftwf3 libfftw3f-3
  PATHS ${FFTW3_POSSIBLE_LIBRARY_PATH})
# MESSAGE("DBG FFTW3_FFTWF_LIBRARY=${FFTW3_FFTWF_LIBRARY}")

find_library(
  FFTW3_FFTWL_LIBRARY
  NAMES fftwl3 fftw3l fftwl libfftwl libfftwl3 libfftw3l-3
  PATHS ${FFTW3_POSSIBLE_LIBRARY_PATH})
# MESSAGE("DBG FFTW3_FFTWF_LIBRARY=${FFTW3_FFTWL_LIBRARY}")

find_library(
  FFTW3_FFTW_SSE2_LIBRARY
  NAMES fftw_sse2 fftw3_sse2 libfftw_sse2 libfftw3_sse2
  PATHS ${FFTW3_POSSIBLE_LIBRARY_PATH})
# MESSAGE("DBG FFTW3_FFTW_SSE2_LIBRARY=${FFTW3_FFTW_SSE2_LIBRARY}")

find_library(
  FFTW3_FFTWF_SSE_LIBRARY
  NAMES fftwf_sse fftwf3_sse libfftwf_sse libfftwf3_sse
  PATHS ${FFTW3_POSSIBLE_LIBRARY_PATH})
# MESSAGE("DBG FFTW3_FFTWF_SSE_LIBRARY=${FFTW3_FFTWF_SSE_LIBRARY}")

# --------------------------------
# select one of the above default:
if(FFTW3_FFTW_LIBRARY)
  set(FFTW3_LIBRARIES ${FFTW3_FFTW_LIBRARY})
endif(FFTW3_FFTW_LIBRARY)
# specialized:
if(FFTW3_USE_SSE2 AND FFTW3_FFTW_SSE2_LIBRARY)
  set(FFTW3_LIBRARIES ${FFTW3_FFTW_SSE2_LIBRARY})
endif(FFTW3_USE_SSE2 AND FFTW3_FFTW_SSE2_LIBRARY)

# --------------------------------

if(FFTW3_LIBRARIES)
  if(FFTW3_INCLUDE_DIR)

    # OK, found all we need
    set(FFTW3_FOUND TRUE)
    get_filename_component(FFTW3_LINK_DIRECTORIES ${FFTW3_LIBRARIES} PATH)

  else(FFTW3_INCLUDE_DIR)
    message("FFTW3 include dir not found. Set FFTW3_DIR to find it.")
  endif(FFTW3_INCLUDE_DIR)
else(FFTW3_LIBRARIES)
  message("FFTW3 lib not found. Set FFTW3_DIR to find it.")
endif(FFTW3_LIBRARIES)

mark_as_advanced(
  FFTW3_INCLUDE_DIR
  FFTW3_LIBRARIES
  FFTW3_FFTW_LIBRARY
  FFTW3_FFTW_SSE2_LIBRARY
  FFTW3_FFTWF_LIBRARY
  FFTW3_FFTWF_SSE_LIBRARY
  FFTW3_FFTWL_LIBRARY
  FFTW3_LINK_DIRECTORIES)
