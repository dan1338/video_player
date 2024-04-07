#include <thread>
#include <queue>
#include <mutex>
#include <atomic>
#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

namespace ffmpeg
{

class AsyncReader
{
private:
    AVFormatContext *_formatContext;
    int _videoStreamIndex;
    AVCodecParameters *_codecParameters;
    AVCodec *_codec;
    AVCodecContext *_codecContext;

    AVFrame *_frame;
    AVPacket *_packet;

    std::thread _thread;
    std::queue<AVFrame*> _queue;
    std::mutex _queue_mtx;
    size_t _max_queue_size;

    std::atomic<bool> _is_eof;
    std::atomic<bool> _force_quit;

    void worker_func(const std::string &path);
public:
    AsyncReader(const std::string &path, size_t max_queue_size);
    ~AsyncReader();

    AVFrame *next_frame();
    bool eof() const { return _is_eof.load(); }
};

}

