#pragma once

#include <string>
#include <atomic>
#include <queue>
#include <memory>
#include <mutex>

#include "PacketQueue.h"

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
#include <SDL.h>
}
#endif

#define MAX_AUDIO_FRAME_SIZE 192000

class MyPicture {
public:
    MyPicture() {};
    MyPicture(AVFrame* frame, double pts, int width, int height) {
        frame_ = frame;
        pts_ = pts;
        width_ = width;
        height_ = height;
    }
    ~MyPicture() {
        if (frame_) {
            av_frame_free(&frame_);
            frame_ = nullptr;
        }
    }

public:
    AVFrame* frame_ = nullptr;
    double pts_ = 0.0;
    int width_ = 0;
    int height_ = 0;
};

class PlayerCtx {
public:
    PlayerCtx();
    ~PlayerCtx();

    std::shared_ptr<MyPicture> getPicture();
    void putPicture(std::shared_ptr<MyPicture> picture, std::atomic<bool>& stop);

public:
    AVFormatContext* formatCtx = nullptr;

    AVCodecContext* aCodecCtx = nullptr;
    AVCodecContext* vCodecCtx = nullptr;

    int             videoStream = -1;
    int             audioStream = -1;

    AVStream* audio_st = nullptr;
    AVStream* video_st = nullptr;

    // picture queue
    std::queue<std::shared_ptr<MyPicture>> pictq;
    std::mutex pictq_mutex;
    PacketQueue     audioq;
    PacketQueue     videoq;

    uint8_t         audio_buf[(MAX_AUDIO_FRAME_SIZE * 3) / 2];
    unsigned int    audio_buf_size = 0;
    unsigned int    audio_buf_index = 0;
    AVFrame* audio_frame = nullptr;
    AVPacket* audio_pkt = nullptr;
    uint8_t* audio_pkt_data = nullptr;
    int             audio_pkt_size = 0;

    // seek flags and pos for seek
    std::atomic<int> seek_req;
    int              seek_flags;
    int64_t          seek_pos;

    // flush flag for seek
    std::atomic<bool> flush_audio_ctx; // = false;
    std::atomic<bool> flush_video_ctx; // = false;

    // for sync
    double          audio_clock = 0.0;
    double          frame_timer = 0.0;
    double          frame_last_pts = 0.0;
    double          frame_last_delay = 0.0;
    double          video_clock = 0.0;

    char            filename[1024];

    SwrContext* swr_ctx = nullptr;

    std::atomic<int> pause; // = UNPAUSE;
};
