#include "FFmpegPlayer.h"
#include "PlayerCtx.h"
#include "DemuxThread.h"
#include "VideoDecodeThread.h"
#include "AudioDecodeThread.h"
#include "AudioPlay.h"
#include "log.h"
#include "SDLApp.h"

#include <functional>

static double get_audio_clock(PlayerCtx &is)
{
    double pts;
    int hw_buf_size, bytes_per_sec, n;

    pts = is.audio_clock;
    hw_buf_size = is.audio_buf_size - is.audio_buf_index;
    bytes_per_sec = 0;
    n = is.aCodecCtx->ch_layout.nb_channels * 2;

    if(is.audio_st) {
        bytes_per_sec = is.aCodecCtx->sample_rate * n;
    }

    if (bytes_per_sec) {
        pts -= (double)hw_buf_size / bytes_per_sec;
    }
    return pts;
}

static Uint32 sdl_refresh_timer_cb(Uint32 /*interval*/, void *opaque)
{
    SDL_Event event;
    event.type = FF_REFRESH_EVENT;
    event.user.data1 = opaque;
    SDL_PushEvent(&event);

    // If the callback returns 0, the periodic alarm is cancelled
    return 0;
}

static void FN_Audio_Cb(void *userdata, Uint8 *stream, int len)
{
    AudioDecodeThread *dt = (AudioDecodeThread*)userdata;
    dt->getAudioData(stream, len);
}

void stream_seek(PlayerCtx *is, int64_t pos, int rel)
{
    if (!is->seek_req) {
        is->seek_pos = pos;
        is->seek_flags = rel < 0 ? AVSEEK_FLAG_BACKWARD : 0;
        is->seek_req = 1;
    }
}

FFmpegPlayer::FFmpegPlayer(RenderView *render)
:m_render(render){
}

FFmpegPlayer::~FFmpegPlayer()
{
}

int FFmpegPlayer::init(const char *filePath)
{
    if (!filePath || strlen(filePath) >= 1024) {
        return -1;
    }   
    m_filePath = filePath;

    // init ctx
    strncpy(playerCtx.filename, m_filePath.c_str(), m_filePath.size());

    // 解封闭线程
    m_demuxThread = new DemuxThread(&playerCtx);
    if (m_demuxThread->init() != 0) {
        ff_log_line("DemuxThread init Failed.");
        return -1;
    }

    // 音频解码线程
    m_audioDecodeThread = new AudioDecodeThread(&playerCtx);
    // 视频解码线程
    m_videoDecodeThread = new VideoDecodeThread(&playerCtx);

    // render audio params
    audio_wanted_spec.freq = 48000;
    audio_wanted_spec.format = AUDIO_S16SYS;
    audio_wanted_spec.channels = 2;
    audio_wanted_spec.silence = 0;
    audio_wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
    audio_wanted_spec.callback = FN_Audio_Cb;
    audio_wanted_spec.userdata = m_audioDecodeThread;
    // create and open audio play device
    m_audioPlay = new AudioPlay;
    if (m_audioPlay->openDevice(&audio_wanted_spec) <= 0) {
        ff_log_line("open audio device Failed.");
        return -1;
    }

    // install player event
    sdlApp->registerEvent(SDL_KEYDOWN, [this](SDL_Event* e) {
        onKeyEvent(e);
    });
    
    return 0;
}

void FFmpegPlayer::start()
{
    m_stop = false;

    m_demuxThread->start();
    // m_videoDecodeThread->start();
    // m_audioDecodeThread->start();
    // m_audioPlay->start();

    // schedule_refresh(&playerCtx, 40);  
}

#define FREE(x) \
    delete x; \
    x = nullptr

void FFmpegPlayer::stop()
{
    m_stop = true;

    // stop audio decode
    ff_log_line("audio decode thread clean...");
    if (m_audioDecodeThread) {
        m_audioDecodeThread->stop();
        FREE(m_audioDecodeThread);
    }
    ff_log_line("audio decode thread finished.");

    // stop audio thread
    ff_log_line("audio play thread clean...");
    if (m_audioPlay) {
        m_audioPlay->stop();
        FREE(m_audioPlay);
    }
    ff_log_line("audio device finished.");

    // stop video decode thread
    ff_log_line("video decode thread clean...");
    if (m_videoDecodeThread) {
        m_videoDecodeThread->stop();
        FREE(m_videoDecodeThread);
    }
    ff_log_line("video decode thread finished.");

    // stop demux thread
    ff_log_line("demux thread clean...");
    if (m_demuxThread) {
        m_demuxThread->stop();
        FREE(m_demuxThread);
    }
    ff_log_line("demux thread finished.");
}

void FFmpegPlayer::pause(PauseState state)
{
    playerCtx.pause = state;

    // reset frame_timer when restore pause state
    playerCtx.frame_timer = av_gettime() / 1000000.0;
}

void FFmpegPlayer::onRefresh()
{
    if (m_stop) {
        return;
    }

    double actual_delay, delay, sync_threshold, ref_clock, diff;
   
    std::shared_ptr<MyPicture> vp = playerCtx.getPicture();
    if (!vp) {
        return;
    }

    delay = vp->pts_ - playerCtx.frame_last_pts;
    
    if(delay <= 0 || delay >= 1.0) {
        delay = playerCtx.frame_last_delay;
    }

    // save for next time
    playerCtx.frame_last_delay = delay;
    playerCtx.frame_last_pts = vp->pts_;
    
    ref_clock = get_audio_clock(playerCtx);
    diff = vp->pts_ - ref_clock;

    sync_threshold = (delay > AV_SYNC_THRESHOLD) ? delay : AV_SYNC_THRESHOLD;
    if (fabs(diff) < AV_NOSYNC_THRESHOLD) {
        if (diff <= -sync_threshold) {
            delay = 0;
        } else if (diff >= sync_threshold) {
            delay = 2 * delay;
        }
    }

    playerCtx.frame_timer += delay;
    actual_delay = playerCtx.frame_timer - (av_gettime() / 1000000.0);
    if (actual_delay < 0.010) {
        actual_delay = 0.010;
    }
    
    m_render->update(vp);
}

void FFmpegPlayer::onKeyEvent(SDL_Event *e)
{
    double incr, pos;
    switch(e->key.keysym.sym) {
    case SDLK_LEFT:
        incr = -10.0;
        goto do_seek;
    case SDLK_RIGHT:
        incr = 10.0;
        goto do_seek;
    case SDLK_UP:
        incr = 60.0;
        goto do_seek;
    case SDLK_DOWN:
        incr = -60.0;
        goto do_seek;
do_seek:
        if (true) {
            pos = get_audio_clock(playerCtx);
            pos += incr;
            if (pos < 0) {
                pos = 0;
            }
            ff_log_line("seek to %lf v:%lf a:%lf", pos, get_audio_clock(playerCtx), get_audio_clock(playerCtx));
            stream_seek(&playerCtx, (int64_t)(pos * AV_TIME_BASE), (int)incr);
        }
        break;
    case SDLK_q:
        // do quit
        ff_log_line("request quit, player will quit");

        // stop player
        stop();

        // quit sdl event loop
        sdlApp->quit();
    case SDLK_SPACE:
        ff_log_line("request pause, cur state=%d", (int)playerCtx.pause);
        if (playerCtx.pause == UNPAUSE) {
            pause(PAUSE);
        } else {
            pause(UNPAUSE);
        }

    default:
        break;
    }
}
