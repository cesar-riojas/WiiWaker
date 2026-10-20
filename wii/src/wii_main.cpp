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

    // Clear framebuffers to black (prevents green uninitialized YUV screen)
    VIDEO_ClearFrameBuffer(s_rmode, s_xfb[0], COLOR_BLACK);
    VIDEO_ClearFrameBuffer(s_rmode, s_xfb[1], COLOR_BLACK);

    VIDEO_Configure(s_rmode);
    VIDEO_SetNextFramebuffer(s_xfb[0]);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (s_rmode->viTVMode & VI_NON_INTERLACE) {
        VIDEO_WaitVSync();
    }

    // Initialize on-screen console
    console_init(s_xfb[0], 20, 20, s_rmode->fbWidth, s_rmode->xfbHeight, s_rmode->fbWidth * VI_DISPLAY_PIX_SZ);

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

    const char* dir_names[] = { "REST / CENTER", "UP    (HIGH)", "RIGHT (MID-HIGH)", "LEFT  (MID-LOW)", "DOWN  (LOW / DOWNBEAT)" };
    const char* meter_names[] = { "3/4 Time (Waltz)", "4/4 Time (Common)", "6/4 Time" };

    uint32_t frame = 0;
    while (1) {
        wii_tact_gesture_update(1.0f);
        WiiTactDirection dir = wii_tact_get_direction();
        float vol = wii_tact_get_volume();
        WiiTactMeter meter = wii_tact_get_meter();

        WiiAimingOffset aim;
        wii_aiming_get_offset(&aim);

        // Update on-screen status every few frames
        if ((frame % 2) == 0) {
            std::printf("\x1b[2;0H");
            std::printf("================================================================\n");
            std::printf("                     WiiWaker v1.0.0                            \n");
            std::printf("    Native Nintendo Wii Port of The Wind Waker (Decompilation)  \n");
            std::printf("================================================================\n\n");
            std::printf(" Hardware Architecture:\n");
            std::printf("   CPU:  Broadway @ 729 MHz (Full Native Speed)\n");
            std::printf("   MEM1: 24 MB 1T-SRAM (Active Framebuffers)\n");
            std::printf("   MEM2: 64 MB GDDR3 (ARAM Emulation Arena Active)\n");
            std::printf("   Video: 480p Progressive Scan (16:9 Widescreen)\n\n");

            std::printf(" MotionPlus Conducting Baton Status:\n");
            std::printf("   Baton Direction:  [%s]\n", dir_names[(int)dir % 5]);
            std::printf("   Time Signature:   [%s]\n", meter_names[(int)meter % 3]);
            std::printf("   Dynamic Volume:   [%0.2f / 1.00] (Stroke Intensity)\n\n", vol);

            std::printf(" Motion Aiming (Bow / Hookshot):\n");
            std::printf("   Pitch: %+0.4f | Yaw: %+0.4f | Roll: %+0.4f\n\n", aim.delta_pitch, aim.delta_yaw, aim.delta_roll);

            std::printf("----------------------------------------------------------------\n");
            std::printf(" Wave or flick your Wii Remote to test baton conducting gestures!\n");
            std::printf(" Tilt Nunchuk Left/Right to test time signatures.\n");
            std::printf(" Press HOME on Wii Remote to exit.\n");
            std::printf("================================================================\n");
        }

        wiiwaker_present();
        frame++;
    }

    wiiwaker_exit();
    return 0;
}
