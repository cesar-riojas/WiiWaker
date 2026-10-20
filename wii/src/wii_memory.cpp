/**
 * WiiWaker - Memory Management (MEM1 + MEM2) & ARAM Translation
 */

#include "wii_memory.h"
#include <cstdlib>
#include <cstring>

#if defined(GEKKO) || defined(__wii__)
#include <gccore.h>
#endif

namespace {

static uint8_t* s_aram_arena = nullptr;
static uint32_t s_aram_size  = 0;
static uint32_t s_aram_used  = 0;

} // namespace

extern "C" {

void wii_memory_init(void) {
#if defined(GEKKO) || defined(__wii__)
    // Standard libogc initialization takes care of basic heap setup
#endif
    // Allocate 24MB ARAM equivalent directly out of Wii's 64MB MEM2
    wii_aram_init(24 * 1024 * 1024);
}

void* wii_mem1_alloc(size_t size) {
#if defined(GEKKO) || defined(__wii__)
    size = (size + 31) & ~31;
    void* lo = SYS_GetArena1Lo();
    void* newLo = (uint8_t*)lo + size;
    if (newLo > SYS_GetArena1Hi()) return nullptr;
    SYS_SetArena1Lo(newLo);
    return lo;
#else
    return std::malloc(size);
#endif
}

void wii_mem1_free(void* ptr) {
#if defined(GEKKO) || defined(__wii__)
    // Arena1 bump allocation
#else
    std::free(ptr);
#endif
}

void* wii_mem2_alloc(size_t size) {
#if defined(GEKKO) || defined(__wii__)
    size = (size + 31) & ~31;
    void* lo = SYS_GetArena2Lo();
    void* newLo = (uint8_t*)lo + size;
    if (newLo > SYS_GetArena2Hi()) return nullptr;
    SYS_SetArena2Lo(newLo);
    return lo;
#else
    return std::malloc(size);
#endif
}

void wii_mem2_free(void* ptr) {
#if defined(GEKKO) || defined(__wii__)
    // Arena2 bump allocation
#else
    std::free(ptr);
#endif
}

void wii_aram_init(uint32_t aram_size_bytes) {
    s_aram_size = aram_size_bytes;
    s_aram_used = 0;
    s_aram_arena = (uint8_t*)wii_mem2_alloc(aram_size_bytes);
    if (s_aram_arena) {
        std::memset(s_aram_arena, 0, aram_size_bytes);
    }
}

void* wii_aram_alloc(uint32_t size) {
    if (!s_aram_arena) return nullptr;
    // Align to 32 bytes (cache line)
    uint32_t aligned_size = (size + 31) & ~31;
    if (s_aram_used + aligned_size > s_aram_size) {
        return nullptr; // Out of ARAM pool
    }
    void* ptr = s_aram_arena + s_aram_used;
    s_aram_used += aligned_size;
    return ptr;
}

void wii_aram_dma_read(void* dst_main_ram, uint32_t src_aram_addr, uint32_t length) {
    if (!s_aram_arena || !dst_main_ram) return;
    std::memcpy(dst_main_ram, s_aram_arena + src_aram_addr, length);
#if defined(GEKKO) || defined(__wii__)
    DCFlushRange(dst_main_ram, length);
#endif
}

void wii_aram_dma_write(uint32_t dst_aram_addr, const void* src_main_ram, uint32_t length) {
    if (!s_aram_arena || !src_main_ram) return;
    std::memcpy(s_aram_arena + dst_aram_addr, src_main_ram, length);
#if defined(GEKKO) || defined(__wii__)
    DCFlushRange(s_aram_arena + dst_aram_addr, length);
#endif
}

} // extern "C"
