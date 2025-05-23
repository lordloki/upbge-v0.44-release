/* SPDX-FileCopyrightText: 2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup RNA
 * \brief RNA property definitions for Rigid Body data-types
 */

#include <cstdlib>
#include <cstring>

#include "BLI_math_base.h"

#include "RNA_define.hh"
#include "RNA_enum_types.hh"

#include "rna_internal.hh"

#include "DNA_rigidbody_types.h"

#include "WM_types.hh"

/* roles of objects in RigidBody Sims */
const EnumPropertyItem rna_enum_rigidbody_object_type_items[] = {
    {RBO_TYPE_ACTIVE,
     "ACTIVE",
     0,
     "Active",
     "Object is directly controlled by simulation results"},
    {RBO_TYPE_PASSIVE,
     "PASSIVE",
     0,
     "Passive",
     "Object is directly controlled by animation system"},
    {0, nullptr, 0, nullptr, nullptr},
};

/* collision shapes of objects in rigid body sim */
const EnumPropertyItem rna_enum_rigidbody_object_shape_items[] = {
    {RB_SHAPE_BOX,
     "BOX",
     ICON_MESH_CUBE,
     "Box",
     "Box-like shapes (i.e. cubes), including planes (i.e. ground planes)"},
    {RB_SHAPE_SPHERE, "SPHERE", ICON_MESH_UVSPHERE, "Sphere", ""},
    {RB_SHAPE_CAPSULE, "CAPSULE", ICON_MESH_CAPSULE, "Capsule", ""},
    {RB_SHAPE_CYLINDER, "CYLINDER", ICON_MESH_CYLINDER, "Cylinder", ""},
    {RB_SHAPE_CONE, "CONE", ICON_MESH_CONE, "Cone", ""},
    {RB_SHAPE_CONVEXH,
     "CONVEX_HULL",
     ICON_MESH_ICOSPHERE,
     "Convex Hull",
     "A mesh-like surface encompassing (i.e. shrinkwrap over) all vertices (best results with "
     "fewer vertices)"},
    {RB_SHAPE_TRIMESH,
     "MESH",
     ICON_MESH_MONKEY,
     "Mesh",
     "Mesh consisting of triangles only, allowing for more detailed interactions than convex "
     "hulls"},
    {RB_SHAPE_COMPOUND,
     "COMPOUND",
     ICON_MESH_DATA,
     "Compound Parent",
     "Combines all of its direct rigid body children into one rigid object"},
    {0, nullptr, 0, nullptr, nullptr},
};

/* collision shapes of constraints in rigid body sim */
const EnumPropertyItem rna_enum_rigidbody_constraint_type_items[] = {
    {RBC_TYPE_FIXED, "FIXED", ICON_NONE, "Fixed", "Glue rigid bodies together"},
    {RBC_TYPE_POINT,
     "POINT",
     ICON_NONE,
     "Point",
     "Constrain rigid bodies to move around common pivot point"},
    {RBC_TYPE_HINGE, "HINGE", ICON_NONE, "Hinge", "Restrict rigid body rotation to one axis"},
    {RBC_TYPE_SLIDER,
     "SLIDER",
     ICON_NONE,
     "Slider",
     "Restrict rigid body translation to one axis"},
    {RBC_TYPE_PISTON,
     "PISTON",
     ICON_NONE,
     "Piston",
     "Restrict rigid body translation and rotation to one axis"},
    {RBC_TYPE_6DOF,
     "GENERIC",
     ICON_NONE,
     "Generic",
     "Restrict translation and rotation to specified axes"},
    {RBC_TYPE_6DOF_SPRING,
     "GENERIC_SPRING",
     ICON_NONE,
     "Generic Spring",
     "Restrict translation and rotation to specified axes with springs"},
    {RBC_TYPE_MOTOR, "MOTOR", ICON_NONE, "Motor", "Drive rigid body around or along an axis"},
    {0, nullptr, 0, nullptr, nullptr},
};

/* bullet spring type */
static const EnumPropertyItem rna_enum_rigidbody_constraint_spring_type_items[] = {
    {RBC_SPRING_TYPE1,
     "SPRING1",
     ICON_NONE,
     "Blender 2.7",
     "Spring implementation used in Blender 2.7. Damping is capped at 1.0"},
    {RBC_SPRING_TYPE2,
     "SPRING2",
     ICON_NONE,
     "Blender 2.8",
     "New implementation available since 2.8"},
    {0, nullptr, 0, nullptr, nullptr},
};

#ifndef RNA_RUNTIME
/* mesh source for collision shape creation */
static const EnumPropertyItem rigidbody_mesh_source_items[] = {
    {RBO_MESH_BASE, "BASE", 0, "Base", "Base mesh"},
    {RBO_MESH_DEFORM, "DEFORM", 0, "Deform", "Deformations (shape keys, deform modifiers)"},
    {RBO_MESH_FINAL, "FINAL", 0, "Final", "All modifiers"},
    {0, nullptr, 0, nullptr, nullptr},
};
#endif

#ifdef RNA_RUNTIME

#  ifdef WITH_BULLET
#    include "RBI_api.h"
#  endif

#  include "BKE_rigidbody.h"

#  include "DEG_depsgraph.hh"
#  include "DEG_depsgraph_build.hh"

#  include "WM_api.hh"

/* ******************************** */

static void rna_RigidBodyWorld_reset(Main * /*bmain*/, Scene * /*scene*/, PointerRNA *ptr)
{
  RigidBodyWorld *rbw = (RigidBodyWorld *)ptr->data;

  BKE_rigidbody_cache_reset(rbw);
}

static std::optional<std::string> rna_RigidBodyWorld_path(const PointerRNA * /*ptr*/)
{
  return "rigidbody_world";
}

static void rna_RigidBodyWorld_num_solver_iterations_set(PointerRNA *ptr, int value)
{
  RigidBodyWorld *rbw = (RigidBodyWorld *)ptr->data;

  rbw->num_solver_iterations = value;

#  ifdef WITH_BULLET
  rbDynamicsWorld *physics_world = BKE_rigidbody_world_physics(rbw);
  if (physics_world) {
    RB_dworld_set_solver_iterations(physics_world, value);
  }
#  endif
}

static void rna_RigidBodyWorld_split_impulse_set(PointerRNA *ptr, bool value)
{
  RigidBodyWorld *rbw = (RigidBodyWorld *)ptr->data;

  SET_FLAG_FROM_TEST(rbw->flag, value, RBW_FLAG_USE_SPLIT_IMPULSE);

#  ifdef WITH_BULLET
  rbDynamicsWorld *physics_world = BKE_rigidbody_world_physics(rbw);
  if (physics_world) {
    RB_dworld_set_split_impulse(physics_world, value);
  }
#  endif
}

static void rna_RigidBodyWorld_objects_collection_update(Main *bmain,
                                                         Scene *scene,
                                                         PointerRNA *ptr)
{
  RigidBodyWorld *rbw = (RigidBodyWorld *)ptr->data;
  BKE_rigidbody_objects_collection_validate(bmain, scene, rbw);
  rna_RigidBodyWorld_reset(bmain, scene, ptr);
}

static void rna_RigidBodyWorld_constraints_collection_update(Main *bmain,
                                                             Scene *scene,
                                                             PointerRNA *ptr)
{
  RigidBodyWorld *rbw = (RigidBodyWorld *)ptr->data;
  BKE_rigidbody_constraints_collection_validate(scene, rbw);
  rna_RigidBodyWorld_reset(bmain, scene, ptr);
}

/* ******************************** */

static void rna_RigidBodyOb_reset(Main * /*bmain*/, Scene *scene, PointerRNA * /*ptr*/)
{
  if (scene != nullptr) {
    RigidBodyWorld *rbw = scene->rigidbody_world;
    BKE_rigidbody_cache_reset(rbw);
  }
}

static void rna_RigidBodyOb_shape_update(Main *bmain, Scene *scene, PointerRNA *ptr)
{
  Object *ob = (Object *)ptr->owner_id;

  rna_RigidBodyOb_reset(bmain, scene, ptr);
  DEG_relations_tag_update(bmain);

  WM_main_add_notifier(NC_OBJECT | ND_DRAW, ob);
}

static void rna_RigidBodyOb_shape_reset(Main * /*bmain*/, Scene *scene, PointerRNA *ptr)
{
  if (scene != nullptr) {
    RigidBodyWorld *rbw = scene->rigidbody_world;
    BKE_rigidbody_cache_reset(rbw);
  }

  RigidBodyOb *rbo = (RigidBodyOb *)ptr->data;
  if (rbo->shared->physics_shape) {
    rbo->flag |= RBO_FLAG_NEEDS_RESHAPE;
  }
}

static void rna_RigidBodyOb_mesh_source_update(Main *bmain, Scene *scene, PointerRNA *ptr)
{
  Object *ob = (Object *)ptr->owner_id;

  rna_RigidBodyOb_reset(bmain, scene, ptr);
  DEG_relations_tag_update(bmain);

  WM_main_add_notifier(NC_OBJECT | ND_DRAW, ob);
}

static std::optional<std::string> rna_RigidBodyOb_path(const PointerRNA * /*ptr*/)
{
  /* NOTE: this hardcoded path should work as long as only Objects have this */
  return "rigid_body";
}

static void rna_RigidBodyOb_type_set(PointerRNA *ptr, int value)
{
  RigidBodyOb *rbo = (RigidBodyOb *)ptr->data;

  rbo->type = value;
  rbo->flag |= RBO_FLAG_NEEDS_VALIDATE;
}

static void rna_RigidBodyOb_shape_set(PointerRNA *ptr, int value)
{
  RigidBodyOb *rbo = (RigidBodyOb *)ptr->data;

  rbo->shape = value;
  rbo->flag |= RBO_FLAG_NEEDS_VALIDATE;
}

