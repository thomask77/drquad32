#pragma once

#include "GL/glu.h"
#include <QOpenGLFunctions_2_1>

class GLTools : protected QOpenGLFunctions_2_1
{
    void drawCappedCylinder(
        GLUquadric *quad, float base_radius, float top_radius,
        float height, float slices, float stacks, float loops
    );

    void drawTeapot(int grid, double scale, int type);

public:
    GLTools();
    float normalizeAngle(float angle);
    void drawXyPlane(float size);
    void drawArrow(float height, float radius);
    void drawBigCoordinateSystem(float size);
    void drawCoordinateSystem(float size);
    void drawWireTeapot(double scale);
    void drawSolidTeapot(double scale);
};
