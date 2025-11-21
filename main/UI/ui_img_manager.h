#ifndef _UI_IMG_MANAGER_H
#define _UI_IMG_MANAGER_H

uint8_t* _ui_load_binary(char* fname, const uint32_t size);
uint8_t* _ui_load_compressed_binary(char* fname, const uint32_t compsize, const uint32_t size );

#define UI_LOAD_IMAGE _ui_load_compressed_binary

// On-demand image loading functions
void ui_load_splash_images(void);
void ui_load_main_images(void);

// Image memory cleanup
void ui_free_splash_images(void);

#endif
