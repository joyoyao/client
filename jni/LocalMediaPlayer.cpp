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

#include "LocalMediaPlayer.h"
#include <string.h>
#include "FFMPEGer.h"

#define LOG_TAG "LocalMediaPlayer"
#include "android/Log.h"

namespace openamedia {

	///////////////////////////////////////
	class LMPSource : public Prefetcher::Source {
	public:
		LMPSource(const char* path);
		virtual ~LMPSource();
		
		virtual bool start();
		virtual bool stop();
		virtual status_t read(MediaBuffer* buffer);
		virtual bool seek(int64_t timeMS);
		bool hasAudio();
		bool hasVideo();

		bool getVideoInfo(VideoInfo* info);
		bool getAudioInfo(AudioInfo* info);

	private:
		FFMPEGer* mFFMPEG;
		
		LMPSource(const LMPSource&);
		LMPSource& operator=(const LMPSource&);
	};

	LMPSource::LMPSource(const char* path){
		mFFMPEG = new FFMPEGer(path);
	}
	
	LMPSource::~LMPSource(){
		if(mFFMPEG)
			delete mFFMPEG;
	}

	bool LMPSource::start(){
		return mFFMPEG->init();
	}
	
	bool LMPSource::stop(){
		mFFMPEG->deInit();

		return true;
	}
	
	status_t LMPSource::read(MediaBuffer* buffer){
		return mFFMPEG->readDecode(buffer);
	}
	
	bool LMPSource::seek(int64_t timeMS){
		return mFFMPEG->seek(timeMS);
	}
	
	bool LMPSource::hasAudio(){
		return mFFMPEG->hasAudio();
	}
	
	bool LMPSource::hasVideo(){
		return mFFMPEG->hasVideo();
	}

	bool LMPSource::getVideoInfo(VideoInfo* info){
		return mFFMPEG->getVideoInfo(info);
	}

	bool LMPSource::getAudioInfo(AudioInfo* info){
		return mFFMPEG->getAudioInfo(info);
	}
	///////////////////////////////////////

	LocalMediaPlayer::LocalMediaPlayer()
		:mState(STATE_IDLE),
		 mListener(NULL),
		 mPrefetcherSource(NULL),
		 mPrefetcher(NULL),
		 mAudioEngine(NULL),
		 mVideoEngine(NULL),
		 mNativeWindow(NULL),
		 mMsgQueue(NULL){

		mMsgQueue = new MessageQueue(HandleMessage, ThreadExit, this);
	}

	LocalMediaPlayer::~LocalMediaPlayer(){
		ALOGV("~LocalMediaPlayer in");
		
		stop();

		ALOGV("~LocalMediaPlayer in 0");
		
		if(mMsgQueue != NULL)
			delete mMsgQueue;
		if(mPrefetcher != NULL)
			delete mPrefetcher;
		if(mPrefetcherSource != NULL)
			delete mPrefetcherSource;
		if(mAudioEngine != NULL)
			delete mAudioEngine;
		if(mVideoEngine != NULL)
			delete mVideoEngine;

		ALOGV("~LocalMediaPlayer out");
	}

	void LocalMediaPlayer::HandleMessage(Message* msg, void* me){
		LocalMediaPlayer* player = (LocalMediaPlayer*)me;
		player->handleMessage(msg);
	}

	void LocalMediaPlayer::handleMessage(Message* msg){
		switch(msg->what){
		case MSG_SET_LISTENER:
			mListener = (MediaPlayerListener*)msg->obj;
			mListener->registerCurThread();
			
			mListener->notify(MediaPlayerListener::NATIVE_MSG_NOTIFY_SET_LISTENER_DONE);
			break;
		case MSG_SET_PATH:
			{
				const char* from = (const char*)msg->obj;
				strncpy(mPath, from, sizeof(mPath));
				free(msg->obj);

				mListener->notify(MediaPlayerListener::NATIVE_MSG_NOTIFY_SET_PATH_DONE);
				break;
			}
		case MSG_SET_VIDEO_SINK:
			mNativeWindow = (ANativeWindow*)msg->obj;

			mListener->notify(MediaPlayerListener::NATIVE_MSG_NOTIFY_SET_VIDEO_SINK_DONE);
			break;
		case MSG_PREPARE:
			onPrepare();
			break;
		case MSG_PLAY:
			onPlay();
			break;
		case MSG_SEEK:
			onSeek(msg->arg3);
			break;
		case MSG_PAUSE:
			onPause();
			break;
		case MSG_NOTIFY:
			onNativeNotify(msg->arg1, msg->arg2, msg->arg3, msg->obj, msg->objSize);
			break;
		default:
			ALOGE("unknown message type of:%d", msg->what);
			break;
		}
	}

