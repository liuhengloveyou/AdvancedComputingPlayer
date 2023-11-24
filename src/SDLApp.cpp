#include <functional>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#include "SDLApp.h"
#include "SDL.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/wasm_worker.h>
#include "emscripten/emscripten_mainloop_stub.h"
using namespace emscripten;
#endif

#define SDL_APP_EVENT_TIMEOUT (1)

static SDLApp *globalInstance = nullptr;

SDLApp::SDLApp()
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) != 0)
    {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        exit(-1);
    }

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    if (!globalInstance)
    {
        globalInstance = this;
    }
    else
    {
        fprintf(stderr, "only one instance allowed\n");
        exit(-1);
    }
}

int SDLApp::exec()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    // while (SDL_WaitEventTimeout(&event, SDL_APP_EVENT_TIMEOUT))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        switch (event.type)
        {
        case SDL_QUIT:
            //SDL_Quit();
            return -1;
        case SDL_USEREVENT:
        {
            std::function<void()> cb = *(std::function<void()> *)event.user.data1;
            cb();
        }
        break;
        default:
            auto iter = m_userEventMaps.find(event.type);
            if (iter != m_userEventMaps.end())
            {
                auto onEventCb = iter->second;
                onEventCb(&event);
            }
            break;
        }
    }

    return 0;
}

void SDLApp::quit()
{
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
}

void SDLApp::registerEvent(int type, const std::function<void(SDL_Event *)> &cb)
{
    m_userEventMaps[type] = cb;
}

SDLApp *SDLApp::instance()
{
    return globalInstance;
}
