#ifndef WII_STORAGE_H
#define WII_STORAGE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize FAT32 filesystem (SD card / USB)
bool wii_storage_init(void);

// Read file directly from SD/USB
void* wii_file_load(const char* relative_path, size_t* out_size);

// Replacement for legacy DVDRead calls
int32_t wii_dvd_read_async(void* dst, uint32_t length, uint32_t offset);

#ifdef __cplusplus
}
#endif

#endif // WII_STORAGE_H
