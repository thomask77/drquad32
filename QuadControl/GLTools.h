#pragma once

#include <QOpenGLFunctions_2_1>

class GLTools : protected QOpenGLFunctions_2_1
{
    void drawTeapot(int grid, double scale, int type);

public:
    GLTools();
    float normalizeAngle(float angle);

    void drawXyPlane(float size);
    void drawCoordinateSystem(float size);

    void drawDisk(double inner, double outer, int slices);
    void drawCylinder(double baseRadius, double topRadius, double height, int slices);

    void drawCappedCylinder(
        float baseRadius, float topRadius, float height, float slices
    );

    void drawArrow(float height, float radius);
    void drawBigCoordinateSystem(float size);
    void drawWireTeapot(double scale);
    void drawSolidTeapot(double scale);
};
