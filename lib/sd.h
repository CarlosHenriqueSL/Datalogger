#ifndef SD_H
#define SD_H

#include "ff.h"        
#include "sd_card.h" 

size_t sd_get_num();
sd_card_t *sd_get_by_num(size_t num);
size_t spi_get_num();
spi_t *spi_get_by_num(size_t num);
static sd_card_t *sd_get_by_name(const char *const name);
static FATFS *sd_get_fs_by_name(const char *name);
void run_setrtc();
void run_format();
void run_mount();
void run_unmount();
void run_getfree();
void run_ls();
void run_cat();
void capture_data_and_save();
void read_file(const char *filename);
void run_help();
void process_stdio(int cRxedChar);

#endif