/*
 * Copyright (c) 2014 Jim Qian
 *
 * This file is part of OpenaMedia-C(client).
 *
 * OpenaMedia-C is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with OpenaMedia-C; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#define LOG_TAG "ANativeWindowRenderer"
#include "android/Log.h"

#include "ANativeWindowRenderer.h"

#include <stdlib.h>
#include <stdio.h>

namespace openamedia {

	ANativeWindowRenderer::ANativeWindowRenderer(ANativeWindow* window, VideoInfo* info)
		:mNativeWindow(window),
		 mConverter(NULL),
		 mInited(false),
		 mCropWidth(0),
		 mCropHeight(0){		
		memcpy(&mInfo, info, sizeof(VideoInfo));

		//TODO: find crop in decoding.
		mRect.left = mRect.top = 0;
		mRect.right = mInfo.width - 1;
        mRect.bottom = mInfo.height - 1;
		
		mCropWidth = mRect.right - mRect.left + 1;
		mCropHeight = mRect.bottom - mRect.top + 1;
	}
	
	ANativeWindowRenderer::~ANativeWindowRenderer(){
		deInit();
	}

	bool ANativeWindowRenderer::init(){
		if(mInited)
			return true;

		ALOGV("init with cropped width:%d, height:%d", mCropWidth, mCropHeight);

		if(mInfo.colorFmt != OMX_COLOR_FormatYUV420Planar){
			ALOGE("nativewindow render does not support color:%d other than OMX_COLOR_FormatYUV420Planar!", mInfo.colorFmt);
			return false;
		}
		
		mConverter = new ColorConverter(mInfo.colorFmt, OMX_COLOR_Format16bitRGB565);
		if(!mConverter->isValid()){
			return false;
		}
		
		int err = ANativeWindow_setBuffersGeometry(mNativeWindow, mInfo.width, mInfo.height, WINDOW_FORMAT_RGB_565);
		if(err != 0){
			ALOGE("fail to native_window_set_buffers_geometry nativewindow!");
			return false;
		}

		mInited = true;

		return true;
	}
	
	bool ANativeWindowRenderer::deInit(){
		if(!mInited)
			return true;

		if(mConverter){
			delete mConverter;
			mConverter = NULL;
		}

		mInited = false;
	}
	
	void ANativeWindowRenderer::render(MediaBuffer* buffer){		
		if(!mInited)
			return;
		
		ANativeWindow_Buffer buf;
		int err;

		ARect bounds(mRect);
		err = ANativeWindow_lock(mNativeWindow, &buf, &bounds);
		if(err != 0){
			ALOGE("fail to ANativeWindow_lock error:%d!", err);
			return;
		}
		
        mConverter->convert(buffer->data,
							mInfo.width, mInfo.height,
							mRect.left, mRect.top, mRect.right, mRect.bottom,
							buf.bits,
							buf.stride, buf.height,
							0, 0, mCropWidth - 1, mCropHeight - 1);

//#define DUMP_YUV_TO_FILE
#ifdef DUMP_YUV_TO_FILE
		FILE* f1 = fopen("/data/yuv", "ab+");
		if(f1 != NULL){
			size_t res = fwrite(buffer->data, 1, buffer->size, f1);
			fclose(f1);
			ALOGV("fwrite %d of %d to /data/yuv!", res, buffer->size);
		}else
			ALOGE("can not fopen /data/yuv!!");
#endif

//#define DUMP_RGB_TO_FILE
#ifdef DUMP_RGB_TO_FILE
		FILE* f2 = fopen("/data/rgb", "ab+");
		if(f2 != NULL){
			size_t res = fwrite(buf.bits, 1, buf.stride*buf.height*2, f2);
			fclose(f2);
			ALOGV("fwrite %d of %d to /data/rgb!", res, buf.stride*buf.height*2);
		}else
			ALOGE("can not fopen /data/rgb!!");
#endif

		err = ANativeWindow_unlockAndPost(mNativeWindow);
		if(err != 0) {
			ALOGE("failed to ANativeWindow_unlockAndPost error %d", err);
			return;
		}
	}


}//namespace
