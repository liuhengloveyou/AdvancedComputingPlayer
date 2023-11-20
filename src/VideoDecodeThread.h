#ifndef VIDEODECODETHREAD_H
#define VIDEODECODETHREAD_H

#include "ThreadBase.h"

struct FFmpegPlayerCtx;
struct AVFrame;

class VideoDecodeThread : public ThreadBase
{
public:
    VideoDecodeThread(FFmpegPlayerCtx *ctx);

    void run();

private:
    int video_entry();
    int queue_picture(AVFrame *pFrame, double pts);

private:
    FFmpegPlayerCtx *playerCtx = nullptr;
};

#endif // VIDEODECODETHREAD_H
