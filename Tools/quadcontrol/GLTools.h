#pragma once

#include "GL/glu.h"

namespace GLTools {

float normalizeAngle(float angle);
void draw_xy_plane(float size);
void draw_capped_cylinder(
        GLUquadric *quad, float base_radius, float top_radius,
        float height, float slices, float stacks, float loops
);
void draw_arrow(float height, float radius);
void draw_big_coordinate_system(float size);
void draw_coordinate_system(float size);

}
