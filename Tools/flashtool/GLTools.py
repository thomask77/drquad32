from OpenGL.GL import *
from OpenGL.GLU import *


def draw_xy_plane(size):
    glBegin(GL_LINES)

    n = 10
    for x in range(-n, n+1):
        glVertex3f(x / float(n) * size, -size, 0)
        glVertex3f(x / float(n) * size, size, 0)

    for y in range(-n, n+1):
        glVertex3f(-size, y / float(n) * size, 0)
        glVertex3f(size, y / float(n) * size, 0)

    glEnd()


def draw_capped_cylinder(quad, base_radius, top_radius, height, slices, stacks, loops):
    glPushMatrix()
    gluCylinder(quad, base_radius, top_radius, height, slices, stacks)

    glRotated(180, 1, 0, 0)

    if base_radius > 0:
        gluDisk(quad, 0, base_radius, slices, loops)

    glRotated(180, 1, 0, 0)
    glTranslated(0, 0, height)

    if top_radius > 0:
        gluDisk(quad, 0, top_radius, slices, loops)

    glPopMatrix()


def draw_arrow(height, radius):
    quad = gluNewQuadric()
    gluQuadricNormals(quad, GLU_SMOOTH)

    glPushMatrix()

    draw_capped_cylinder(quad, radius * 0.25, radius * 0.25, height-radius * 2, 16, 8, 2)

    glTranslated(0, 0, height - radius*2)
    draw_capped_cylinder(quad, radius, 0, radius * 2, 16, 4, 4)

    glPopMatrix()

    gluDeleteQuadric(quad)


def draw_big_coordinate_system(size):
    glPushAttrib(GL_ALL_ATTRIB_BITS)

    glPushMatrix()
    glColor3ub(237, 28, 36)
    glRotated(90, 1, 0, 0)
    glRotated(90, 0, 1, 0)
    draw_arrow(size, size * 0.1)
    glPopMatrix()

    glPushMatrix()
    glColor3ub(34, 177, 76)
    glRotated(90, 1, 0, 0)
    glRotated(180, 0, 1, 0)
    draw_arrow(size, size * 0.1)
    glPopMatrix()

    glPushMatrix()
    glColor3ub(0, 162, 232)
    draw_arrow(size, size * 0.1)
    glPopMatrix()

    glPopAttrib()


def draw_coordinate_system(size):
    # TODO: Bug PyOpenGL?
    # glPushAttrib(GL_ALL_ATTRIB_BITS)
    # print GL_ALL_ATTRIB_BITS   # -2147483647
    glPushAttrib(0xFFFFFFFF)

    glDisable(GL_LIGHTING)

    glBegin(GL_LINES)

    glColor3ub(237, 28, 36)
    glVertex3d(0, 0, 0)
    glVertex3d(size, 0, 0)

    glColor3ub(34, 177, 76)
    glVertex3d(0, 0, 0)
    glVertex3d(0, size, 0)

    glColor3ub(0, 162, 232)
    glVertex3d(0, 0, 0)
    glVertex3d(0, 0, size)
    glEnd()

    glPopAttrib()
