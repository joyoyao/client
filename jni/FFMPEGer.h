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

#ifndef OPENAMEDIA_FFMPEGER_H
#define OPENAMEDIA_FFMPEGER_H

#include "android/Errors.h"
#include "MediaInfo.h"
#include "Common.h"

extern "C"
{
#include "libavutil/imgutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/timestamp.h"
#include <libavutil/opt.h>
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
}

namespace openamedia {

	class FFMPEGer {
	public:
		FFMPEGer(const char* path);
		~FFMPEGer();
		
		bool init();
		void deInit();

		bool hasVideo();
		bool hasAudio();

		bool flush();
		bool readDemux(MediaBuffer* buffer);
		status_t readDecode(MediaBuffer* buffer);

		bool seek(int64_t timeMS);

		bool getVideoInfo(VideoInfo* info);
		bool getAudioInfo(AudioInfo* info);
				
	private:
		char mPath[MAX_STRING_PATH_LEN];
		
		AVFormatContext *fmt_ctx;
		AVCodecContext *video_dec_ctx;
		AVCodecContext *audio_dec_ctx;
		AVStream *video_stream;
		AVStream *audio_stream;
		int video_stream_idx;
		int audio_stream_idx;
		AVFrame *frame;
		AVPacket pkt;
		AVPacket orig_pkt;
		int video_frame_count;
		int audio_frame_count;
		SwrContext *swr;

		uint8_t *video_dst_data[4];
		int video_dst_linesize[4];
		int video_dst_bufsize;

		uint8_t* audio_swr_dst_data;
		int audio_swr_dst_datasize;
		int audio_swr_dst_capacity;

		int unpadded_linesize;

		int open_codec_context(int* stream_idx, AVFormatContext* fmt_ctx, enum AVMediaType type);
		int decode_packet(int *got_frame, int cached);
		
		bool mInited;
		void pureDeInit();

		FFMPEGer(const FFMPEGer&);
		FFMPEGer& operator=(const FFMPEGer&);
	};

}

#endif
