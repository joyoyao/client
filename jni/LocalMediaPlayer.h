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

#ifndef OPENAMEDIA_LOCAL_MEDIA_PLAYER_H
#define OPENAMEDIA_LOCAL_MEDIA_PLAYER_H

#include "MediaPlayer.h"
#include "Common.h"
#include "Prefetcher.h"
#include "android/TimedEventQueue.h"

#include "FFMPEGer.h"
#include "io/ANativeWindowRenderer.h"
#include "AudioEngine.h"
#include "VideoEngine.h"
#include "MessageQueue.h"
#include "SyncTimer.h"

namespace openamedia {

	class LMPEvent;
	class LMPSource;
	
	class LocalMediaPlayer : public MediaPlayer {
	public:
		LocalMediaPlayer();

		enum MsgType {
			MSG_SET_LISTENER = 0,
			MSG_SET_PATH,
			MSG_SET_VIDEO_SINK,
			MSG_PREPARE,
			MSG_PLAY,
			MSG_SEEK,
			MSG_PAUSE,
			MSG_STOP,
			MSG_NOTIFY,
		};
		
		virtual void setListener(MediaPlayerListener* listener);
		virtual void setPath(const char* path);
		virtual void setVideoSink(ANativeWindow* window);
		virtual void prepare();
		virtual void play();
		virtual void seek(int64_t timeUS);
		virtual void pause();
		virtual void stop();
		virtual int getDuration();
		virtual int getPosition();
		virtual void notify(int msg, int ext1 = 0, int ext2 = 0, const void* data = NULL, int dataSize = 0);
		virtual void notifyLog(const char* log);

	protected:
		virtual ~LocalMediaPlayer();
				
	private:
		State mState;
		MediaPlayerListener* mListener;
		char mPath[MAX_STRING_PATH_LEN];
		
		LMPSource* mPrefetcherSource;
		Prefetcher* mPrefetcher;

		AudioEngine* mAudioEngine;
		VideoEngine* mVideoEngine;
		ANativeWindow* mNativeWindow;

		SyncTimer mTimer;

		void onPrepare();
		void onPlay();
		void onSeek(int64_t);
		void onPause();
		void onNativeNotify(int msg, int ext1, int ext2, const void* data, int dataSize);

		MessageQueue* mMsgQueue;
		static void HandleMessage(Message* msg, void* me);
		void handleMessage(Message* msg);
		static void ThreadExit(void* me);
		void threadExit();

		LocalMediaPlayer(const LocalMediaPlayer&);
		LocalMediaPlayer& operator=(const LocalMediaPlayer&);
	};

}//namespace

#endif//LOCAL_MEDIA_PLAYER_H
