#include "GL/glu.h"


static float normalizeAngle(float angle)
{
    while (angle < 0)
        angle += 360;

    while (angle >= 360)
        angle -= 360;

    return angle;
}


static void draw_xy_plane(float size)
{
    const int n = 10;

    glBegin(GL_LINES);

    for (int x = -n; x <= n; x++) {
        glVertex3f(x / float(n) * size, -size, 0);
        glVertex3f(x / float(n) * size, size, 0);
    }

    for (int y = -n; y <= n; y++) {
        glVertex3f(-size, y / float(n) * size, 0);
        glVertex3f(size, y / float(n) * size, 0);
    }

    glEnd();
}


static void draw_capped_cylinder(
        GLUquadric *quad, float base_radius, float top_radius,
        float height, float slices, float stacks, float loops
)
{
    glPushMatrix();
    gluCylinder(quad, base_radius, top_radius, height, slices, stacks);

    glRotated(180, 1, 0, 0);

    if (base_radius > 0)
        gluDisk(quad, 0, base_radius, slices, loops);

    glRotated(180, 1, 0, 0);
    glTranslated(0, 0, height);

    if (top_radius > 0)
        gluDisk(quad, 0, top_radius, slices, loops);

    glPopMatrix();
}


static void draw_arrow(float height, float radius)
{
    auto quad = gluNewQuadric();
    gluQuadricNormals(quad, GLU_SMOOTH);

    glPushMatrix();

    draw_capped_cylinder(quad, radius * 0.25, radius * 0.25, height-radius * 2, 16, 8, 2);

    glTranslated(0, 0, height - radius*2);
    draw_capped_cylinder(quad, radius, 0, radius * 2, 16, 4, 4);

    glPopMatrix();

    gluDeleteQuadric(quad);
}


static void draw_big_coordinate_system(float size)
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glPushMatrix();
    glColor3ub(237, 28, 36);
    glRotated(90, 1, 0, 0);
    glRotated(90, 0, 1, 0);
    draw_arrow(size, size * 0.1);
    glPopMatrix();

    glPushMatrix();
    glColor3ub(34, 177, 76);
    glRotated(90, 1, 0, 0);
    glRotated(180, 0, 1, 0);
    draw_arrow(size, size * 0.1);
    glPopMatrix();

    glPushMatrix();
    glColor3ub(0, 162, 232);
    draw_arrow(size, size * 0.1);
    glPopMatrix();

    glPopAttrib();
}


static void draw_coordinate_system(float size)
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glDisable(GL_LIGHTING);

    glBegin(GL_LINES);

    glColor3ub(237, 28, 36);
    glVertex3d(0, 0, 0);
    glVertex3d(size, 0, 0);

    glColor3ub(34, 177, 76);
    glVertex3d(0, 0, 0);
    glVertex3d(0, size, 0);

    glColor3ub(0, 162, 232);
    glVertex3d(0, 0, 0);
    glVertex3d(0, 0, size);
    glEnd();

    glPopAttrib();
}
