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

#ifndef OPENAMEDIA_MEDIA_PLAYER_H
#define OPENAMEDIA_MEDIA_PLAYER_H

#include <stdint.h>
#include <android/native_window_jni.h>

#include "Prefetcher.h"
#include "SyncTimer.h"

namespace openamedia {

	class Engine {
	public:
		virtual ~Engine(){}
		
		virtual bool setTimeSync(SyncTimer* timer) = 0;
		virtual bool start() = 0;
		virtual bool stop() = 0;
		virtual bool pause() = 0;
		virtual bool resume() = 0;
		virtual bool seek(int64_t time) = 0;

	protected:
		Engine(){}
		
	private:
		Engine(const Engine&);
		Engine& operator=(const Engine&);
	};
	
	class MediaPlayerListener {
	public:
		virtual ~MediaPlayerListener(){}
		
		static const int NATIVE_MSG_NOTIFY_VIDEO_SIZE = 0;//must keep the same with APlayer.java.
		static const int NATIVE_MSG_NOTIFY_LOG_INFO = 1;
		static const int NATIVE_MSG_NOTIFY_SET_LISTENER_DONE = 2;
		static const int NATIVE_MSG_NOTIFY_SET_PATH_DONE = 3;
		static const int NATIVE_MSG_NOTIFY_SET_VIDEO_SINK_DONE = 4;
		static const int NATIVE_MSG_NOTIFY_PREPARE_DONE = 5;
		static const int NATIVE_MSG_NOTIFY_PLAY_DONE = 6;
		static const int NATIVE_MSG_NOTIFY_SEEK_DONE = 7;
		static const int NATIVE_MSG_NOTIFY_PAUSE_DONE = 8;
		static const int NATIVE_MSG_NOTIFY_ERROR = 9;
		static const int NATIVE_MSG_NOTIFY_EOS = 10;
	
		virtual void notify(int msg, int ext1 = 0, int ext2 = 0, const void* data = NULL, int dataSize = 0) = 0;
		virtual void notifyLog(const char* log) = 0;
	
		virtual void registerCurThread() = 0;
		virtual void unRegisterCurThread() = 0;

	protected:
		MediaPlayerListener(){}		
		
	private:
		MediaPlayerListener(const MediaPlayerListener&);
		MediaPlayerListener& operator=(const MediaPlayerListener&);
	};

	class MediaPlayer {
	public:
		virtual ~MediaPlayer(){}
		
		virtual void setListener(MediaPlayerListener* listener) = 0;
		virtual void setPath(const char* path) = 0;
		virtual void setVideoSink(ANativeWindow* window) = 0;
		virtual void prepare() = 0;
		virtual void play() = 0;
		virtual void seek(int64_t timeUS) = 0;
		virtual void pause() = 0;
		virtual void stop() = 0;
		virtual int getDuration() = 0;//int unit of sec.
		virtual int getPosition() = 0;

		enum State {
			STATE_IDLE,
			STATE_INITIALIZED,
			STATE_PREPARED,
			STATE_PLAYING,
			STATE_PAUSED,
			STATE_SEEKING,
			STATE_STOPPED,
			STATE_EOS,
			STATE_ERROR,
		};

	protected:
		MediaPlayer(){}

	private:
		MediaPlayer(const MediaPlayer&);
		MediaPlayer& operator=(const MediaPlayer&);
	};

}//namespace

#endif//MEDIA_PLAYER_H
