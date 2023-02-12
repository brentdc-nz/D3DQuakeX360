#ifndef CSYSINPUTXBOX_H
#define CSYSINPUTXBOX_H

#include "..\quakedef.h"

void IN_XboxMove(usercmd_t *cmd);
static short filter_axis(short axis, int deadzone);

#endif CSYSINPUTXBOX_H