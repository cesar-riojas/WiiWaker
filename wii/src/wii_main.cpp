/**
 * WiiWaker - Main Wii Entry Point & Hardware Initialization
 */

#include "wii_main.h"
#include "wii_memory.h"
#include "wii_storage.h"
#include "wii_wpad.h"

#include <cstdio>
#include <cstdlib>

#if defined(GEKKO) || defined(__wii__)
#include <gccore.h>
#endif

namespace {

#if defined(GEKKO) || defined(__wii__)
static void* s_xfb[2] = { nullptr, nullptr };
static int s_fb_index = 0;
static GXRModeObj* s_rmode = nullptr;
#endif

} // namespace

extern "C" {

int wiiwaker_init(void) {
#if defined(GEKKO) || defined(__wii__)
    // 1. Initialize Video subsystem
    VIDEO_Init();

    // Prefer 480p Progressive Component (EDTV/HDTV), fallback to standard NTSC/PAL
    s_rmode = VIDEO_GetPreferredMode(nullptr);
    if (CONF_GetProgressiveScan() > 0 && s_rmode->viTVMode & VI_NON_INTERLACE) {
        s_rmode = &TVNtsc480Prog;
    }

    // Allocate double framebuffers in fast MEM1
    s_xfb[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(s_rmode));
    s_xfb[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(s_rmode));

    VIDEO_Configure(s_rmode);
    VIDEO_SetNextFramebuffer(s_xfb[0]);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (s_rmode->viTVMode & VI_NON_INTERLACE) {
        VIDEO_WaitVSync();
    }

    // 2. Initialize Audio
    AUDIO_Init(nullptr);

    // 3. Initialize Memory (MEM1 + MEM2 + ARAM emulation pool)
    wii_memory_init();

    // 4. Initialize Storage (FAT32 SD / USB)
    wii_storage_init();

    // 5. Initialize Controllers (Wii Remote + Nunchuk + MotionPlus)
    wii_wpad_init();

#endif
    return 0;
}

void wiiwaker_present(void) {
#if defined(GEKKO) || defined(__wii__)
    s_fb_index ^= 1;
    VIDEO_SetNextFramebuffer(s_xfb[s_fb_index]);
    VIDEO_Flush();
    VIDEO_WaitVSync();

    // Poll inputs
    wii_wpad_update();
#endif
}

void wiiwaker_exit(void) {
#if defined(GEKKO) || defined(__wii__)
    exit(0);
#endif
}

} // extern "C"

int main(int argc, char** argv) {
    wiiwaker_init();

    while (1) {
        wiiwaker_present();
    }

    wiiwaker_exit();
    return 0;
}
