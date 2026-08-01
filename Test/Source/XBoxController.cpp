#include "XBoxController.h"

//----------------------------------- XBoxController -----------------------------------

XBoxController::XBoxController(DWORD controllerNumber)
{
	::memset(this->stateBuffer, 0, sizeof(this->stateBuffer));
	this->controllerNumber = controllerNumber;
	this->updateCount = 0;
}

/*virtual*/ XBoxController::~XBoxController()
{
}

void XBoxController::Update()
{
	DWORD result = XInputGetState(this->controllerNumber, &this->stateBuffer[this->updateCount % 2]);
	if (result != ERROR_SUCCESS)
		return;

	if (++this->updateCount < 2)
		return;

	DWORD button = 1;
	do
	{
		std::list<std::weak_ptr<XBoxControllerButtonHandler>>::iterator iter = this->buttonHandlerList.begin();
		while(iter != this->buttonHandlerList.end())
		{
			auto nextIter = iter;
			nextIter++;

			std::shared_ptr<XBoxControllerButtonHandler> buttonHandler = (*iter).lock();
			if (!buttonHandler.get())
			{
				this->buttonHandlerList.erase(iter);
			}
			else
			{

				if (this->IsButtonDown(button))
					buttonHandler->OnButtonDown(button);
				if (this->IsButtonUp(button))
					buttonHandler->OnButtonUp(button);
				if (this->WasButtonPressed(button))
					buttonHandler->OnButtonPressed(button);
				if (this->WasButtonReleased(button))
					buttonHandler->OnButtonReleased(button);
			}

			iter = nextIter;
		}

		button <<= 1;
	} while (button != 0x00010000);
}

void XBoxController::AddButtonHandler(std::weak_ptr<XBoxControllerButtonHandler> buttonHandler)
{
	this->buttonHandlerList.push_back(buttonHandler);
}

void XBoxController::RemoveAllButtonHandlers()
{
	this->buttonHandlerList.clear();
}

double XBoxController::GetTrigger(DWORD button)
{
	const XINPUT_STATE* currentState = this->GetCurrentState();

	BYTE trigger = 0;

	switch (button)
	{
	case XINPUT_GAMEPAD_LEFT_THUMB:
	case XINPUT_GAMEPAD_LEFT_SHOULDER:
		trigger = currentState->Gamepad.bLeftTrigger;
		break;
	case XINPUT_GAMEPAD_RIGHT_THUMB:
	case XINPUT_GAMEPAD_RIGHT_SHOULDER:
		trigger = currentState->Gamepad.bRightTrigger;
		break;
	}

	if (trigger < XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
		return 0.0;

	return double(trigger) / 255.0;
}

HappyMath::Vector2 XBoxController::GetAnalogJoyStick(DWORD button)
{
	const XINPUT_STATE* currentState = this->GetCurrentState();

	int thumbX = 0, thumbY = 0;
	int deadZone = 0;

	switch (button)
	{
	case XINPUT_GAMEPAD_LEFT_THUMB:
		thumbX = currentState->Gamepad.sThumbLX;
		thumbY = currentState->Gamepad.sThumbLY;
		deadZone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
		break;
	case XINPUT_GAMEPAD_RIGHT_THUMB:
		thumbX = currentState->Gamepad.sThumbRX;
		thumbY = currentState->Gamepad.sThumbRY;
		deadZone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
		break;
	}

	if (::abs(thumbX) < deadZone)
		thumbX = 0;
	if (::abs(thumbY) < deadZone)
		thumbY = 0;

	return HappyMath::Vector2(double(thumbX), double(thumbY)) / 32768.0;
}

bool XBoxController::WasButtonPressed(DWORD button)
{
	const XINPUT_STATE* currentState = this->GetCurrentState();
	const XINPUT_STATE* previousState = this->GetPreviousState();
	return (previousState->Gamepad.wButtons & button) == 0 && (currentState->Gamepad.wButtons & button) != 0;
}

bool XBoxController::WasButtonReleased(DWORD button)
{
	const XINPUT_STATE* currentState = this->GetCurrentState();
	const XINPUT_STATE* previousState = this->GetPreviousState();
	return (previousState->Gamepad.wButtons & button) != 0 && (currentState->Gamepad.wButtons & button) == 0;
}

bool XBoxController::IsButtonDown(DWORD button)
{
	const XINPUT_STATE* currentState = this->GetCurrentState();
	return (currentState->Gamepad.wButtons & button) != 0;
}

bool XBoxController::IsButtonUp(DWORD button)
{
	const XINPUT_STATE* currentState = this->GetCurrentState();
	return (currentState->Gamepad.wButtons & button) == 0;
}

const XINPUT_STATE* XBoxController::GetCurrentState()
{
	return &this->stateBuffer[(this->updateCount + 1) % 2];
}

const XINPUT_STATE* XBoxController::GetPreviousState()
{
	return &this->stateBuffer[this->updateCount % 2];
}

//----------------------------------- XBoxControllerButtonHandler -----------------------------------

XBoxControllerButtonHandler::XBoxControllerButtonHandler()
{
}

/*virtual*/ XBoxControllerButtonHandler::~XBoxControllerButtonHandler()
{
}

/*virtual*/ void XBoxControllerButtonHandler::OnButtonPressed(DWORD button)
{
}

/*virtual*/ void XBoxControllerButtonHandler::OnButtonReleased(DWORD button)
{
}

/*virtual*/ void XBoxControllerButtonHandler::OnButtonDown(DWORD button)
{
}

/*virtual*/ void XBoxControllerButtonHandler::OnButtonUp(DWORD button)
{
}