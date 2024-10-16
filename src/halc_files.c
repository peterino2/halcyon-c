#include <inttypes.h>

#include "halc_files.h"
#include "halc_allocators.h"
#include "halc_strings.h"

FILE* h_fopen(const char* filePath, const char* opts) 
{
    FILE* file;
#if defined(_WIN32) || defined(_WIN64)
    errno_t err = fopen_s(&file, filePath, opts);
    if (err != 0) {
        return NULL;
    }
#else
    file = fopen(filePath, opts);
#endif

    return file;
}

errc load_file(hstr* out, const hstr* filePath)
{
    errc error_code = ERR_OK; 

    FILE* file = h_fopen(filePath->buffer, "rb");
    if(!file)
    {
        fprintf(stderr, "\n  Unable to open file: %s\n", filePath->buffer);
        halc_raise(ERR_UNABLE_TO_OPEN_FILE);
    }

    // seek for the size of the file
    if(fseek(file, 0, SEEK_END) != 0)
    {
        error_code = ERR_FILE_SEEK_ERROR;
        goto exitCloseFile;
    }

    isize fileSize = ftell(file);
    if(fileSize == -1)
    {
        error_code = ERR_FILE_SEEK_ERROR;
        goto exitCloseFile;
    }

    char* buffer;
    halloc(&buffer, fileSize);

    if(fseek(file, 0, SEEK_SET) != 0)
    {
        error_code = ERR_FILE_SEEK_ERROR;
        goto exitCloseFile;
    }

    usize bytesRead = fread(buffer, 1, fileSize, file);
    if(bytesRead != fileSize)
    {
        fprintf(stderr,
            "read file but didn't get the right size? expected %" PRId64 " got %" PRId64, 
            (i64)fileSize, (i64)bytesRead);
    }

    out->buffer = buffer;
    out->len = (u32) bytesRead;
    out->cap = (u32) fileSize;

    fclose(file);
    halc_end;
    
exitCloseFile:
    fclose(file);
    halc_end;
}

errc load_and_decode_from_file(hstr* out, const hstr* filePath)
{
    hstr file;
    halc_try(load_file(&file, filePath));
    halc_try(hstr_normalize(&file, out));

    hstr_free(&file);
    halc_end;
}
