/*!
 * @file 		libav.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date		29.10.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef LIBAV_H_
#define LIBAV_H_
#include "yuri/core/utils/new_types.h"
#include "yuri/core/forward.h"
extern "C" {
	#include "libavcodec/avcodec.h"
}
namespace yuri {
namespace libav {

PixelFormat avpixelformat_from_yuri(yuri::format_t format);
CodecID avcodec_from_yuri_format(yuri::format_t codec);

yuri::format_t yuri_pixelformat_from_av(PixelFormat format);
yuri::format_t yuri_format_from_avcodec(CodecID codec);
yuri::format_t yuri_audio_from_av(AVSampleFormat format);

core::pRawVideoFrame yuri_frame_from_av(const AVFrame& frame);
}
}


#endif /* LIBAV_H_ */
