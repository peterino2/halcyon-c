#ifndef _HALC_TYPES_H_
#define _HALC_TYPES_H_

#ifdef __cplusplus
#define EXTERN_C_BEGIN extern "C" {
#define EXTERN_C_END }
#else 
#define EXTERN_C_BEGIN
#define EXTERN_C_END
#endif

typedef char b8;
typedef char i8;
typedef unsigned char u8;

#define TRUE 1
#define FALSE 0

#ifndef NULL
#define NULL 0 
#endif

typedef int i32;
typedef unsigned int u32;

#ifdef __linux__

typedef long i64;
typedef long isize;
typedef unsigned long u64;
typedef unsigned long usize;

#elif __APPLE__

typedef long long i64;
typedef long isize;
typedef unsigned long long u64;
typedef unsigned long usize;

#elif _WIN32

typedef long long i64;
typedef long long isize;
typedef unsigned long long u64;
typedef unsigned long long usize;

#endif

#ifndef __func__
#define __func__ __FUNCTION__
#endif

#endif
