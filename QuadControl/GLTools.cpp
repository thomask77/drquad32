/**
 * Copyright (C)2015 Thomas Kindler <mail_drquad@t-kindler.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "GLTools.h"
#include <cmath>


GLTools::GLTools()
{
    initializeOpenGLFunctions();
}


float GLTools::normalizeAngle(float angle)
{
    while (angle < 0)
        angle += 360;

    while (angle >= 360)
        angle -= 360;

    return angle;
}


void GLTools::drawXyPlane(float size)
{
    const int n = 10;

    glBegin(GL_LINES);
    glNormal3d(0, 0, 1);

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


void GLTools::drawCoordinateSystem(float size)
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


void GLTools::drawCylinder(double baseRadius, double topRadius, double height, int slices)
{
    double alpha  = atan((topRadius - baseRadius) / height);
    double cos_alpha = cos(alpha);
    double sin_alpha = sin(alpha);

    glBegin(GL_TRIANGLE_STRIP);

    for (int s=0; s<slices + 1; s++) {
        double beta = (s % slices) * 2*M_PI / slices;
        double x1 = cos(beta);
        double y1 = sin(beta);

        glNormal3d(cos_alpha * x1, cos_alpha * y1, -sin_alpha);
        glVertex3d(x1 * topRadius,  y1 * topRadius,  height);
        glVertex3d(x1 * baseRadius, y1 * baseRadius, 0);
    }

    glEnd();
}


void GLTools::drawDisk(double inner, double outer, int slices)
{
    drawCylinder(outer, inner, 0, slices);
}


void GLTools::drawCappedCylinder(float baseRadius, float topRadius, float height, float slices)
{
    glPushMatrix();
    drawCylinder(baseRadius, topRadius, height, slices);

    glRotated(180, 1, 0, 0);

    if (baseRadius > 0)
        drawDisk(0, baseRadius, slices);

    glRotated(180, 1, 0, 0);
    glTranslated(0, 0, height);

    if (topRadius > 0)
        drawDisk(0, topRadius, slices);

    glPopMatrix();
}


void GLTools::drawArrow(float height, float radius)
{
//    auto quad = gluNewQuadric();
//    gluQuadricNormals(quad, GLU_SMOOTH);

    glPushMatrix();

    drawCappedCylinder(radius * 0.25, radius * 0.25, height-radius * 2, 16);

    glTranslated(0, 0, height - radius*2);
    drawCappedCylinder(radius, 0, radius * 2, 16);

    glPopMatrix();

//    gluDeleteQuadric(quad);
}


void GLTools::drawBigCoordinateSystem(float size)
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glPushMatrix();
    glColor3ub(237, 28, 36);
    glRotated(90, 1, 0, 0);
    glRotated(90, 0, 1, 0);
    drawArrow(size, size * 0.1);
    glPopMatrix();

    glPushMatrix();
    glColor3ub(34, 177, 76);
    glRotated(90, 1, 0, 0);
    glRotated(180, 0, 1, 0);
    drawArrow(size, size * 0.1);
    glPopMatrix();

    glPushMatrix();
    glColor3ub(0, 162, 232);
    drawArrow(size, size * 0.1);
    glPopMatrix();

    glPopAttrib();
}


/**
 *  The classic Utah teapot
 *
 *  Copyright (c) Mark J. Kilgard, 1994.
 *  (c) Copyright 1993, Silicon Graphics, Inc.
 *
 *  ALL RIGHTS RESERVED
 *
 *  Permission to use, copy, modify, and distribute this software
 *  for any purpose and without fee is hereby granted, provided
 *  that the above copyright notice appear in all copies and that
 *  both the copyright notice and this permission notice appear in
 *  supporting documentation, and that the name of Silicon
 *  Graphics, Inc. not be used in advertising or publicity
 *  pertaining to distribution of the software without specific,
 *  written prior permission.
 */

