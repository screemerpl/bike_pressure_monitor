#include "ui.h"
#include "fastlz.h"

uint8_t* _ui_load_binary(char* fname, const uint32_t size)
{
    lv_fs_file_t f;
    lv_fs_res_t res;
    res = lv_fs_open(&f, fname, LV_FS_MODE_RD);
    if (res != LV_FS_RES_OK) return NULL; // file not found
    uint32_t read_num;
    uint8_t* buf = lv_malloc(sizeof(uint8_t) * size);
    res = lv_fs_read(&f, buf, size, &read_num);
    if (res != LV_FS_RES_OK || read_num != size)
    {
        lv_free(buf);
        return NULL;
    }
    lv_fs_close(&f);
    return buf;
}


uint8_t* _ui_load_compressed_binary(char* fname, const uint32_t compsize, const uint32_t size )
{
    uint8_t* zip = _ui_load_binary(fname, compsize);
    if (zip == NULL) return NULL;
    uint8_t* buf = lv_malloc(size);
    size_t outsize=0;
    size_t excepted_size = size;
    fastlz_decompress(zip, compsize, buf, size);
    lv_free(zip);
    return buf;
}
