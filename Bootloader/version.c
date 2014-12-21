#include "version.h"
#include "git_version.h"
#include "stm32f4xx.h"

#define PRODUCT_NAME    "f00b00t"
#define VERSION_MAJOR   1
#define VERSION_MINOR   0
#define VERSION_PATCH   0

// -----

#define STRINGIFY_(x)       #x
#define STRINGIFY(x)        STRINGIFY_(x)


const struct version_info version_info = {
    .product_name = PRODUCT_NAME,
    .major        = VERSION_MAJOR,
    .minor        = VERSION_MINOR,
    .patch        = VERSION_PATCH,
    .string       = "v" STRINGIFY(VERSION_MAJOR) "."
                        STRINGIFY(VERSION_MINOR) "."
                        STRINGIFY(VERSION_PATCH),
    .git_version  = GIT_VERSION,
    .build_user   = BUILD_USER,
    .build_host   = BUILD_HOST,
    .build_date   = BUILD_DATE,
    .build_time   = BUILD_TIME
};

