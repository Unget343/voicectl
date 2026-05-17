cmake_minimum_required(VERSION 3.16)

project(voicectl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(voicectl src/main.cpp)

# --- Vosk ---------------------------------------------------------------
#
# This repo vendors Vosk API sources/headers under include/vosk-api/src,
# but building libvosk requires Kaldi (+ OpenFST and a BLAS/LAPACK impl).
#
# Two modes:
#   1) Use prebuilt/system libvosk: set -DVOSK_LIBRARY=/path/to/libvosk.so
#   2) Build libvosk from the vendored sources: set -DVOSK_BUILD=ON and provide Kaldi paths.

option(VOSK_BUILD "Build libvosk.so from include/vosk-api/src (requires Kaldi toolchain)" OFF)

set(VOSK_API_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/vosk-api/src")
set(VOSK_BUNDLED_LIBVOSK_SO "${CMAKE_CURRENT_SOURCE_DIR}/libvosk.so")
set(VOSK_BUNDLED_LIBVOSK_A  "${CMAKE_CURRENT_SOURCE_DIR}/libvosk.a")
set(VOSK_BUNDLED_LIBVOSK_SO_IN_API "${VOSK_API_SRC_DIR}/libvosk.so")
set(VOSK_BUNDLED_LIBVOSK_A_IN_API  "${VOSK_API_SRC_DIR}/libvosk.a")

if(VOSK_BUILD)
  # If VOSK_BUILD was enabled accidentally (or via an env var) but Kaldi isn't actually
  # available, fall back to a bundled/prebuilt libvosk if present.
  set(KALDI_ROOT "$ENV{KALDI_ROOT}" CACHE PATH "Path to Kaldi root (must contain src/ and built libs)")
  if(KALDI_ROOT STREQUAL "" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/kaldi/src/base/kaldi-common.h")
    set(KALDI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/include/kaldi" CACHE PATH "Path to Kaldi root (must contain src/ and built libs)" FORCE)
  endif()
  if(NOT EXISTS "${KALDI_ROOT}/src/base/kaldi-common.h")
    if(EXISTS "${VOSK_BUNDLED_LIBVOSK_SO}" OR EXISTS "${VOSK_BUNDLED_LIBVOSK_A}" OR
       EXISTS "${VOSK_BUNDLED_LIBVOSK_SO_IN_API}" OR EXISTS "${VOSK_BUNDLED_LIBVOSK_A_IN_API}")
      message(WARNING
        "VOSK_BUILD=ON but KALDI_ROOT='${KALDI_ROOT}' is invalid (missing src/base/kaldi-common.h). "
        "Falling back to prebuilt libvosk.")
      set(VOSK_BUILD OFF CACHE BOOL "Build libvosk.so from include/vosk-api/src (requires Kaldi toolchain)" FORCE)
    else()
      message(FATAL_ERROR
        "VOSK_BUILD=ON requires a valid Kaldi checkout at -DKALDI_ROOT=/path/to/kaldi (must contain src/base/kaldi-common.h), "
        "or provide a prebuilt libvosk via -DVOSK_LIBRARY=/path/to/libvosk.so.")
    endif()
  endif()
endif()

if(VOSK_BUILD)
  add_library(vosk SHARED
    "${VOSK_API_SRC_DIR}/recognizer.cc"
    "${VOSK_API_SRC_DIR}/language_model.cc"
    "${VOSK_API_SRC_DIR}/model.cc"
    "${VOSK_API_SRC_DIR}/spk_model.cc"
    "${VOSK_API_SRC_DIR}/vosk_api.cc"
    "${VOSK_API_SRC_DIR}/postprocessor.cc"
  )
  set_target_properties(vosk PROPERTIES
    OUTPUT_NAME vosk
    POSITION_INDEPENDENT_CODE ON
  )
  target_include_directories(vosk PUBLIC "${VOSK_API_SRC_DIR}")

  # Mirrors include/vosk-api/src/Makefile defaults. Override these cache vars as needed.
  if(KALDI_ROOT STREQUAL "")
    message(FATAL_ERROR "VOSK_BUILD=ON requires -DKALDI_ROOT=/path/to/kaldi (built).")
  endif()

  set(OPENFST_ROOT "${KALDI_ROOT}/tools/openfst" CACHE PATH "Path to OpenFST root")
  set(OPENBLAS_ROOT "${KALDI_ROOT}/tools/OpenBLAS/install" CACHE PATH "Path to OpenBLAS install prefix")
  option(KALDI_USE_SHARED "Link against shared Kaldi libs (libkaldi-*.so) instead of static .a archives" OFF)

  target_compile_definitions(vosk PRIVATE FST_NO_DYNAMIC_LINKING=1)
  target_include_directories(vosk PRIVATE
    "${KALDI_ROOT}/src"
    "${OPENFST_ROOT}/include"
    "${OPENBLAS_ROOT}/include"
  )
  target_compile_options(vosk PRIVATE -Wno-deprecated-declarations)

  if(KALDI_USE_SHARED)
    target_link_directories(vosk PRIVATE "${KALDI_ROOT}/libs")
    target_link_libraries(vosk PRIVATE
      kaldi-online2 kaldi-decoder kaldi-ivector kaldi-gmm kaldi-tree
      kaldi-feat kaldi-lat kaldi-lm kaldi-rnnlm kaldi-hmm kaldi-nnet3
      kaldi-transform kaldi-cudamatrix kaldi-matrix kaldi-fstext
      kaldi-util kaldi-base
      fst fstngram
      openblas lapack blas f2c
    )
  else()
    target_link_libraries(vosk PRIVATE
      "${KALDI_ROOT}/src/online2/kaldi-online2.a"
      "${KALDI_ROOT}/src/decoder/kaldi-decoder.a"
      "${KALDI_ROOT}/src/ivector/kaldi-ivector.a"
      "${KALDI_ROOT}/src/gmm/kaldi-gmm.a"
      "${KALDI_ROOT}/src/tree/kaldi-tree.a"
      "${KALDI_ROOT}/src/feat/kaldi-feat.a"
      "${KALDI_ROOT}/src/lat/kaldi-lat.a"
      "${KALDI_ROOT}/src/lm/kaldi-lm.a"
      "${KALDI_ROOT}/src/rnnlm/kaldi-rnnlm.a"
      "${KALDI_ROOT}/src/hmm/kaldi-hmm.a"
      "${KALDI_ROOT}/src/nnet3/kaldi-nnet3.a"
      "${KALDI_ROOT}/src/transform/kaldi-transform.a"
      "${KALDI_ROOT}/src/cudamatrix/kaldi-cudamatrix.a"
      "${KALDI_ROOT}/src/matrix/kaldi-matrix.a"
      "${KALDI_ROOT}/src/fstext/kaldi-fstext.a"
      "${KALDI_ROOT}/src/util/kaldi-util.a"
      "${KALDI_ROOT}/src/base/kaldi-base.a"
      "${OPENFST_ROOT}/lib/libfst.a"
      "${OPENFST_ROOT}/lib/libfstngram.a"
      "${OPENBLAS_ROOT}/lib/libopenblas.a"
      "${OPENBLAS_ROOT}/lib/liblapack.a"
      "${OPENBLAS_ROOT}/lib/libblas.a"
      "${OPENBLAS_ROOT}/lib/libf2c.a"
    )
  endif()

  target_link_libraries(voicectl PRIVATE vosk)
  target_include_directories(voicectl PRIVATE "${VOSK_API_SRC_DIR}")
else()
  # Use prebuilt/system Vosk library (libvosk.so / libvosk.a).
  set(VOSK_LIBRARY "" CACHE FILEPATH "Path to Vosk library (libvosk.so or libvosk.a)")
  if(VOSK_LIBRARY STREQUAL "")
    if(EXISTS "${VOSK_BUNDLED_LIBVOSK_SO}")
      set(VOSK_LIBRARY "${VOSK_BUNDLED_LIBVOSK_SO}" CACHE FILEPATH "Path to Vosk library (libvosk.so or libvosk.a)" FORCE)
    elseif(EXISTS "${VOSK_BUNDLED_LIBVOSK_A}")
      set(VOSK_LIBRARY "${VOSK_BUNDLED_LIBVOSK_A}" CACHE FILEPATH "Path to Vosk library (libvosk.so or libvosk.a)" FORCE)
    elseif(EXISTS "${VOSK_BUNDLED_LIBVOSK_SO_IN_API}")
      set(VOSK_LIBRARY "${VOSK_BUNDLED_LIBVOSK_SO_IN_API}" CACHE FILEPATH "Path to Vosk library (libvosk.so or libvosk.a)" FORCE)
	    elseif(EXISTS "${VOSK_BUNDLED_LIBVOSK_A_IN_API}")
	      set(VOSK_LIBRARY "${VOSK_BUNDLED_LIBVOSK_A_IN_API}" CACHE FILEPATH "Path to Vosk library (libvosk.so or libvosk.a)" FORCE)
	    else()
	      find_library(VOSK_LIBRARY NAMES vosk)
	    endif()
	  endif()
  if(NOT VOSK_LIBRARY)
    message(FATAL_ERROR "Vosk library not found. Set -DVOSK_LIBRARY=/path/to/libvosk.so (or set -DVOSK_BUILD=ON and provide -DKALDI_ROOT=...).")
  endif()
  target_link_libraries(voicectl PRIVATE "${VOSK_LIBRARY}")
  target_include_directories(voicectl PRIVATE "${VOSK_API_SRC_DIR}")

  get_filename_component(VOSK_LIBRARY_DIR "${VOSK_LIBRARY}" DIRECTORY)
  if(VOSK_LIBRARY_DIR)
    set_target_properties(voicectl PROPERTIES BUILD_RPATH "${VOSK_LIBRARY_DIR}")
  endif()
endif()

# PortAudio
find_library(PORTAUDIO_LIBRARY NAMES portaudio portaudio-2.0)
if(NOT PORTAUDIO_LIBRARY)
  message(FATAL_ERROR "PortAudio library not found. Install PortAudio dev package or provide it on the linker path.")
endif()
target_link_libraries(voicectl PRIVATE "${PORTAUDIO_LIBRARY}")