static void rna_RigidBodyOb_disabled_set(PointerRNA *ptr, bool value)
{
  RigidBodyOb *rbo = (RigidBodyOb *)ptr->data;

  SET_FLAG_FROM_TEST(rbo->flag, !value, RBO_FLAG_DISABLED);

#  ifdef WITH_BULLET
  /* update kinematic state if necessary - only needed for active bodies */
  if ((rbo->shared->physics_object) && (rbo->type == RBO_TYPE_ACTIVE)) {
    RB_body_set_mass(static_cast<rbRigidBody *>(rbo->shared->physics_object), RBO_GET_MASS(rbo));
    RB_body_set_kinematic_state(static_cast<rbRigidBody *>(rbo->shared->physics_object), !value);
    rbo->flag |= RBO_FLAG_NEEDS_VALIDATE;
  }
#  endif
}

static void rna_RigidBodyOb_mass_set(PointerRNA *ptr, float value)
{
  RigidBodyOb *rbo = (RigidBodyOb *)ptr->data;

  rbo->mass = value;

#  ifdef WITH_BULLET
  /* only active bodies need mass update */
  if ((rbo->shared->physics_object) && (rbo->type == RBO_TYPE_ACTIVE)) {
    RB_body_set_mass(static_cast<rbRigidBody *>(rbo->shared->physics_object), RBO_GET_MASS(rbo));
  }
#  endif
}

static void rna_RigidBodyOb_friction_set(PointerRNA *ptr, float value)
{
  RigidBodyOb *rbo = (RigidBodyOb *)ptr->data;

  rbo->friction = value;

#  ifdef WITH_BULLET
  if (rbo->shared->physics_object) {
    RB_body_set_friction(static_cast<rbRigidBody *>(rbo->shared->physics_object), value);
  }
#  endif
}

static void rna_RigidBodyOb_restitution_set(PointerRNA *ptr, float value)
{
  RigidBodyOb *rbo = (RigidBodyOb *)ptr->data;

  rbo->restitution = value;
#  ifdef WITH_BULLET
  if (rbo->shared->physics_object) {
    RB_body_set_restitution(static_cast<rbRigidBody *>(rbo->shared->physics_object), value);
  }
#  endif
}

static void rna_RigidBodyOb_collision_margin_set(PointerRNA *ptr, float value)
{
  RigidBodyOb *rbo = (RigidBodyOb *)ptr->data;

  rbo->margin = value;

#  ifdef WITH_BULLET
  if (rbo->shared->physics_shape) {
    RB_shape_set_margin(static_cast<rbCollisionShape *>(rbo->shared->physics_shape),
                        RBO_GET_MARGIN(rbo));
  }
#  endif
}

static void rna_RigidBodyOb_collision_collections_set(PointerRNA *ptr, const bool *values)
{
  RigidBodyOb *rbo = (RigidBodyOb *)ptr->data;
  int i;

  for (i = 0; i < 20; i++) {
    if (values[i]) {
      rbo->col_groups |= (1 << i);
    }
    else {
      rbo->col_groups &= ~(1 << i);
    }
  }
  rbo->flag |= RBO_FLAG_NEEDS_VALIDATE;
}

static void rna_RigidBodyOb_kinematic_state_set(PointerRNA *ptr, bool value)
{
  RigidBodyOb *rbo = (RigidBodyOb *)ptr->data;

  SET_FLAG_FROM_TEST(rbo->flag, value, RBO_FLAG_KINEMATIC);

#  ifdef WITH_BULLET
  /* update kinematic state if necessary */
  if (rbo->shared->physics_object) {
    RB_body_set_mass(static_cast<rbRigidBody *>(rbo->shared->physics_object), RBO_GET_MASS(rbo));
    RB_body_set_kinematic_state(static_cast<rbRigidBody *>(rbo->shared->physics_object), value);
    rbo->flag |= RBO_FLAG_NEEDS_VALIDATE;
  }
#  endif
}

static void rna_RigidBodyOb_activation_state_set(PointerRNA *ptr, bool value)
{
  RigidBodyOb *rbo = (RigidBodyOb *)ptr->data;

  SET_FLAG_FROM_TEST(rbo->flag, value, RBO_FLAG_USE_DEACTIVATION);

#  ifdef WITH_BULLET
  /* update activation state if necessary - only active bodies can be deactivated */
  if ((rbo->shared->physics_object) && (rbo->type == RBO_TYPE_ACTIVE)) {
    RB_body_set_activation_state(static_cast<rbRigidBody *>(rbo->shared->physics_object), value);
  }
#  endif
}

static void rna_RigidBodyOb_linear_sleepThresh_set(PointerRNA *ptr, float value)
{
  RigidBodyOb *rbo = (RigidBodyOb *)ptr->data;

  rbo->lin_sleep_thresh = value;

#  ifdef WITH_BULLET
  /* only active bodies need sleep threshold update */
  if ((rbo->shared->physics_object) && (rbo->type == RBO_TYPE_ACTIVE)) {
    RB_body_set_linear_sleep_thresh(static_cast<rbRigidBody *>(rbo->shared->physics_object),
                                    value);
  }
#  endif
}

static void rna_RigidBodyOb_angular_sleepThresh_set(PointerRNA *ptr, float value)
{
  RigidBodyOb *rbo = (RigidBodyOb *)ptr->data;

  rbo->ang_sleep_thresh = value;

#  ifdef WITH_BULLET
  /* only active bodies need sleep threshold update */
  if ((rbo->shared->physics_object) && (rbo->type == RBO_TYPE_ACTIVE)) {
    RB_body_set_angular_sleep_thresh(static_cast<rbRigidBody *>(rbo->shared->physics_object),
                                     value);
  }
#  endif
}

static void rna_RigidBodyOb_linear_damping_set(PointerRNA *ptr, float value)
{
  RigidBodyOb *rbo = (RigidBodyOb *)ptr->data;

  rbo->lin_damping = value;

#  ifdef WITH_BULLET
  /* only active bodies need damping update */
  if ((rbo->shared->physics_object) && (rbo->type == RBO_TYPE_ACTIVE)) {
    RB_body_set_linear_damping(static_cast<rbRigidBody *>(rbo->shared->physics_object), value);
  }
#  endif
}

static void rna_RigidBodyOb_angular_damping_set(PointerRNA *ptr, float value)
{
  RigidBodyOb *rbo = (RigidBodyOb *)ptr->data;

  rbo->ang_damping = value;

#  ifdef WITH_BULLET
  /* only active bodies need damping update */
  if ((rbo->shared->physics_object) && (rbo->type == RBO_TYPE_ACTIVE)) {
    RB_body_set_angular_damping(static_cast<rbRigidBody *>(rbo->shared->physics_object), value);
  }
#  endif
}

static std::optional<std::string> rna_RigidBodyCon_path(const PointerRNA * /*ptr*/)
{
  /* NOTE: this hardcoded path should work as long as only Objects have this */
  return "rigid_body_constraint";
}

static void rna_RigidBodyCon_type_set(PointerRNA *ptr, int value)
{
  RigidBodyCon *rbc = (RigidBodyCon *)ptr->data;

  rbc->type = value;
  rbc->flag |= RBC_FLAG_NEEDS_VALIDATE;
}

static void rna_RigidBodyCon_spring_type_set(PointerRNA *ptr, int value)
{
  RigidBodyCon *rbc = (RigidBodyCon *)ptr->data;

  rbc->spring_type = value;
  rbc->flag |= RBC_FLAG_NEEDS_VALIDATE;
}

static void rna_RigidBodyCon_enabled_set(PointerRNA *ptr, bool value)
{
  RigidBodyCon *rbc = (RigidBodyCon *)ptr->data;

  SET_FLAG_FROM_TEST(rbc->flag, value, RBC_FLAG_ENABLED);

#  ifdef WITH_BULLET
  if (rbc->physics_constraint) {
    RB_constraint_set_enabled(static_cast<rbConstraint *>(rbc->physics_constraint), value);
  }
#  endif
}

static void rna_RigidBodyCon_disable_collisions_set(PointerRNA *ptr, bool value)
{
  RigidBodyCon *rbc = (RigidBodyCon *)ptr->data;

  SET_FLAG_FROM_TEST(rbc->flag, value, RBC_FLAG_DISABLE_COLLISIONS);

  rbc->flag |= RBC_FLAG_NEEDS_VALIDATE;
}

static void rna_RigidBodyCon_use_breaking_set(PointerRNA *ptr, bool value)
{
  RigidBodyCon *rbc = (RigidBodyCon *)ptr->data;

  if (value) {
    rbc->flag |= RBC_FLAG_USE_BREAKING;
#  ifdef WITH_BULLET
    if (rbc->physics_constraint) {
      RB_constraint_set_breaking_threshold(static_cast<rbConstraint *>(rbc->physics_constraint),
                                           rbc->breaking_threshold);
    }
#  endif
  }
  else {
    rbc->flag &= ~RBC_FLAG_USE_BREAKING;
#  ifdef WITH_BULLET
    if (rbc->physics_constraint) {
      RB_constraint_set_breaking_threshold(static_cast<rbConstraint *>(rbc->physics_constraint),
                                           FLT_MAX);
    }
#  endif
  }
}

static void rna_RigidBodyCon_breaking_threshold_set(PointerRNA *ptr, float value)
{
  RigidBodyCon *rbc = (RigidBodyCon *)ptr->data;

  rbc->breaking_threshold = value;

#  ifdef WITH_BULLET
  if (rbc->physics_constraint && (rbc->flag & RBC_FLAG_USE_BREAKING)) {
    RB_constraint_set_breaking_threshold(static_cast<rbConstraint *>(rbc->physics_constraint),
                                         value);
  }
#  endif
}

