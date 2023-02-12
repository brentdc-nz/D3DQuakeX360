#include "in_xbox.h"
#include "xb_input.h"

void IN_UpdateThumbStickAxes(usercmd_t *cmd, float rawvalue, int iAxis)
{
	float value;
	float svalue;
	float speed, aspeed;

	if(iAxis && /*invert_look.value*/0 > 0) // TODO!!!
	rawvalue -= rawvalue *2;

	// Convert value
	value = (rawvalue / 128.0);
	svalue = value * 3.5/*rthumb_sensitivity.value*/; // TODO!!!

	// Handle +speed
	if (in_speed.state & 1)
	{
		speed = cl_movespeedkey.value;
	}
	else
	{
		speed = 1;
		aspeed = speed * host_frametime;
	}

	if(fabs(value) > 0.15)
	{
		if (/*invert_look.value*/0 > 0) // TODO!!!
		{
			cl.viewangles[iAxis] += svalue * aspeed * 0.15;
		}
		else
		{
			cl.viewangles[iAxis] -= svalue * aspeed * 0.15;
		}
	}

	// bounds check pitch
	if (cl.viewangles[PITCH] > 80.0)
		cl.viewangles[PITCH] = 80.0;

	if (cl.viewangles[PITCH] < -70.0)
		cl.viewangles[PITCH] = -70.0;
}

void IN_XboxMove(usercmd_t *cmd)
{
	GamepadUpdate();

	// Left thumb stick
	cmd->forwardmove = GamepadThumbLY();
	cmd->sidemove = GamepadThumbLX();

	// Right thumb stick
	IN_UpdateThumbStickAxes(cmd, GamepadThumbRY(), PITCH);
	IN_UpdateThumbStickAxes(cmd, GamepadThumbRX(), YAW);
}