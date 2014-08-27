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

#include "Prefetcher.h"

#define LOG_TAG "Prefetcher"
#include "android/Log.h"

#include "LocalMediaPlayer.h"

namespace openamedia {

	///////////////////////////////////////
	Prefetcher::SubSource::SubSource(int capacity)
		:mDone(false),
		 mLastReadDone(true),
		 mLastWriteDone(true){
		for(int i = 0; i < capacity; ++i){
			MediaBuffer* buf = new MediaBuffer;
			mFreeList.push_back(buf);
		}
	}
	
	Prefetcher::SubSource::~SubSource(){
		stop();
		
		List<MediaBuffer*>::iterator it = mBusyList.begin();
		while(it != mBusyList.end()){
			MediaBuffer* buf = *it;
			free(buf->data);
			free(buf);
			it = mBusyList.erase(it);
		}

		it = mFreeList.begin();
		while(it != mFreeList.end()){
			MediaBuffer* buf = *it;
			free(buf->data);
			free(buf);
			it = mFreeList.erase(it);
		}
	}

	void Prefetcher::SubSource::stop(){
		Mutex::Autolock ao(mLock);

		mDone = true;
	}
	
	bool Prefetcher::SubSource::acquireRead(){
		Mutex::Autolock ao(mLock);

		if(mDone)
			return false;

		int times = 2;
		if(times-- > 0 &&
		   mBusyList.empty()){
			mCond.waitRelative(mLock, 50 * 1000000);
		}

		return !mBusyList.empty();//timeout or what.
	}
	
	bool Prefetcher::SubSource::read(MediaBuffer** buffer){
		Mutex::Autolock ao(mLock);

		if(!mLastReadDone)
			return false;
		
		while(!mDone && mBusyList.empty()){
			mCond.waitRelative(mLock, 50 * 1000000);
		}

		if(mDone)
			return false;

		MediaBuffer* buf = *mBusyList.begin();

		*buffer = buf;

		mLastReadDone = false;
		
		return true;
	}
	
	bool Prefetcher::SubSource::doneRead(){
		Mutex::Autolock ao(mLock);

		if(mLastReadDone)
			return false;
		
		MediaBuffer* buf = *mBusyList.begin();
		mBusyList.erase(mBusyList.begin());
		mFreeList.push_back(buf);
		mCond.signal();

		mLastReadDone = true;
	}
	
	bool Prefetcher::SubSource::acquireWrite(){
		Mutex::Autolock ao(mLock);

		if(mDone)
			return false;

		int times = 2;
		if(times-- > 0 &&
		   mFreeList.empty()){
			mCond.waitRelative(mLock, 50 * 1000000);
		}

		return !mFreeList.empty();
	}
	
	bool Prefetcher::SubSource::write(MediaBuffer* buffer){
		Mutex::Autolock ao(mLock);

		if(!mLastWriteDone)
			return false;
		
		if(buffer->size <= 0 ||
		   buffer->size > MAX_SUB_SOURCE_BUFFER_SIZE){
			return false;
		}
		
		while(!mDone && mFreeList.empty()){
			mCond.waitRelative(mLock, 50 * 1000000);
		}

		if(mDone)
			return false;

		MediaBuffer* buf = *mFreeList.begin();

		if(buf->data != NULL &&
		   buf->size < buffer->size){
			free(buf->data);
			buf->data = NULL;
		}
		
		if(buf->data == NULL){
			buf->data = malloc(buffer->size + 1);//extra 1 for safe..
			if(buf->data == NULL){
				ALOGE("fail to alloc data size of:%d", buffer->size);
				return false;
			}
		}
		
		memcpy(buf->data, buffer->data, buffer->size);
		buf->size = buffer->size;
		buf->type = buffer->type;
		buf->ptsMS = buffer->ptsMS;
		buf->decompressed = buffer->decompressed;
		memcpy(&(buf->videoInfo), &(buffer->videoInfo), sizeof(VideoInfo));
		memcpy(&(buf->audioInfo), &(buffer->audioInfo), sizeof(AudioInfo));
		
		mLastWriteDone = false;
		
		return true;
	}

	bool Prefetcher::SubSource::doneWrite(){
		Mutex::Autolock ao(mLock);

		if(mLastWriteDone)
			return false;
		
		MediaBuffer* buf = *mFreeList.begin();
		mFreeList.erase(mFreeList.begin());
		mBusyList.push_back(buf);
		mCond.signal();

		mLastWriteDone = true;
	}

