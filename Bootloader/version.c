#include "version.h"
#include "Shared/ansi.h"
#include <stdio.h>
#include <string.h>


const struct version_info version_info = {
    .vcs_info_start = VCS_INFO_START,
    .product_name = "f00b00t",
    .major        = 1,
    .minor        = 4,
    .patch        = 0,
    .vcs_info_end = VCS_INFO_END
};


const struct version_info *find_version_info(const void *buf, size_t size)
{
    for (size_t i = 0; i < size - sizeof(struct version_info); i += 4) {
        const struct version_info *info =
                (const struct version_info *)((uint32_t)buf + i);

        if (!memcmp(info->vcs_info_start, VCS_INFO_START, 16) &&
            !memcmp(info->vcs_info_end,   VCS_INFO_END,   16)  )
            return info;
    }

    return NULL;
}


void print_version_info(const struct version_info *info, int verbose)
{
    printf(
        ANSI_FG_LTGREEN "%s" ANSI_NORMAL " v%d.%d.%d %s\n"
        "Copyright (c)2015 Thomas Kindler <mail@t-kindler.de>\n\n",
        version_info.product_name,
        version_info.major,
        version_info.minor,
        version_info.patch,
        version_info.vcs_id
    );

    if (verbose > 0) {
        printf(
            "  Compiled  %s %s\n"
            "            by %s on %s\n"
            "  Image     %08lx, %lu bytes",
            version_info.build_date, version_info.build_time,
            version_info.build_user, version_info.build_host,
            version_info.image_start, version_info.image_size
        );
    }
}

