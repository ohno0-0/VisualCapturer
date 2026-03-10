#include "FFmpegVideoWriter.h"

FFmpegVideoWriter::FFmpegVideoWriter() : fmt_ctx(nullptr), codec_ctx(nullptr), stream(nullptr), sws_ctx(nullptr), frame(nullptr), frame_pts(0)
{

}

bool FFmpegVideoWriter::open(const std::string& filename, int width, int height, int fps, int bitrate, const std::string& codec_name)
{
    // 初始化FFmpeg相关结构
    avformat_alloc_output_context2(&fmt_ctx, NULL, NULL, filename.c_str());
    if (!fmt_ctx) {
        fprintf(stderr, "无法创建输出上下文\n");
        return false;
    }

    // 查找编码器
    const AVCodec* codec = avcodec_find_encoder_by_name(codec_name.c_str());
    if (!codec) {
        // 如果找不到指定名称的编码器，尝试查找通用的H.264编码器
        codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if (!codec) {
            fprintf(stderr, "未找到合适的编码器\n");
            return false;
        }
    }

    // 创建视频流
    stream = avformat_new_stream(fmt_ctx, codec);
    if (!stream) {
        fprintf(stderr, "无法创建视频流\n");
        return false;
    }

    // 配置编码器上下文
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        fprintf(stderr, "无法分配编码器上下文\n");
        return false;
    }

    // 设置关键编码参数
    codec_ctx->codec_id = codec->id;
    codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    codec_ctx->bit_rate = bitrate; // 设置目标码率，控制文件大小的关键参数
    codec_ctx->width = width;
    codec_ctx->height = height;
    codec_ctx->time_base = { 1, fps }; // 时间基，与帧率相关
    codec_ctx->framerate = { fps, 1 }; // 帧率
    codec_ctx->gop_size = 12; // 关键帧间隔（GOP），影响压缩效率和视频 Seek
    codec_ctx->max_b_frames = 0; // B帧数量，可提高压缩率，但增加解码复杂度
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P; // 编码器通常要求的像素格式

    // 使用CRF模式控制质量，值越小质量越高（通常18-28）
    av_opt_set(codec_ctx->priv_data, "crf", "20", 0);
    // 使用"慢"预设可以获取更好的压缩比
    av_opt_set(codec_ctx->priv_data, "preset", "medium", 0);

    //// 设置编码器预设（preset）和调优（tune），这些可选设置能优化编码速度和质量
    //AVDictionary* opts = NULL;
    //av_dict_set(&opts, "preset", "medium", 0); // 编码速度与质量的平衡
    //av_dict_set(&opts, "tune", "film", 0);    // 针对内容类型优化（如film、animation）

    // 打开编码器
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        fprintf(stderr, "无法打开编码器\n");
        avcodec_free_context(&codec_ctx);
        //av_dict_free(&opts);
        return false;
    }
    //av_dict_free(&opts);

    // 将编码器参数复制到流
    if (avcodec_parameters_from_context(stream->codecpar, codec_ctx) < 0) {
        fprintf(stderr, "无法复制编码器参数到流\n");
        return false;
    }

    // 如果输出格式需要，则创建并打开输出文件
    if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&fmt_ctx->pb, filename.c_str(), AVIO_FLAG_WRITE) < 0) {
            fprintf(stderr, "无法打开输出文件\n");
            return false;
        }
    }

    // 写入文件头
    if (avformat_write_header(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "写入文件头失败\n");
        return false;
    }

    // 分配帧缓冲区
    frame = av_frame_alloc();
    frame->format = codec_ctx->pix_fmt;
    frame->width = codec_ctx->width;
    frame->height = codec_ctx->height;
    if (av_frame_get_buffer(frame, 0) < 0) {
        fprintf(stderr, "无法为视频帧分配内存\n");
        return false;
    }

    // 初始化图像转换上下文（SwsContext），用于将OpenCV的BGR图像转换为编码器需要的YUV420P格式
    sws_ctx = sws_getContext(width, height, AV_PIX_FMT_BGR24, // 输入：OpenCV默认BGR格式
        width, height, AV_PIX_FMT_YUV420P, // 输出：编码器常用格式
        SWS_BILINEAR, NULL, NULL, NULL);
    if (!sws_ctx) {
        fprintf(stderr, "无法初始化图像转换上下文\n");
        return false;
    }

    frame_pts = 0;
    return true;
}

