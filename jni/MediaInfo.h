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

#ifndef OPENAMEDIA_MEDIA_INFO_H
#define OPENAMEDIA_MEDIA_INFO_H

#include <stdint.h>

#include "openmax/OMX_Video.h"

namespace openamedia {
	
	struct VideoInfo {
		VideoInfo();
		~VideoInfo();

		char codecName[32];//keep the same size with AVCodecContext::codec_name.
		int width;
		int height;
		int frameRate;
		OMX_COLOR_FORMATTYPE colorFmt;
		int64_t streamDuration;
		int bitRate;
	};

	struct AudioInfo {
		AudioInfo();
		~AudioInfo();

		char codecName[32];
		int sampleRate;
		int channels;
		int bitsPerSample;
		int64_t streamDuration;
		int bitRate;
	};

	enum MediaType {
		MEDIA_TYPE_UNKNOWN = 0,
		MEDIA_TYPE_VIDEO,
		MEDIA_TYPE_AUDIO,
	};

	struct MediaBuffer {
		MediaBuffer();
		~MediaBuffer();
		
		MediaType type;
		int64_t ptsMS;
		void* data;
		int size;
		bool decompressed;

		VideoInfo videoInfo;
		AudioInfo audioInfo;
	};

}//namespace

#endif//MEDIA_INFO_H
