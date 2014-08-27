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

#include "VideoEngine.h"

#define LOG_TAG "VideoEngine"
#include "android/Log.h"

namespace openamedia {

	VideoEngine::VideoEngine()
		:mStarted(false),
		 mDone(false),
		 mThreadExited(false),
		 mPausing(false),
		 mSeeking(false),
		 mSeekTime(-1),
		 mNativeWindow(NULL),
		 mRenderer(NULL){
		
	}
	
	VideoEngine::~VideoEngine(){

	}

	bool VideoEngine::setSource(Prefetcher::SubSource* src){
		Mutex::Autolock ao(mLock);
		
		mSource = src;

		return true;
	}

	bool VideoEngine::setSink(ANativeWindow* window){
		Mutex::Autolock ao(mLock);

		if(mRenderer != NULL){
			delete mRenderer;
			mRenderer = NULL;
		}
		
		mNativeWindow = window;

		return true;
	}
	
	bool VideoEngine::setTimeSync(SyncTimer* timer){
		mTimer = timer;
		
		return true;
	}
	
	bool VideoEngine::start(){
		Mutex::Autolock ao(mLock);

		if(mStarted)
			return true;

		if(mSource == NULL ||
		   mNativeWindow == NULL){
			ALOGE("VideoEngine can not start by source or nativewindow is null!!");
			return false;
		}
		
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		pthread_create(&mThread, &attr, ThreadWrapper, this);
		pthread_attr_destroy(&attr);

		mStarted = true;

		return true;
	}

	void* VideoEngine::ThreadWrapper(void* me){
		VideoEngine* engine = (VideoEngine*)me;
		engine->threadEntry();

		return NULL;
	}
	
	void VideoEngine::threadEntry(){
		for(;;){
			Mutex::Autolock ao(mLock);

			bool res;

			if(mDone){
				break;
			}

			if(mPausing){
				mCond.waitRelative(mLock, 40000000);
				continue;
			}

			if(mSeeking){

			}

			res = mSource->acquireRead();
			if(!res){//temp out of filled buffer.
				ALOGV("tmp out of filled buffer and continue");
				mCond.waitRelative(mLock, 20000000);
				continue;
			}

			MediaBuffer* buffer;
			res = mSource->read(&buffer);
			if(!res){
				//shall never get here!!!
				break;
			}
			
			if(mRenderer == NULL){
				mRenderer = new ANativeWindowRenderer(mNativeWindow, &(buffer->videoInfo));
				if(!mRenderer->init()){
					mSource->doneRead();//whenever subsource read completed, doneRead it to clean/recycle.
					break;
				}
			}

			int64_t nowMS = mTimer->getTime();
			int64_t delay = nowMS - buffer->ptsMS;
			if(delay >= 100){
				ALOGE("video too late, drop frame!!");
				mSource->doneRead();
				continue;//too late and drop frame.
			}else if(delay < -20){
				if(delay < -1000)
					delay = -1000;
				//ALOGV("video early, wait some. -delay:%lld", -delay);
				
				mCond.waitRelative(mLock, -delay * 1000000);
			}
			
			mRenderer->render(buffer);
			mSource->doneRead();
		}

		{
			Mutex::Autolock ao(mLock);
			mThreadExited = true;
			mCond.signal();
		}		
	}
	
	bool VideoEngine::stop(){
		Mutex::Autolock ao(mLock);
		
		if(!mStarted)
			return true;

		mDone = true;
		while(!mThreadExited){
			mCond.waitRelative(mLock, 40000000);
		}
		
		mStarted = false;

		return true;
	}
	
	bool VideoEngine::pause(){
		Mutex::Autolock ao(mLock);

		if(!mStarted)
			return false;

		mPausing = true;

		return true;
	}

	bool VideoEngine::resume(){
		Mutex::Autolock ao(mLock);

		if(!mStarted)
			return false;
		
		mPausing = false;

		return true;
	}
	
	bool VideoEngine::seek(int64_t time){
		Mutex::Autolock ao(mLock);

		mSeeking = true;
		mSeekTime = time;

		return true;
	}

	
}//namespace
