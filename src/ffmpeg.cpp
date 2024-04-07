#include <iostream>
#include "ffmpeg.h"

namespace ffmpeg
{

void AsyncReader::worker_func(const std::string &path)
{
    _frame = av_frame_alloc();
    _packet = av_packet_alloc();

    if (avformat_open_input(&_formatContext, path.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "Error opening input file" << std::endl;
        return;
    }

    if (avformat_find_stream_info(_formatContext, nullptr) < 0) {
        std::cerr << "Error finding stream information" << std::endl;
        avformat_close_input(&_formatContext);
        return;
    }

    for (unsigned int i = 0; i < _formatContext->nb_streams; i++) {
        if (_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            _videoStreamIndex = i;
            break;
        }
    }

    if (_videoStreamIndex == -1) {
        std::cerr << "No video stream found in the input file" << std::endl;
        avformat_close_input(&_formatContext);
        return;
    }

    _codecParameters = _formatContext->streams[_videoStreamIndex]->codecpar;
    _codec = avcodec_find_decoder(_codecParameters->codec_id);
    if (_codec == nullptr) {
        std::cerr << "Unsupported codec" << std::endl;
        avformat_close_input(&_formatContext);
        return;
    }

    _codecContext = avcodec_alloc_context3(_codec);
    if (avcodec_parameters_to_context(_codecContext, _codecParameters) < 0) {
        std::cerr << "Failed to copy codec parameters to codec context" << std::endl;
        avcodec_free_context(&_codecContext);
        avformat_close_input(&_formatContext);
        return;
    }

    if (avcodec_open2(_codecContext, _codec, nullptr) < 0) {
        std::cerr << "Failed to open codec" << std::endl;
        avcodec_free_context(&_codecContext);
        avformat_close_input(&_formatContext);
        return;
    }

    while (av_read_frame(_formatContext, _packet) >= 0) {
        if (_packet->stream_index == _videoStreamIndex) {
            if (avcodec_send_packet(_codecContext, _packet) == 0) {
                while (avcodec_receive_frame(_codecContext, _frame) == 0) {
                    while (!_force_quit.load())
                    {
                        std::lock_guard<std::mutex> lock(_queue_mtx);

                        if (_queue.size() < _max_queue_size)
                        {
                            _queue.push(av_frame_clone(_frame));
                            break;
                        }
                    }

                    if (_force_quit.load())
                        goto exit_loop;
                }
            }
        }
        av_packet_unref(_packet);
    }

exit_loop:
    _is_eof.store(true);

    av_packet_free(&_packet);
    av_frame_free(&_frame);
    avcodec_free_context(&_codecContext);
    avformat_close_input(&_formatContext);
}

AsyncReader::AsyncReader(const std::string &path, size_t max_queue_size):
    _formatContext(nullptr),
    _codecParameters(nullptr),
    _codec(nullptr),
    _codecContext(nullptr),
    _thread([&](){ worker_func(path); }),
    _max_queue_size(max_queue_size),
    _is_eof(false),
    _force_quit(false)
{
}

AsyncReader::~AsyncReader()
{
    _force_quit.store(true);
    _thread.join();

    while (AVFrame *frame = next_frame())
        av_frame_free(&frame);
}

AVFrame *AsyncReader::next_frame()
{
    std::lock_guard<std::mutex> lock(_queue_mtx);

    if (!_queue.empty())
    {
        auto ret = _queue.front();
        _queue.pop();
        return ret;
    }

    return nullptr;
}

}

