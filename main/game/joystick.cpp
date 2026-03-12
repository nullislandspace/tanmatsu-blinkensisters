// Tanmatsu port: joystick.cpp replaced by PAL input
#include "joystick.h"
#include "globals.h"
#include "pal/pal_input.h"

void initJoystick() {
    // PAL input is already initialized in main.cpp via PAL_InputInit()
}

void deInitJoystick() {
    // Nothing to do
}

Uint32 getJoystickMoves() {
    return PAL_GetJoystickMoves();
}