static const int patchdata[][16] = {
    // rim
    {102, 103, 104, 105, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    // body
    {12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27},
    {24, 25, 26, 27, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40},
    // lid
    {96, 96, 96, 96, 97, 98, 99, 100, 101, 101, 101, 101, 0, 1, 2, 3,},
    {0, 1, 2, 3, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117},
    // bottom
    {118, 118, 118, 118, 124, 122, 119, 121, 123, 126, 125, 120, 40, 39, 38, 37},
    // handle
    {41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56},
    {53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 28, 65, 66, 67},
    // spout
    {68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83},
    {80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95}
};


static const float cpdata[][3] = {
    {0.2, 0, 2.7}, {0.2, -0.112, 2.7}, {0.112, -0.2, 2.7},
    {0,-0.2, 2.7}, {1.3375, 0, 2.53125}, {1.3375, -0.749, 2.53125}, {0.749, -1.3375, 2.53125},
    {0, -1.3375, 2.53125}, {1.4375, 0, 2.53125}, {1.4375, -0.805, 2.53125}, {0.805, -1.4375, 2.53125},
    {0, -1.4375, 2.53125}, {1.5, 0, 2.4}, {1.5, -0.84, 2.4}, {0.84, -1.5, 2.4},
    {0, -1.5, 2.4}, {1.75, 0, 1.875}, {1.75, -0.98, 1.875}, {0.98, -1.75, 1.875},
    {0, -1.75, 1.875}, {2, 0, 1.35}, {2, -1.12, 1.35}, {1.12, -2, 1.35},
    {0, -2, 1.35}, {2, 0, 0.9}, {2, -1.12, 0.9}, {1.12, -2, 0.9},
    {0, -2, 0.9}, {-2, 0, 0.9}, {2, 0, 0.45}, {2, -1.12, 0.45},
    {1.12, -2, 0.45}, {0, -2, 0.45}, {1.5, 0, 0.225}, {1.5, -0.84, 0.225},
    {0.84, -1.5, 0.225}, {0, -1.5, 0.225}, {1.5, 0, 0.15}, {1.5, -0.84, 0.15},
    {0.84, -1.5, 0.15}, {0, -1.5, 0.15}, {-1.6, 0, 2.025}, {-1.6, -0.3, 2.025},
    {-1.5, -0.3, 2.25}, {-1.5, 0, 2.25}, {-2.3, 0, 2.025}, {-2.3, -0.3, 2.025},
    {-2.5, -0.3, 2.25}, {-2.5, 0, 2.25}, {-2.7, 0, 2.025}, {-2.7, -0.3, 2.025},
    {-3, -0.3, 2.25}, {-3, 0, 2.25}, {-2.7, 0, 1.8}, {-2.7, -0.3, 1.8},
    {-3, -0.3, 1.8}, {-3, 0, 1.8}, {-2.7, 0, 1.575}, {-2.7, -0.3, 1.575},
    {-3, -0.3, 1.35}, {-3, 0, 1.35}, {-2.5, 0, 1.125}, {-2.5, -0.3, 1.125},
    {-2.65, -0.3, 0.9375}, {-2.65, 0, 0.9375}, {-2, -0.3, 0.9}, {-1.9, -0.3, 0.6},
    {-1.9, 0, 0.6}, {1.7, 0, 1.425}, {1.7, -0.66, 1.425}, {1.7, -0.66, 0.6},
    {1.7, 0, 0.6}, {2.6, 0, 1.425}, {2.6, -0.66, 1.425}, {3.1, -0.66, 0.825},
    {3.1, 0, 0.825}, {2.3, 0, 2.1}, {2.3, -0.25, 2.1}, {2.4, -0.25, 2.025},
    {2.4, 0, 2.025}, {2.7, 0, 2.4}, {2.7, -0.25, 2.4}, {3.3, -0.25, 2.4},
    {3.3, 0, 2.4}, {2.8, 0, 2.475}, {2.8, -0.25, 2.475}, {3.525, -0.25, 2.49375},
    {3.525, 0, 2.49375}, {2.9, 0, 2.475}, {2.9, -0.15, 2.475}, {3.45, -0.15, 2.5125},
    {3.45, 0, 2.5125}, {2.8, 0, 2.4}, {2.8, -0.15, 2.4}, {3.2, -0.15, 2.4},
    {3.2, 0, 2.4}, {0, 0, 3.15}, {0.8, 0, 3.15}, {0.8, -0.45, 3.15},
    {0.45, -0.8, 3.15}, {0, -0.8, 3.15}, {0, 0, 2.85}, {1.4, 0, 2.4},
    {1.4, -0.784, 2.4}, {0.784, -1.4, 2.4}, {0, -1.4, 2.4}, {0.4, 0, 2.55},
    {0.4, -0.224, 2.55}, {0.224, -0.4, 2.55}, {0, -0.4, 2.55}, {1.3, 0, 2.55},
    {1.3, -0.728, 2.55}, {0.728, -1.3, 2.55}, {0, -1.3, 2.55}, {1.3, 0, 2.4},
    {1.3, -0.728, 2.4},  {0.728, -1.3, 2.4}, {0, -1.3, 2.4}, {0, 0, 0},
    {1.425, -0.798, 0}, {1.5, 0, 0.075}, {1.425, 0, 0}, {0.798, -1.425, 0},
    {0, -1.5, 0.075}, {0, -1.425, 0}, {1.5, -0.84, 0.075}, {0.84, -1.5, 0.075}
};


static const float tex[2][2][2] = {
    { {0, 0}, {1, 0} },
    { {0, 1}, {1, 1} }
};


void GLTools::drawTeapot(int grid, double scale, int type)
{
    glPushAttrib(GL_ENABLE_BIT | GL_EVAL_BIT);
    glEnable(GL_AUTO_NORMAL);
    glEnable(GL_NORMALIZE);
    glEnable(GL_MAP2_VERTEX_3);
    glEnable(GL_MAP2_TEXTURE_COORD_2);
    glPushMatrix();
    glRotatef(270.0, 1.0, 0.0, 0.0);
    glScalef(0.5 * scale, 0.5 * scale, 0.5 * scale);
    glTranslatef(0.0, 0.0, -1.5);

    for (int i = 0; i < 10; i++) {
        float p[4][4][3], q[4][4][3], r[4][4][3], s[4][4][3];

        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                for (int l = 0; l < 3; l++) {
                    p[j][k][l] = cpdata[patchdata[i][j * 4 + k]][l];
                    q[j][k][l] = cpdata[patchdata[i][j * 4 + (3 - k)]][l];

                    if (l == 1)
                        q[j][k][l] *= -1.0;

                    if (i < 6) {
                        r[j][k][l] =
                                cpdata[patchdata[i][j * 4 + (3 - k)]][l];
                        if (l == 0)
                            r[j][k][l] *= -1.0;
                        s[j][k][l] = cpdata[patchdata[i][j * 4 + k]][l];
                        if (l == 0)
                            s[j][k][l] *= -1.0;
                        if (l == 1)
                            s[j][k][l] *= -1.0;
                    }
                }
            }
        }

        glMap2f(GL_MAP2_TEXTURE_COORD_2, 0, 1, 2, 2, 0, 1, 4, 2, &tex[0][0][0]);
        glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4, &p[0][0][0]);
        glMapGrid2f(grid, 0.0, 1.0, grid, 0.0, 1.0);
        glEvalMesh2(type, 0, grid, 0, grid);
        glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4, &q[0][0][0]);
        glEvalMesh2(type, 0, grid, 0, grid);

        if (i < 6) {
            glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4, &r[0][0][0]);
            glEvalMesh2(type, 0, grid, 0, grid);
            glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4, &s[0][0][0]);
            glEvalMesh2(type, 0, grid, 0, grid);
        }
    }

    glPopMatrix();
    glPopAttrib();
}


void GLTools::drawWireTeapot(double scale)
{
    drawTeapot(10, scale, GL_LINE);
}


void GLTools::drawSolidTeapot(double scale)
{
    drawTeapot(14, scale, GL_FILL);
}
