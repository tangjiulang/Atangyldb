#ifndef __COMMON_VERSION_H__
#define __COMMON_VERSION_H__
namespace common {

#ifndef MAIJOR_VER
#define MAIJOR_VER 1
#endif

#ifndef MINOR_VER
#define MINOR_VER 0
#endif

#ifndef PATCH_VER
#define PATCH_VER 0
#endif

#ifndef OTHER_VER
#define OTHER_VER 1
#endif

#define STR1(R) #R
#define STR2(R) STR1(R)

#define VERSION_STR (STR2(MAIJOR_VER) "." STR2(MINOR_VER) "." STR2(PATCH_VER) "." STR2(OTHER_VER))
#define VERSION_NUM (MAIJOR_VER << 24 | MINOR_VER << 16 | PATCH_VER << 8 | OTHER_VER)

}

#endif