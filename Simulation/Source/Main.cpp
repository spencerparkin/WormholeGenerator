#include "Main.h"
#include "App.h"

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int showCmd)
{
	App app(instance);
	App::Set(&app);

	if (app.Initialize())
	{
		while (app.Run())
		{
		}
	}

	app.Shutdown();

	return 0;
}