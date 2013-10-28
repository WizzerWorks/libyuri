/*!
 * @file 		FrameInfo.cpp
 * @author 		<Your name>
 * @date		28.10.2013
 * @copyright	Institute of Intermedia, 2013
 * 				Distributed BSD License
 *
 */

#include "FrameInfo.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/core/frame/raw_frame_params.h"
#include "yuri/core/frame/CompressedVideoFrame.h"
#include "yuri/core/frame/compressed_frame_params.h"
#include "yuri/core/frame/RawAudioFrame.h"
#include "yuri/core/frame/raw_audio_frame_params.h"

namespace yuri {
namespace frame_info {


IOTHREAD_GENERATOR(FrameInfo)

MODULE_REGISTRATION_BEGIN("frame_info")
		REGISTER_IOTHREAD("frame_info",FrameInfo)
MODULE_REGISTRATION_END()

core::Parameters FrameInfo::configure()
{
	core::Parameters p = core::IOFilter::configure();
	p.set_description("FrameInfo");
	p["print_all"]["Print info about every frame. If set to false, only frames after change will be printed"]=false;
	return p;
}

namespace {

void print_frame(log::Log& log, core::pRawVideoFrame frame)
{
	//const auto& fi = core::raw_format::get_format_info(frame->get_format());
	const std::string& fname = core::raw_format::get_format_name(frame->get_format());
	log[log::info] << "Frame with format '" << fname << "', resolution " << frame->get_resolution();
}
void print_frame(log::Log& log, core::pCompressedVideoFrame frame)
{
	//const auto& fi = core::raw_format::get_format_info(frame->get_format());
	const std::string& fname = core::compressed_frame::get_format_name(frame->get_format());
	log[log::info] << "Frame with format '" << fname << "', resolution " << frame->get_resolution();
}
void print_frame(log::Log& log, core::pRawAudioFrame frame)
{
	//const auto& fi = core::raw_format::get_format_info(frame->get_format());
	const std::string& fname = core::raw_audio_format::get_format_name(frame->get_format());
	log[log::info] << "Frame with format '" << fname << "', sampling rate " << frame->get_sampling_frequency()
					<< "Hz, " << frame->get_channel_count() << " channels";
}

void print_frame(log::Log& log, core::pFrame frame) {
	if (auto f = dynamic_pointer_cast<core::RawVideoFrame>(frame)) {
		print_frame(log, f);
	} else if (auto f = dynamic_pointer_cast<core::CompressedVideoFrame>(frame)) {
		print_frame(log, f);
	} else if (auto f = dynamic_pointer_cast<core::RawAudioFrame>(frame)) {
		print_frame(log, f);
	} else {
		log[log::info] << "Unknown format (" << frame->get_format() << ")";
	}
}

bool same_format(const core::pRawVideoFrame& a, const core::pRawVideoFrame& b)
{
	return (a->get_format() == b->get_format()) &&
			(a->get_resolution() == b->get_resolution());
}
bool same_format(const core::pCompressedVideoFrame& a, const core::pCompressedVideoFrame& b)
{
	return (a->get_format() == b->get_format()) &&
			(a->get_resolution() == b->get_resolution());
}
bool same_format(const core::pRawAudioFrame& a, const core::pRawAudioFrame& b)
{
	return (a->get_format() == b->get_format()) &&
			(a->get_sampling_frequency() == b->get_sampling_frequency()) &&
			(a->get_channel_count() == b->get_channel_count());
}

bool same_format(const core::pFrame& a, const core::pFrame& b)
{
	if (a->get_format() != b->get_format()) return false;
	{
		auto fa = dynamic_pointer_cast<core::RawVideoFrame>(a);
		auto fb = dynamic_pointer_cast<core::RawVideoFrame>(b);
		if (fa && fb) return same_format(fa, fb);
	}
	{
		auto fa = dynamic_pointer_cast<core::CompressedVideoFrame>(a);
		auto fb = dynamic_pointer_cast<core::CompressedVideoFrame>(b);
		if (fa && fb) return same_format(fa, fb);
	}
	{
		auto fa = dynamic_pointer_cast<core::RawAudioFrame>(a);
		auto fb = dynamic_pointer_cast<core::RawAudioFrame>(b);
		if (fa && fb) return same_format(fa, fb);
	}
	return true;
}


}


FrameInfo::FrameInfo(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters):
core::IOFilter(log_,parent,std::string("frame_info")),
print_all_(false)
{
	IOTHREAD_INIT(parameters)
}

FrameInfo::~FrameInfo() noexcept
{
}

core::pFrame FrameInfo::do_simple_single_step(const core::pFrame& frame)
{
	try {
		if (print_all_ || !last_frame_ || !same_format(last_frame_, frame)) {
			print_frame(log, frame);
		}
	}
	catch (std::exception&){}
	last_frame_ = frame;
	return frame;
}
bool FrameInfo::set_param(const core::Parameter& param)
{
	if (param.get_name() == "print_all") {
		print_all_ = param.get<bool>();
	} else return core::IOFilter::set_param(param);
	return true;
}

} /* namespace frame_info */
} /* namespace yuri */
