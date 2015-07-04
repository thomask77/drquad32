#ifndef VERSIONINFO_H
#define VERSIONINFO_H

#include <stdint.h>

#define VCS_INFO_START  { 0x49534356, 0x324F464E, 0x4154535F, 0x3E2D5452 }
#define VCS_INFO_END    { 0x2D2D2D3C, 0x49534356, 0x324F464E, 0x444E455F }


struct VersionInfo
{
    uint32_t    vcs_info_start[4];

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

    uint32_t    vcs_info_end[4];
};

extern volatile const VersionInfo  versionInfo;

#endif
