#include "VideoDecodeThread.h"

#include "FFmpegPlayer.h"
#include "log.h"

VideoDecodeThread::VideoDecodeThread(PlayerCtx *ctx)
    :playerCtx(ctx) 
{
}

void VideoDecodeThread::run()
{
    int ret = -1;
    double pts = 0;
    AVPacket* packet = av_packet_alloc();

    while (!m_stop) {
        if (playerCtx->pause == PAUSE) {
            SDL_Delay(5);
            continue;
        }

        if (playerCtx->flush_video_ctx) {
            ff_log_line("avcodec_flush_buffers(vCodecCtx) for seeking");
            avcodec_flush_buffers(playerCtx->vCodecCtx);
            playerCtx->flush_video_ctx = false;
            continue;
        }

        av_packet_unref(packet);
        if (playerCtx->videoq.packetGet(packet, m_stop) < 0) {
            break;
        }

        // Decode video frame
        ret = avcodec_send_packet(playerCtx->vCodecCtx, packet);
        if (ret < 0) {
            fprintf(stderr, "avcodec_send_packet Error\n");
            exit(1);
        }
        while (ret >= 0) {
            AVFrame* pFrame = av_frame_alloc();
            if (!pFrame) {
                ff_log_line("av_frame_alloc error");
                return;
            }

            ret = avcodec_receive_frame(playerCtx->vCodecCtx, pFrame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                fprintf(stderr, "Error during decoding\n");
                exit(1);
            }

            if (packet->dts == AV_NOPTS_VALUE && pFrame->opaque && *(uint64_t*)pFrame->opaque != AV_NOPTS_VALUE) {
                pts = (double)*(uint64_t*)pFrame->opaque;
            } else if (packet->dts != AV_NOPTS_VALUE) {
                pts = (double)packet->dts;
            } else {
                pts = 0;
            }
            pts *= av_q2d(playerCtx->video_st->time_base);

            // frame ready
            if (ret == 0) {
                pts = synchronize_video(playerCtx, pFrame, pts);
                auto ptr = std::make_shared<MyPicture>(pFrame, pts, playerCtx->vCodecCtx->width, playerCtx->vCodecCtx->height);
                playerCtx->putPicture(ptr, m_stop);
            }
        }
    }

    //av_frame_free(&pFrame); // ÓÃÍêÔÙfree
    av_packet_free(&packet);

    return;
}

double VideoDecodeThread::synchronize_video(PlayerCtx* playerCtx, AVFrame* src_frame, double pts)
{
    double frame_delay;

    if (pts != 0) {
        // if we have pts, set video clock to it
        playerCtx->video_clock = pts;
    }
    else {
        // if we aren't given a pts, set it to the clock
        pts = playerCtx->video_clock;
    }
    // update the video clock
    frame_delay = av_q2d(playerCtx->vCodecCtx->time_base);
    // if we are repeating a frame, adjust clock accordingly
    frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
    playerCtx->video_clock += frame_delay;

    return pts;
}