	void LocalMediaPlayer::ThreadExit(void* me){
		LocalMediaPlayer* player = (LocalMediaPlayer*)me;
		player->threadExit();
	}
	
	void LocalMediaPlayer::threadExit(){		
		mListener->unRegisterCurThread();
	}

	void LocalMediaPlayer::setListener(MediaPlayerListener* listener){
		Message* msg = mMsgQueue->obtainMessage();
		msg->what = MSG_SET_LISTENER;
		msg->obj = listener;
		mMsgQueue->sendMessage(msg);
	}

	void LocalMediaPlayer::setPath(const char* path){
		Message* msg = mMsgQueue->obtainMessage();
		msg->what = MSG_SET_PATH;

		int len = strlen(path) + 1;
		char* cpath = (char*)calloc(len, 1);
		memcpy(cpath, path, len);
		
		msg->obj = cpath;
		mMsgQueue->sendMessage(msg);
	}

	void LocalMediaPlayer::setVideoSink(ANativeWindow* window){
		Message* msg = mMsgQueue->obtainMessage();
		msg->what = MSG_SET_VIDEO_SINK;
		msg->obj = window;
		mMsgQueue->sendMessage(msg);
	}

	void LocalMediaPlayer::prepare(){
		Message* msg = mMsgQueue->obtainMessage();
		msg->what = MSG_PREPARE;
		mMsgQueue->sendMessage(msg);
	}

	void LocalMediaPlayer::play(){
		Message* msg = mMsgQueue->obtainMessage();
		msg->what = MSG_PLAY;
		mMsgQueue->sendMessage(msg);
	}

	void LocalMediaPlayer::seek(int64_t timeUS){
		Message* msg = mMsgQueue->obtainMessage();
		msg->what = MSG_SEEK;
		msg->arg3 = timeUS;
		mMsgQueue->sendMessage(msg);
	}

	void LocalMediaPlayer::pause(){
		Message* msg = mMsgQueue->obtainMessage();
		msg->what = MSG_PAUSE;
		mMsgQueue->sendMessage(msg);
	}

	//stop is sync because the pre-destructed of java surfaceview could crash the render with unpredictable error.
	void LocalMediaPlayer::stop(){
		ALOGV("stop");
		
		if(mState != STATE_STOPPED){
			if(mVideoEngine != NULL)
				mVideoEngine->stop();
			
			if(mAudioEngine != NULL)
				mAudioEngine->stop();

			if(mPrefetcher != NULL)
				mPrefetcher->stop();
			
			if(mPrefetcherSource != NULL)
				mPrefetcherSource->stop();

			mState = STATE_STOPPED;
		}
	}

	int LocalMediaPlayer::getDuration(){
		int duration = 0;

		VideoInfo vInfo;
		bool res = mPrefetcherSource->getVideoInfo(&vInfo);
		if(res){
			duration = (int)(vInfo.streamDuration / 1000);
		}else{
			AudioInfo aInfo;
			res = mPrefetcherSource->getAudioInfo(&aInfo);
			if(res){
				duration = (int)(aInfo.streamDuration / 1000);
			}
		}
		
		return duration;
	}
	
	int LocalMediaPlayer::getPosition(){
		return mTimer.getTime() / 1000;
	}

	void LocalMediaPlayer::notify(int notifyMsg, int ext1, int ext2, const void* data, int data_size){
		Message* msg = mMsgQueue->obtainMessage();
		msg->what = MSG_NOTIFY;
		msg->arg1 = notifyMsg;
		msg->arg2 = ext1;
		msg->arg3 = (int64_t)ext2;
		msg->obj = const_cast<void*>(data);
		msg->objSize = data_size;
		mMsgQueue->sendMessage(msg);
	}
	
	void LocalMediaPlayer::notifyLog(const char* log){
		Message* msg = mMsgQueue->obtainMessage();
		msg->what = MSG_NOTIFY;
		msg->arg1 = MediaPlayerListener::NATIVE_MSG_NOTIFY_LOG_INFO;
		msg->obj = const_cast<char*>(log);
		mMsgQueue->sendMessage(msg);
	}

