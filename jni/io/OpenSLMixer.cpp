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

#include "OpenSLMixer.h"

#define LOG_TAG "OpenSLMixer"
#include "android/Log.h"

namespace openamedia {
		
	SLEnvironmentalReverbSettings OpenSLMixer::reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;
	
	OpenSLMixer::OpenSLMixer(bool (*fillBuffer)(void** data, int* dataSize, void* context), void* context, AudioInfo* info)
		:mInited(false),
		 engineObject(NULL),
		 engineEngine(NULL),
		 outputMixObject(NULL),
		 outputMixEnvironmentalReverb(NULL),
		 bqPlayerObject(NULL),
		 bqPlayerPlay(NULL),
		 bqPlayerBufferQueue(NULL),
		 bqPlayerEffectSend(NULL),
		 bqPlayerMuteSolo(NULL),
		 bqPlayerVolume(NULL),
		 mFillBuffer(fillBuffer),
		 mFillBufferContext(context){
		memcpy(&mInfo, info, sizeof(AudioInfo));
	}
	
	OpenSLMixer::~OpenSLMixer(){
		deInit();
	}

	bool OpenSLMixer::init(){
		if(mInited)
			return true;

		SLresult result;

		do{
			// create engine
			result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
			if(SL_RESULT_SUCCESS != result){
				ALOGE("failed to slCreateEngine!!");
				break;
			}
			(void)result;//??for what?

			// realize the engine
			result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
			if(SL_RESULT_SUCCESS != result){
				ALOGE("failed to realize slengine obj!!");
				break;
			}
			(void)result;

			// get the engine interface, which is needed in order to create other objects
			result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
			if(SL_RESULT_SUCCESS != result){
				ALOGE("failed to get slengine interface!!");
				break;
			}
			(void)result;

			// create output mix, with environmental reverb specified as a non-required interface
			const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
			const SLboolean req[1] = {SL_BOOLEAN_FALSE};
			result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids, req);
			if(SL_RESULT_SUCCESS != result){
				ALOGE("failed to create sl output mix!!");
				break;
			}
			(void)result;

			// realize the output mix
			result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
			if(SL_RESULT_SUCCESS != result){
				ALOGE("failed to realize sl outputmix obj!!");
				break;
			}
			(void)result;

#if 0
			// get the environmental reverb interface
			// this could fail if the environmental reverb effect is not available,
			// either because the feature is not present, excessive CPU load, or
			// the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
			result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
													  &outputMixEnvironmentalReverb);
			if (SL_RESULT_SUCCESS != result) {
				result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(outputMixEnvironmentalReverb, &reverbSettings);
				(void)result;
			}
#endif
			// ignore unsuccessful result codes for environmental reverb, as it is optional for this example

			// configure audio source
			SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
			SLDataFormat_PCM format_pcm;
			format_pcm.formatType = SL_DATAFORMAT_PCM;
			format_pcm.samplesPerSec = mInfo.sampleRate * 1000;
			format_pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
			format_pcm.containerSize = format_pcm.bitsPerSample;
			format_pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;
			format_pcm.numChannels = mInfo.channels;
			if(format_pcm.numChannels == 1){
				format_pcm.channelMask = SL_SPEAKER_FRONT_CENTER;
			}else{
				format_pcm.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
			}
						
			SLDataSource audioSrc = {&loc_bufq, &format_pcm};

			// configure audio sink
			SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
			SLDataSink audioSnk = {&loc_outmix, NULL};


			//**************** create audio player *****************//
			const SLInterfaceID ids2[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND,
										   /*SL_IID_MUTESOLO,*/ SL_IID_VOLUME};
			const SLboolean req2[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
									   /*SL_BOOLEAN_TRUE,*/ SL_BOOLEAN_TRUE};
			result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSrc, &audioSnk,
														3, ids2, req2);
			if(SL_RESULT_SUCCESS != result){
				ALOGE("failed to create sl audioplayer!!");
				break;
			}
			(void)result;

			// realize the player
			result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
			if(SL_RESULT_SUCCESS != result){
				ALOGE("failed to realize sl audioplayer!!");
				break;
			}
			(void)result;

			// get the play interface
			result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
			if(SL_RESULT_SUCCESS != result){
				ALOGE("failed to get sl play interface!!");
				break;
			}
			(void)result;

			// get the buffer queue interface
			result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
													 &bqPlayerBufferQueue);
			if(SL_RESULT_SUCCESS != result){
				ALOGE("failed to get sl buffer queue interface!!");
				break;
			}
			(void)result;

			// register callback on the buffer queue
			result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, this);
			if(SL_RESULT_SUCCESS != result){
				ALOGE("failed to register sl buffer queue callback!!");
				break;
			}
			(void)result;

			// get the effect send interface
			result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_EFFECTSEND,
													 &bqPlayerEffectSend);
			if(SL_RESULT_SUCCESS != result){
				ALOGE("failed to get sl effect send interface!!");
				//break;
			}
			(void)result;

