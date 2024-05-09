#include <stdio.h>

#define __STDC_CONSTANT_MACROS
//�ú�ȷ����C++������ʹ��FFmpeg��ʱ�ļ�����

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}
//��Ϊffmpegʹ��C����д�ģ���c++��ʹ����Ҫ����extern "C" ȷ����ȷ����


int main(int argc, char* argv[])
{
    AVFormatContext *pFormatCtx;                            //���ڴ洢��ý���ļ��ĸ�ʽ��������Ϣ��
    int videoindex;                                         //���ڴ洢��Ƶ����������
    AVCodecContext *pCodecCtx;                              //������������ģ��洢��Ƶ���ı������Ϣ��
    const AVCodec *pCodec;                                  //��������ṹ�壬��ʾ�ҵ�����Ƶ���������
    AVFrame *pFrame, *pFrameYUV;                            //pFrame���ڴ洢ԭʼ֡���ݣ�pFrameYUV���ڴ洢ת�����YUV֡���ݡ�
    uint8_t *out_buffer;                                    //���ڴ洢YUV֡���ݵ������������
    AVPacket *packet;                                       //���ڴ洢��ȡ�������ݰ���
    struct SwsContext *img_convert_ctx;                     //ͼ��ת�������ģ����ڽ�֡��һ�ָ�ʽת������һ�ָ�ʽ��
    AVCodecParameters *pCodecPar;                           //���ڴ洢����Ƶ���ı���������������������͡���ȡ��߶ȡ������ʵ���Ϣ��
    char filepath[] = "Titanic.ts";                         // �����ļ�·��


    pFormatCtx = avformat_alloc_context();                  //���䲢��ʼ��һ��AVFormatContext�ṹ��

    if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0) {             //�������ļ������pFormatCtx��
        printf("Couldn't open input stream.\n");
        return -1;
    }
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {                                  //��ȡ��ý���ļ�����Ϣ�����pFormatCtx������Ϣ��
        printf("Couldn't find stream information.\n");
        return -1;
    }
    videoindex = -1;                                                                                    //��ʼ����Ƶ������
    for (unsigned int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {                       //������������������Ƶ��������
            videoindex = i;
            break;
        }
    }
    if (videoindex == -1) {
        printf("Didn't find a video stream.\n");
        return -1;
    }

    pCodecPar = pFormatCtx->streams[videoindex]->codecpar;                                               //��ȡ��Ƶ���ı���������
    pCodec = avcodec_find_decoder(pCodecPar->codec_id);                                               //�ҵ���Ӧ�ı��������
    if (pCodec == NULL) {
        printf("Codec not found.\n");
        return -1;
    }

    pCodecCtx = avcodec_alloc_context3(pCodec);                                                     //���䲢��ʼ��������������ġ�
    if (avcodec_parameters_to_context(pCodecCtx, pCodecPar) < 0) {                             //�������������Ƶ������ġ�
        printf("Failed to copy codec parameters to codec context.\n");
        return -1;
    }
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {                                 //�򿪱��������
        printf("Could not open codec.\n");
        return -1;
    }

    pFrame = av_frame_alloc();                                                                              //���䲢��ʼ��һ��AVFrame�ṹ���ڴ洢ԭʼ֡���ݡ�
    pFrameYUV = av_frame_alloc();                                                                           //���䲢��ʼ����һ��AVFrame�ṹ���ڴ洢YUV֡���ݡ�
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);        //����YUV֡��������Ļ�������С��
    out_buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));                                     //ΪYUV���ݷ����ڴ档
    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);     //����������pFrameYUV��data��linesize����������
    packet = av_packet_alloc();                                                                              //���䲢��ʼ��һ��AVPacket�ṹ���ڴ洢��ȡ�������ݰ���

    printf("--------------- File Information ----------------\n");
    av_dump_format(pFormatCtx, 0, filepath, 0);                                     //��ӡ�ļ���Ϣ��
    printf("-------------------------------------------------\n");
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,                           //��ʼ������ͼ���ʽת����SwsContext��
                                     pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
    int frame_cnt = 0;                                                                                       //��ʼ�������֡��������
    while (av_read_frame(pFormatCtx, packet) >= 0) {                                                  //�����ȡ���ݰ���
        if (packet->stream_index == videoindex) {
            int ret = avcodec_send_packet(pCodecCtx, packet);                                   //�����ݰ����͵�������������ġ�
            if (ret < 0) {
                printf("Error sending a packet for decoding: %d\n", ret);
                continue;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(pCodecCtx, pFrame);                                 //�ӽ������н��ս�����֡��
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    printf("Error during decoding: %d\n", ret);
                    return -1;
                }
                //yuv���ұ���һ���հ����򣬲ü��ұ�
                sws_scale(img_convert_ctx, (const uint8_t *const *)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
                          pFrameYUV->data, pFrameYUV->linesize);                              //��ԭʼ֡ת��ΪYUV��ʽ���洢��pFrameYUV�С�
                printf("Decoded frame index: %d\n", frame_cnt);

                // �ڴ˴�������YUV�Ĵ���
                // ȡ����pFrameYUV��ʹ��fwrite()
                FILE *yuv_file = fopen("output.yuv", "ab");                                  //��׷��ģʽ��YUV�ļ���
                int y_size = pCodecCtx->width * pCodecCtx->height;                                           //����Yƽ��Ĵ�С��
                fwrite(pFrameYUV->data[0], 1, y_size, yuv_file);         // Y
                fwrite(pFrameYUV->data[1], 1, y_size / 4, yuv_file);     // U
                fwrite(pFrameYUV->data[2], 1, y_size / 4, yuv_file);     // V
                fclose(yuv_file);

                frame_cnt++;
            }
        }
        av_packet_unref(packet);                                                       //���ս�����֡�ĺ�����
    }

    sws_freeContext(img_convert_ctx);                                           //�ͷ�ͼ��ת�������ġ�

    av_frame_free(&pFrameYUV);                                                       //�ͷ�pFrameYUV�ṹ��
    av_frame_free(&pFrame);                                                          //�ͷ�pFrame�ṹ��
    avcodec_free_context(&pCodecCtx);                                                //�ͷű�����������ġ�
    avformat_close_input(&pFormatCtx);                                                   //�ر������ļ����ͷ�pFormatCtx�ṹ��
    av_packet_free(&packet);                                                           //�ͷ�packet�ṹ��
    av_free(out_buffer);                                                               //�ͷ�YUV�����������

    return 0;
}
