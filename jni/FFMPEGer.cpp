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

#include "FFMPEGer.h"

#define LOG_TAG "FFMPEGer"
#include "android/Log.h"

namespace openamedia {
	
	FFMPEGer::FFMPEGer(const char* path)
		:fmt_ctx(NULL),
		 video_dec_ctx(NULL),
		 audio_dec_ctx(NULL),
		 video_stream(NULL),
		 audio_stream(NULL),
		 video_stream_idx(-1),
		 audio_stream_idx(-1),
		 frame(NULL),
		 video_frame_count(0),
		 audio_frame_count(0),
		 swr(NULL),
		 video_dst_bufsize(0),
		 audio_swr_dst_data(NULL),
		 audio_swr_dst_datasize(0),
		 audio_swr_dst_capacity(0),
		 unpadded_linesize(0),
		 mInited(false){
		strncpy(mPath, path, sizeof(mPath));
	}

	FFMPEGer::~FFMPEGer(){
		deInit();
	}

	bool FFMPEGer::init(){
		if(mInited)
			return true;
		
		do{
			int res;
			av_register_all();
			res = avformat_open_input(&fmt_ctx, mPath, NULL, NULL);
			if(res < 0){
				ALOGE("fail to open source file:%s", mPath);
				break;
			}

			res = avformat_find_stream_info(fmt_ctx, NULL);
			if(res < 0){
				ALOGE("fail to find stream info!");
				break;
			}

			res = open_codec_context(&video_stream_idx, fmt_ctx, AVMEDIA_TYPE_VIDEO);
			if(res >= 0){
				video_stream = fmt_ctx->streams[video_stream_idx];
				video_dec_ctx = video_stream->codec;
				
				res = av_image_alloc(video_dst_data, video_dst_linesize, video_dec_ctx->width, video_dec_ctx->height, video_dec_ctx->pix_fmt, 1);
				if(res < 0){
					ALOGE("fail to av_image_alloc raw video buffer!");
					break;
				}
				
				video_dst_bufsize = res;
				ALOGV("video stream and codec found!!");
			}
			
			res = open_codec_context(&audio_stream_idx, fmt_ctx, AVMEDIA_TYPE_AUDIO);
			if(res >= 0){
				audio_stream = fmt_ctx->streams[audio_stream_idx];
				audio_dec_ctx = audio_stream->codec;

				ALOGV("ffmpeg detect audio sample_rate:%d, channels:%d, fmt:%d", audio_dec_ctx->sample_rate, audio_dec_ctx->channels, audio_dec_ctx->sample_fmt);

				if(audio_dec_ctx->sample_fmt != AV_SAMPLE_FMT_S16){//??ok for FLTP, but what about AV_SAMPLE_FMT_U8?
					// Set up SWR context once you've got codec information
					ALOGV("the non-s16 sample fmt requires swr");
					swr = swr_alloc();
					if(swr == NULL){
						ALOGE("fail to swr_alloc for non-s16 sample fmt!!!");
						break;
					}
					
					av_opt_set_int(swr, "in_channel_layout",  audio_dec_ctx->channel_layout, 0);
					av_opt_set_int(swr, "out_channel_layout", audio_dec_ctx->channel_layout,  0);
					av_opt_set_int(swr, "in_sample_rate",     audio_dec_ctx->sample_rate, 0);
					av_opt_set_int(swr, "out_sample_rate",    audio_dec_ctx->sample_rate, 0);
					av_opt_set_sample_fmt(swr, "in_sample_fmt",  audio_dec_ctx->sample_fmt, 0);
					av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_S16,  0);

					res = swr_init(swr);
					if(res < 0){
						ALOGE("fail to swr_init for non-s16 sample fmt!!");
						break;
					}
				}

				ALOGV("audio stream and codec found!!");
			}
			
			if(video_stream == NULL && audio_stream == NULL){
				ALOGE("fail to find any video or audio stream!");
				break;
			}

			frame = av_frame_alloc();
			if(frame == NULL){
				ALOGE("fail to alloc AFrame!");
				break;
			}

			av_init_packet(&pkt);
			pkt.data = NULL;
			pkt.size = 0;

			mInited = true;
		}while(0);

		if(!mInited){
			pureDeInit();
		}

