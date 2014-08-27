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

#include "com_openamedia_player_APlayer.h"
#include "LocalMediaPlayer.h"

#include <jni.h>
#include <stdint.h>
#include <android/native_window_jni.h>

#define  LOG_TAG "APlayer-jni"
#include "android/Log.h"

using namespace openamedia;

static jfieldID JCLASS_FIELD_ID_NATIVE_CONTEXT = NULL;
static jmethodID JCLASS_METHOD_ID_POST_EVENT_FROM_NATIVE = NULL;

static JavaVM* g_jvm = NULL;

/////////////////////////////////////
class JNIMediaPlayerListener : public MediaPlayerListener {
public:
	JNIMediaPlayerListener(JNIEnv *env, jobject thiz, jobject weak_this);
	~JNIMediaPlayerListener();

	virtual void notify(int msg, int ext1, int ext2, const void* data, int data_size);
	virtual void notifyLog(const char* log);
	
	virtual void registerCurThread();
	virtual void unRegisterCurThread();
	
private:
	JNIEnv* mEnv;
	jclass mClass;
	jobject mObject;
};

JNIMediaPlayerListener::JNIMediaPlayerListener(JNIEnv *env, jobject thiz, jobject weak_this){
	jclass clazz = env->GetObjectClass(thiz);
	if(clazz == NULL){
		ALOGE("can not get jclass from jobject");
		return;
	}
	
	// Hold onto the APlayer class for use in calling the static method
    // that posts events to the application thread.
	mClass = (jclass)env->NewGlobalRef(clazz);

	// We use a weak reference so the APlayer object can be garbage collected.
    // The reference is only used as a proxy for callbacks.
	mObject = env->NewGlobalRef(weak_this);

	mEnv = env;
}

JNIMediaPlayerListener::~JNIMediaPlayerListener(){
	// remove global references
    mEnv->DeleteGlobalRef(mObject);
    mEnv->DeleteGlobalRef(mClass);
}

//call back to java
void JNIMediaPlayerListener::notify(int msg, int ext1, int ext2, const void* data, int data_size){
	jbyteArray array = NULL;
	if(data != NULL){
		array = mEnv->NewByteArray(data_size);
		if(!array){
			ALOGE("fail to new byteArray for notify!!");
		}else{
			jbyte* bytes = mEnv->GetByteArrayElements(array, NULL);
			if (bytes != NULL) {
				memcpy(bytes, data, data_size);
				mEnv->ReleaseByteArrayElements(array, bytes, 0);
			}
		}
	}
	
	mEnv->CallStaticVoidMethod(mClass, JCLASS_METHOD_ID_POST_EVENT_FROM_NATIVE, mObject, msg, ext1, ext2, array);

	//check while native call back to java.
	if (mEnv->ExceptionCheck()) {
        ALOGE("An exception occurred while notifying an event.");
        mEnv->ExceptionClear();
    }
}

void JNIMediaPlayerListener::notifyLog(const char* log){
	notify(NATIVE_MSG_NOTIFY_LOG_INFO, 0, 0, log, strlen(log) + 1);
}

void JNIMediaPlayerListener::registerCurThread(){
	g_jvm->AttachCurrentThread(&mEnv, NULL);
}

void JNIMediaPlayerListener::unRegisterCurThread(){
	g_jvm->DetachCurrentThread();
}

/////////////////////////////////////


JNIEXPORT void JNICALL Java_com_openamedia_player_APlayer_nativeInit(JNIEnv *env, jclass clazz){
	JCLASS_FIELD_ID_NATIVE_CONTEXT = env->GetFieldID(clazz, "mNativeContext", "I");
	JCLASS_METHOD_ID_POST_EVENT_FROM_NATIVE = env->GetStaticMethodID(clazz, "postEventFromNative", "(Ljava/lang/Object;III[B)V");
}

//call back to java
static MediaPlayer* getMediaPlayer(JNIEnv* env, jobject thiz){
	MediaPlayer* mp = (MediaPlayer*)env->GetIntField(thiz, JCLASS_FIELD_ID_NATIVE_CONTEXT);
	return mp;
}

//call back to java
static void setMediaPlayer(JNIEnv* env, jobject thiz, const MediaPlayer* player){
	MediaPlayer* old = getMediaPlayer(env, thiz);
	if(old){
		delete old;
		old = NULL;
	}

	if(player){
		env->SetIntField(thiz, JCLASS_FIELD_ID_NATIVE_CONTEXT, (int)player);	
	}
}

