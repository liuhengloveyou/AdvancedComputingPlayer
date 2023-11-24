#pragma once

#include <SDL.h>
#include <list>
#include <mutex>

#include "FFmpegPlayer.h"

/*
#ifdef __cplusplus
extern "C" {
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/time.h>
#include <libavutil/imgutils.h>
}
#endif
*/

class RenderView
{
public:
    RenderView();
    ~RenderView() {};

    void setNativeHandle(void *handle);
    void update(std::shared_ptr<MyPicture> vp);

    SDL_Window* getWindow() { return m_sdlWindow; };
    SDL_Renderer* getRender() { return m_sdlRender; };

private:
    SDL_Window* m_sdlWindow = nullptr;
    SDL_Renderer* m_sdlRender = nullptr;
    SDL_Texture* m_sdlTexture = nullptr;
    // SDL_Rect rect;

    void* m_nativeHandle = nullptr;
};