		return mInited;
	}

	int FFMPEGer::open_codec_context(int* stream_idx, AVFormatContext* fmt_ctx, enum AVMediaType type){
		int ret;
		AVStream *st;
		AVCodecContext *dec_ctx = NULL;
		AVCodec *dec = NULL;
		AVDictionary *opts = NULL;
		
		ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
		if (ret < 0) {
			ALOGE("Could not find %s stream in input file.", av_get_media_type_string(type));
			return ret;
		} else {
			*stream_idx = ret;
			st = fmt_ctx->streams[*stream_idx];
			/* find decoder for the stream */
			dec_ctx = st->codec;
			dec = avcodec_find_decoder(dec_ctx->codec_id);
			if (!dec) {
				ALOGE("Failed to find %s codec.", av_get_media_type_string(type));
				return AVERROR(EINVAL);
			}
			/* Init the decoders, without reference counting */
			if ((ret = avcodec_open2(dec_ctx, dec, &opts)) < 0) {
				ALOGE("Failed to open %s codec.", av_get_media_type_string(type));
				return ret;
			}
		}
		
		return 0;
	}
	
	void FFMPEGer::deInit(){
		if(!mInited)
			return;

		pureDeInit();

		mInited = false;
	}

	void FFMPEGer::pureDeInit(){
		if(video_dec_ctx != NULL)
			avcodec_close(video_dec_ctx);

		if(audio_dec_ctx != NULL)
			avcodec_close(audio_dec_ctx);

		if(fmt_ctx != NULL)
			avformat_close_input(&fmt_ctx);

		if(frame != NULL)
			av_frame_free(&frame);
		
		av_free(video_dst_data[0]);

		if(swr != NULL)
			swr_free(&swr);
		
		free(audio_swr_dst_data);

		fmt_ctx = NULL;
		video_stream_idx = -1;
		video_dec_ctx = NULL;
		video_stream = NULL;
		memset(video_dst_data, 0, sizeof(video_dst_data));
		audio_stream_idx = -1;
		audio_dec_ctx = NULL;
		audio_stream = NULL;
		frame = NULL;
		swr = NULL;
		audio_swr_dst_data = NULL;

		pkt.data = NULL;
		pkt.size = 0;
	}

	bool FFMPEGer::flush(){
		/* flush cached frames */
		int got_frame;
		
		pkt.data = NULL;
		pkt.size = 0;
		do {
			decode_packet(&got_frame, 1);
		} while (got_frame);
	}

	bool FFMPEGer::hasVideo(){
		if(video_stream_idx >= 0)
			return true;

		return false;
	}
	
	bool FFMPEGer::hasAudio(){
		if(audio_stream_idx >= 0)
			return true;

		return false;
	}

	bool FFMPEGer::readDemux(MediaBuffer* buffer){
		return false;
	}

	status_t FFMPEGer::readDecode(MediaBuffer* buffer){
		int res;
		res = av_read_frame(fmt_ctx, &pkt);
		if(res < 0){
			ALOGE("fail to av_read_frame!!");
			return ERROR_END_OF_STREAM;
		}
		orig_pkt = pkt;
		
		int got_frame;
		do{
			res = decode_packet(&got_frame, 0);
			if (res < 0){//drop the packet and go on.
				ALOGE("fail to decode_packet!!");
				av_free_packet(&orig_pkt);
				orig_pkt.data = NULL;
				orig_pkt.size = 0;
				return ERROR_DECODE_FAILED;
			}

			pkt.data += res;
			pkt.size -= res;
		}while(pkt.size > 0);
		
		av_free_packet(&orig_pkt);
		orig_pkt.data = NULL;
		orig_pkt.size = 0;

		if (pkt.stream_index == video_stream_idx){
			buffer->type = MEDIA_TYPE_VIDEO;
			if(video_stream->time_base.den == 0 ||
			   video_stream->time_base.num == 0){
				ALOGE("video stream time base den or num is 0, may cause later crash when caculate pts!!!");
			}
			buffer->ptsMS = (int64_t)(av_q2d(video_stream->time_base) * frame->pkt_pts * 1000);//pkt_pts is a decoder_reorder_pts regards ffplay.c, and pkt_pts is namely from demuxer(packet), so use AVStream::time_base, not AVCodecContext::time_base.
			if(buffer->ptsMS < 0)
				buffer->ptsMS = 0;
			buffer->data = video_dst_data[0];
			buffer->size = video_dst_bufsize;
			buffer->decompressed = true;
			buffer->videoInfo.width = video_dec_ctx->width;
			buffer->videoInfo.height = video_dec_ctx->height;
			if(video_dec_ctx->pix_fmt == AV_PIX_FMT_YUV420P)
				buffer->videoInfo.colorFmt = OMX_COLOR_FormatYUV420Planar;
			else
				buffer->videoInfo.colorFmt = OMX_COLOR_FormatUnused;
			//ALOGV("video pts:%lld, pkt_pts:%lld, dts:%lld, final:%lld", frame->pts, frame->pkt_pts, frame->pkt_dts, buffer->ptsMS);
		}else if(pkt.stream_index == audio_stream_idx){
			buffer->type = MEDIA_TYPE_AUDIO;
			if(audio_stream->time_base.den == 0 ||
			   audio_stream->time_base.num == 0){
				ALOGE("video stream time base den or num is 0, may cause later crash when caculate pts!!!");
			}
			buffer->ptsMS = (int64_t)(av_q2d(audio_stream->time_base) * frame->pkt_pts * 1000);
			if(buffer->ptsMS < 0)
				buffer->ptsMS = 0;
			buffer->decompressed = true;
			buffer->audioInfo.sampleRate = audio_dec_ctx->sample_rate;
			buffer->audioInfo.channels = audio_dec_ctx->channels;
			buffer->audioInfo.bitsPerSample = 16;
			//ALOGV("audio pts:%lld, pkt_pts:%lld, dts:%lld, final:%lld", frame->pts, frame->pkt_pts, frame->pkt_dts, buffer->ptsMS);
			if(swr != NULL){//from ffmpeg2.2, audio out fmt for aac has been changed to float, not s16 anymore. we need a swr to be compative with android output.
				int need_capacity = av_samples_get_buffer_size(&audio_swr_dst_datasize, audio_dec_ctx->channels, frame->nb_samples, AV_SAMPLE_FMT_S16, 1);

				if(audio_swr_dst_data == NULL ||
				   need_capacity > audio_swr_dst_capacity){
					audio_swr_dst_data = (uint8_t*)malloc(need_capacity);
					if(audio_swr_dst_data == NULL){
						ALOGE("fail to alloc dst buffer size of :%d for swr!!", need_capacity);
						return NO_MEMORY;
					}
					audio_swr_dst_capacity = need_capacity;
				}

				res = swr_convert(swr, &audio_swr_dst_data, frame->nb_samples, (const uint8_t**)frame->extended_data, frame->nb_samples);
				if(res < 0){
					ALOGE("fail to swr_convert!!!");
					return ERROR_AUDIO_RESAMPLE_FAILED;
				}
				
				buffer->data = audio_swr_dst_data;
				buffer->size = audio_swr_dst_datasize;
			}else{
				buffer->data = frame->extended_data[0];
				buffer->size = unpadded_linesize;
			}

			//#define DUMP_PCM_TO_FILE
#ifdef DUMP_PCM_TO_FILE
				FILE* f1 = fopen("/data/pcm1", "ab+");
				if(f1 != NULL){
					size_t res = fwrite(buffer->data, 1, buffer->size, f1);
					fclose(f1);
					ALOGV("fwrite %d of %d to /data/pcm1!", res, buffer->size);
				}else
					ALOGE("can not fopen /data/pcm1!!");
#endif

		}else{
			ALOGE("unkown pkt stream idx:%d", pkt.stream_index);
			return BAD_INDEX;
		}
		
		return OK;
	}
	
	int FFMPEGer::decode_packet(int *got_frame, int cached){
		int ret = 0;
		int decoded = pkt.size;
		*got_frame = 0;
		if (pkt.stream_index == video_stream_idx) {
			/* decode video frame */
			ret = avcodec_decode_video2(video_dec_ctx, frame, got_frame, &pkt);
			if (ret < 0) {
				ALOGE("Error decoding video frame! (%s)", av_err2str(ret));
				return ret;
			}
			if (*got_frame) {
				/*
				  ALOGV("video_frame%s n:%d coded_n:%d pts:%s\n",
				  cached ? "(cached)" : "",
				  video_frame_count++, frame->coded_picture_number,
				  av_ts2timestr(frame->pts, &video_dec_ctx->time_base));
				*/
				/* copy decoded frame to destination buffer:
				 * this is required since rawvideo expects non aligned data */
				av_image_copy(video_dst_data, video_dst_linesize,
							  (const uint8_t **)(frame->data), frame->linesize,
							  video_dec_ctx->pix_fmt, video_dec_ctx->width, video_dec_ctx->height);

				/* write to rawvideo file */
				//fwrite(video_dst_data[0], 1, video_dst_bufsize, video_dst_file);
			}
		} else if (pkt.stream_index == audio_stream_idx) {
			/* decode audio frame */
			ret = avcodec_decode_audio4(audio_dec_ctx, frame, got_frame, &pkt);
			if (ret < 0) {
				ALOGE("Error decoding audio frame (%s)", av_err2str(ret));
				return ret;
			}
			/* Some audio decoders decode only part of the packet, and have to be
			 * called again with the remainder of the packet data.
			 * Sample: fate-suite/lossless-audio/luckynight-partial.shn
			 * Also, some decoders might over-read the packet. */
			decoded = FFMIN(ret, pkt.size);
			if (*got_frame) {
				unpadded_linesize = frame->nb_samples * av_get_bytes_per_sample((AVSampleFormat)frame->format);
				/*
				  ALOGV("audio_frame%s n:%d nb_samples:%d pts:%s\n",
				  cached ? "(cached)" : "",
				  audio_frame_count++, frame->nb_samples,
				  av_ts2timestr(frame->pts, &audio_dec_ctx->time_base));
				*/
				/* Write the raw audio data samples of the first plane. This works
				 * fine for packed formats (e.g. AV_SAMPLE_FMT_S16). However,
				 * most audio decoders output planar audio, which uses a separate
				 * plane of audio samples for each channel (e.g. AV_SAMPLE_FMT_S16P).
				 * In other words, this code will write only the first audio channel
				 * in these cases.
				 * You should use libswresample or libavfilter to convert the frame
				 * to packed data. */
				//fwrite(frame->extended_data[0], 1, unpadded_linesize, audio_dst_file);
			}
		}
		
		return decoded;
	}

	bool FFMPEGer::seek(int64_t timeMS){
		int64_t time = (int64_t)(timeMS / (av_q2d(video_stream->time_base) * 1000));
		int res = av_seek_frame(fmt_ctx, -1, timeMS * 1000, AVSEEK_FLAG_BACKWARD);
		if(res < 0){
			ALOGE("fail to av_seek_frame video timeMS:%lld", timeMS);
		}
		
		return false;
	}

	bool FFMPEGer::getVideoInfo(VideoInfo* info){
		if(!mInited)
			return false;

		if(video_stream == NULL ||
		   video_dec_ctx == NULL)
			return false;

		strncpy(info->codecName, video_dec_ctx->codec->name, sizeof(info->codecName));
		info->width = video_dec_ctx->width;
		info->height = video_dec_ctx->height;
		if(video_dec_ctx->time_base.den == 0 ||
		   video_dec_ctx->time_base.num == 0){
			ALOGE("video stream time base den or num is 0, may cause later crash when caculate duration!!!");
		}
		info->frameRate = video_dec_ctx->time_base.den / video_dec_ctx->time_base.num;
		if(video_dec_ctx->pix_fmt == AV_PIX_FMT_YUV420P)
			info->colorFmt = OMX_COLOR_FormatYUV420Planar;
		else
			info->colorFmt = OMX_COLOR_FormatUnused;
		if(video_stream->time_base.den == 0 ||
		   video_stream->time_base.num == 0){
			ALOGE("video stream time base den or num is 0, may cause later crash when caculate duration!!!");
		}
		info->streamDuration = (int64_t)(fmt_ctx->duration / AV_TIME_BASE * 1000);//(int64_t)(video_stream->duration * av_q2d(video_stream->time_base) * 1000);//real_time(sec) = pts * (time_base.num / time_base.den).
		info->bitRate = video_dec_ctx->bit_rate;

		//ALOGV("video_stream->duration:%lld, %lf, fmt_ctx:%lld, sec:%d", video_stream->duration, av_q2d(video_stream->time_base), fmt_ctx->duration, (int)(fmt_ctx->duration/AV_TIME_BASE));

		return true;
	}

	bool FFMPEGer::getAudioInfo(AudioInfo* info){
		if(!mInited)
			return false;

		if(audio_stream == NULL ||
		   audio_dec_ctx == NULL)
			return false;

		strncpy(info->codecName, audio_dec_ctx->codec->name, sizeof(info->codecName));
		info->sampleRate = audio_dec_ctx->sample_rate;
		info->channels = audio_dec_ctx->channels;
		info->bitsPerSample = 16;
		info->streamDuration = (int64_t)(fmt_ctx->duration / AV_TIME_BASE * 1000);
		info->bitRate = audio_dec_ctx->bit_rate;

		return true;
	}

}//namespace
