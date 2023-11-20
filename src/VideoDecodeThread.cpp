#include "VideoDecodeThread.h"

#include "FFmpegPlayer.h"
#include "log.h"

static double synchronize_video(FFmpegPlayerCtx *is, AVFrame *src_frame, double pts)
{
    double frame_delay;

    if(pts != 0) {
        // if we have pts, set video clock to it
        is->video_clock = pts;
    } else {
        // if we aren't given a pts, set it to the clock
        pts = is->video_clock;
    }
    // update the video clock
    frame_delay = av_q2d(is->vCodecCtx->time_base);
    // if we are repeating a frame, adjust clock accordingly
    frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
    is->video_clock += frame_delay;

    return pts;
}

VideoDecodeThread::VideoDecodeThread(FFmpegPlayerCtx *ctx)
{
    playerCtx = ctx;
}

void VideoDecodeThread::run()
{
    int ret = video_entry();
    ff_log_line("VideoDecodeThread finished, ret=%d", ret);
}

int VideoDecodeThread::video_entry()
{
    AVPacket *packet = av_packet_alloc();
    int ret = -1;
    double pts = 0;

    AVFrame *pFrame = av_frame_alloc();
    if (!pFrame) {
        ff_log_line("av_frame_alloc error");
        return -1;
    }
    AVFrame *pFrameRGB = av_frame_alloc();
    if (!pFrameRGB) {
        ff_log_line("av_frame_alloc error");
        return -1;
    }

    av_image_alloc(pFrameRGB->data, pFrameRGB->linesize, playerCtx->vCodecCtx->width, playerCtx->vCodecCtx->height, AV_PIX_FMT_RGB24, 32);

    while (!m_stop) {
        if (playerCtx->pause == PAUSE) {
            SDL_Delay(5);
            continue;
        }

        if (playerCtx->flush_vctx) {
            ff_log_line("avcodec_flush_buffers(vCodecCtx) for seeking");
            avcodec_flush_buffers(playerCtx->vCodecCtx);
            playerCtx->flush_vctx = false;
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
            ret = avcodec_receive_frame(playerCtx->vCodecCtx, pFrame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            else if (ret < 0) {
                fprintf(stderr, "Error during decoding\n");
                exit(1);
            }

            if (packet->dts == AV_NOPTS_VALUE && pFrame->opaque && *(uint64_t*)pFrame->opaque != AV_NOPTS_VALUE) {
                pts = (double)*(uint64_t*)pFrame->opaque;
            }
            else if (packet->dts != AV_NOPTS_VALUE) {
                pts = (double)packet->dts;
            }
            else {
                pts = 0;
            }
            pts *= av_q2d(playerCtx->video_st->time_base);

            // frame ready
            if (ret == 0) {
                ret = sws_scale(playerCtx->sws_ctx, (uint8_t const* const*)pFrame->data, pFrame->linesize, 0, playerCtx->vCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
                pts = synchronize_video(playerCtx, pFrame, pts);
                if (ret == playerCtx->vCodecCtx->height) {
                    if (queue_picture(pFrameRGB, pts) < 0) {
                        break;
                    }
                }
            }
        }
    }

    av_frame_free(&pFrame);
    av_frame_free(&pFrameRGB);
    av_packet_free(&packet);

    return 0;
}

int VideoDecodeThread::queue_picture(AVFrame *pFrame, double pts)
{
    VideoPicture *vp;

    // wait until we have space for a new pic
    SDL_LockMutex(playerCtx->pictq_mutex);
    while (playerCtx->pictq_size >= VIDEO_PICTURE_QUEUE_SIZE) {
        SDL_CondWaitTimeout(playerCtx->pictq_cond, playerCtx->pictq_mutex, 500);
        if (m_stop) {
            break;
        }
    }
    SDL_UnlockMutex(playerCtx->pictq_mutex);

    if (m_stop) {
        return 0;
    }

    // windex is set to 0 initially
    vp = &playerCtx->pictq[playerCtx->pictq_windex];

    if (!vp->bmp) {
        SDL_LockMutex(playerCtx->pictq_mutex);
        vp->bmp = av_frame_alloc();
        av_image_alloc(vp->bmp->data, vp->bmp->linesize, playerCtx->vCodecCtx->width, playerCtx->vCodecCtx->height, AV_PIX_FMT_RGB24, 32);
        SDL_UnlockMutex(playerCtx->pictq_mutex);
    }

    // Copy the pic data and set pts
    memcpy(vp->bmp->data[0], pFrame->data[0], playerCtx->vCodecCtx->height * pFrame->linesize[0]);
    vp->pts = pts;

    // now we inform our display thread that we have a pic ready
    if(++playerCtx->pictq_windex == VIDEO_PICTURE_QUEUE_SIZE) {
        playerCtx->pictq_windex = 0;
    }
    SDL_LockMutex(playerCtx->pictq_mutex);
    playerCtx->pictq_size++;
    SDL_UnlockMutex(playerCtx->pictq_mutex);

    return 0;
}
