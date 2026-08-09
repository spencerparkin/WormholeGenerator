#include "Main.h"
#include "App.h"
#include <Windows.h>
#include <chrono>

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevIntance, LPSTR cmdLine, int showCmd)
{
	App app(instance);

	if (!app.Setup())
		return 1;

	auto lastTime = std::chrono::steady_clock::now();

	while (true)
	{
		auto currentTime = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsedTime{ currentTime - lastTime };
		lastTime = currentTime;

		if (!app.Run(elapsedTime.count()))
			break;
	}

	if (!app.Shutdown())
		return 1;

	return 0;
}