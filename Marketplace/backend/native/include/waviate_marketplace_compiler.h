#pragma once

#include <stddef.h>

#if defined(_WIN32) && defined(WAVIATESCRIPT_MARKETPLACE_COMPILER_DLL)
#if defined(WAVIATESCRIPT_MARKETPLACE_COMPILER_BUILDING_DLL)
#define WAVIATE_MARKETPLACE_COMPILER_API __declspec(dllexport)
#else
#define WAVIATE_MARKETPLACE_COMPILER_API __declspec(dllimport)
#endif
#else
#define WAVIATE_MARKETPLACE_COMPILER_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

WAVIATE_MARKETPLACE_COMPILER_API const char* waviate_marketplace_compiler_version(void);

WAVIATE_MARKETPLACE_COMPILER_API int waviate_marketplace_compile_wlsl(
    const char* source,
    char* diagnostics,
    size_t diagnostics_size);

#ifdef __cplusplus
}
#endif

#undef WAVIATE_MARKETPLACE_COMPILER_API
