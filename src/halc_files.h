#ifndef _HALC_FILES_H_
#define _HALC_FILES_H_

#include <stdio.h>

#include "halc_errors.h"
#include "halc_strings.h"

EXTERN_C_BEGIN

// load file into an hstr struct
errc load_file(hstr* out, const hstr* filePath);
errc load_and_decode_from_file(hstr* out, const hstr* filePath);

EXTERN_C_END

#endif
