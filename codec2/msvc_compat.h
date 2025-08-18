#ifndef MSVC_COMPAT_H
#define MSVC_COMPAT_H

/*
 * MSVC Compatibility Header for Codec2
 * Provides Variable Length Array (VLA) compatibility for MSVC
 */

#ifdef _MSC_VER
// MSVC doesn't support C99 VLAs, use alloca instead
#include <malloc.h>
#define VLA(type, name, size) type *name = (type*)_alloca(sizeof(type) * (size))
#else
// Other compilers support C99 VLAs normally
#define VLA(type, name, size) type name[size]
#endif

#endif // MSVC_COMPAT_H