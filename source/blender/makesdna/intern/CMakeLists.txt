# SPDX-FileCopyrightText: 2006 Blender Authors
#
# SPDX-License-Identifier: GPL-2.0-or-later

set(INC
  PUBLIC ..
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  ../../blenlib
  ../../imbuf
  ../../imbuf/movie
)

set(INC_SYS

)

set(LIB
  PRIVATE bf::intern::atomic
  PRIVATE bf::intern::guardedalloc
  PRIVATE bf::dependencies::optional::tbb
  PRIVATE bf::extern::fmtlib
)

add_definitions(-DWITH_DNA_GHASH)

# Needed for `mallocn.c`.
if(HAVE_MALLOC_STATS_H)
  add_definitions(-DHAVE_MALLOC_STATS_H)
endif()

set(dna_header_include_file "${CMAKE_CURRENT_BINARY_DIR}/dna_includes_all.h")
set(dna_header_string_file "${CMAKE_CURRENT_BINARY_DIR}/dna_includes_as_strings.h")

set(DNA_INCLUDE_TEXT "/* Do not edit manually, changes will be overwritten. */\n")
set(DNA_FILE_LIST "/* Do not edit manually, changes will be overwritten. */\n")
foreach(header ${SRC_DNA_INC})
  get_filename_component(dna_header_file ${header} NAME)
  string(APPEND DNA_INCLUDE_TEXT "#include \"${header}\"\n")
  string(APPEND DNA_FILE_LIST "\t\"${dna_header_file}\",\n")
endforeach()

file(GENERATE OUTPUT ${dna_header_include_file} CONTENT "${DNA_INCLUDE_TEXT}")
file(GENERATE OUTPUT ${dna_header_string_file} CONTENT "${DNA_FILE_LIST}")

# -----------------------------------------------------------------------------
# Build `makesdna` executable.

# Subset of `blenlib` used by `makesdna`.
set(BLENLIB_SRC
  ../../blenlib/intern/BLI_assert.c
  ../../blenlib/intern/BLI_ghash.c
  ../../blenlib/intern/BLI_ghash_utils.cc
  ../../blenlib/intern/BLI_linklist.c
  ../../blenlib/intern/BLI_memarena.c
  ../../blenlib/intern/BLI_mempool.c
  ../../blenlib/intern/endian_switch.c
  ../../blenlib/intern/listbase.cc
  ../../blenlib/intern/string.c

  # Dependency of BLI_ghash.c
  ../../blenlib/intern/hash_mm2a.cc

  # Dependencies of BLI_mempool.c when ASAN is enabled.
  ../../blenlib/intern/gsqueue.c
  ../../blenlib/intern/threads.cc
  ../../blenlib/intern/time.c
)

set(SRC
  dna_utils.cc
  makesdna.cc
  ${BLENLIB_SRC}
  ${dna_header_include_file}
  ${dna_header_string_file}
)

# BLI_mempool.c needs these files for ThreadMutex when
# compiling in address sanitizer mode.
if(WITH_COMPILER_ASAN)
  list(APPEND SRC
  )
endif()

# `SRC_DNA_INC` is defined in the parent directory.

add_cc_flags_custom_test(makesdna)

add_executable(makesdna ${SRC} ${SRC_DNA_INC})
setup_platform_linker_flags(makesdna)
setup_platform_linker_libs(makesdna)
blender_target_include_dirs(makesdna ${INC})
blender_target_include_dirs_sys(makesdna ${INC_SYS})
target_link_libraries(makesdna ${LIB})

if(WIN32 AND NOT UNIX)
  if(DEFINED PTHREADS_LIBRARIES)
    target_link_libraries(makesdna PRIVATE ${PTHREADS_LIBRARIES})
  endif()
endif()

# Output dna.c
add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/dna.c
    ${CMAKE_CURRENT_BINARY_DIR}/dna_type_offsets.h
    ${CMAKE_CURRENT_BINARY_DIR}/dna_verify.c
  COMMAND
    ${CMAKE_COMMAND} -E env ${PLATFORM_ENV_BUILD}
    "$<TARGET_FILE:makesdna>"
    ${CMAKE_CURRENT_BINARY_DIR}/dna.c
    ${CMAKE_CURRENT_BINARY_DIR}/dna_type_offsets.h
    ${CMAKE_CURRENT_BINARY_DIR}/dna_verify.c
    ${CMAKE_SOURCE_DIR}/source/blender/makesdna/
  DEPENDS makesdna
)


# -----------------------------------------------------------------------------
# Build bf_dna library
set(SRC
  dna_defaults.c
  dna_genfile.cc
  dna_utils.cc
  ${CMAKE_CURRENT_BINARY_DIR}/dna.c
  ${CMAKE_CURRENT_BINARY_DIR}/dna_verify.c
  ${SRC_DNA_INC}

  ${CMAKE_CURRENT_BINARY_DIR}/dna_type_offsets.h
  dna_rename_defs.h
  dna_utils.h
)

set_source_files_properties(
  ${CMAKE_CURRENT_BINARY_DIR}/dna.c
  ${CMAKE_CURRENT_BINARY_DIR}/dna_type_offsets.h
  ${CMAKE_CURRENT_BINARY_DIR}/dna_verify.c
  PROPERTIES GENERATED TRUE
)

blender_add_lib(bf_dna "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(bf::dna ALIAS bf_dna)



# -----------------------------------------------------------------------------
# Build bf_dna_blenlib library
set(INC
  ..
  ../../blenlib
)

set(INC_SYS

)

set(SRC
  ${BLENLIB_SRC}
  ${SRC_DNA_DEFAULTS_INC}
  ${SRC_DNA_OTHER_INC}
)

set(LIB
  PRIVATE bf::intern::atomic
  PRIVATE bf::intern::guardedalloc
  PRIVATE bf::dependencies::optional::tbb
)

blender_add_lib(bf_dna_blenlib "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
