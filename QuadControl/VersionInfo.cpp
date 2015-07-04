#include "VersionInfo.h"

volatile const VersionInfo versionInfo
{
    VCS_INFO_START,     // vcs_info_start
    0,                  // image_crc
    0,                  // image_start
    0,                  // image_size
    "",                 // vcs_id
    "",                 // build_user
    "",                 // build_host
    "",                 // build_date
    "",                 // build_time
    "QuadControl",      // product_name
    1,                  // major
    0,                  // minor
    0,                  // patch
    VCS_INFO_END        // vcs_info_end
};
