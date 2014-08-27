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

#include "AudioEngine.h"

#define LOG_TAG "AudioEngine"
#include "android/Log.h"

namespace openamedia {
	
	AudioEngine::AudioEngine()
		:mDone(false),
		 mStarted(false),
		 mReadyForReadDone(false),
		 mPausing(false),
		 mSeeking(false),
		 mSeekTime(-1),
		 mSource(NULL),
		 mMixer(NULL){
		
	}
	
	AudioEngine::~AudioEngine(){

	}

	bool AudioEngine::setSource(Prefetcher::SubSource* src){
		Mutex::Autolock ao(mLock);

		mSource = src;

		return true;
	}
	
	bool AudioEngine::setTimeSync(SyncTimer* timer){
		Mutex::Autolock ao(mLock);
		
		mTimer = timer;
		
		return true;
	}
	
	bool AudioEngine::start(){
		Mutex::Autolock ao(mLock);
		
		if(mStarted)
			return true;
		
		bool res;
		MediaBuffer* buffer = NULL;
		
		int tryTimes = 50;
		for(;;){
			res = mSource->acquireRead();
			if(!res){//temp out of filled buffer.
				mCond.waitRelative(mLock, 40000000);
				if(tryTimes-- <= 0)
					break;
				else
					continue;
			}
			
			res = mSource->read(&buffer);
			if(!res){
				//shall never get here!!
				break;
			}
		}

		if(buffer == NULL){
			ALOGE("fail to get a first media buffer to init mixer!!!");
			return false;
		}

		mMixer = new OpenSLMixer(&FillBuffer, this, &buffer->audioInfo);
		res = mMixer->init();
		if(!res){
			mSource->doneRead();
			return false;
		}
		
		mMixer->write(buffer->data, buffer->size);
		
		mReadyForReadDone = true;

		mStarted = true;

		return true;
	}
	
	bool AudioEngine::stop(){
		{
			Mutex::Autolock ao(mLock);
			
			if(!mStarted)
				return true;
			
			mDone = true;			
			
			mStarted = false;
		}
		
		mMixer->deInit();//must be placed outside the mLock. otherwise, the hold mLock will block the callback fillbuffer.
		
		return true;
	}
	
	bool AudioEngine::pause(){
		Mutex::Autolock ao(mLock);
		
		if(!mStarted)
			return false;

		mMixer->pause();

		return true;
	}
	
	bool AudioEngine::resume(){
		Mutex::Autolock ao(mLock);
		
		if(!mStarted)
			return false;
		
		mMixer->resume();
		
		return true;		
	}
	
	bool AudioEngine::seek(int64_t time){
		Mutex::Autolock ao(mLock);
		
		return false;
	}
	
	bool AudioEngine::FillBuffer(void** data, int* dataSize, void* context){
		AudioEngine* engine = (AudioEngine*)context;
		return engine->fillBuffer(data, dataSize);
	}
	
	bool AudioEngine::fillBuffer(void** data, int* dataSize){
		Mutex::Autolock ao(mLock);

		if(!mStarted)
			return false;

		if(mReadyForReadDone){
			mSource->doneRead();
			mReadyForReadDone = false;
		}

		bool res;
		MediaBuffer* buffer = NULL;
		
		for(;;){
			if(mDone){
				return false;
			}
			res = mSource->acquireRead();
			if(!res){//temp out of filled buffer.
				ALOGV("tmp out of filled buffer and continue");
				mCond.waitRelative(mLock, 20 * 1000000);
				continue;
			}
			
			res = mSource->read(&buffer);
			if(!res){
				//shall never get here!!
				break;
			}
			mReadyForReadDone = true;
		}

		if(buffer == NULL){
			return false;
		}

		*data = buffer->data;
		*dataSize = buffer->size;

		mTimer->setTime(buffer->ptsMS);
		
		return true;
	}
	
}//namespace