#if 0
			// mute/solo is not supported for sources that are known to be mono, as this is
			// get the mute/solo interface
			result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_MUTESOLO, &bqPlayerMuteSolo);
			if(SL_RESULT_SUCCESS != result){
				ALOGE("failed to get sl mute solo interface!!");
				break;
			}
			(void)result;
#endif

			// get the volume interface
			result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
			if(SL_RESULT_SUCCESS != result){
				ALOGE("failed to get sl volume!!");
				//break;
			}
			(void)result;
			
			// set the player's state to playing
			result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
			if(SL_RESULT_SUCCESS != result){
				ALOGE("failed to set sl play state to play!!");
				break;
			}
			(void)result;
			
			mInited = true;
		}while(0);

		return SL_RESULT_SUCCESS == result;
	}

	bool OpenSLMixer::deInit(){
		if(!mInited)
			return true;

		stop();
		
		// destroy buffer queue audio player object, and invalidate all associated interfaces
		if (bqPlayerObject != NULL) {
			(*bqPlayerObject)->Destroy(bqPlayerObject);
			bqPlayerObject = NULL;
			bqPlayerPlay = NULL;
			bqPlayerBufferQueue = NULL;
			bqPlayerEffectSend = NULL;
			bqPlayerMuteSolo = NULL;
			bqPlayerVolume = NULL;
		}

		// destroy output mix object, and invalidate all associated interfaces
		if (outputMixObject != NULL) {
			(*outputMixObject)->Destroy(outputMixObject);
			outputMixObject = NULL;
			outputMixEnvironmentalReverb = NULL;
		}
		
		// destroy engine object, and invalidate all associated interfaces
		if (engineObject != NULL) {
			(*engineObject)->Destroy(engineObject);
			engineObject = NULL;
			engineEngine = NULL;
		}

		mInited = false;

		return true;
	}

	void OpenSLMixer::stop(){
		if(!mInited)
			return;

		SLresult result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);
		if(SL_RESULT_SUCCESS != result)
			ALOGE("failed to set sl play state to pause!!");
        (void)result;		
	}

	void OpenSLMixer::write(void* data, int dataSize){
		SLresult result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, data, dataSize);
		if(SL_RESULT_SUCCESS != result)
			ALOGE("write: failed to enqueue bufferqueue with datasize:%d!!", dataSize);
		(void)result;
	}
	
	void OpenSLMixer::pause(){
		if(!mInited)
			return;

		SLresult result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PAUSED);
		if(SL_RESULT_SUCCESS != result)
			ALOGE("failed to set sl play state to pause!!");
        (void)result;
	}
	
	void OpenSLMixer::resume(){
		if(!mInited)
			return;
		
		SLresult result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
		if(SL_RESULT_SUCCESS != result)
			ALOGE("failed to set sl play state to play!!");
		(void)result;		
	}

	void OpenSLMixer::bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context){
		OpenSLMixer* mixer = (OpenSLMixer*)context;
		
		mixer->bufferQueueCallback(bq);
	}
	
	void OpenSLMixer::bufferQueueCallback(SLAndroidSimpleBufferQueueItf bq){
		if(bq != bqPlayerBufferQueue){
			ALOGE("callback params bq not the same with the passed one!!");
			return;
		}

		bool res;		
		void* data;
		int dataSize;
		res = mFillBuffer(&data, &dataSize, mFillBufferContext);
		
		if(res){
			//enqueue another buffer
			SLresult result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, data, dataSize);

			//#define DUMP_PCM_TO_FILE
#ifdef DUMP_PCM_TO_FILE
			FILE* f1 = fopen("/data/pcm", "ab+");
			if(f1 != NULL){
				size_t res = fwrite(data, 1, dataSize, f1);
				fclose(f1);
				ALOGV("fwrite %d of %d to /data/pcm!", res, dataSize);
			}else
				ALOGE("can not fopen /data/pcm!!");
#endif
						
			// the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
			// which for this code example would indicate a programming error
			if(SL_RESULT_SUCCESS != result)
				ALOGE("failed to enqueue bufferqueue with datasize:%d!!", dataSize);
			(void)result;
		}
	}
	
}//namespace
