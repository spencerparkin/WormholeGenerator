#include "Main.h"
#include "WormholeGenerator/WormholeRenderer.h"
#include <SDL3/SDL.h>
#include <glad/gl.h>

int main(int argc, char** argv)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
        return 1;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window* window = SDL_CreateWindow("Wormhole Simulation", 1920, 1080, SDL_WINDOW_OPENGL);
    if (!window)
        return 1;

    //SDL_SetWindowFullscreen(window, true);

    SDL_GLContext context = SDL_GL_CreateContext(window);

    if (!context)
        return 1;

    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress))
        return 1;

    SDL_GL_SetSwapInterval(1);   // Enable VSync

    WormholeGenerator::WormholeRenderer wormholeRenderer;
    if (wormholeRenderer.LoadWormholeData("D:/Misc/Test/test.wormhole"))
    {
        Uint64 lastCount = SDL_GetPerformanceCounter();

        bool keepRunning = true;
        while (keepRunning)
        {
            Uint64 currentCount = SDL_GetPerformanceCounter();

            double deltaTime = static_cast<double>(currentCount - lastCount) / static_cast<double>(SDL_GetPerformanceFrequency());
            lastCount = currentCount;

            SDL_Event event;

            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
                case SDL_EVENT_QUIT:
                {
                    keepRunning = false;
                    break;
                }
                case SDL_EVENT_KEY_DOWN:
                {
                    if (event.key.key == SDLK_ESCAPE)
                        keepRunning = false;

                    break;
                }
                }
            }

            glViewport(0, 0, 1920, 1080);

            glClearColor(0.2f, 0.4f, 0.8f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            wormholeRenderer.Advance(deltaTime);
            wormholeRenderer.Render();

            SDL_GL_SwapWindow(window);
        }

        wormholeRenderer.Clear();
    }

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}