	void LocalMediaPlayer::onPrepare(){
		ALOGV("onPrepare");
		if(mState == STATE_IDLE){
			ALOGV("onPrepareEvent STATE_IDLE");

			mState = STATE_INITIALIZED;
		}
		
		if(mState == STATE_INITIALIZED){
			ALOGV("onPrepareEvent STATE_INITIALIZED");
			
			mPrefetcherSource = new LMPSource(mPath);
			bool res = mPrefetcherSource->start();
			if(!res){
				ALOGE("fail to start PrefetcherSource!!");
				mListener->notifyLog("prefetchersource start error!!");
				mState == STATE_ERROR;
				return;
			}

			ALOGV("onPrepareEvent STATE_INITIALIZED 1");
			
			mPrefetcher = new Prefetcher(mPrefetcherSource, this);

			bool hasVideo = mPrefetcherSource->hasVideo();
			bool hasAudio = mPrefetcherSource->hasAudio();

			if(!hasVideo && !hasAudio){
				ALOGE("no video or audio stream found, so quit the prepare!");
				mListener->notifyLog("no video or audio stream found, so quit the prepare!");

				mState == STATE_ERROR;
				return;
			}

			if(hasVideo){
				mVideoEngine = new VideoEngine;
				mVideoEngine->setSink(mNativeWindow);
				mVideoEngine->setTimeSync(&mTimer);
				mVideoEngine->setSource(mPrefetcher->getSource(MEDIA_TYPE_VIDEO));

				VideoInfo info;
				mPrefetcherSource->getVideoInfo(&info);
				ALOGE("mPrefetcherSource->getVideoInfo wid:%d, hei:%d", info.width, info.height);
				mListener->notify(MediaPlayerListener::NATIVE_MSG_NOTIFY_VIDEO_SIZE, info.width, info.height);

				char log[128] = {0};
				sprintf(log, "VIDEO INFO:\ncodec:%s\nwidth*height:%d*%d\nframe_rate:%d\nstream_duration:%llds\nbit_rate:%d\n", info.codecName, info.width, info.height, info.frameRate, info.streamDuration/1000, info.bitRate);
				mListener->notifyLog(log);
			}
			
			if(hasAudio){
				mAudioEngine = new AudioEngine;
				mAudioEngine->setTimeSync(&mTimer);
				mAudioEngine->setSource(mPrefetcher->getSource(MEDIA_TYPE_AUDIO));
				
				AudioInfo info;
				mPrefetcherSource->getAudioInfo(&info);

				char log[128] = {0};
				sprintf(log, "AUDIO INFO:\ncodec:%s\nsample_rate:%d\nchannels:%d\n", info.codecName, info.sampleRate, info.channels);
				mListener->notifyLog(log);
			}
			
			mState = STATE_PREPARED;
			
			mListener->notify(MediaPlayerListener::NATIVE_MSG_NOTIFY_PREPARE_DONE);
		}
	}

	void LocalMediaPlayer::onPlay(){
		ALOGV("onPlay");
		
		if(mState == STATE_PREPARED){
			ALOGV("onPlay STATE_PREPARED");
				
			mPrefetcher->start();
			
			if(mVideoEngine != NULL)
				mVideoEngine->start();

			if(mAudioEngine != NULL)
				mAudioEngine->start();

			mState = STATE_PLAYING;
			
			mListener->notify(MediaPlayerListener::NATIVE_MSG_NOTIFY_PLAY_DONE);
		}else if(mState == STATE_PAUSED){
			ALOGV("onPlay STATE_PAUSED");
			
			if(mVideoEngine != NULL)
				mVideoEngine->resume();

			if(mAudioEngine != NULL)
				mAudioEngine->resume();

			mState = STATE_PLAYING;

			mListener->notify(MediaPlayerListener::NATIVE_MSG_NOTIFY_PLAY_DONE);
		}
	}

	void LocalMediaPlayer::onSeek(int64_t time){
		ALOGV("onSeek 0");
		mPrefetcher->seek(time);
		
		mListener->notify(MediaPlayerListener::NATIVE_MSG_NOTIFY_SEEK_DONE);
		ALOGV("onSeek 1");
	}

	void LocalMediaPlayer::onPause(){
		ALOGV("onPause");
		if(mState == STATE_PLAYING){
			if(mVideoEngine != NULL)
				mVideoEngine->pause();

			if(mAudioEngine != NULL)
				mAudioEngine->pause();
			
			mState = STATE_PAUSED;

			mListener->notify(MediaPlayerListener::NATIVE_MSG_NOTIFY_PAUSE_DONE);
		}
	}

	void LocalMediaPlayer::onNativeNotify(int nativeMsg, int ext1, int ext2, const void* data, int dataSize){
		if(nativeMsg == MediaPlayerListener::NATIVE_MSG_NOTIFY_LOG_INFO){
			mListener->notifyLog((const char*)data);
		}else{
			mListener->notify(nativeMsg, ext1, ext2, data, dataSize);
		}
	}
	
}//namespace