bool FFmpegVideoWriter::writeFrame(const cv::Mat& image)
{
    // 确保输入图像尺寸与编码器设置匹配
    if (image.cols != codec_ctx->width || image.rows != codec_ctx->height) {
        fprintf(stderr, "输入图像分辨率不匹配\n");
        return false;
    }

    // 将OpenCV的BGR图像数据转换为YUV420P格式，并填充到AVFrame
    const uint8_t* src_data[1] = { image.data };
    int src_linesize[1] = { static_cast<int>(image.step) };
    sws_scale(sws_ctx, src_data, src_linesize, 0, image.rows, frame->data, frame->linesize);

    // 设置帧的显示时间戳（PTS）
    frame->pts = frame_pts++;

    // 发送帧到编码器
    if (avcodec_send_frame(codec_ctx, frame) < 0) {
        fprintf(stderr, "发送帧到编码器失败\n");
        return false;
    }

    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        return false;
    }

    pkt->data = NULL;
    pkt->size = 0;

    // 循环接收编码后的数据包
    while (avcodec_receive_packet(codec_ctx, pkt) >= 0) {
        // 设置包的时间戳和流索引
        av_packet_rescale_ts(pkt, codec_ctx->time_base, stream->time_base);
        pkt->stream_index = stream->index;

        // 将编码后的数据包写入输出文件
        if (av_interleaved_write_frame(fmt_ctx, pkt) < 0) {
            fprintf(stderr, "写入帧失败\n");
            av_packet_unref(pkt);
            return false;
        }
        av_packet_unref(pkt);
    }
    return true;

}

void FFmpegVideoWriter::close()
{
    // 刷新编码器（处理缓冲区中可能剩余的帧）
    if (codec_ctx) {
        // 首先检查并发送刷新信号（NULL帧）到编码器
        int ret_send = avcodec_send_frame(codec_ctx, NULL);
        if (ret_send < 0 && ret_send != AVERROR_EOF) {
            // 处理发送失败，但继续执行刷新逻辑
            fprintf(stderr, "Warning: avcodec_send_frame failed while flushing: %d\n", ret_send);
        }

        AVPacket* pkt = av_packet_alloc();
        if (!pkt) {
            fprintf(stderr, "Failed to allocate packet for flushing\n");
        }
        else {
            // 循环接收编码器缓冲区中所有剩余的数据包
            while (true) {
                int ret_receive = avcodec_receive_packet(codec_ctx, pkt);

                if (ret_receive == AVERROR_EOF || ret_receive == AVERROR(EAGAIN)) {
                    // 编码器已刷新完成或无更多数据包
                    break;
                }
                else if (ret_receive < 0) {
                    // 其他错误
                    fprintf(stderr, "Error during flushing: %d\n", ret_receive);
                    break;
                }

                // 成功接收到数据包，处理它
                if (fmt_ctx && stream) { // 确保输出上下文和流有效
                    av_packet_rescale_ts(pkt, codec_ctx->time_base, stream->time_base);
                    pkt->stream_index = stream->index;
                    av_interleaved_write_frame(fmt_ctx, pkt);
                }

                av_packet_unref(pkt); // 释放当前数据包资源
            }

            av_packet_free(&pkt); // 释放数据包结构体本身
        }
    }

    // 写入文件尾
    if (fmt_ctx) {
        av_write_trailer(fmt_ctx);
    }

    // 释放资源
    if (sws_ctx) {
        sws_freeContext(sws_ctx);
    }
    if (frame) {
        av_frame_free(&frame);
    }
    if (codec_ctx) {
        avcodec_free_context(&codec_ctx);
    }
    if (fmt_ctx && !(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&fmt_ctx->pb);
    }
    if (fmt_ctx) {
        avformat_free_context(fmt_ctx);
    }
}

bool FFmpegVideoWriter::isopen()
{
    if (!codec_ctx) return false;
    return true;
}
