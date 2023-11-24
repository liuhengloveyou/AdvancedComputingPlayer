#include "RenderView.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#define SDL_WINDOW_DEFAULT_WIDTH (1280)
#define SDL_WINDOW_DEFAULT_HEIGHT (720)

RenderView::RenderView()
{
    /*
    rect.x = 0;
    rect.y = 0;
    rect.w = SDL_WINDOW_DEFAULT_WIDTH;
    rect.h = SDL_WINDOW_DEFAULT_HEIGHT;
    */

    if (m_nativeHandle)
    {
        m_sdlWindow = SDL_CreateWindowFrom(m_nativeHandle);
    }
    else
    {
        SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        m_sdlWindow = SDL_CreateWindow("AdvancedComputingPlayer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOW_DEFAULT_WIDTH, SDL_WINDOW_DEFAULT_HEIGHT, window_flags);
    }

    if (!m_sdlWindow)
    {
        SDL_LogError(1, "SDL_CreateWindow error");
        exit(-1);
    }

    m_sdlRender = SDL_CreateRenderer(m_sdlWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_sdlRender)
    {
        SDL_LogError(1, "SDL_CreateRenderer error");
        exit(-1);
    }

    SDL_RenderSetLogicalSize(m_sdlRender, SDL_WINDOW_DEFAULT_WIDTH, SDL_WINDOW_DEFAULT_HEIGHT);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    Uint32 pixformat = SDL_PIXELFORMAT_IYUV;
    m_sdlTexture = SDL_CreateTexture(m_sdlRender, pixformat, SDL_TEXTUREACCESS_STREAMING, SDL_WINDOW_DEFAULT_WIDTH, SDL_WINDOW_DEFAULT_HEIGHT);
    if (!m_sdlTexture)
    {
        SDL_LogError(1, "SDL_CreateTexture error");
        exit(-1);
    }

    SDL_RendererInfo info;
    SDL_GetRendererInfo(m_sdlRender, &info);
    SDL_Log("current SDL_Renderer: %s", info.name);
}

void RenderView::setNativeHandle(void *handle)
{
    m_nativeHandle = handle;
}

void RenderView::update(std::shared_ptr<MyPicture> vp)
{
    SDL_UpdateYUVTexture(m_sdlTexture, NULL,
                         vp->frame_->data[0], vp->frame_->linesize[0],
                         vp->frame_->data[1], vp->frame_->linesize[1],
                         vp->frame_->data[2], vp->frame_->linesize[2]);

    /*
    rect.x = 0;
    rect.y = 0;
    rect.w = vp->width_;
    rect.h = vp->height_;
    */

    // SDL_RenderClear(m_sdlRender); // �����Ⱦ������
    SDL_RenderCopy(m_sdlRender, m_sdlTexture, NULL, NULL); // ���������Ƶ���Ⱦ��
    // SDL_RenderPresent(m_sdlRender); // SDL��Ⱦ
}
