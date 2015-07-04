#pragma once

#include <stdint.h>
#include <stdlib.h>

#define VCS_INFO_START "VCSINFO2_START->"
#define VCS_INFO_END   "<---VCSINFO2_END"


struct version_info {
    char        vcs_info_start[16];

    // set by add-version-info.py
    //
    uint32_t    image_crc;
    uint32_t    image_start;
    uint32_t    image_size;

    char        vcs_id[32];
    char        build_user[16];
    char        build_host[16];
    char        build_date[16];
    char        build_time[16];

    // set at compile-time
    //
    char        product_name[32];
    int         major;
    int         minor;
    int         patch;

    char        vcs_info_end[16];
};

extern const struct version_info  version_info;

const struct version_info *find_version_info(const void *buf, size_t size);
void print_version_info(const struct version_info *info, int verbose);
