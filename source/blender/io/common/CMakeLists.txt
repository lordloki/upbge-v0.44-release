# SPDX-FileCopyrightText: 2020 Blender Authors
#
# SPDX-License-Identifier: GPL-2.0-or-later

set(INC
  .
  ../../makesrna
)

set(INC_SYS
)

set(SRC
  intern/abstract_hierarchy_iterator.cc
  intern/dupli_parent_finder.cc
  intern/dupli_persistent_id.cc
  intern/object_identifier.cc
  intern/orientation.cc
  intern/path_util.cc
  intern/subdiv_disabler.cc

  IO_abstract_hierarchy_iterator.h
  IO_dupli_persistent_id.hh
  IO_orientation.hh
  IO_path_util.hh
  IO_path_util_types.hh
  IO_subdiv_disabler.hh
  IO_types.hh
  intern/dupli_parent_finder.hh
)

set(LIB
  PRIVATE bf::blenkernel
  PRIVATE bf::blenlib
  PRIVATE bf::depsgraph
  PRIVATE bf::dna
  PRIVATE bf::intern::clog
  PRIVATE bf::intern::guardedalloc
  PRIVATE bf::extern::fmtlib
)

blender_add_lib(bf_io_common "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")

target_link_libraries(bf_io_common INTERFACE)

if(WITH_GTESTS)
  set(TEST_SRC
    intern/abstract_hierarchy_iterator_test.cc
    intern/hierarchy_context_order_test.cc
    intern/object_identifier_test.cc
  )
  set(TEST_INC
    ../../blenloader
  )
  set(TEST_LIB
    bf_blenloader_test_util
    bf_io_common
  )
  blender_add_test_suite_lib(io_common "${TEST_SRC}" "${INC};${TEST_INC}" "${INC_SYS}" "${LIB};${TEST_LIB}")
endif()
