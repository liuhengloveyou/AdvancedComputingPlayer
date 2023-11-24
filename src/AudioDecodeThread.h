#pragma once

#include "ThreadBase.h"
#include "PlayerCtx.h"

class AudioDecodeThread : public ThreadBase
{
public:
    AudioDecodeThread(PlayerCtx *ctx);

    void getAudioData(unsigned char *stream, int len);
    void run();

private:
    int audio_decode_frame(PlayerCtx *is, double *pts_ptr);

private:
    PlayerCtx *playerCtx = nullptr;
};