static void rna_RigidBodyCon_override_solver_iterations_set(PointerRNA *ptr, bool value)
{
  RigidBodyCon *rbc = (RigidBodyCon *)ptr->data;

  if (value) {
    rbc->flag |= RBC_FLAG_OVERRIDE_SOLVER_ITERATIONS;
#  ifdef WITH_BULLET
    if (rbc->physics_constraint) {
      RB_constraint_set_solver_iterations(static_cast<rbConstraint *>(rbc->physics_constraint),
                                          rbc->num_solver_iterations);
    }
#  endif
  }
  else {
    rbc->flag &= ~RBC_FLAG_OVERRIDE_SOLVER_ITERATIONS;
#  ifdef WITH_BULLET
    if (rbc->physics_constraint) {
      RB_constraint_set_solver_iterations(static_cast<rbConstraint *>(rbc->physics_constraint),
                                          -1);
    }
#  endif
  }
}

static void rna_RigidBodyCon_num_solver_iterations_set(PointerRNA *ptr, int value)
{
  RigidBodyCon *rbc = (RigidBodyCon *)ptr->data;

  rbc->num_solver_iterations = value;

#  ifdef WITH_BULLET
  if (rbc->physics_constraint && (rbc->flag & RBC_FLAG_OVERRIDE_SOLVER_ITERATIONS)) {
    RB_constraint_set_solver_iterations(static_cast<rbConstraint *>(rbc->physics_constraint),
                                        value);
  }
#  endif
}

#  ifdef WITH_BULLET
static void rna_RigidBodyCon_do_set_spring_stiffness(RigidBodyCon *rbc,
                                                     float value,
                                                     int flag,
                                                     int axis)
{
  if (rbc->physics_constraint && rbc->type == RBC_TYPE_6DOF_SPRING && (rbc->flag & flag)) {
    switch (rbc->spring_type) {
      case RBC_SPRING_TYPE1:
        RB_constraint_set_stiffness_6dof_spring(
            static_cast<rbConstraint *>(rbc->physics_constraint), axis, value);
        break;
      case RBC_SPRING_TYPE2:
        RB_constraint_set_stiffness_6dof_spring2(
            static_cast<rbConstraint *>(rbc->physics_constraint), axis, value);
        break;
    }
  }
}
#  endif

static void rna_RigidBodyCon_spring_stiffness_x_set(PointerRNA *ptr, float value)
{
  RigidBodyCon *rbc = (RigidBodyCon *)ptr->data;

  rbc->spring_stiffness_x = value;

#  ifdef WITH_BULLET
  rna_RigidBodyCon_do_set_spring_stiffness(rbc, value, RBC_FLAG_USE_SPRING_X, RB_LIMIT_LIN_X);
#  endif
}

static void rna_RigidBodyCon_spring_stiffness_y_set(PointerRNA *ptr, float value)
{
  RigidBodyCon *rbc = (RigidBodyCon *)ptr->data;

  rbc->spring_stiffness_y = value;

#  ifdef WITH_BULLET
  rna_RigidBodyCon_do_set_spring_stiffness(rbc, value, RBC_FLAG_USE_SPRING_Y, RB_LIMIT_LIN_Y);
#  endif
}

static void rna_RigidBodyCon_spring_stiffness_z_set(PointerRNA *ptr, float value)
{
  RigidBodyCon *rbc = (RigidBodyCon *)ptr->data;

  rbc->spring_stiffness_z = value;

#  ifdef WITH_BULLET
  rna_RigidBodyCon_do_set_spring_stiffness(rbc, value, RBC_FLAG_USE_SPRING_Z, RB_LIMIT_LIN_Z);
#  endif
}

static void rna_RigidBodyCon_spring_stiffness_ang_x_set(PointerRNA *ptr, float value)
{
  RigidBodyCon *rbc = (RigidBodyCon *)ptr->data;

  rbc->spring_stiffness_ang_x = value;

#  ifdef WITH_BULLET
  rna_RigidBodyCon_do_set_spring_stiffness(rbc, value, RBC_FLAG_USE_SPRING_ANG_X, RB_LIMIT_ANG_X);
#  endif
}

static void rna_RigidBodyCon_spring_stiffness_ang_y_set(PointerRNA *ptr, float value)
{
  RigidBodyCon *rbc = (RigidBodyCon *)ptr->data;

  rbc->spring_stiffness_ang_y = value;

#  ifdef WITH_BULLET
  rna_RigidBodyCon_do_set_spring_stiffness(rbc, value, RBC_FLAG_USE_SPRING_ANG_Y, RB_LIMIT_ANG_Y);
#  endif
}

static void rna_RigidBodyCon_spring_stiffness_ang_z_set(PointerRNA *ptr, float value)
{
  RigidBodyCon *rbc = (RigidBodyCon *)ptr->data;

  rbc->spring_stiffness_ang_z = value;

#  ifdef WITH_BULLET
  rna_RigidBodyCon_do_set_spring_stiffness(rbc, value, RBC_FLAG_USE_SPRING_ANG_Z, RB_LIMIT_ANG_Z);
#  endif
}

#  ifdef WITH_BULLET
static void rna_RigidBodyCon_do_set_spring_damping(RigidBodyCon *rbc,
                                                   float value,
                                                   int flag,
                                                   int axis)
{
  if (rbc->physics_constraint && rbc->type == RBC_TYPE_6DOF_SPRING && (rbc->flag & flag)) {
    switch (rbc->spring_type) {
      case RBC_SPRING_TYPE1:
        RB_constraint_set_damping_6dof_spring(
            static_cast<rbConstraint *>(rbc->physics_constraint), axis, value);
        break;
      case RBC_SPRING_TYPE2:
        RB_constraint_set_damping_6dof_spring2(
            static_cast<rbConstraint *>(rbc->physics_constraint), axis, value);
        break;
    }
  }
}
#  endif

static void rna_RigidBodyCon_spring_damping_x_set(PointerRNA *ptr, float value)
{
  RigidBodyCon *rbc = (RigidBodyCon *)ptr->data;

  rbc->spring_damping_x = value;

#  ifdef WITH_BULLET
  rna_RigidBodyCon_do_set_spring_damping(rbc, value, RBC_FLAG_USE_SPRING_X, RB_LIMIT_LIN_X);
#  endif
}

static void rna_RigidBodyCon_spring_damping_y_set(PointerRNA *ptr, float value)
{
  RigidBodyCon *rbc = (RigidBodyCon *)ptr->data;

  rbc->spring_damping_y = value;
#  ifdef WITH_BULLET
  rna_RigidBodyCon_do_set_spring_damping(rbc, value, RBC_FLAG_USE_SPRING_Y, RB_LIMIT_LIN_Y);
#  endif
}

static void rna_RigidBodyCon_spring_damping_z_set(PointerRNA *ptr, float value)
{
  RigidBodyCon *rbc = (RigidBodyCon *)ptr->data;

  rbc->spring_damping_z = value;
#  ifdef WITH_BULLET
  rna_RigidBodyCon_do_set_spring_damping(rbc, value, RBC_FLAG_USE_SPRING_Z, RB_LIMIT_LIN_Z);
#  endif
}

static void rna_RigidBodyCon_spring_damping_ang_x_set(PointerRNA *ptr, float value)
{
  RigidBodyCon *rbc = (RigidBodyCon *)ptr->data;

  rbc->spring_damping_ang_x = value;

#  ifdef WITH_BULLET
  rna_RigidBodyCon_do_set_spring_damping(rbc, value, RBC_FLAG_USE_SPRING_ANG_X, RB_LIMIT_ANG_X);
#  endif
}

static void rna_RigidBodyCon_spring_damping_ang_y_set(PointerRNA *ptr, float value)
{
  RigidBodyCon *rbc = (RigidBodyCon *)ptr->data;

  rbc->spring_damping_ang_y = value;
#  ifdef WITH_BULLET
  rna_RigidBodyCon_do_set_spring_damping(rbc, value, RBC_FLAG_USE_SPRING_ANG_Y, RB_LIMIT_ANG_Y);
#  endif
}

static void rna_RigidBodyCon_spring_damping_ang_z_set(PointerRNA *ptr, float value)
{
  RigidBodyCon *rbc = (RigidBodyCon *)ptr->data;

  rbc->spring_damping_ang_z = value;
#  ifdef WITH_BULLET
  rna_RigidBodyCon_do_set_spring_damping(rbc, value, RBC_FLAG_USE_SPRING_ANG_Z, RB_LIMIT_ANG_Z);
#  endif
}

static void rna_RigidBodyCon_motor_lin_max_impulse_set(PointerRNA *ptr, float value)
{
  RigidBodyCon *rbc = (RigidBodyCon *)ptr->data;

  rbc->motor_lin_max_impulse = value;

#  ifdef WITH_BULLET
  if (rbc->physics_constraint && rbc->type == RBC_TYPE_MOTOR) {
    RB_constraint_set_max_impulse_motor(
        static_cast<rbConstraint *>(rbc->physics_constraint), value, rbc->motor_ang_max_impulse);
  }
#  endif
}

static void rna_RigidBodyCon_use_motor_lin_set(PointerRNA *ptr, bool value)
{
  RigidBodyCon *rbc = (RigidBodyCon *)ptr->data;

  SET_FLAG_FROM_TEST(rbc->flag, value, RBC_FLAG_USE_MOTOR_LIN);

#  ifdef WITH_BULLET
  if (rbc->physics_constraint) {
    RB_constraint_set_enable_motor(static_cast<rbConstraint *>(rbc->physics_constraint),
                                   rbc->flag & RBC_FLAG_USE_MOTOR_LIN,
                                   rbc->flag & RBC_FLAG_USE_MOTOR_ANG);
  }
#  endif
}