JNIEXPORT void JNICALL Java_com_openamedia_player_APlayer_nativeSetup(JNIEnv *env, jobject thiz, jobject weak_this){
	MediaPlayer* mp = new LocalMediaPlayer();
	if (mp == NULL) {
        ALOGE("create mediaplayer failed by Out of memory");
        return;
    }

	JNIMediaPlayerListener* listener = new JNIMediaPlayerListener(env, thiz, weak_this);
	mp->setListener(listener);

	setMediaPlayer(env, thiz, mp);
}

JNIEXPORT void JNICALL Java_com_openamedia_player_APlayer_nativeSetPath(JNIEnv *env, jobject thiz, jstring path){
	MediaPlayer* mp = getMediaPlayer(env, thiz);
	if(mp == NULL){
		ALOGE("no mediaplayer found for setPath");
		return;
	}

	if(path == NULL){
		ALOGE("no path str found for setPath");
		return;
	}

	const char *tmp = env->GetStringUTFChars(path, NULL);
    if (tmp == NULL) {
		ALOGE("fail to get utfchars from path jstr by Out of memory");
        return;
    }
	
	mp->setPath(tmp);
	
	env->ReleaseStringUTFChars(path, tmp);
	tmp = NULL;
}

JNIEXPORT void JNICALL Java_com_openamedia_player_APlayer_nativeSetSurface(JNIEnv *env, jobject thiz, jobject surface){
	MediaPlayer* mp = getMediaPlayer(env, thiz);
	if(mp == NULL){
		ALOGE("no mediaplayer found for setSurface");
		return;
	}

	ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
	if(window == NULL){
		ALOGE("no window found for the surface");
		return;
	}

	mp->setVideoSink(window);
}

JNIEXPORT void JNICALL Java_com_openamedia_player_APlayer_nativePrepare(JNIEnv *env, jobject thiz){
	MediaPlayer* mp = getMediaPlayer(env, thiz);
	if(mp == NULL){
		ALOGE("no mediaplayer found for prepare");
		return;
	}

	mp->prepare();
}

JNIEXPORT void JNICALL Java_com_openamedia_player_APlayer_nativePlay(JNIEnv *env, jobject thiz){
	MediaPlayer* mp = getMediaPlayer(env, thiz);
	if(mp == NULL){
		ALOGE("no mediaplayer found for play");
		return;
	}

	mp->play();
}

JNIEXPORT void JNICALL Java_com_openamedia_player_APlayer_nativeSeek(JNIEnv *env, jobject thiz, jint time){
	MediaPlayer* mp = getMediaPlayer(env, thiz);
	if(mp == NULL){
		ALOGE("no mediaplayer found for seek");
		return;
	}
	
	int64_t ms = (int)time * 1000;
	
	mp->seek(ms);
}

JNIEXPORT void JNICALL Java_com_openamedia_player_APlayer_nativePause(JNIEnv *env, jobject thiz){
	MediaPlayer* mp = getMediaPlayer(env, thiz);
	if(mp == NULL){
		ALOGE("no mediaplayer found for pause");
		return;
	}

	mp->pause();
}

JNIEXPORT void JNICALL Java_com_openamedia_player_APlayer_nativeStop(JNIEnv *env, jobject thiz){
	MediaPlayer* mp = getMediaPlayer(env, thiz);
	if(mp == NULL){
		ALOGE("no mediaplayer found for stop");
		return;
	}
	
	mp->stop();
}

JNIEXPORT int JNICALL Java_com_openamedia_player_APlayer_nativeGetDuration(JNIEnv *env, jobject thiz){
	MediaPlayer* mp = getMediaPlayer(env, thiz);
	if(mp == NULL){
		ALOGE("no mediaplayer found for stop");
		return 0;
	}

	return mp->getDuration();
}

JNIEXPORT int JNICALL Java_com_openamedia_player_APlayer_nativeGetPosition(JNIEnv *env, jobject thiz){
	MediaPlayer* mp = getMediaPlayer(env, thiz);
	if(mp == NULL){
		ALOGE("no mediaplayer found for stop");
		return 0;
	}

	return mp->getPosition();
}


JNIEXPORT void JNICALL Java_com_openamedia_player_APlayer_nativeRelease(JNIEnv *env, jobject thiz){
	setMediaPlayer(env, thiz, NULL);
}

jint JNI_OnLoad(JavaVM* vm, void* reserved){
	g_jvm = vm;

	return JNI_VERSION_1_4;
}
