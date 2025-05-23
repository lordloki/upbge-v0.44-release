# SPDX-FileCopyrightText: 2023 Blender Authors
#
# SPDX-License-Identifier: GPL-2.0-or-later

set(INC
  ../asset
  ../include
  ../../makesrna
  ../../modifiers
  ../../../../intern/mantaflow/extern

  # RNA_prototypes.hh
  ${CMAKE_BINARY_DIR}/source/blender/makesrna
)

set(INC_SYS
)

set(SRC
  drawobject.cc
  space_view3d.cc
  view3d_buttons.cc
  view3d_camera_control.cc
  view3d_context.cc
  view3d_cursor_snap.cc
  view3d_draw.cc
  view3d_edit.cc
  view3d_gizmo_armature.cc
  view3d_gizmo_camera.cc
  view3d_gizmo_empty.cc
  view3d_gizmo_forcefield.cc
  view3d_gizmo_geometry_nodes.cc
  view3d_gizmo_light.cc
  view3d_gizmo_navigate.cc
  view3d_gizmo_navigate_type.cc
  view3d_gizmo_preselect.cc
  view3d_gizmo_preselect_type.cc
  view3d_gizmo_ruler.cc
  view3d_gizmo_tool_generic.cc
  view3d_header.cc
  view3d_iterators.cc
  view3d_navigate.cc
  view3d_navigate_fly.cc
  view3d_navigate_smoothview.cc
  view3d_navigate_view_all.cc
  view3d_navigate_view_axis.cc
  view3d_navigate_view_camera.cc
  view3d_navigate_view_center_cursor.cc
  view3d_navigate_view_center_pick.cc
  view3d_navigate_view_dolly.cc
  view3d_navigate_view_move.cc
  view3d_navigate_view_ndof.cc
  view3d_navigate_view_orbit.cc
  view3d_navigate_view_pan.cc
  view3d_navigate_view_roll.cc
  view3d_navigate_view_rotate.cc
  view3d_navigate_view_zoom.cc
  view3d_navigate_walk.cc
  view3d_navigate_zoom_border.cc
  view3d_ops.cc
  view3d_placement.cc
  view3d_project.cc
  view3d_select.cc
  view3d_snap.cc
  view3d_utils.cc
  view3d_view.cc

  view3d_intern.hh
  view3d_navigate.hh
)

set(LIB
  PRIVATE bf::animrig
  PRIVATE bf::asset_system
  PRIVATE bf::blenfont
  PRIVATE bf::blenkernel
  PRIVATE bf::blenlib
  PRIVATE bf::blenloader
  PRIVATE bf::blentranslation
  PRIVATE bf::bmesh
  PRIVATE bf::depsgraph
  PRIVATE bf::dna
  PRIVATE bf::draw
  bf_editor_curves
  bf_editor_lattice
  bf_editor_mesh
  PRIVATE bf::extern::fmtlib
  PRIVATE bf::functions
  PRIVATE bf::gpu
  PRIVATE bf::imbuf
  PRIVATE bf::intern::guardedalloc
  PRIVATE bf::nodes
  PRIVATE bf::render
  PRIVATE bf::windowmanager
)

if(WITH_PYTHON)
  list(APPEND INC ../../python)
  add_definitions(-DWITH_PYTHON)
endif()

if(WITH_GAMEENGINE)
  list(APPEND LIB
    ge_launcher
    ge_blender_routines
  )

  list(APPEND INC
    ../../../gameengine/Launcher
    ../../../gameengine/BlenderRoutines
  )

  add_definitions(-DWITH_GAMEENGINE)
endif()

if(WITH_FREESTYLE)
  add_definitions(-DWITH_FREESTYLE)
endif()

if(WITH_XR_OPENXR)
  add_definitions(-DWITH_XR_OPENXR)
endif()

blender_add_lib(bf_editor_space_view3d "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")

# RNA_prototypes.hh
add_dependencies(bf_editor_space_view3d bf_rna)