static void rna_RigidBodyCon_use_motor_ang_set(PointerRNA *ptr, bool value)
{
  RigidBodyCon *rbc = (RigidBodyCon *)ptr->data;

  SET_FLAG_FROM_TEST(rbc->flag, value, RBC_FLAG_USE_MOTOR_ANG);

#  ifdef WITH_BULLET
  if (rbc->physics_constraint) {
    RB_constraint_set_enable_motor(static_cast<rbConstraint *>(rbc->physics_constraint),
                                   rbc->flag & RBC_FLAG_USE_MOTOR_LIN,
                                   rbc->flag & RBC_FLAG_USE_MOTOR_ANG);
  }
#  endif
}

static void rna_RigidBodyCon_motor_lin_target_velocity_set(PointerRNA *ptr, float value)
{
  RigidBodyCon *rbc = (RigidBodyCon *)ptr->data;

  rbc->motor_lin_target_velocity = value;

#  ifdef WITH_BULLET
  if (rbc->physics_constraint && rbc->type == RBC_TYPE_MOTOR) {
    RB_constraint_set_target_velocity_motor(static_cast<rbConstraint *>(rbc->physics_constraint),
                                            value,
                                            rbc->motor_ang_target_velocity);
  }
#  endif
}

static void rna_RigidBodyCon_motor_ang_max_impulse_set(PointerRNA *ptr, float value)
{
  RigidBodyCon *rbc = (RigidBodyCon *)ptr->data;

  rbc->motor_ang_max_impulse = value;

#  ifdef WITH_BULLET
  if (rbc->physics_constraint && rbc->type == RBC_TYPE_MOTOR) {
    RB_constraint_set_max_impulse_motor(
        static_cast<rbConstraint *>(rbc->physics_constraint), rbc->motor_lin_max_impulse, value);
  }
#  endif
}

static void rna_RigidBodyCon_motor_ang_target_velocity_set(PointerRNA *ptr, float value)
{
  RigidBodyCon *rbc = (RigidBodyCon *)ptr->data;

  rbc->motor_ang_target_velocity = value;

#  ifdef WITH_BULLET
  if (rbc->physics_constraint && rbc->type == RBC_TYPE_MOTOR) {
    RB_constraint_set_target_velocity_motor(static_cast<rbConstraint *>(rbc->physics_constraint),
                                            rbc->motor_lin_target_velocity,
                                            value);
  }
#  endif
}

/* Sweep test */
static void rna_RigidBodyWorld_convex_sweep_test(RigidBodyWorld *rbw,
                                                 ReportList *reports,
                                                 Object *object,
                                                 const float ray_start[3],
                                                 const float ray_end[3],
                                                 float r_location[3],
                                                 float r_hitpoint[3],
                                                 float r_normal[3],
                                                 int *r_hit)
{
#  ifdef WITH_BULLET
  RigidBodyOb *rob = object->rigidbody_object;
  rbDynamicsWorld *physics_world = BKE_rigidbody_world_physics(rbw);

  if (physics_world != nullptr && rob->shared->physics_object != nullptr) {
    RB_world_convex_sweep_test(physics_world,
                               static_cast<rbRigidBody *>(rob->shared->physics_object),
                               ray_start,
                               ray_end,
                               r_location,
                               r_hitpoint,
                               r_normal,
                               r_hit);
    if (*r_hit == -2) {
      BKE_report(reports,
                 RPT_ERROR,
                 "A non convex collision shape was passed to the function, use only convex "
                 "collision shapes");
    }
  }
  else {
    *r_hit = -1;
    BKE_report(reports,
               RPT_ERROR,
               "Rigidbody world was not properly initialized, need to step the simulation first");
  }
#  else
  UNUSED_VARS(rbw, reports, object, ray_start, ray_end, r_location, r_hitpoint, r_normal, r_hit);
#  endif
}

static PointerRNA rna_RigidBodyWorld_PointCache_get(PointerRNA *ptr)
{
  RigidBodyWorld *rbw = static_cast<RigidBodyWorld *>(ptr->data);
  return rna_pointer_inherit_refine(ptr, &RNA_PointCache, rbw->shared->pointcache);
}

#else

