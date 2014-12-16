#pragma once

#include <stdbool.h>

extern const struct version_info {
    const char *product_name;

    int   major;
    int   minor;
    int   patch;
    const char *string;

    const char *git_version;
    const char *build_user;
    const char *build_host;
    const char *build_date;
    const char *build_time;
} version_info;

void print_version_info(int verbose);

