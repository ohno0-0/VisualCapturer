#pragma once

#include <opencv2/opencv.hpp>
extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/opt.h>
}

class FFmpegVideoWriter {
private:
    AVFormatContext* fmt_ctx;
    AVCodecContext* codec_ctx;
    AVStream* stream;
    SwsContext* sws_ctx;
    AVFrame* frame;
    int64_t frame_pts;

public:
    FFmpegVideoWriter();

    bool open(const std::string& filename, int width, int height, int fps, int bitrate, const std::string& codec_name = "libx264");

    bool writeFrame(const cv::Mat& image);

    void close();

    bool isopen();
};