static void rna_def_rigidbody_world(BlenderRNA *brna)
{
  StructRNA *srna;
  PropertyRNA *prop;

  FunctionRNA *func;
  PropertyRNA *parm;

  srna = RNA_def_struct(brna, "RigidBodyWorld", nullptr);
  RNA_def_struct_sdna(srna, "RigidBodyWorld");
  RNA_def_struct_ui_text(
      srna, "Rigid Body World", "Self-contained rigid body simulation environment and settings");
  RNA_def_struct_path_func(srna, "rna_RigidBodyWorld_path");

  /* groups */
  prop = RNA_def_property(srna, "collection", PROP_POINTER, PROP_NONE);
  RNA_def_property_struct_type(prop, "Collection");
  RNA_def_property_pointer_sdna(prop, nullptr, "group");
  RNA_def_property_flag(prop, PROP_EDITABLE | PROP_ID_SELF_CHECK | PROP_ID_REFCOUNT);
  RNA_def_property_override_flag(prop, PROPOVERRIDE_OVERRIDABLE_LIBRARY);
  RNA_def_property_ui_text(
      prop, "Collection", "Collection containing objects participating in this simulation");
  RNA_def_property_update(prop, NC_SCENE, "rna_RigidBodyWorld_objects_collection_update");

  prop = RNA_def_property(srna, "constraints", PROP_POINTER, PROP_NONE);
  RNA_def_property_struct_type(prop, "Collection");
  RNA_def_property_flag(prop, PROP_EDITABLE | PROP_ID_SELF_CHECK | PROP_ID_REFCOUNT);
  RNA_def_property_override_flag(prop, PROPOVERRIDE_OVERRIDABLE_LIBRARY);
  RNA_def_property_ui_text(
      prop, "Constraints", "Collection containing rigid body constraint objects");
  RNA_def_property_update(prop, NC_SCENE, "rna_RigidBodyWorld_constraints_collection_update");

  /* booleans */
  prop = RNA_def_property(srna, "enabled", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_negative_sdna(prop, nullptr, "flag", RBW_FLAG_MUTED);
  RNA_def_property_ui_text(prop, "Enabled", "Simulation will be evaluated");
  RNA_def_property_update(prop, NC_SCENE, nullptr);

  /* time scale */
  prop = RNA_def_property(srna, "time_scale", PROP_FLOAT, PROP_NONE);
  RNA_def_property_float_sdna(prop, nullptr, "time_scale");
  RNA_def_property_range(prop, 0.0f, 100.0f);
  RNA_def_property_ui_range(prop, 0.0f, 10.0f, 1, 3);
  RNA_def_property_float_default(prop, 1.0f);
  RNA_def_property_ui_text(prop, "Time Scale", "Change the speed of the simulation");
  RNA_def_property_update(prop, NC_SCENE, "rna_RigidBodyWorld_reset");

  /* timestep */
  prop = RNA_def_property(srna, "substeps_per_frame", PROP_INT, PROP_NONE);
  RNA_def_property_int_sdna(prop, nullptr, "substeps_per_frame");
  RNA_def_property_range(prop, 1, SHRT_MAX);
  RNA_def_property_ui_range(prop, 1, 1000, 1, -1);
  RNA_def_property_int_default(prop, 10);
  RNA_def_property_ui_text(
      prop,
      "Substeps Per Frame",
      "Number of simulation steps taken per frame (higher values are more accurate "
      "but slower)");
  RNA_def_property_update(prop, NC_SCENE, "rna_RigidBodyWorld_reset");

  /* constraint solver iterations */
  prop = RNA_def_property(srna, "solver_iterations", PROP_INT, PROP_NONE);
  RNA_def_property_int_sdna(prop, nullptr, "num_solver_iterations");
  RNA_def_property_range(prop, 1, 1000);
  RNA_def_property_ui_range(prop, 10, 100, 1, -1);
  RNA_def_property_int_default(prop, 10);
  RNA_def_property_int_funcs(
      prop, nullptr, "rna_RigidBodyWorld_num_solver_iterations_set", nullptr);
  RNA_def_property_ui_text(
      prop,
      "Solver Iterations",
      "Number of constraint solver iterations made per simulation step (higher values are more "
      "accurate but slower)");
  RNA_def_property_update(prop, NC_SCENE, "rna_RigidBodyWorld_reset");

  /* split impulse */
  prop = RNA_def_property(srna, "use_split_impulse", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, nullptr, "flag", RBW_FLAG_USE_SPLIT_IMPULSE);
  RNA_def_property_boolean_funcs(prop, nullptr, "rna_RigidBodyWorld_split_impulse_set");
  RNA_def_property_ui_text(
      prop,
      "Split Impulse",
      "Reduce extra velocity that can build up when objects collide (lowers simulation "
      "stability a little so use only when necessary)");
  RNA_def_property_update(prop, NC_SCENE, "rna_RigidBodyWorld_reset");

  /* cache */
  prop = RNA_def_property(srna, "point_cache", PROP_POINTER, PROP_NONE);
  RNA_def_property_flag(prop, PROP_NEVER_NULL);
  RNA_def_property_pointer_funcs(
      prop, "rna_RigidBodyWorld_PointCache_get", nullptr, nullptr, nullptr);
  RNA_def_property_struct_type(prop, "PointCache");
  RNA_def_property_ui_text(prop, "Point Cache", "");

  /* effector weights */
  prop = RNA_def_property(srna, "effector_weights", PROP_POINTER, PROP_NONE);
  RNA_def_property_struct_type(prop, "EffectorWeights");
  RNA_def_property_clear_flag(prop, PROP_EDITABLE);
  RNA_def_property_override_flag(prop, PROPOVERRIDE_OVERRIDABLE_LIBRARY);
  RNA_def_property_ui_text(prop, "Effector Weights", "");

  /* Sweep test */
  func = RNA_def_function(srna, "convex_sweep_test", "rna_RigidBodyWorld_convex_sweep_test");
  RNA_def_function_ui_description(
      func, "Sweep test convex rigidbody against the current rigidbody world");
  RNA_def_function_flag(func, FUNC_USE_REPORTS);
  parm = RNA_def_pointer(
      func, "object", "Object", "", "Rigidbody object with a convex collision shape");
  RNA_def_parameter_flags(parm, PROP_NEVER_NULL, PARM_REQUIRED);
  RNA_def_parameter_clear_flags(parm, PROP_THICK_WRAP, ParameterFlag(0));
  /* ray start and end */
  parm = RNA_def_float_vector(func, "start", 3, nullptr, -FLT_MAX, FLT_MAX, "", "", -1e4, 1e4);
  RNA_def_parameter_flags(parm, PropertyFlag(0), PARM_REQUIRED);
  parm = RNA_def_float_vector(func, "end", 3, nullptr, -FLT_MAX, FLT_MAX, "", "", -1e4, 1e4);
  RNA_def_parameter_flags(parm, PropertyFlag(0), PARM_REQUIRED);
  parm = RNA_def_float_vector(func,
                              "object_location",
                              3,
                              nullptr,
                              -FLT_MAX,
                              FLT_MAX,
                              "Location",
                              "The hit location of this sweep test",
                              -1e4,
                              1e4);
  RNA_def_parameter_flags(parm, PROP_THICK_WRAP, ParameterFlag(0));
  RNA_def_function_output(func, parm);
  parm = RNA_def_float_vector(func,
                              "hitpoint",
                              3,
                              nullptr,
                              -FLT_MAX,
                              FLT_MAX,
                              "Hitpoint",
                              "The hit location of this sweep test",
                              -1e4,
                              1e4);
  RNA_def_parameter_flags(parm, PROP_THICK_WRAP, ParameterFlag(0));
  RNA_def_function_output(func, parm);
  parm = RNA_def_float_vector(func,
                              "normal",
                              3,
                              nullptr,
                              -FLT_MAX,
                              FLT_MAX,
                              "Normal",
                              "The face normal at the sweep test hit location",
                              -1e4,
                              1e4);
  RNA_def_parameter_flags(parm, PROP_THICK_WRAP, ParameterFlag(0));
  RNA_def_function_output(func, parm);
  parm = RNA_def_int(func,
                     "has_hit",
                     0,
                     0,
                     0,
                     "",
                     "If the function has found collision point, value is 1, otherwise 0",
                     0,
                     0);
  RNA_def_function_output(func, parm);
}

static void rna_def_rigidbody_object(BlenderRNA *brna)
{
  StructRNA *srna;
  PropertyRNA *prop;

  srna = RNA_def_struct(brna, "RigidBodyObject", nullptr);
  RNA_def_struct_sdna(srna, "RigidBodyOb");
  RNA_def_struct_ui_text(
      srna, "Rigid Body Object", "Settings for object participating in Rigid Body Simulation");
  RNA_def_struct_path_func(srna, "rna_RigidBodyOb_path");

  /* Enums */
  prop = RNA_def_property(srna, "type", PROP_ENUM, PROP_NONE);
  RNA_def_property_enum_sdna(prop, nullptr, "type");
  RNA_def_property_enum_items(prop, rna_enum_rigidbody_object_type_items);
  RNA_def_property_enum_funcs(prop, nullptr, "rna_RigidBodyOb_type_set", nullptr);
  RNA_def_property_ui_text(prop, "Type", "Role of object in Rigid Body Simulations");
  RNA_def_property_clear_flag(prop, PROP_ANIMATABLE);
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "mesh_source", PROP_ENUM, PROP_NONE);
  RNA_def_property_enum_sdna(prop, nullptr, "mesh_source");
  RNA_def_property_enum_items(prop, rigidbody_mesh_source_items);
  RNA_def_property_ui_text(
      prop, "Mesh Source", "Source of the mesh used to create collision shape");
  RNA_def_property_clear_flag(prop, PROP_ANIMATABLE);
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_mesh_source_update");

  /* booleans */
  prop = RNA_def_property(srna, "enabled", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_negative_sdna(prop, nullptr, "flag", RBO_FLAG_DISABLED);
  RNA_def_property_boolean_funcs(prop, nullptr, "rna_RigidBodyOb_disabled_set");
  RNA_def_property_ui_text(prop, "Enabled", "Rigid Body actively participates to the simulation");
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "collision_shape", PROP_ENUM, PROP_NONE);
  RNA_def_property_enum_sdna(prop, nullptr, "shape");
  RNA_def_property_enum_items(prop, rna_enum_rigidbody_object_shape_items);
  RNA_def_property_enum_funcs(prop, nullptr, "rna_RigidBodyOb_shape_set", nullptr);
  RNA_def_property_ui_text(
      prop, "Collision Shape", "Collision Shape of object in Rigid Body Simulations");
  RNA_def_property_clear_flag(prop, PROP_ANIMATABLE);
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_shape_update");

  prop = RNA_def_property(srna, "kinematic", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, nullptr, "flag", RBO_FLAG_KINEMATIC);
  RNA_def_property_boolean_funcs(prop, nullptr, "rna_RigidBodyOb_kinematic_state_set");
  RNA_def_property_ui_text(
      prop, "Kinematic", "Allow rigid body to be controlled by the animation system");
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "use_deform", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, nullptr, "flag", RBO_FLAG_USE_DEFORM);
  RNA_def_property_ui_text(prop, "Deforming", "Rigid body deforms during simulation");
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_reset");

  /* Physics Parameters */
  prop = RNA_def_property(srna, "mass", PROP_FLOAT, PROP_UNIT_MASS);
  RNA_def_property_float_sdna(prop, nullptr, "mass");
  RNA_def_property_range(prop, 0.001f, FLT_MAX); /* range must always be positive (and non-zero) */
  RNA_def_property_float_default(prop, 1.0f);
  RNA_def_property_float_funcs(prop, nullptr, "rna_RigidBodyOb_mass_set", nullptr);
  RNA_def_property_ui_text(prop, "Mass", "How much the object 'weighs' irrespective of gravity");
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_reset");

  /* Dynamics Parameters - Activation */
  /* TODO: define and figure out how to implement these. */

  /* Dynamics Parameters - Deactivation */
  prop = RNA_def_property(srna, "use_deactivation", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, nullptr, "flag", RBO_FLAG_USE_DEACTIVATION);
  RNA_def_property_boolean_default(prop, true);
  RNA_def_property_boolean_funcs(prop, nullptr, "rna_RigidBodyOb_activation_state_set");
  RNA_def_property_ui_text(
      prop,
      "Enable Deactivation",
      "Enable deactivation of resting rigid bodies (increases performance and stability "
      "but can cause glitches)");
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "use_start_deactivated", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, nullptr, "flag", RBO_FLAG_START_DEACTIVATED);
  RNA_def_property_ui_text(
      prop, "Start Deactivated", "Deactivate rigid body at the start of the simulation");
  RNA_def_property_clear_flag(prop, PROP_ANIMATABLE);
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "deactivate_linear_velocity", PROP_FLOAT, PROP_UNIT_VELOCITY);
  RNA_def_property_float_sdna(prop, nullptr, "lin_sleep_thresh");
  RNA_def_property_range(
      prop, FLT_MIN, FLT_MAX); /* range must always be positive (and non-zero) */
  RNA_def_property_float_default(prop, 0.4f);
  RNA_def_property_float_funcs(prop, nullptr, "rna_RigidBodyOb_linear_sleepThresh_set", nullptr);
  RNA_def_property_ui_text(prop,
                           "Linear Velocity Deactivation Threshold",
                           "Linear Velocity below which simulation stops simulating object");
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "deactivate_angular_velocity", PROP_FLOAT, PROP_UNIT_VELOCITY);
  RNA_def_property_float_sdna(prop, nullptr, "ang_sleep_thresh");
  RNA_def_property_range(
      prop, FLT_MIN, FLT_MAX); /* range must always be positive (and non-zero) */
  RNA_def_property_float_default(prop, 0.5f);
  RNA_def_property_float_funcs(prop, nullptr, "rna_RigidBodyOb_angular_sleepThresh_set", nullptr);
  RNA_def_property_ui_text(prop,
                           "Angular Velocity Deactivation Threshold",
                           "Angular Velocity below which simulation stops simulating object");
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_reset");

  /* Dynamics Parameters - Damping Parameters */
  prop = RNA_def_property(srna, "linear_damping", PROP_FLOAT, PROP_FACTOR);
  RNA_def_property_float_sdna(prop, nullptr, "lin_damping");
  RNA_def_property_range(prop, 0.0f, 1.0f);
  RNA_def_property_float_default(prop, 0.04f);
  RNA_def_property_float_funcs(prop, nullptr, "rna_RigidBodyOb_linear_damping_set", nullptr);
  RNA_def_property_ui_text(
      prop, "Linear Damping", "Amount of linear velocity that is lost over time");
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "angular_damping", PROP_FLOAT, PROP_FACTOR);
  RNA_def_property_float_sdna(prop, nullptr, "ang_damping");
  RNA_def_property_range(prop, 0.0f, 1.0f);
  RNA_def_property_float_default(prop, 0.1f);
  RNA_def_property_float_funcs(prop, nullptr, "rna_RigidBodyOb_angular_damping_set", nullptr);
  RNA_def_property_ui_text(
      prop, "Angular Damping", "Amount of angular velocity that is lost over time");
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_reset");

  /* Collision Parameters - Surface Parameters */
  prop = RNA_def_property(srna, "friction", PROP_FLOAT, PROP_FACTOR);
  RNA_def_property_float_sdna(prop, nullptr, "friction");
  RNA_def_property_range(prop, 0.0f, FLT_MAX);
  RNA_def_property_ui_range(prop, 0.0f, 1.0f, 1, 3);
  RNA_def_property_float_default(prop, 0.5f);
  RNA_def_property_float_funcs(prop, nullptr, "rna_RigidBodyOb_friction_set", nullptr);
  RNA_def_property_ui_text(prop, "Friction", "Resistance of object to movement");
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "restitution", PROP_FLOAT, PROP_FACTOR);
  RNA_def_property_float_sdna(prop, nullptr, "restitution");
  RNA_def_property_range(prop, 0.0f, FLT_MAX);
  RNA_def_property_ui_range(prop, 0.0f, 1.0f, 1, 3);
  RNA_def_property_float_default(prop, 0.0f);
  RNA_def_property_float_funcs(prop, nullptr, "rna_RigidBodyOb_restitution_set", nullptr);
  RNA_def_property_ui_text(prop,
                           "Bounciness",
                           "Tendency of object to bounce after colliding with another "
                           "(0 = stays still, 1 = perfectly elastic)");
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_reset");

  /* Collision Parameters - Sensitivity */
  prop = RNA_def_property(srna, "use_margin", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, nullptr, "flag", RBO_FLAG_USE_MARGIN);
  RNA_def_property_boolean_default(prop, false);
  RNA_def_property_ui_text(
      prop,
      "Collision Margin",
      "Use custom collision margin (some shapes will have a visible gap around them)");
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_shape_reset");

  prop = RNA_def_property(srna, "collision_margin", PROP_FLOAT, PROP_UNIT_LENGTH);
  RNA_def_property_float_sdna(prop, nullptr, "margin");
  RNA_def_property_range(prop, 0.0f, 1.0f);
  RNA_def_property_ui_range(prop, 0.0f, 1.0f, 0.01, 3);
  RNA_def_property_float_default(prop, 0.04f);
  RNA_def_property_float_funcs(prop, nullptr, "rna_RigidBodyOb_collision_margin_set", nullptr);
  RNA_def_property_ui_text(
      prop,
      "Collision Margin",
      "Threshold of distance near surface where collisions are still considered "
      "(best results when non-zero)");
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_shape_reset");

  prop = RNA_def_property(srna, "collision_collections", PROP_BOOLEAN, PROP_LAYER_MEMBER);
  RNA_def_property_boolean_bitset_array_sdna(prop, nullptr, "col_groups", 1 << 0, 20);
  RNA_def_property_boolean_funcs(prop, nullptr, "rna_RigidBodyOb_collision_collections_set");
  RNA_def_property_ui_text(
      prop, "Collision Collections", "Collision collections rigid body belongs to");
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_reset");
  RNA_def_property_flag(prop, PROP_LIB_EXCEPTION);
}

