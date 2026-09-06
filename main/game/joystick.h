// BlinkenSisters - Hunt for the Lost Pixels
//     Bringing back the fun of the 80s
//
// (C) 2005-07 Rene Schickbauer, Wolfgang Dautermann
//
// See License.txt for licensing information
//


#ifndef JOYSTICK_H
#define JOYSTICK_H

// Pull in Uint32 type (either from globals.h or pal_types.h directly)
#ifndef GLOBALS_H
#include "pal/pal_types.h"
#endif

typedef enum _JOYSTICK_MOVE {
	JOYSTICK_NONE = 0,
	JOYSTICK_LEFT = 1,
	JOYSTICK_RIGHT = 2,
	JOYSTICK_UP = 4,
	JOYSTICK_DOWN = 8,
	JOYSTICK_JUMP = 16,
	JOYSTICK_ACTION = 32,
	JOYSTICK_TURBO = 64,
	JOYSTICK_PAUSE = 128
} JOYSTICK_MOVE;

#ifdef STD_USB_JOY_PAD
#define USB_JOY_NAME "Standard USB Joy Pad"
typedef enum _USB_JOY {
	USB_JOY_LEFT = 0,
	USB_JOY_UP = 0,
	USB_JOY_RIGHT = 0,
	USB_JOY_DOWN = 0,
	USB_JOY_JUMP = 1,
	USB_JOY_ACTION = 2,
	USB_JOY_PAUSE = 6,
	USB_JOY_TURBO = 7
	} USB_JOY;
#define USB_JOY_LOWER -1000
#define USB_JOY_UPPER  1000
#endif // STD_USB_JOY_PAD

#ifdef TRUSTMASTER_FIRESTORM_JOY_PAD
#define USB_JOY_NAME "Firestorm Wireless Gamepad"
typedef enum _USB_JOY {
	USB_JOY_LEFT = 11,
	USB_JOY_UP = 7,
	USB_JOY_RIGHT = 11,
	USB_JOY_DOWN = 11,
	USB_JOY_JUMP = 0,
	USB_JOY_ACTION = 2,
	USB_JOY_PAUSE = 9,
	USB_JOY_TURBO = 1
} USB_JOY;
#define USB_JOY_LOWER -20
#define USB_JOY_UPPER  20
#endif //TRUSTMASTER_FIRESTORM_JOY_PAD

#ifdef TRUSTMASTER_WIRELESS_JOY_PAD
#define USB_JOY_NAME "Thrustmaster T Mini Wireless"
typedef enum _USB_JOY {
	USB_JOY_LEFT = 11,
	USB_JOY_UP = 7,
	USB_JOY_RIGHT = 11,
	USB_JOY_DOWN = 11,
	USB_JOY_JUMP = 0,
	USB_JOY_ACTION = 5,
	USB_JOY_PAUSE = 9,
	USB_JOY_TURBO = 3
} USB_JOY;
#define USB_JOY_LOWER 125
#define USB_JOY_UPPER  129
#endif //TRUSTMASTER_WIRELESS_JOY_PAD

#ifdef TRUST_USB_STEERING_WHEEL
#define USB_JOY_NAME "Trust F1 Steering Wheel"
typedef enum _USB_JOY {
	USB_JOY_LEFT = 0,
	USB_JOY_UP = 1,
	USB_JOY_RIGHT = 2,
	USB_JOY_DOWN = 3,
	USB_JOY_JUMP = 7,
	USB_JOY_ACTION = 10,
	USB_JOY_PAUSE = 6,
	USB_JOY_TURBO = 8
} USB_JOY;
#define USB_JOY_LOWER 125
#define USB_JOY_UPPER  129
#endif //TRUST_USB_STEERING_WHEEL
void initJoystick();
void deInitJoystick();
Uint32 getJoystickMoves();

/* Buttons released since the last call (JOYSTICK_* bitmask), cleared on read.
   Menus use this so an action fires once, on release: acting on the press left
   the matching release queued for the screen the action opened, which then ate
   it as its own input. */
Uint32 getJoystickReleases();

/* Drop queued key events and pending release edges (use when changing screen). */
void flushJoystick();

#endif // JOYSTICK_H
