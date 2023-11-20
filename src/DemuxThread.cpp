#include "DemuxThread.h"

#include <functional>

#include "log.h"
#include "FFmpegPlayer.h"

DemuxThread::DemuxThread(FFmpegPlayerCtx *ctx)
{
    playerCtx = ctx;
}

int DemuxThread::init()
{
    AVFormatContext *formatCtx = NULL;
    if (avformat_open_input(&formatCtx, playerCtx->filename, NULL, NULL) != 0) {
        ff_log_line("avformat_open_input Failed.");
        return -1;
    }

    playerCtx->formatCtx = formatCtx;

    if (avformat_find_stream_info(formatCtx, NULL) < 0) {
        ff_log_line("avformat_find_stream_info Failed.");
        return -1;
    }

    av_dump_format(formatCtx, 0, playerCtx->filename, 0);

    if (stream_open(AVMEDIA_TYPE_AUDIO) < 0) {
        ff_log_line("open audio stream Failed.");
        return -1;
    }

    if (stream_open(AVMEDIA_TYPE_VIDEO) < 0) {
        ff_log_line("open video stream Failed.");
        return -1;
    }

    return 0;
}

DemuxThread::~DemuxThread()
{
    if (playerCtx->formatCtx) {
        avformat_close_input(&playerCtx->formatCtx);
        playerCtx->formatCtx = nullptr;
    }

    if (playerCtx->aCodecCtx) {
        avcodec_free_context(&playerCtx->aCodecCtx);
        playerCtx->aCodecCtx = nullptr;
    }

    if (playerCtx->vCodecCtx) {
        avcodec_free_context(&playerCtx->vCodecCtx);
        playerCtx->vCodecCtx = nullptr;
    }

    if (playerCtx->swr_ctx) {
        swr_free(&playerCtx->swr_ctx);
        playerCtx->swr_ctx = nullptr;
    }

    if (playerCtx->sws_ctx) {
        sws_freeContext(playerCtx->sws_ctx);
        playerCtx->sws_ctx = nullptr;
    }
}

void DemuxThread::run()
{
    AVPacket *packet = av_packet_alloc();

    for(;;) {
        if(m_stop) {
            ff_log_line("request quit while decode_loop");
            break;
        }

        // begin seek
        if (playerCtx->seek_req) {
            int stream_index= -1;
            int64_t seek_target = playerCtx->seek_pos;

            if (playerCtx->videoStream >= 0) {
                stream_index = playerCtx->videoStream;
            } else if(playerCtx->audioStream >= 0) {
                stream_index = playerCtx->audioStream;
            }

            if (stream_index >= 0) {
                seek_target= av_rescale_q(seek_target, AVRational{1, AV_TIME_BASE}, playerCtx->formatCtx->streams[stream_index]->time_base);
            }

            if (av_seek_frame(playerCtx->formatCtx, stream_index, seek_target, playerCtx->seek_flags) < 0) {
                ff_log_line("%s: error while seeking\n", playerCtx->filename);
            } else {
                if(playerCtx->audioStream >= 0) {
                    playerCtx->audioq.packetFlush();
                    playerCtx->flush_actx = true;
                }
                if (playerCtx->videoStream >= 0) {
                    playerCtx->videoq.packetFlush();
                    playerCtx->flush_vctx = true;
                }
            }

            // reset to zero when seeking done
            playerCtx->seek_req = 0;
        }

        if (playerCtx->audioq.packetSize() > MAX_AUDIOQ_SIZE || playerCtx->videoq.packetSize() > MAX_VIDEOQ_SIZE) {
            SDL_Delay(10);
            continue;
        }

        if (av_read_frame(playerCtx->formatCtx, packet) < 0) {
            ff_log_line("av_read_frame error");
            break;
        }

        if (packet->stream_index == playerCtx->videoStream) {
            playerCtx->videoq.packetPut(packet);
        } else if (packet->stream_index == playerCtx->audioStream) {
            playerCtx->audioq.packetPut(packet);
        } else {
            av_packet_unref(packet);
        }
    }

    while (!m_stop) {
        SDL_Delay(100);
    }

    av_packet_free(&packet);

    SDL_Event event;
    event.type = FF_QUIT_EVENT;
    event.user.data1 = playerCtx;
    SDL_PushEvent(&event);

    return;
}

int DemuxThread::stream_open(int media_type)
{
    AVFormatContext *formatCtx = playerCtx->formatCtx;
    AVCodecContext *codecCtx = NULL;
    AVCodec *codec = NULL;

    int stream_index = av_find_best_stream(formatCtx, (AVMediaType)media_type, -1, -1, (const AVCodec **)&codec, 0);
    if (stream_index < 0 || stream_index >= (int)formatCtx->nb_streams) {
        ff_log_line("Cannot find a audio stream in the input file\n");
        return -1;
    }

    codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, formatCtx->streams[stream_index]->codecpar);

    if (avcodec_open2(codecCtx, codec, NULL) < 0) {
        ff_log_line("Failed to open codec for stream #%d\n", stream_index);
        return -1;
    }

    switch(codecCtx->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
        playerCtx->audioStream = stream_index;
        playerCtx->aCodecCtx = codecCtx;
        playerCtx->audio_st = formatCtx->streams[stream_index];
        playerCtx->swr_ctx = swr_alloc();

        av_opt_set_chlayout(playerCtx->swr_ctx, "in_chlayout", &codecCtx->ch_layout, 0);
        av_opt_set_int(playerCtx->swr_ctx, "in_sample_rate",       codecCtx->sample_rate, 0);
        av_opt_set_sample_fmt(playerCtx->swr_ctx, "in_sample_fmt", codecCtx->sample_fmt, 0);

        AVChannelLayout outLayout;
        // use stereo
        av_channel_layout_default(&outLayout, 2);

        av_opt_set_chlayout(playerCtx->swr_ctx, "out_chlayout", &outLayout, 0);
        av_opt_set_int(playerCtx->swr_ctx, "out_sample_rate",       48000, 0);
        av_opt_set_sample_fmt(playerCtx->swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
        swr_init(playerCtx->swr_ctx);

        break;
    case AVMEDIA_TYPE_VIDEO:
        playerCtx->videoStream = stream_index;
        playerCtx->vCodecCtx   = codecCtx;
        playerCtx->video_st    = formatCtx->streams[stream_index];
        playerCtx->frame_timer = (double)av_gettime() / 1000000.0;
        playerCtx->frame_last_delay = 40e-3;
        playerCtx->sws_ctx = sws_getContext(
                    codecCtx->width,
                    codecCtx->height,
                    codecCtx->pix_fmt,
                    codecCtx->width,
                    codecCtx->height,
                    AV_PIX_FMT_RGB24,
                    SWS_BILINEAR,
                    NULL,
                    NULL,
                    NULL
                    );
        break;
    default:
        break;
    }

    return 0;
}
