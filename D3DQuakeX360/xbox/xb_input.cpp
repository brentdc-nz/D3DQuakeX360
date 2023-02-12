#include "xb_input.h"
#include "AtgInput.h"

ATG::GAMEPAD* pGamepad = NULL;

WORD GamepadUpdate()
{
	WORD wButtonsReturn = 0;

	pGamepad = ATG::Input::GetMergedInput();

//	if(!pGamepad)
//		return 0;

	if(pGamepad->wButtons == XINPUT_GAMEPAD_START)
		wButtonsReturn = K_XBOX_START;

	if(pGamepad->wButtons == XINPUT_GAMEPAD_BACK)
		wButtonsReturn = K_XBOX_BACK;

	if(pGamepad->wButtons == XINPUT_GAMEPAD_A)
		wButtonsReturn = K_XBOX_A;

	if(pGamepad->wButtons == XINPUT_GAMEPAD_B)
		wButtonsReturn = K_XBOX_B;

	if(pGamepad->wButtons == XINPUT_GAMEPAD_X)
		wButtonsReturn = K_XBOX_X;

	if(pGamepad->wButtons == XINPUT_GAMEPAD_Y)
		wButtonsReturn = K_XBOX_Y;

	if(pGamepad->wButtons == XINPUT_GAMEPAD_DPAD_UP)
		wButtonsReturn = K_XBOX_UP;

	if(pGamepad->wButtons == XINPUT_GAMEPAD_DPAD_DOWN)
		wButtonsReturn = K_XBOX_DOWN;

	if(pGamepad->wButtons == XINPUT_GAMEPAD_DPAD_LEFT)
		wButtonsReturn = K_XBOX_LEFT;

	if(pGamepad->wButtons == XINPUT_GAMEPAD_DPAD_RIGHT)
		wButtonsReturn = K_XBOX_RIGHT;

	if(pGamepad->bLeftTrigger)
		wButtonsReturn = K_XBOX_LTRIG;

	if(pGamepad->bRightTrigger)
		wButtonsReturn = K_XBOX_RTRIG;

	return wButtonsReturn;
}

int GamepadThumbLY()
{
//	if(pGamepad)
		return pGamepad->sThumbLY;
}

int GamepadThumbLX()
{
//	if(pGamepad)
		return pGamepad->sThumbLX;
}

int GamepadThumbRY()
{
//	if(pGamepad)
		return pGamepad->sThumbRY;
}

int GamepadThumbRX()
{
//	if(pGamepad)
		return pGamepad->sThumbRX;
}