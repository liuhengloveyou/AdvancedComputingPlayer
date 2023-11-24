#pragma once

#include <string>
#include <functional>
#include <atomic>
#include <queue>

#include "DemuxThread.h"
#include "VideoDecodeThread.h"
#include "AudioDecodeThread.h"
#include "AudioPlay.h"
#include "PlayerCtx.h"
#include "PacketQueue.h"
#include "RenderView.h"
#include "log.h"

class RenderView;

enum PauseState {
    UNPAUSE = 0,
    PAUSE = 1
};

#define FF_BASE_EVENT   (SDL_USEREVENT + 100)
#define FF_REFRESH_EVENT (FF_BASE_EVENT + 20)
#define FF_QUIT_EVENT    (FF_BASE_EVENT + 30)

#define AV_SYNC_THRESHOLD 0.01
#define AV_NOSYNC_THRESHOLD 10.0

#define SDL_AUDIO_BUFFER_SIZE (1024)

class FFmpegPlayer
{
public:
    FFmpegPlayer(RenderView *render);
    ~FFmpegPlayer();

    int init(const char *filePath);
    void start();
    void stop();
    void pause(PauseState state);

public:
    void refresh();
    void onKeyEvent(SDL_Event *e);

private:
    PlayerCtx playerCtx;
    std::string m_filePath;
    SDL_AudioSpec audio_wanted_spec;
    std::atomic<bool> m_stop;

private:
    DemuxThread *m_demuxThread = nullptr;
    VideoDecodeThread *m_videoDecodeThread = nullptr;
    AudioDecodeThread *m_audioDecodeThread = nullptr;
    AudioPlay *m_audioPlay = nullptr;
    RenderView *m_render = nullptr;
};
