#pragma once

#include "HappyMath/Vector2.h"
#include <Windows.h>
#include <Xinput.h>
#include <memory>
#include <list>

class XBoxControllerButtonHandler;

/**
 * Provide an interface to an X-box controller.
 */
class XBoxController
{
public:
	XBoxController(DWORD controllerNumber);
	virtual ~XBoxController();

	void Update();
	double GetTrigger(DWORD button);
	HappyMath::Vector2 GetAnalogJoyStick(DWORD button);
	bool WasButtonPressed(DWORD button);
	bool WasButtonReleased(DWORD button);
	bool IsButtonDown(DWORD button);
	bool IsButtonUp(DWORD button);

	void AddButtonHandler(std::weak_ptr<XBoxControllerButtonHandler> buttonHandler);
	void RemoveAllButtonHandlers();

private:
	const XINPUT_STATE* GetCurrentState();
	const XINPUT_STATE* GetPreviousState();

	XINPUT_STATE stateBuffer[2];
	DWORD controllerNumber;
	UINT64 updateCount;

	std::list<std::weak_ptr<XBoxControllerButtonHandler>> buttonHandlerList;
};

/**
 * 
 */
class XBoxControllerButtonHandler
{
public:
	XBoxControllerButtonHandler();
	virtual ~XBoxControllerButtonHandler();

	virtual void OnButtonPressed(DWORD button);
	virtual void OnButtonReleased(DWORD button);
	virtual void OnButtonDown(DWORD button);
	virtual void OnButtonUp(DWORD button);
};