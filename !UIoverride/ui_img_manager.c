#include "ui.h"
#include "fastlz.h"
#include "esp_log.h"

static const char* TAG = "ui_img_manager";

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
    ESP_LOGI(TAG, "Decompressing image %s: compsize=%u -> size=%u", fname, (unsigned int)compsize, (unsigned int)size);
    if (buf != NULL)
        ESP_LOGI(TAG, "Allocated buffer at %p", (void*)buf);
        else
        ESP_LOGI(TAG, "Failed to allocate buffer");
    size_t outsize=0;
    size_t excepted_size = size;
    fastlz_decompress(zip, compsize, buf, size);
    lv_free(zip);
    return buf;
}
