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

/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_openamedia_player_APlayer */

#ifndef _Included_com_openamedia_player_APlayer
#define _Included_com_openamedia_player_APlayer
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_openamedia_player_APlayer
 * Method:    nativeInit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_openamedia_player_APlayer_nativeInit
  (JNIEnv *, jclass);

/*
 * Class:     com_openamedia_player_APlayer
 * Method:    nativeSetup
 * Signature: (Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_com_openamedia_player_APlayer_nativeSetup
  (JNIEnv *, jobject, jobject);

/*
 * Class:     com_openamedia_player_APlayer
 * Method:    nativeSetPath
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_openamedia_player_APlayer_nativeSetPath
  (JNIEnv *, jobject, jstring);

/*
 * Class:     com_openamedia_player_APlayer
 * Method:    nativeSetSurface
 * Signature: (Landroid/view/Surface;)V
 */
JNIEXPORT void JNICALL Java_com_openamedia_player_APlayer_nativeSetSurface
  (JNIEnv *, jobject, jobject);

/*
 * Class:     com_openamedia_player_APlayer
 * Method:    nativePrepare
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_openamedia_player_APlayer_nativePrepare
  (JNIEnv *, jobject);

/*
 * Class:     com_openamedia_player_APlayer
 * Method:    nativePlay
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_openamedia_player_APlayer_nativePlay
  (JNIEnv *, jobject);

/*
 * Class:     com_openamedia_player_APlayer
 * Method:    nativeSeek
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_openamedia_player_APlayer_nativeSeek
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_openamedia_player_APlayer
 * Method:    nativePause
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_openamedia_player_APlayer_nativePause
  (JNIEnv *, jobject);

/*
 * Class:     com_openamedia_player_APlayer
 * Method:    nativeStop
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_openamedia_player_APlayer_nativeStop
  (JNIEnv *, jobject);

/*
 * Class:     com_openamedia_player_APlayer
 * Method:    nativeGetDuration
 * Signature: ()V
 */
JNIEXPORT int JNICALL Java_com_openamedia_player_APlayer_nativeGetDuration
  (JNIEnv *, jobject);


/*
 * Class:     com_openamedia_player_APlayer
 * Method:    nativeGetPosition
 * Signature: ()V
 */
JNIEXPORT int JNICALL Java_com_openamedia_player_APlayer_nativeGetPosition
  (JNIEnv *, jobject);


/*
 * Class:     com_openamedia_player_APlayer
 * Method:    nativeRelease
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_openamedia_player_APlayer_nativeRelease
  (JNIEnv *, jobject);

#ifdef __cplusplus
}
#endif
#endif