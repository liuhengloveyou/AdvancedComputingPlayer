#pragma once

#include <string>

#include "ThreadBase.h"
#include "PlayerCtx.h"


class DemuxThread : public ThreadBase
{
public:
    DemuxThread(PlayerCtx *ctx);
    ~DemuxThread();
    
    int init();
    void run();

private:
    int stream_open(int media_type);

private:
    PlayerCtx *playerCtx = nullptr;
};

