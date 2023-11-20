#ifndef DEMUXTHREAD_H
#define DEMUXTHREAD_H

#include "ThreadBase.h"

#include <string>

struct FFmpegPlayerCtx;

class DemuxThread : public ThreadBase
{
public:
    DemuxThread(FFmpegPlayerCtx *ctx);
    ~DemuxThread();
    
    int init();
    void run();

private:
    int stream_open(int media_type);

private:
    FFmpegPlayerCtx *playerCtx = nullptr;
};

#endif // DEMUXTHREAD_H
