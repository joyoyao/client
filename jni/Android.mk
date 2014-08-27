LOCAL_PATH := $(call my-dir)

#include $(CLEAR_VARS)
#LOCAL_MODULE := avutil
#LOCAL_SRC_FILES := $(LOCAL_PATH)/ffmpeg/lib/libavutil-52.so
#include $(PREBUILT_SHARED_LIBRARY)

#include $(CLEAR_VARS)
#LOCAL_MODULE := avformat
#LOCAL_SRC_FILES := $(LOCAL_PATH)/ffmpeg/lib/libavformat-55.so
#include $(PREBUILT_SHARED_LIBRARY)

#include $(CLEAR_VARS)
#LOCAL_MODULE := avcodec
#LOCAL_SRC_FILES := $(LOCAL_PATH)/ffmpeg/lib/libavcodec-55.so
#include $(PREBUILT_SHARED_LIBRARY)

#include $(CLEAR_VARS)
#LOCAL_MODULE := swresample
#LOCAL_SRC_FILES := $(LOCAL_PATH)/ffmpeg/lib/libswresample-0.so
#include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := static-avutil
LOCAL_SRC_FILES := ffmpeg/lib/libavutil.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := static-avformat
LOCAL_SRC_FILES := ffmpeg/lib/libavformat.a
include $(PREBUILT_STATIC_LIBRARY) 

include $(CLEAR_VARS)
LOCAL_MODULE    := static-avcodec
LOCAL_SRC_FILES := ffmpeg/lib/libavcodec.a
include $(PREBUILT_STATIC_LIBRARY) 

include $(CLEAR_VARS)
LOCAL_MODULE    := static-swresample
LOCAL_SRC_FILES := ffmpeg/lib/libswresample.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE    := aplayer-jni

LOCAL_SRC_FILES := 	android/TimedEventQueue.cpp \
					android/ColorConverter.cpp \
					io/ANativeWindowRenderer.cpp \
					io/OpenSLMixer.cpp \
					Common.cpp \
					Prefetcher.cpp \
					MessageQueue.cpp \
					MediaInfo.cpp \
					FFMPEGer.cpp \
					AudioEngine.cpp \
					VideoEngine.cpp \
					LocalMediaPlayer.cpp \
					com_openamedia_player_APlayer.cpp

#LOCAL_SHARED_LIBRARIES := 	avutil \
							avformat \
							avcodec \
							swresample

LOCAL_STATIC_LIBRARIES :=	static-avformat \
							static-avcodec \
							static-swresample \
							static-avutil


LOCAL_C_INCLUDES :=	ffmpeg/include \
					openmax \

LOCAL_LDLIBS    += 	-llog \
					-landroid \
					-lOpenSLES \
					-lz

LOCAL_CFLAGS    += -UNDEBUG

#-l targeted so can only be found at system/lib. but LOCAL_SHARED_LIBRARIES ones will be automaticly copied to libs and can be found at loading.
#LOCAL_LDFLAGS += 	-L$(LOCAL_PATH)/ffmpeg/lib \
					-lavutil-52 \
					-lavformat-55 \
					-lavcodec-55 \
					-lavfilter-4 \
					-lavdevice-55 \
					-lswresample-0 \
					-lswscale-2

include $(BUILD_SHARED_LIBRARY)