#include <memory>
#include <mutex>

#include "PlayerCtx.h"


PlayerCtx::PlayerCtx() {
    audio_frame = av_frame_alloc();
    audio_pkt = av_packet_alloc();
}

PlayerCtx::~PlayerCtx() {
    if (audio_frame) {
        av_frame_free(&audio_frame);
    }

    if (audio_pkt) {
        av_packet_free(&audio_pkt);
    }

    if (formatCtx) {
        avformat_close_input(&formatCtx);
        formatCtx = nullptr;
    }

    if (aCodecCtx) {
        avcodec_free_context(&aCodecCtx);
        aCodecCtx = nullptr;
    }

    if (vCodecCtx) {
        avcodec_free_context(&vCodecCtx);
        vCodecCtx = nullptr;
    }

    if (swr_ctx) {
        swr_free(&swr_ctx);
        swr_ctx = nullptr;
    }
}

std::shared_ptr<MyPicture> PlayerCtx::getPicture() {
    std::shared_ptr<MyPicture> tmp = nullptr;

    {
        std::lock_guard<std::mutex> lck(pictq_mutex);

        if (!pictq.empty() || pictq.size() > 0) {
            tmp = pictq.front();
            pictq.pop();
        }
    }

    return tmp;
}

void PlayerCtx::putPicture(std::shared_ptr<MyPicture> picture, std::atomic<bool>& stop) {
    while (true) {
        if (stop) {
            return;
        }

        {
            std::lock_guard<std::mutex> lck(pictq_mutex);
            if (pictq.size() < 1000) {
                break;
            }
        }

        av_usleep(3);
    }

    std::lock_guard<std::mutex> lck(pictq_mutex);
    pictq.push(picture);
    // printf(">>>putPicture:: %ld\n", pictq.size());

    return;
}