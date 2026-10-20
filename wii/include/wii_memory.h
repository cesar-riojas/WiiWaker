#ifndef WII_MEMORY_H
#define WII_MEMORY_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize Wii MEM1 (24MB) and MEM2 (64MB) arenas
void wii_memory_init(void);

// Allocate from MEM1 (fast 1T-SRAM, 24MB, critical rendering & cache)
void* wii_mem1_alloc(size_t size);
void wii_mem1_free(void* ptr);

// Allocate from MEM2 (fast GDDR3, 64MB, textures, stages, audio buffers)
void* wii_mem2_alloc(size_t size);
void wii_mem2_free(void* ptr);

// Legacy GameCube ARAM Emulation via MEM2 arena
void  wii_aram_init(uint32_t aram_size_bytes);
void* wii_aram_alloc(uint32_t size);
void  wii_aram_dma_read(void* dst_main_ram, uint32_t src_aram_addr, uint32_t length);
void  wii_aram_dma_write(uint32_t dst_aram_addr, const void* src_main_ram, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif // WII_MEMORY_H
