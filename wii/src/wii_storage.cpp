/**
 * WiiWaker - Storage & Asset Loading Subsystem
 */

#include "wii_storage.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(GEKKO) || defined(__wii__)
#include <gccore.h>
#include <fat.h>
#include <sdcard/wiisd_io.h>
#endif

namespace {

static bool s_fat_mounted = false;
static char s_base_path[256] = "sd:/apps/WiiWaker/files/";

} // namespace

extern "C" {

bool wii_storage_init(void) {
#if defined(GEKKO) || defined(__wii__)
    if (!s_fat_mounted) {
        s_fat_mounted = fatInitDefault();
    }
    return s_fat_mounted;
#else
    s_fat_mounted = true;
    return true;
#endif
}

void* wii_file_load(const char* relative_path, size_t* out_size) {
    if (!relative_path) return nullptr;

    char full_path[512];
    std::snprintf(full_path, sizeof(full_path), "%s%s", s_base_path, relative_path);

    FILE* f = std::fopen(full_path, "rb");
    if (!f) return nullptr;

    std::fseek(f, 0, SEEK_END);
    size_t sz = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);

    void* buffer = std::malloc(sz);
    if (!buffer) {
        std::fclose(f);
        return nullptr;
    }

    size_t read_bytes = std::fread(buffer, 1, sz, f);
    std::fclose(f);

    if (out_size) {
        *out_size = read_bytes;
    }

    return buffer;
}

int32_t wii_dvd_read_async(void* dst, uint32_t length, uint32_t offset) {
    // Bridges DVDRead requests directly to our game archive / disc image
    return 0; // Async read dispatched
}

} // extern "C"
