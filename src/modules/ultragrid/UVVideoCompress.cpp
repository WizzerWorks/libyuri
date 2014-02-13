/*!
 * @file 		UVVideoCompress.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		24.10.2013
 * @date		21.11.2013
 * @copyright	CESNET, z.s.p.o, 2013
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#include "UVVideoCompress.h"
#include "yuri/core/Module.h"
#include "YuriUltragrid.h"

namespace yuri {
namespace ultragrid {
UVVideoCompress::UVVideoCompress(const log::Log &log_, core::pwThreadBase parent, const std::string& name, detail::uv_video_compress_params uv_compress_params)
:core::SpecializedIOFilter<core::RawVideoFrame>(log_,parent, name),
encoder_(nullptr), uv_compress_params_(uv_compress_params),uv_frame(nullptr, [](video_frame*p){vf_free(p);})
{

}

UVVideoCompress::~UVVideoCompress() noexcept
{

}
bool UVVideoCompress::init_compressor(const std::string& params)
{
	// Ewwww, this is NOT nice, but init_func shouldn't modify the string...
	char *fmt = const_cast<char*>(params.c_str());
	if (uv_compress_params_.init_func) {
		encoder_ = uv_compress_params_.init_func(fmt);
	}
	return (encoder_ != nullptr);
}

core::pFrame UVVideoCompress::do_special_single_step(const core::pRawVideoFrame& frame)
{
	if (!uv_frame) {
		uv_frame = ultragrid::allocate_uv_frame(frame);
	} else {
		ultragrid::copy_to_uv_frame(frame, uv_frame);
	}

	video_frame * out_uv_frame  = nullptr;

	if (uv_compress_params_.compress_func) {
		out_uv_frame  = uv_compress_params_.compress_func(encoder_, uv_frame.get(), 0);
	} else if (uv_compress_params_.compress_tile_func) {
		video_desc desc = video_desc_from_frame(uv_frame.get());
		tile * out_tile = uv_compress_params_.compress_tile_func(encoder_, &uv_frame->tiles[0], &desc, 0);
#warning NOT implemented tile output...
	}

	if (out_uv_frame) {
		core::pFrame out_frame = ultragrid::copy_from_from_uv(out_uv_frame, log);
		if (out_frame) return {out_frame};
	}
	return {};
}
core::pFrame UVVideoCompress::do_convert_frame(core::pFrame input_frame, format_t target_format)
{
	return {};
}

}
}

