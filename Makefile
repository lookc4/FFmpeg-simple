# ������
CC = g++

# �����־
CFLAGS = -Wall -std=c++11

# FFmpeg ����·���Ϳ�
FFMPEG_INC = -I/usr/include
FFMPEG_LIB = -lavformat -lavcodec -lavutil -lswscale -lswresample -ldrm -lpthread -lz

# �����ִ���ļ�����
TARGET = ffmpeg_example

# Դ�ļ�
SRCS = main.cpp

# ����Ŀ���ļ�
OBJS = $(SRCS:.cpp=.o)

# ����
all: $(TARGET) clean_objects

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(FFMPEG_LIB)

%.o: %.cpp
	$(CC) $(CFLAGS) $(FFMPEG_INC) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS) output.yuv

clean_objects:
	rm -f $(OBJS)
