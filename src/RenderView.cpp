#include "RenderView.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#define SDL_WINDOW_DEFAULT_WIDTH  (1280)
#define SDL_WINDOW_DEFAULT_HEIGHT (720)

static SDL_Rect makeRect(int x, int y, int w, int h)
{
    SDL_Rect r;
    r.x = x;
    r.y = y;
    r.w = w;
    r.h = h;

    return r;
}

RenderView::RenderView()
{

}

void RenderView::setNativeHandle(void *handle)
{
    m_nativeHandle = handle;
}

int RenderView::initSDL(SDL_Window** window, SDL_Renderer** renderer)
{
    if (m_nativeHandle) {
        m_sdlWindow = SDL_CreateWindowFrom(m_nativeHandle);
    } else {
        SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        m_sdlWindow = SDL_CreateWindow("AdvancedComputingPlayer",
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOW_DEFAULT_WIDTH,
                                       SDL_WINDOW_DEFAULT_HEIGHT,
                                       window_flags);
    }

    if (!m_sdlWindow) {
        return -1;
    }

    m_sdlRender = SDL_CreateRenderer(m_sdlWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_sdlRender) {
        return -1;
    }

    SDL_RenderSetLogicalSize(m_sdlRender, SDL_WINDOW_DEFAULT_WIDTH, SDL_WINDOW_DEFAULT_HEIGHT);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    *window = m_sdlWindow;
    *renderer = m_sdlRender;

    return 0;
}

RenderItem *RenderView::createRGB24Texture(int w, int h)
{
    m_updateMutex.lock();

    /*
     pixformat = SDL_PIXELFORMAT_IYUV;

    texture = SDL_CreateTexture(renderer, pixformat, SDL_TEXTUREACCESS_STREAMING, w, h);

    if (!texture)

    {

        printf("can not create texture: %s\n", SDL_GetError());

        return -1;

    }

    */
    RenderItem *ret = new RenderItem;
    SDL_Texture *tex = SDL_CreateTexture(m_sdlRender, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, w, h);
    ret->texture = tex;
    ret->srcRect = makeRect(0, 0, w, h);
    ret->dstRect = makeRect(0, 0, SDL_WINDOW_DEFAULT_WIDTH, SDL_WINDOW_DEFAULT_HEIGHT);

    m_items.push_back(ret);

    m_updateMutex.unlock();

    return ret;
}

void RenderView::updateTexture(RenderItem *item, unsigned char *pixelData, int rows)
{
    printf("RenderView::updateTexture>>>>>>");
    /*
    
     // SDL Ë¢ĞÂ

        SDL_UpdateYUVTexture(texture, NULL,

                             frame->data[0], frame->linesize[0],

                             frame->data[1], frame->linesize[1],

                             frame->data[2], frame->linesize[2]);

        rect.x = 0;

        rect.y = 0;

        rect.w = width;

        rect.h = height;



        SDL_RenderClear(renderer);                      // Çå¿ÕäÖÈ¾Æ÷ÄÚÈİ

        SDL_RenderCopy(renderer, texture, NULL, &rect); // ½«ÎÆÀí¸´ÖÆµ½äÖÈ¾Æ÷

        SDL_RenderPresent(renderer);

    */
    m_updateMutex.lock();

    void *pixels = nullptr;
    int pitch;
    SDL_LockTexture(item->texture, NULL, &pixels, &pitch);
    memcpy(pixels, pixelData, pitch * rows);
    SDL_UnlockTexture(item->texture);

    std::list<RenderItem *>::iterator iter;
    SDL_RenderClear(m_sdlRender);
    for (iter = m_items.begin(); iter != m_items.end(); iter++)
    {
        RenderItem *item = *iter;
        SDL_RenderCopy(m_sdlRender, item->texture, &item->srcRect, &item->dstRect);
    }

    m_updateMutex.unlock();
}

void RenderView::onRefresh()
{
    printf("RenderView::onRefresh>>>>>>");
    m_updateMutex.lock();

    if (m_sdlRender) {
        SDL_RenderPresent(m_sdlRender);
    }

    m_updateMutex.unlock();
}
