#include "VideoCompressor.h"

#include <QFile>

VideoCompressor::VideoCompressor(QObject* parent)
{

}

VideoCompressor::~VideoCompressor()
{
    this->quit();
    this->wait();
}

void VideoCompressor::SetVideoName(std::string VideoName)
{
	m_VideoName = VideoName;
}

void VideoCompressor::run()
{
    // 初始化FFmpeg网络库（非必须，但建议）
    avformat_network_init();

    if (m_VideoName == "")return;
    std::string OutName = m_VideoName.substr(0, m_VideoName.find(".mp4")) + "OutPut" + ".mp4";

    const char* input_filename = m_VideoName.c_str();
    const char* output_filename = OutName.c_str();

    // 打开输入文件
    AVFormatContext* input_format_ctx = nullptr;
    if (avformat_open_input(&input_format_ctx, input_filename, nullptr, nullptr) < 0) {
        return;
    }

    // 获取流信息
    if (avformat_find_stream_info(input_format_ctx, nullptr) < 0) {
        avformat_close_input(&input_format_ctx);
        return;
    }

    // 查找视频流索引
    int video_stream_index = -1;
    for (int i = 0; i < input_format_ctx->nb_streams; i++) {
        if (input_format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }
    if (video_stream_index == -1) {
        avformat_close_input(&input_format_ctx);
        return;
    }

    // 在打开输入文件并找到视频流索引后，添加解码器初始化
    AVCodecParameters* codec_par = input_format_ctx->streams[video_stream_index]->codecpar;
    const AVCodec* decoder = avcodec_find_decoder(codec_par->codec_id);
    if (!decoder) {
        // 错误处理：未找到解码器
        avformat_close_input(&input_format_ctx);
        return;
    }

    AVCodecContext* decoder_ctx = avcodec_alloc_context3(decoder);
    if (avcodec_parameters_to_context(decoder_ctx, codec_par) < 0) {
        // 错误处理：参数复制失败
        avcodec_free_context(&decoder_ctx);
        avformat_close_input(&input_format_ctx);
        return;
    }

    if (avcodec_open2(decoder_ctx, decoder, nullptr) < 0) {
        // 错误处理：无法打开解码器
        avcodec_free_context(&decoder_ctx);
        avformat_close_input(&input_format_ctx);
        return;
    }

    // 创建输出格式上下文
    AVFormatContext* output_format_ctx = nullptr;
    if (avformat_alloc_output_context2(&output_format_ctx, nullptr, nullptr, output_filename) < 0) {
        avformat_close_input(&input_format_ctx);
        return;
    }

    // 为输出文件创建视频流
    AVStream* output_stream = avformat_new_stream(output_format_ctx, nullptr);
    if (!output_stream) {
        avformat_free_context(output_format_ctx);
        avformat_close_input(&input_format_ctx);
        return;
    }

    // 以下代码没有用
    // 查找编码器（这里使用查找可能存在的所有GPU编码器,没有找到就回退为软件编码器H.264）
    //const AVCodec* codec = avcodec_find_encoder_by_name("h264_nvenc"); //英伟达GPU编码器
    //if (!codec) {
    //    codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    //}
    //// 验证找到的确实是编码器
    //if (!av_codec_is_encoder(codec)) {
    //    codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    //}
    //codec = avcodec_find_encoder_by_name("h264_amf");//AMD-GPU编码器
    //if (!codec) {
    //    codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    //}
    //codec = avcodec_find_encoder_by_name("h264_qsv");//Intel集成显卡编码器
    //if (!codec) {
    //    codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    //}

    const AVCodec* codec = avcodec_find_encoder_by_name("h264_nvenc"); //avcodec_find_encoder_by_name("h264_amf"); 
    //avcodec_find_encoder(AV_CODEC_ID_MJPEG);//avcodec_find_encoder_by_name("h264_amf");

    if (!codec) {
        // ... 错误处理，释放资源
        return;
    }

    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    // 设置合理的帧率（匹配输入视频）
    // 应该从输入流获取实际帧率，而不是硬编码60fps
    AVStream* in_stream = input_format_ctx->streams[video_stream_index];
    // 配置编码器参数
    //codec_ctx->time_base = in_stream->time_base;// 时间基（帧率相关）这行代码有问题，获取的时间基为1比60000
    //codec_ctx->framerate = in_stream->avg_frame_rate;// 帧率
    codec_ctx->bit_rate = 5000000;  // 目标码率设置为5 Mbps(市面上通用的码率大小)
    codec_ctx->width = 1920;         // 输出视频宽度
    codec_ctx->height = 1080;        // 输出视频高度
    codec_ctx->time_base = { 1, 60 };  // 时间基（帧率相关）
    codec_ctx->framerate = { 60, 1 };  // 帧率60fps
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P; // 像素格式
    codec_ctx->gop_size = 30;  // 每30帧一个关键帧
    codec_ctx->max_b_frames = 0;  // 使用B帧提高压缩率
    // 显式设置编码线程数，0为自动检测
    codec_ctx->thread_count = 0; // 或者设置为您的CPU核心数

    // 使用CRF模式控制质量，值越小质量越高（通常18-28）
    av_opt_set(codec_ctx->priv_data, "crf", "20", 0);
    // 使用"慢"预设可以获取更好的压缩比
    av_opt_set(codec_ctx->priv_data, "preset", "veryfast", 0);
    //av_opt_set(codec_ctx->priv_data, "quality", "quality", 0); // 尝试设置高质量模式

    // 打开编码器
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        // ... 错误处理，释放资源
        avcodec_free_context(&codec_ctx);
        return;
    }

    // 将编码器参数复制到输出流
    if (avcodec_parameters_from_context(output_stream->codecpar, codec_ctx) < 0) {
        // ... 错误处理，释放资源
        avcodec_free_context(&codec_ctx);
        return;
    }

    // 打开输出文件
    if (!(output_format_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&output_format_ctx->pb, output_filename, AVIO_FLAG_WRITE) < 0) {
            // ... 错误处理，释放资源
            return;
        }
    }

    // 写入文件头
    if (avformat_write_header(output_format_ctx, nullptr) < 0) {
        // ... 错误处理，释放资源
        return;
    }

    AVPacket* input_packet = av_packet_alloc();
    AVFrame* decoded_frame = av_frame_alloc(); // 用于存储解码后的原始帧
    AVPacket* output_packet = av_packet_alloc(); // 用于存储编码后的数据包

    if (!input_packet || !decoded_frame || !output_packet) {
        // 错误处理：内存分配失败
        return;
    }

    // 主循环：读取、解码、编码、写入
    while (av_read_frame(input_format_ctx, input_packet) >= 0) {
        // 只处理视频流
        if (input_packet->stream_index == video_stream_index) {
            // 步骤1: 将压缩数据包发送给解码器
            int ret_send = avcodec_send_packet(decoder_ctx, input_packet);
            if (ret_send < 0) {
                // 处理发送失败，但可能继续循环
                av_packet_unref(input_packet);
                continue;
            }

            // 步骤2: 循环从解码器接收解码后的原始帧
            while (avcodec_receive_frame(decoder_ctx, decoded_frame) >= 0) {
                //转换时间戳
                AVFrame* frame_for_encoding = av_frame_clone(decoded_frame);
                if (!frame_for_encoding) {
                    av_frame_unref(decoded_frame);
                    break;
                }

                // 从解码器时间基转换为编码器时间基
                frame_for_encoding->pts = av_rescale_q(decoded_frame->pts,
                    decoder_ctx->time_base,
                    codec_ctx->time_base);

                // 确保PTS是递增的
                if (last_pts != AV_NOPTS_VALUE && frame_for_encoding->pts <= last_pts) {
                    // 如果当前 PTS 不大于上一帧的 PTS，则强制将其设为上一帧 PTS + 1
                    frame_for_encoding->pts = last_pts + 1;
                }
                last_pts = frame_for_encoding->pts;

                // 步骤3: 将解码后的原始帧发送给编码器
                if (avcodec_send_frame(codec_ctx, frame_for_encoding) < 0) {
                    // 处理发送帧失败
                    av_frame_unref(frame_for_encoding);
                    av_frame_unref(decoded_frame);  //释放帧
                    break;
                }

                // 释放复制的帧
                av_frame_free(&frame_for_encoding);
                // 立即释放解码帧
                av_frame_unref(decoded_frame);

                // 步骤4: 循环从编码器接收编码后的数据包
                while (avcodec_receive_packet(codec_ctx, output_packet) >= 0) {
                    // 设置输出包的时间戳、流索引等
                    output_packet->stream_index = 0; // 输出流索引

                    // 数据包的时间戳转换：从编码器时间基转换为输出流时间基
                    av_packet_rescale_ts(output_packet, codec_ctx->time_base,output_format_ctx->streams[0]->time_base);

                    //av_packet_rescale_ts(output_packet, decoder_ctx->time_base, codec_ctx->time_base);

                    // 写入编码后的数据包到输出文件
                    av_interleaved_write_frame(output_format_ctx, output_packet);
                    av_packet_unref(output_packet); // 释放输出包资源
                }
            }
        }
        // 释放输入包资源，准备读取下一个包
        av_packet_unref(input_packet);
    }

    //// 主循环：读取视频帧，编码，写入
    //AVPacket* packet = av_packet_alloc();
    //AVFrame* frame = av_frame_alloc();
    //while (av_read_frame(input_format_ctx, packet) >= 0) {
    //    if (packet->stream_index == video_stream_index) {
    //        // 发送原始帧数据到编码器
    //        if (avcodec_send_frame(codec_ctx, frame) >= 0) {
    //            // 接收编码后的数据包
    //            while (avcodec_receive_packet(codec_ctx, packet) >= 0) {
    //                // 写入编码后的数据包到输出文件
    //                av_interleaved_write_frame(output_format_ctx, packet);
    //                av_packet_unref(packet);
    //            }
    //        }
    //    }
    //    av_packet_unref(packet);
    //}

    // 刷新解码器
    avcodec_send_frame(decoder_ctx, nullptr);
    while (avcodec_receive_packet(decoder_ctx, input_packet) >= 0) {
        av_interleaved_write_frame(output_format_ctx, input_packet);
        av_packet_unref(input_packet);
    }

    // 刷新编码器
    avcodec_send_frame(codec_ctx, nullptr);
    while (avcodec_receive_packet(codec_ctx, input_packet) >= 0) {
        av_interleaved_write_frame(output_format_ctx, input_packet);
        av_packet_unref(input_packet);
    }

    // 刷新解码器缓冲区（处理缓冲区中剩余的帧）
    avcodec_send_packet(decoder_ctx, nullptr);
    while (avcodec_receive_frame(decoder_ctx, decoded_frame) >= 0) {
        av_frame_unref(decoded_frame);  // 重要：释放刷新产生的帧
    }

    // 刷新编码器缓冲区
    avcodec_send_frame(codec_ctx, nullptr);
    while (avcodec_receive_packet(codec_ctx, output_packet) >= 0) {
        av_interleaved_write_frame(output_format_ctx, output_packet);
        av_packet_unref(output_packet);
    }

    // 写入文件尾
    av_write_trailer(output_format_ctx);

    // 释放所有资源
    av_packet_free(&input_packet);
    av_frame_free(&decoded_frame);
    av_packet_free(&output_packet);
    avcodec_free_context(&codec_ctx);
    avcodec_free_context(&decoder_ctx); // 释放解码器上下文
    if (!(output_format_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&output_format_ctx->pb);
    }
    avformat_free_context(output_format_ctx);
    avformat_close_input(&input_format_ctx);

    QFile::remove(input_filename);

}
