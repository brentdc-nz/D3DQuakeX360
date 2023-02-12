#ifndef XBINPUT_H
#define XBINPUT_H

#include <xtl.h>

#define	K_XBOX_START	207
#define	K_XBOX_BACK		208

#define	K_XBOX_A		209
#define	K_XBOX_B		210
#define	K_XBOX_X		211
#define	K_XBOX_Y		212

#define	K_XBOX_UP		213
#define	K_XBOX_DOWN		214
#define	K_XBOX_LEFT		215
#define	K_XBOX_RIGHT	216

#define	K_XBOX_LTRIG	217
#define	K_XBOX_RTRIG	218

#ifdef __cplusplus
extern "C" {
#endif

WORD GamepadUpdate();
int GamepadLTrigerDown();
int GamepadRTrigerDown();
int GamepadThumbLY();
int GamepadThumbLX();
int GamepadThumbRY();
int GamepadThumbRX();

#ifdef __cplusplus
}
#endif

#endif //XBINPUT_H