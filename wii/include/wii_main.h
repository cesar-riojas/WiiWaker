#ifndef WII_MAIN_H
#define WII_MAIN_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Boot and initialize WiiWaker hardware
int wiiwaker_init(void);

// Frame swap and vsync
void wiiwaker_present(void);

// Exit back to Homebrew Channel
void wiiwaker_exit(void);

#ifdef __cplusplus
}
#endif

#endif // WII_MAIN_H
