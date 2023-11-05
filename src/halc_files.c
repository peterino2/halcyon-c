#include "halc_files.h"
#include "halc_allocators.h"

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

errc loadFile(hstr* out, const hstr* filePath)
{
    errc error_code = ERR_OK; 

    FILE* file = h_fopen(filePath->buffer, "rb");
    if(!file)
    {
        fprintf(stderr, "\n  Unable to open file: %s", filePath->buffer);
        herror(ERR_UNABLE_TO_OPEN_FILE);
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
    try(halloc(&buffer, fileSize));

    if(fseek(file, 0, SEEK_SET) != 0)
    {
        error_code = ERR_FILE_SEEK_ERROR;
        goto exitCloseFile;
    }

    usize bytesRead = fread(buffer, 1, fileSize, file);
    if(bytesRead != fileSize)
    {
        fprintf(stderr,
            "read file but didn't get the right size? expected %lld got %lld", 
            fileSize, bytesRead);
    }

    out->buffer = buffer;
    out->len = bytesRead;

    fclose(file);
    ok;
    
exitCloseFile:
    fclose(file);
    herror(error_code);
    ok;
}

errc loadAndDecodeFromFile(hstr* out, const hstr* filePath)
{
    hstr file;
    try(loadFile(&file, filePath));
    try(hstr_decodeUtf8(&file, out));

    hstr_free(&file);
    ok;
}