static void rna_def_rigidbody_constraint(BlenderRNA *brna)
{
  StructRNA *srna;
  PropertyRNA *prop;

  srna = RNA_def_struct(brna, "RigidBodyConstraint", nullptr);
  RNA_def_struct_sdna(srna, "RigidBodyCon");
  RNA_def_struct_ui_text(srna,
                         "Rigid Body Constraint",
                         "Constraint influencing Objects inside Rigid Body Simulation");
  RNA_def_struct_path_func(srna, "rna_RigidBodyCon_path");

  /* Enums */
  prop = RNA_def_property(srna, "type", PROP_ENUM, PROP_NONE);
  RNA_def_property_enum_sdna(prop, nullptr, "type");
  RNA_def_property_enum_items(prop, rna_enum_rigidbody_constraint_type_items);
  RNA_def_property_enum_funcs(prop, nullptr, "rna_RigidBodyCon_type_set", nullptr);
  RNA_def_property_ui_text(prop, "Type", "Type of Rigid Body Constraint");
  RNA_def_property_clear_flag(prop, PROP_ANIMATABLE);
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "spring_type", PROP_ENUM, PROP_NONE);
  RNA_def_property_enum_sdna(prop, nullptr, "spring_type");
  RNA_def_property_enum_items(prop, rna_enum_rigidbody_constraint_spring_type_items);
  RNA_def_property_enum_funcs(prop, nullptr, "rna_RigidBodyCon_spring_type_set", nullptr);
  RNA_def_property_ui_text(prop, "Spring Type", "Which implementation of spring to use");
  RNA_def_property_clear_flag(prop, PROP_ANIMATABLE);
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "enabled", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, nullptr, "flag", RBC_FLAG_ENABLED);
  RNA_def_property_boolean_funcs(prop, nullptr, "rna_RigidBodyCon_enabled_set");
  RNA_def_property_ui_text(prop, "Enabled", "Enable this constraint");
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "disable_collisions", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, nullptr, "flag", RBC_FLAG_DISABLE_COLLISIONS);
  RNA_def_property_boolean_funcs(prop, nullptr, "rna_RigidBodyCon_disable_collisions_set");
  RNA_def_property_ui_text(
      prop, "Disable Collisions", "Disable collisions between constrained rigid bodies");
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "object1", PROP_POINTER, PROP_NONE);
  RNA_def_property_pointer_sdna(prop, nullptr, "ob1");
  RNA_def_property_flag(prop, PROP_EDITABLE);
  RNA_def_property_override_flag(prop, PROPOVERRIDE_OVERRIDABLE_LIBRARY);
  RNA_def_property_ui_text(prop, "Object 1", "First Rigid Body Object to be constrained");
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "object2", PROP_POINTER, PROP_NONE);
  RNA_def_property_pointer_sdna(prop, nullptr, "ob2");
  RNA_def_property_flag(prop, PROP_EDITABLE);
  RNA_def_property_override_flag(prop, PROPOVERRIDE_OVERRIDABLE_LIBRARY);
  RNA_def_property_ui_text(prop, "Object 2", "Second Rigid Body Object to be constrained");
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_reset");

  /* Breaking Threshold */
  prop = RNA_def_property(srna, "use_breaking", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, nullptr, "flag", RBC_FLAG_USE_BREAKING);
  RNA_def_property_boolean_funcs(prop, nullptr, "rna_RigidBodyCon_use_breaking_set");
  RNA_def_property_ui_text(
      prop, "Breakable", "Constraint can be broken if it receives an impulse above the threshold");
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "breaking_threshold", PROP_FLOAT, PROP_NONE);
  RNA_def_property_float_sdna(prop, nullptr, "breaking_threshold");
  RNA_def_property_range(prop, 0.0f, FLT_MAX);
  RNA_def_property_ui_range(prop, 0.0f, 1000.0f, 100.0, 2);
  RNA_def_property_float_default(prop, 10.0f);
  RNA_def_property_float_funcs(prop, nullptr, "rna_RigidBodyCon_breaking_threshold_set", nullptr);
  RNA_def_property_ui_text(prop,
                           "Breaking Threshold",
                           "Impulse threshold that must be reached for the constraint to break");
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_reset");

  /* Solver Iterations */
  prop = RNA_def_property(srna, "use_override_solver_iterations", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, nullptr, "flag", RBC_FLAG_OVERRIDE_SOLVER_ITERATIONS);
  RNA_def_property_boolean_funcs(prop, nullptr, "rna_RigidBodyCon_override_solver_iterations_set");
  RNA_def_property_ui_text(prop,
                           "Override Solver Iterations",
                           "Override the number of solver iterations for this constraint");
  RNA_def_property_update(prop, NC_OBJECT | ND_POINTCACHE, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "solver_iterations", PROP_INT, PROP_NONE);
  RNA_def_property_int_sdna(prop, nullptr, "num_solver_iterations");
  RNA_def_property_range(prop, 1, 1000);
  RNA_def_property_ui_range(prop, 1, 100, 1, -1);
  RNA_def_property_int_default(prop, 10);
  RNA_def_property_int_funcs(prop, nullptr, "rna_RigidBodyCon_num_solver_iterations_set", nullptr);
  RNA_def_property_ui_text(
      prop,
      "Solver Iterations",
      "Number of constraint solver iterations made per simulation step (higher values are more "
      "accurate but slower)");
  RNA_def_property_update(prop, NC_OBJECT, "rna_RigidBodyOb_reset");

  /* Limits */
  prop = RNA_def_property(srna, "use_limit_lin_x", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, nullptr, "flag", RBC_FLAG_USE_LIMIT_LIN_X);
  RNA_def_property_ui_text(prop, "X Axis", "Limit translation on X axis");
  RNA_def_property_update(prop, NC_OBJECT | ND_DRAW, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "use_limit_lin_y", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, nullptr, "flag", RBC_FLAG_USE_LIMIT_LIN_Y);
  RNA_def_property_ui_text(prop, "Y Axis", "Limit translation on Y axis");
  RNA_def_property_update(prop, NC_OBJECT | ND_DRAW, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "use_limit_lin_z", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, nullptr, "flag", RBC_FLAG_USE_LIMIT_LIN_Z);
  RNA_def_property_ui_text(prop, "Z Axis", "Limit translation on Z axis");
  RNA_def_property_update(prop, NC_OBJECT | ND_DRAW, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "use_limit_ang_x", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, nullptr, "flag", RBC_FLAG_USE_LIMIT_ANG_X);
  RNA_def_property_ui_text(prop, "X Angle", "Limit rotation around X axis");
  RNA_def_property_update(prop, NC_OBJECT | ND_DRAW, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "use_limit_ang_y", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, nullptr, "flag", RBC_FLAG_USE_LIMIT_ANG_Y);
  RNA_def_property_ui_text(prop, "Y Angle", "Limit rotation around Y axis");
  RNA_def_property_update(prop, NC_OBJECT | ND_DRAW, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "use_limit_ang_z", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, nullptr, "flag", RBC_FLAG_USE_LIMIT_ANG_Z);
  RNA_def_property_ui_text(prop, "Z Angle", "Limit rotation around Z axis");
  RNA_def_property_update(prop, NC_OBJECT | ND_DRAW, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "use_spring_x", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, nullptr, "flag", RBC_FLAG_USE_SPRING_X);
  RNA_def_property_ui_text(prop, "X Spring", "Enable spring on X axis");
  RNA_def_property_update(prop, NC_OBJECT, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "use_spring_y", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, nullptr, "flag", RBC_FLAG_USE_SPRING_Y);
  RNA_def_property_ui_text(prop, "Y Spring", "Enable spring on Y axis");
  RNA_def_property_update(prop, NC_OBJECT, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "use_spring_z", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, nullptr, "flag", RBC_FLAG_USE_SPRING_Z);
  RNA_def_property_ui_text(prop, "Z Spring", "Enable spring on Z axis");
  RNA_def_property_update(prop, NC_OBJECT, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "use_spring_ang_x", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, nullptr, "flag", RBC_FLAG_USE_SPRING_ANG_X);
  RNA_def_property_ui_text(prop, "X Angle Spring", "Enable spring on X rotational axis");
  RNA_def_property_update(prop, NC_OBJECT, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "use_spring_ang_y", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, nullptr, "flag", RBC_FLAG_USE_SPRING_ANG_Y);
  RNA_def_property_ui_text(prop, "Y Angle Spring", "Enable spring on Y rotational axis");
  RNA_def_property_update(prop, NC_OBJECT, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "use_spring_ang_z", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, nullptr, "flag", RBC_FLAG_USE_SPRING_ANG_Z);
  RNA_def_property_ui_text(prop, "Z Angle Spring", "Enable spring on Z rotational axis");
  RNA_def_property_update(prop, NC_OBJECT, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "use_motor_lin", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, nullptr, "flag", RBC_FLAG_USE_MOTOR_LIN);
  RNA_def_property_boolean_funcs(prop, nullptr, "rna_RigidBodyCon_use_motor_lin_set");
  RNA_def_property_ui_text(prop, "Linear Motor", "Enable linear motor");
  RNA_def_property_update(prop, NC_OBJECT, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "use_motor_ang", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, nullptr, "flag", RBC_FLAG_USE_MOTOR_ANG);
  RNA_def_property_boolean_funcs(prop, nullptr, "rna_RigidBodyCon_use_motor_ang_set");
  RNA_def_property_ui_text(prop, "Angular Motor", "Enable angular motor");
  RNA_def_property_update(prop, NC_OBJECT, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "limit_lin_x_lower", PROP_FLOAT, PROP_UNIT_LENGTH);
  RNA_def_property_float_sdna(prop, nullptr, "limit_lin_x_lower");
  RNA_def_property_float_default(prop, -1.0f);
  RNA_def_property_ui_text(prop, "Lower X Limit", "Lower limit of X axis translation");
  RNA_def_property_update(prop, NC_OBJECT | ND_DRAW, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "limit_lin_x_upper", PROP_FLOAT, PROP_UNIT_LENGTH);
  RNA_def_property_float_sdna(prop, nullptr, "limit_lin_x_upper");
  RNA_def_property_float_default(prop, 1.0f);
  RNA_def_property_ui_text(prop, "Upper X Limit", "Upper limit of X axis translation");
  RNA_def_property_update(prop, NC_OBJECT | ND_DRAW, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "limit_lin_y_lower", PROP_FLOAT, PROP_UNIT_LENGTH);
  RNA_def_property_float_sdna(prop, nullptr, "limit_lin_y_lower");
  RNA_def_property_float_default(prop, -1.0f);
  RNA_def_property_ui_text(prop, "Lower Y Limit", "Lower limit of Y axis translation");
  RNA_def_property_update(prop, NC_OBJECT | ND_DRAW, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "limit_lin_y_upper", PROP_FLOAT, PROP_UNIT_LENGTH);
  RNA_def_property_float_sdna(prop, nullptr, "limit_lin_y_upper");
  RNA_def_property_float_default(prop, 1.0f);
  RNA_def_property_ui_text(prop, "Upper Y Limit", "Upper limit of Y axis translation");
  RNA_def_property_update(prop, NC_OBJECT | ND_DRAW, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "limit_lin_z_lower", PROP_FLOAT, PROP_UNIT_LENGTH);
  RNA_def_property_float_sdna(prop, nullptr, "limit_lin_z_lower");
  RNA_def_property_float_default(prop, -1.0f);
  RNA_def_property_ui_text(prop, "Lower Z Limit", "Lower limit of Z axis translation");
  RNA_def_property_update(prop, NC_OBJECT | ND_DRAW, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "limit_lin_z_upper", PROP_FLOAT, PROP_UNIT_LENGTH);
  RNA_def_property_float_sdna(prop, nullptr, "limit_lin_z_upper");
  RNA_def_property_float_default(prop, 1.0f);
  RNA_def_property_ui_text(prop, "Upper Z Limit", "Upper limit of Z axis translation");
  RNA_def_property_update(prop, NC_OBJECT | ND_DRAW, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "limit_ang_x_lower", PROP_FLOAT, PROP_ANGLE);
  RNA_def_property_float_sdna(prop, nullptr, "limit_ang_x_lower");
  RNA_def_property_range(prop, -M_PI * 2, M_PI * 2);
  RNA_def_property_float_default(prop, -M_PI_4);
  RNA_def_property_ui_text(prop, "Lower X Angle Limit", "Lower limit of X axis rotation");
  RNA_def_property_update(prop, NC_OBJECT | ND_DRAW, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "limit_ang_x_upper", PROP_FLOAT, PROP_ANGLE);
  RNA_def_property_float_sdna(prop, nullptr, "limit_ang_x_upper");
  RNA_def_property_range(prop, -M_PI * 2, M_PI * 2);
  RNA_def_property_float_default(prop, M_PI_4);
  RNA_def_property_ui_text(prop, "Upper X Angle Limit", "Upper limit of X axis rotation");
  RNA_def_property_update(prop, NC_OBJECT | ND_DRAW, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "limit_ang_y_lower", PROP_FLOAT, PROP_ANGLE);
  RNA_def_property_float_sdna(prop, nullptr, "limit_ang_y_lower");
  RNA_def_property_range(prop, -M_PI * 2, M_PI * 2);
  RNA_def_property_float_default(prop, -M_PI_4);
  RNA_def_property_ui_text(prop, "Lower Y Angle Limit", "Lower limit of Y axis rotation");
  RNA_def_property_update(prop, NC_OBJECT | ND_DRAW, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "limit_ang_y_upper", PROP_FLOAT, PROP_ANGLE);
  RNA_def_property_float_sdna(prop, nullptr, "limit_ang_y_upper");
  RNA_def_property_range(prop, -M_PI * 2, M_PI * 2);
  RNA_def_property_float_default(prop, M_PI_4);
  RNA_def_property_ui_text(prop, "Upper Y Angle Limit", "Upper limit of Y axis rotation");
  RNA_def_property_update(prop, NC_OBJECT | ND_DRAW, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "limit_ang_z_lower", PROP_FLOAT, PROP_ANGLE);
  RNA_def_property_float_sdna(prop, nullptr, "limit_ang_z_lower");
  RNA_def_property_range(prop, -M_PI * 2, M_PI * 2);
  RNA_def_property_float_default(prop, -M_PI_4);
  RNA_def_property_ui_text(prop, "Lower Z Angle Limit", "Lower limit of Z axis rotation");
  RNA_def_property_update(prop, NC_OBJECT | ND_DRAW, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "limit_ang_z_upper", PROP_FLOAT, PROP_ANGLE);
  RNA_def_property_float_sdna(prop, nullptr, "limit_ang_z_upper");
  RNA_def_property_range(prop, -M_PI * 2, M_PI * 2);
  RNA_def_property_float_default(prop, M_PI_4);
  RNA_def_property_ui_text(prop, "Upper Z Angle Limit", "Upper limit of Z axis rotation");
  RNA_def_property_update(prop, NC_OBJECT | ND_DRAW, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "spring_stiffness_x", PROP_FLOAT, PROP_NONE);
  RNA_def_property_float_sdna(prop, nullptr, "spring_stiffness_x");
  RNA_def_property_range(prop, 0.0f, FLT_MAX);
  RNA_def_property_ui_range(prop, 0.0f, 100.0f, 1, 3);
  RNA_def_property_float_default(prop, 10.0f);
  RNA_def_property_float_funcs(prop, nullptr, "rna_RigidBodyCon_spring_stiffness_x_set", nullptr);
  RNA_def_property_ui_text(prop, "X Axis Stiffness", "Stiffness on the X axis");
  RNA_def_property_update(prop, NC_OBJECT, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "spring_stiffness_y", PROP_FLOAT, PROP_NONE);
  RNA_def_property_float_sdna(prop, nullptr, "spring_stiffness_y");
  RNA_def_property_range(prop, 0.0f, FLT_MAX);
  RNA_def_property_ui_range(prop, 0.0f, 100.0f, 1, 3);
  RNA_def_property_float_default(prop, 10.0f);
  RNA_def_property_float_funcs(prop, nullptr, "rna_RigidBodyCon_spring_stiffness_y_set", nullptr);
  RNA_def_property_ui_text(prop, "Y Axis Stiffness", "Stiffness on the Y axis");
  RNA_def_property_update(prop, NC_OBJECT, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "spring_stiffness_z", PROP_FLOAT, PROP_NONE);
  RNA_def_property_float_sdna(prop, nullptr, "spring_stiffness_z");
  RNA_def_property_range(prop, 0.0f, FLT_MAX);
  RNA_def_property_ui_range(prop, 0.0f, 100.0f, 1, 3);
  RNA_def_property_float_default(prop, 10.0f);
  RNA_def_property_float_funcs(prop, nullptr, "rna_RigidBodyCon_spring_stiffness_z_set", nullptr);
  RNA_def_property_ui_text(prop, "Z Axis Stiffness", "Stiffness on the Z axis");
  RNA_def_property_update(prop, NC_OBJECT, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "spring_stiffness_ang_x", PROP_FLOAT, PROP_NONE);
  RNA_def_property_float_sdna(prop, nullptr, "spring_stiffness_ang_x");
  RNA_def_property_range(prop, 0.0f, FLT_MAX);
  RNA_def_property_ui_range(prop, 0.0f, 100.0f, 1, 3);
  RNA_def_property_float_default(prop, 10.0f);
  RNA_def_property_float_funcs(
      prop, nullptr, "rna_RigidBodyCon_spring_stiffness_ang_x_set", nullptr);
  RNA_def_property_ui_text(prop, "X Angle Stiffness", "Stiffness on the X rotational axis");
  RNA_def_property_update(prop, NC_OBJECT, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "spring_stiffness_ang_y", PROP_FLOAT, PROP_NONE);
  RNA_def_property_float_sdna(prop, nullptr, "spring_stiffness_ang_y");
  RNA_def_property_range(prop, 0.0f, FLT_MAX);
  RNA_def_property_ui_range(prop, 0.0f, 100.0f, 1, 3);
  RNA_def_property_float_default(prop, 10.0f);
  RNA_def_property_float_funcs(
      prop, nullptr, "rna_RigidBodyCon_spring_stiffness_ang_y_set", nullptr);
  RNA_def_property_ui_text(prop, "Y Angle Stiffness", "Stiffness on the Y rotational axis");
  RNA_def_property_update(prop, NC_OBJECT, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "spring_stiffness_ang_z", PROP_FLOAT, PROP_NONE);
  RNA_def_property_float_sdna(prop, nullptr, "spring_stiffness_ang_z");
  RNA_def_property_range(prop, 0.0f, FLT_MAX);
  RNA_def_property_ui_range(prop, 0.0f, 100.0f, 1, 3);
  RNA_def_property_float_default(prop, 10.0f);
  RNA_def_property_float_funcs(
      prop, nullptr, "rna_RigidBodyCon_spring_stiffness_ang_z_set", nullptr);
  RNA_def_property_ui_text(prop, "Z Angle Stiffness", "Stiffness on the Z rotational axis");
  RNA_def_property_update(prop, NC_OBJECT, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "spring_damping_x", PROP_FLOAT, PROP_NONE);
  RNA_def_property_float_sdna(prop, nullptr, "spring_damping_x");
  RNA_def_property_range(prop, 0.0f, FLT_MAX);
  RNA_def_property_float_default(prop, 0.5f);
  RNA_def_property_float_funcs(prop, nullptr, "rna_RigidBodyCon_spring_damping_x_set", nullptr);
  RNA_def_property_ui_text(prop, "Damping X", "Damping on the X axis");
  RNA_def_property_update(prop, NC_OBJECT, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "spring_damping_y", PROP_FLOAT, PROP_NONE);
  RNA_def_property_float_sdna(prop, nullptr, "spring_damping_y");
  RNA_def_property_range(prop, 0.0f, FLT_MAX);
  RNA_def_property_float_default(prop, 0.5f);
  RNA_def_property_float_funcs(prop, nullptr, "rna_RigidBodyCon_spring_damping_y_set", nullptr);
  RNA_def_property_ui_text(prop, "Damping Y", "Damping on the Y axis");
  RNA_def_property_update(prop, NC_OBJECT, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "spring_damping_z", PROP_FLOAT, PROP_NONE);
  RNA_def_property_float_sdna(prop, nullptr, "spring_damping_z");
  RNA_def_property_range(prop, 0.0f, FLT_MAX);
  RNA_def_property_float_default(prop, 0.5f);
  RNA_def_property_float_funcs(prop, nullptr, "rna_RigidBodyCon_spring_damping_z_set", nullptr);
  RNA_def_property_ui_text(prop, "Damping Z", "Damping on the Z axis");
  RNA_def_property_update(prop, NC_OBJECT, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "spring_damping_ang_x", PROP_FLOAT, PROP_NONE);
  RNA_def_property_float_sdna(prop, nullptr, "spring_damping_ang_x");
  RNA_def_property_range(prop, 0.0f, FLT_MAX);
  RNA_def_property_float_default(prop, 0.5f);
  RNA_def_property_float_funcs(
      prop, nullptr, "rna_RigidBodyCon_spring_damping_ang_x_set", nullptr);
  RNA_def_property_ui_text(prop, "Damping X Angle", "Damping on the X rotational axis");
  RNA_def_property_update(prop, NC_OBJECT, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "spring_damping_ang_y", PROP_FLOAT, PROP_NONE);
  RNA_def_property_float_sdna(prop, nullptr, "spring_damping_ang_y");
  RNA_def_property_range(prop, 0.0f, FLT_MAX);
  RNA_def_property_float_default(prop, 0.5f);
  RNA_def_property_float_funcs(
      prop, nullptr, "rna_RigidBodyCon_spring_damping_ang_y_set", nullptr);
  RNA_def_property_ui_text(prop, "Damping Y Angle", "Damping on the Y rotational axis");
  RNA_def_property_update(prop, NC_OBJECT, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "spring_damping_ang_z", PROP_FLOAT, PROP_NONE);
  RNA_def_property_float_sdna(prop, nullptr, "spring_damping_ang_z");
  RNA_def_property_range(prop, 0.0f, FLT_MAX);
  RNA_def_property_float_default(prop, 0.5f);
  RNA_def_property_float_funcs(
      prop, nullptr, "rna_RigidBodyCon_spring_damping_ang_z_set", nullptr);
  RNA_def_property_ui_text(prop, "Damping Z Angle", "Damping on the Z rotational axis");
  RNA_def_property_update(prop, NC_OBJECT, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "motor_lin_target_velocity", PROP_FLOAT, PROP_UNIT_VELOCITY);
  RNA_def_property_float_sdna(prop, nullptr, "motor_lin_target_velocity");
  RNA_def_property_range(prop, -FLT_MAX, FLT_MAX);
  RNA_def_property_ui_range(prop, -100.0f, 100.0f, 1, 3);
  RNA_def_property_float_default(prop, 1.0f);
  RNA_def_property_float_funcs(
      prop, nullptr, "rna_RigidBodyCon_motor_lin_target_velocity_set", nullptr);
  RNA_def_property_ui_text(prop, "Target Velocity", "Target linear motor velocity");
  RNA_def_property_update(prop, NC_OBJECT, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "motor_lin_max_impulse", PROP_FLOAT, PROP_NONE);
  RNA_def_property_float_sdna(prop, nullptr, "motor_lin_max_impulse");
  RNA_def_property_range(prop, 0.0f, FLT_MAX);
  RNA_def_property_ui_range(prop, 0.0f, 100.0f, 1, 3);
  RNA_def_property_float_default(prop, 1.0f);
  RNA_def_property_float_funcs(
      prop, nullptr, "rna_RigidBodyCon_motor_lin_max_impulse_set", nullptr);
  RNA_def_property_ui_text(prop, "Max Impulse", "Maximum linear motor impulse");
  RNA_def_property_update(prop, NC_OBJECT, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "motor_ang_target_velocity", PROP_FLOAT, PROP_NONE);
  RNA_def_property_float_sdna(prop, nullptr, "motor_ang_target_velocity");
  RNA_def_property_range(prop, -FLT_MAX, FLT_MAX);
  RNA_def_property_ui_range(prop, -100.0f, 100.0f, 1, 3);
  RNA_def_property_float_default(prop, 1.0f);
  RNA_def_property_float_funcs(
      prop, nullptr, "rna_RigidBodyCon_motor_ang_target_velocity_set", nullptr);
  RNA_def_property_ui_text(prop, "Target Velocity", "Target angular motor velocity");
  RNA_def_property_update(prop, NC_OBJECT, "rna_RigidBodyOb_reset");

  prop = RNA_def_property(srna, "motor_ang_max_impulse", PROP_FLOAT, PROP_NONE);
  RNA_def_property_float_sdna(prop, nullptr, "motor_ang_max_impulse");
  RNA_def_property_range(prop, 0.0f, FLT_MAX);
  RNA_def_property_ui_range(prop, 0.0f, 100.0f, 1, 3);
  RNA_def_property_float_default(prop, 1.0f);
  RNA_def_property_float_funcs(
      prop, nullptr, "rna_RigidBodyCon_motor_ang_max_impulse_set", nullptr);
  RNA_def_property_ui_text(prop, "Max Impulse", "Maximum angular motor impulse");
  RNA_def_property_update(prop, NC_OBJECT, "rna_RigidBodyOb_reset");
}

void RNA_def_rigidbody(BlenderRNA *brna)
{
  rna_def_rigidbody_world(brna);
  rna_def_rigidbody_object(brna);
  rna_def_rigidbody_constraint(brna);
}

#endif
