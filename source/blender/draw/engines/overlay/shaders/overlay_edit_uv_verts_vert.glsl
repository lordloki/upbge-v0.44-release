/* SPDX-FileCopyrightText: 2020-2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "draw_model_lib.glsl"
#include "draw_view_lib.glsl"

void main()
{
  /* TODO: Theme? */
  const vec4 pinned_col = vec4(1.0, 0.0, 0.0, 1.0);

  bool is_selected = (flag & (VERT_UV_SELECT | FACE_UV_SELECT)) != 0u;
  bool is_pinned = (flag & VERT_UV_PINNED) != 0u;
  vec4 deselect_col = (is_pinned) ? pinned_col : vec4(color.rgb, 1.0);
  fillColor = (is_selected) ? colorVertexSelect : deselect_col;
  outlineColor = (is_pinned) ? pinned_col : vec4(fillColor.rgb, 0.0);

  vec3 world_pos = vec3(au, 0.0);
  /* Move selected vertices to the top
   * Vertices are between 0.0 and 0.2, Edges between 0.2 and 0.4
   * actual pixels are at 0.75, 1.0 is used for the background. */
  float depth = is_selected ? (is_pinned ? 0.05 : 0.10) : 0.15;
  gl_Position = vec4(drw_point_world_to_homogenous(world_pos).xy, depth, 1.0);
  gl_PointSize = pointSize;

  /* calculate concentric radii in pixels */
  float radius = 0.5 * pointSize;

  /* start at the outside and progress toward the center */
  radii[0] = radius;
  radii[1] = radius - 1.0;
  radii[2] = radius - outlineWidth;
  radii[3] = radius - outlineWidth - 1.0;

  /* convert to PointCoord units */
  radii /= pointSize;
}
