#include <stdio.h>

#define __STDC_CONSTANT_MACROS
//该宏确保在C++程序中使用FFmpeg库时的兼容性

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}
//因为ffmpeg使用C语言写的，在c++中使用需要声明extern "C" 确保正确引用


int main(int argc, char* argv[])
{
    AVFormatContext *pFormatCtx;                            //用于存储多媒体文件的格式上下文信息。
    int videoindex;                                         //用于存储视频流的索引。
    AVCodecContext *pCodecCtx;                              //编解码器上下文，存储视频流的编解码信息。
    const AVCodec *pCodec;                                  //编解码器结构体，表示找到的视频编解码器。
    AVFrame *pFrame, *pFrameYUV;                            //pFrame用于存储原始帧数据，pFrameYUV用于存储转换后的YUV帧数据。
    uint8_t *out_buffer;                                    //用于存储YUV帧数据的输出缓冲区。
    AVPacket *packet;                                       //用于存储读取到的数据包。
    struct SwsContext *img_convert_ctx;                     //图像转换上下文，用于将帧从一种格式转换到另一种格式。
    AVCodecParameters *pCodecPar;                           //用于存储音视频流的编解码参数，例如编码器类型、宽度、高度、比特率等信息。
    char filepath[] = "Titanic.ts";                         // 输入文件路径


    pFormatCtx = avformat_alloc_context();                  //分配并初始化一个AVFormatContext结构。

    if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0) {             //打开输入文件并填充pFormatCtx。
        printf("Couldn't open input stream.\n");
        return -1;
    }
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {                                  //读取多媒体文件的信息，填充pFormatCtx的流信息。
        printf("Couldn't find stream information.\n");
        return -1;
    }
    videoindex = -1;                                                                                    //初始化视频索引。
    for (unsigned int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {                       //遍历所有流，查找视频流索引。
            videoindex = i;
            break;
        }
    }
    if (videoindex == -1) {
        printf("Didn't find a video stream.\n");
        return -1;
    }

    pCodecPar = pFormatCtx->streams[videoindex]->codecpar;                                               //获取视频流的编解码参数。
    pCodec = avcodec_find_decoder(pCodecPar->codec_id);                                               //找到对应的编解码器。
    if (pCodec == NULL) {
        printf("Codec not found.\n");
        return -1;
    }

    pCodecCtx = avcodec_alloc_context3(pCodec);                                                     //分配并初始化编解码器上下文。
    if (avcodec_parameters_to_context(pCodecCtx, pCodecPar) < 0) {                             //将编解码参数复制到上下文。
        printf("Failed to copy codec parameters to codec context.\n");
        return -1;
    }
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {                                 //打开编解码器。
        printf("Could not open codec.\n");
        return -1;
    }

    pFrame = av_frame_alloc();                                                                              //分配并初始化一个AVFrame结构用于存储原始帧数据。
    pFrameYUV = av_frame_alloc();                                                                           //分配并初始化另一个AVFrame结构用于存储YUV帧数据。
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);        //计算YUV帧数据所需的缓冲区大小。
    out_buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));                                     //为YUV数据分配内存。
    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);     //将缓冲区与pFrameYUV的data和linesize关联起来。
    packet = av_packet_alloc();                                                                              //分配并初始化一个AVPacket结构用于存储读取到的数据包。

    printf("--------------- File Information ----------------\n");
    av_dump_format(pFormatCtx, 0, filepath, 0);                                     //打印文件信息。
    printf("-------------------------------------------------\n");
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,                           //初始化用于图像格式转换的SwsContext。
                                     pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
    int frame_cnt = 0;                                                                                       //初始化解码的帧计数器。
    while (av_read_frame(pFormatCtx, packet) >= 0) {                                                  //逐个读取数据包。
        if (packet->stream_index == videoindex) {
            int ret = avcodec_send_packet(pCodecCtx, packet);                                   //将数据包发送到编解码器上下文。
            if (ret < 0) {
                printf("Error sending a packet for decoding: %d\n", ret);
                continue;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(pCodecCtx, pFrame);                                 //从解码器中接收解码后的帧。
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    printf("Error during decoding: %d\n", ret);
                    return -1;
                }
                //yuv在右边有一个空白区域，裁剪右边
                sws_scale(img_convert_ctx, (const uint8_t *const *)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
                          pFrameYUV->data, pFrameYUV->linesize);                              //将原始帧转换为YUV格式并存储在pFrameYUV中。
                printf("Decoded frame index: %d\n", frame_cnt);

                // 在此处添加输出YUV的代码
                // 取自于pFrameYUV，使用fwrite()
                FILE *yuv_file = fopen("output.yuv", "ab");                                  //以追加模式打开YUV文件。
                int y_size = pCodecCtx->width * pCodecCtx->height;                                           //计算Y平面的大小。
                fwrite(pFrameYUV->data[0], 1, y_size, yuv_file);         // Y
                fwrite(pFrameYUV->data[1], 1, y_size / 4, yuv_file);     // U
                fwrite(pFrameYUV->data[2], 1, y_size / 4, yuv_file);     // V
                fclose(yuv_file);

                frame_cnt++;
            }
        }
        av_packet_unref(packet);                                                       //接收解码后的帧的函数。
    }

    sws_freeContext(img_convert_ctx);                                           //释放图像转换上下文。

    av_frame_free(&pFrameYUV);                                                       //释放pFrameYUV结构。
    av_frame_free(&pFrame);                                                          //释放pFrame结构。
    avcodec_free_context(&pCodecCtx);                                                //释放编解码器上下文。
    avformat_close_input(&pFormatCtx);                                                   //关闭输入文件并释放pFormatCtx结构。
    av_packet_free(&packet);                                                           //释放packet结构。
    av_free(out_buffer);                                                               //释放YUV输出缓冲区。

    return 0;
}