	void Prefetcher::SubSource::clear(){
		Mutex::Autolock ao(mLock);

		List<MediaBuffer*>::iterator it = mBusyList.begin();
		while(it != mBusyList.end()){
			MediaBuffer* buf = *it;
			mFreeList.push_back(buf);
			
			it = mBusyList.erase(it);
		}

		mLastWriteDone = true;
		mLastReadDone = true;
	}
	///////////////////////////////////

	Prefetcher::Prefetcher(Source* src, LocalMediaPlayer* player)
		:mStarted(false),
		 mSource(src),
		 mSeeking(false),
		 mSeekTime(0),
		 mDone(false),
		 mThreadExited(false),
		 mEOS(false),
		 mPlayer(player){
		mVideoSource = new SubSource(DEFAULT_VIDEO_SUB_SOURCE_CAPACITY);
		mAudioSource = new SubSource(DEFAULT_AUDIO_SUB_SOURCE_CAPACITY);		
	}
	
	Prefetcher::~Prefetcher(){
		stop();
	}

	void Prefetcher::start(){
		if(mStarted)
			return;

		pthread_attr_t attr;
		pthread_attr_init(&attr);
		//pthread_attr_setdetachstate(&attr, PTREAD_CREATE_JOINABLE);
		pthread_create(&mThread, &attr, ThreadWrapper, this);
		pthread_attr_destroy(&attr);

		mStarted = true;
	}
	
	void Prefetcher::stop(){
		Mutex::Autolock ao(mLock);
		if(!mStarted)
			return;

		mDone = true;
		while(!mThreadExited){
			mCond.waitRelative(mLock, 40 * 1000000);
		}

		delete mVideoSource;
		mVideoSource = NULL;
		delete mAudioSource;
		mAudioSource = NULL;

		mSource->stop();

		mStarted = false;
	}

	void Prefetcher::seek(int64_t timeMS){
		Mutex::Autolock ao(mLock);

		if(!mStarted)
			return;

		mSeeking = true;
		mSeekTime = timeMS;

		mSource->seek(timeMS);

		mVideoSource->clear();
		mAudioSource->clear();

		mEOS = false;
	}

	Prefetcher::SubSource* Prefetcher::getSource(MediaType type){
		if(type == MEDIA_TYPE_VIDEO)
			return mVideoSource;
		else if(type == MEDIA_TYPE_AUDIO)
			return mAudioSource;
		else
			return NULL;
	}

	void* Prefetcher::ThreadWrapper(void* me){
		Prefetcher* p = (Prefetcher*)me;
		p->threadEntry();

		return NULL;
	}
	
	void Prefetcher::threadEntry(){
		for(;;){
			Mutex::Autolock ao(mLock);

			if(mDone){
				break;
			}

			if(mEOS){
				mCond.waitRelative(mLock, 200 * 1000000);
				continue;
			}
			
			bool res;
			MediaBuffer buffer;

			if(mLastReadBuffer.data != NULL){
				memcpy(&buffer, &mLastReadBuffer, sizeof(MediaBuffer));

				memset(&mLastReadBuffer, 0, sizeof(MediaBuffer));
			}else{
				status_t ret = mSource->read(&buffer);
				if(ret != OK){
					if(ret == ERROR_END_OF_STREAM){
						mPlayer->notify(MediaPlayerListener::NATIVE_MSG_NOTIFY_EOS);
						mEOS = true;
					}
					
					ALOGE("fail to read src and continue!");
					mCond.waitRelative(mLock, 20 * 1000000);
					continue;//wait shall be placed before every 'continue' to previent the too much frequent token of mLock. which will block the other mLock(in stop()).
				}
			}

			SubSource* sub = NULL;
			if(buffer.type == MEDIA_TYPE_VIDEO){
				sub = mVideoSource;
			}else if(buffer.type == MEDIA_TYPE_AUDIO){
				sub = mAudioSource;
			}else{
				ALOGE("unknown buffer type:%d read from source!!", buffer.type);
				break;
			}
			
			res = sub->acquireWrite();
			if(!res){//timeout
				memcpy(&mLastReadBuffer, &buffer, sizeof(MediaBuffer));
				mCond.waitRelative(mLock, 20 * 1000000);
				continue;
			}
						
			sub->write(&buffer);
			sub->doneWrite();
		}

		{
			Mutex::Autolock ao(mLock);
			mThreadExited = true;
			mCond.signal();
		}
	}

}
