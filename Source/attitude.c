#include "attitude.h"
#include "util.h"
#include "sensors.h"
#include <stdio.h>


struct dcm dcm = {
    .matrix    = MAT3_IDENTITY,
    //.acc_kp = 1, .acc_ki = 0.0001
    // .mag_kp = 0, .mag_ki = 0
};


static const vec3f down_ref   = { 0, 0, 1 };  // down direction
static const vec3f north_ref  = { 1, 0, 0 };  // north direction


/**
 * Apply an infinitesimal rotation w*dt to a direction cosine matrix A.
 * An orthonormalization step is added to prevent rounding errors.
 *
 * Without the normalization, this would be a simple matrix multiplication:
 *
 * return mat3_matmul( A, (mat3f) {
 *        1,   -w.z,  w.y,
 *      w.z,    1, -w.x,
 *     -w.y,  w.x,    1
 * };
 */
static mat3f dcm_integrate(mat3f A, vec3f w, float dt)
{
    w = vec3f_scale(w, dt);

    // Calculate the new x and y axes. z is calculated later.
    //
    vec3f x = vec3f_matmul(A, (vec3f){    1, w.z, -w.y } );
    vec3f y = vec3f_matmul(A, (vec3f){ -w.z,   1,  w.x } );

    // Orthonormalization
    //
    // First we compute the dot product of the x and y rows of the matrix, which
    // is supposed to be zero, so the result is a measure of how much the X and Y
    // rows are rotating toward each other
    //
    float error = vec3f_dot(x, y);

    // We apportion half of the error each to the x and y rows, and approximately
    // rotate the X and Y rows in the opposite direction by cross coupling:
    //
    vec3f xo, yo, zo;
    xo = vec3f_sub(x, vec3f_scale(y, 0.5 * error));
    yo = vec3f_sub(y, vec3f_scale(x, 0.5 * error));

    // Scale them to unit length and take a cross product for the Z axis
    //
    xo = vec3f_norm(xo);
    yo = vec3f_norm(yo);
    zo = vec3f_cross(xo, yo);

    return (mat3f) {
        xo.x, yo.x, zo.x,
        xo.y, yo.y, zo.y,
        xo.z, yo.z, zo.z
    };
}


void dcm_update(const struct sensor_data *sensor, float dt)
{
    // Apply accelerometer correction
    //
    dcm.down        = vec3f_matmul(dcm.matrix, vec3f_scale(sensor->acc, -1));
    dcm.down_error  = vec3f_cross( vec3f_norm(dcm.down), down_ref );

    // down_error hat jetzt sin(error) in Weltkoordinaten
    // arcsin(x) ~= sin(x) für kleine x

    // Umrechnung in Vehicle-Koordinaten
    //
    dcm.down_error = vec3f_matmul( mat3f_trans(dcm.matrix), dcm.down_error);

    dcm.offset_p = vec3f_zero;
    dcm.offset_p = vec3f_add(dcm.offset_p, vec3f_scale(dcm.down_error, dcm.acc_kp));
    dcm.offset_i = vec3f_add(dcm.offset_i, vec3f_scale(dcm.down_error, dcm.acc_ki));

    // Calculate drift-corrected roll, pitch and yaw angles
    //
    dcm.omega = sensor->gyro;
    dcm.omega = vec3f_add(dcm.omega, dcm.offset_p);
    dcm.omega = vec3f_add(dcm.omega, dcm.offset_i);

    // Apply rotation to the direction cosine matrix
    //
    dcm.matrix = dcm_integrate(dcm.matrix, dcm.omega, dt);
    dcm.euler = mat3f_to_euler(dcm.matrix);
}


void  dcm_reset(void)
{
    dcm.matrix   = mat3f_identity;
    dcm.euler    = vec3f_zero;
    dcm.omega    = vec3f_zero;
    dcm.offset_p = vec3f_zero;
    dcm.offset_i = vec3f_zero;
}

// -----

#include <string.h>
#include "command.h"


static void cmd_dcm_show(void)
{
    struct dcm d;
    memcpy(&d, &dcm, sizeof(d));

    printf("                x/roll    y/pitch      z/yaw\n");
    printf("down      : %10.4f %10.4f %10.4f\n", d.down.x,  d.down.y,  d.down.z );
    printf("down_err  : %10.4f %10.4f %10.4f\n", d.down_error.x,  d.down_error.y,  d.down_error.z );
    printf("\n");
    printf("north     : %10.4f %10.4f %10.4f\n", d.north.x, d.north.y, d.north.z);
    printf("north_err : %10.4f %10.4f %10.4f\n", d.north_error.x, d.north_error.y, d.north_error.z);
    printf("\n");
    printf("offset_p  : %10.4f %10.4f %10.4f\n", d.offset_p.x, d.offset_p.y, d.offset_p.z);
    printf("offset_i  : %10.4f %10.4f %10.4f\n", d.offset_i.x, d.offset_i.y, d.offset_i.z);
    printf("\n");
    printf("dcm_omega : %10.4f %10.4f %10.4f\n", d.omega.x, d.omega.y, d.omega.z);
    printf("dcm_euler : %10.4f %10.4f %10.4f\n", d.euler.x, d.euler.y, d.euler.z);
    printf("\n");
    printf("dcm_matrix: %10.4f %10.4f %10.4f\n", d.matrix.m00, d.matrix.m01, d.matrix.m02);
    printf("            %10.4f %10.4f %10.4f\n", d.matrix.m10, d.matrix.m11, d.matrix.m12);
    printf("            %10.4f %10.4f %10.4f\n", d.matrix.m20, d.matrix.m21, d.matrix.m22);
}

SHELL_CMD(dcm_show, (cmdfunc_t)cmd_dcm_show, "show dcm values")
SHELL_CMD(dcm_reset, (cmdfunc_t)dcm_reset, "reset dcm values")
