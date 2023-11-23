#pragma once

#include "ThreadBase.h"
#include "PlayerCtx.h"

class VideoDecodeThread : public ThreadBase
{
public:
    VideoDecodeThread(PlayerCtx *ctx);

    void run();

private:
    double synchronize_video(PlayerCtx* playerCtx, AVFrame* src_frame, double pts);

private:
    PlayerCtx *playerCtx = nullptr;
};
