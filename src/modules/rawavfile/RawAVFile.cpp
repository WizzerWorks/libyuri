/*!
 * @file 		RawAVFile.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date		09.02.2012
 *  * @date		02.04.2015
 * @copyright	Institute of Intermedia, CTU in Prague, 2012 - 2015
 * 				Distributed under BSD Licence, details in file doc/LICENSE
 *
 */

#include "RawAVFile.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/core/frame/RawAudioFrame.h"
#include "yuri/core/frame/raw_frame_types.h"
#include "yuri/core/frame/raw_frame_params.h"
#include "yuri/core/frame/raw_audio_frame_types.h"
#include "yuri/core/frame/CompressedVideoFrame.h"
#include "yuri/core/frame/compressed_frame_types.h"
#include "yuri/core/frame/compressed_frame_params.h"
#include "yuri/core/utils/irange.h"
#include "yuri/core/utils/assign_events.h"
#include <cassert>
#include "libavcodec/version.h"
namespace yuri {
namespace rawavfile {

IOTHREAD_GENERATOR(RawAVFile)

MODULE_REGISTRATION_BEGIN("rawavfile")
	REGISTER_IOTHREAD("rawavsource",RawAVFile)
MODULE_REGISTRATION_END()


namespace {
	const std::string unknown_format = "Unknown";
	const std::string& get_format_name_no_throw(format_t fmt) {
		try {
			return core::raw_format::get_format_name(fmt);
		}
		catch(std::exception&){}
		try {
			return core::compressed_frame::get_format_name(fmt);
		}
		catch(std::exception&){}
		return unknown_format;
	}

}

struct RawAVFile::stream_detail_t {
		stream_detail_t(AVStream* stream=nullptr, AVCodec* codec = nullptr, format_t fmt = 0, format_t fmt_out = 0)
		:stream(stream),ctx(stream?stream->codec:nullptr),codec(codec),format(fmt),format_out(fmt_out) {}
		AVStream* stream;
		AVCodecContext *ctx;
		AVCodec* codec;
		format_t format;
		format_t format_out;
		resolution_t resolution;
		duration_t delta;
	};

core::Parameters RawAVFile::configure()
{
	core::Parameters p = IOThread::configure();
	p["block"]["Threat output pipes as blocking. Specify as max number of frames in output pipe."]=0;
	p["filename"]["File to open"]="";
	p["decode"]["Decode the stream and push out raw video"]=true;
	p["format"]["Format to decode to"]="YUV422";
	p["fps"]["Override framerate. Specify 0 to use original, or negative value to maximal speed."]=0;
	p["max_video"]["Maximal number of video streams to process"]=1;
	p["max_audio"]["Maximal number of audio streams to process"]=0;
	p["loop"]["Loop the video"]=true;
	p["allow_empty"]["Allow empty input file"]=false;
	return p;
}

// TODO: number of output streams should be -1 and custom connect_out should be implemented.
RawAVFile::RawAVFile(const log::Log &_log, core::pwThreadBase parent, const core::Parameters &parameters)
	:IOThread(_log,parent,0,1024,"RawAVSource"),
	 BasicEventConsumer(log),
	fmtctx_(nullptr,avformat_free_context),video_format_out_(0),
	decode_(true),fps_(0.0),max_video_streams_(1),max_audio_streams_(1),
	loop_(true),reset_(false),allow_empty_(false)
{
	IOTHREAD_INIT(parameters)
	set_latency (10_us);
#ifdef BROKEN_FFMPEG
// We probably using BROKEN fork of ffmpeg (libav) or VERY old ffmpeg.
	if (max_audio_streams_ > 0) {
		log[log::warning] << "Using unsupported version of FFMPEG, probably the FAKE libraries distributed by libav project. Audio suport disabled";
		max_audio_streams_ = 0;
	}
#endif
	resize(0,max_video_streams_+max_audio_streams_);
	libav::init_libav();


	if (filename_.empty()) {
		if (!allow_empty_) throw exception::InitializationFailed("No filename specified!");
		log[log::info] << "No filename specified, starting without an active video";
	} else {
		if (!open_file(filename_)) {
			if (!allow_empty_) throw exception::InitializationFailed("Failed to open file");
			log[log::warning] << "Failed to open file, but allow_empty was specified, so waiting for new filename";
		}
	}

}

RawAVFile::~RawAVFile() noexcept
{

}

bool RawAVFile::open_file(const std::string& filename)
{
	video_streams_.clear();
	audio_streams_.clear();
	frames_.clear();

	fmtctx_.reset();
	avformat_open_input(&fmtctx_.get_ptr_ref(), filename.c_str(),0, 0);
	if (!fmtctx_) {
		log[log::error] << "Failed to allocate Format context";
		return false;
	}


	if (avformat_find_stream_info(fmtctx_,0)<0) {
		log[log::fatal] << "Failed to retrieve stream info!";
		return false;
	}


	for (size_t i = 0; i< fmtctx_->nb_streams; ++i) {
		if (fmtctx_->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			log[log::debug] << "Found video stream with id " << i << ".";
			if (video_streams_.size() < max_video_streams_ && fmtctx_->streams[i]->codec) {
				video_streams_.push_back({fmtctx_->streams[i], nullptr, 0, video_format_out_});
			} else {
				fmtctx_->streams[i]->discard = AVDISCARD_ALL;
			}
		} else if (fmtctx_->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			log[log::debug] << "Found audio stream with id " << i << ".";
			if (audio_streams_.size() < max_audio_streams_ && fmtctx_->streams[i]->codec) {
				audio_streams_.push_back(fmtctx_->streams[i]);
			} else {
				fmtctx_->streams[i]->discard = AVDISCARD_ALL;
			}
		}  else {
			fmtctx_->streams[i]->discard = AVDISCARD_ALL;
		}
	}
	if (video_streams_.empty() && audio_streams_.empty()) {
		log[log::error] << "No stream in input file!";
		return false;
	}

	frames_.resize(video_streams_.size());
	if (!decode_) {
		for (size_t i=0;i<video_streams_.size();++i) {
			video_streams_[i].format = libav::yuri_format_from_avcodec(video_streams_[i].ctx->codec_id);
			if (video_streams_[i].format == 0) {
				log[log::error] << "Unknown format for video stream " << i;
				return false;
			}
			log[log::info] << "Found video stream with format " << get_format_name_no_throw(video_streams_[i].format) <<
			" and resolution " << video_streams_[i].ctx->width << "x" << video_streams_[i].ctx->height;
		}
	} else {
		for (size_t i=0;i<video_streams_.size();++i) {
			video_streams_[i].codec = avcodec_find_decoder(video_streams_[i].ctx->codec_id);
			if (!video_streams_[i].codec) {
				log[log::error] << "Failed to find decoder for video stream " << i;
				return false;
			}
			if(video_streams_[i].codec ->capabilities & CODEC_CAP_TRUNCATED)
				video_streams_[i].ctx->flags|=CODEC_FLAG_TRUNCATED;
			if (video_streams_[i].format_out != 0) {
				video_streams_[i].ctx->pix_fmt = libav::avpixelformat_from_yuri(video_streams_[i].format_out);
			}

			if (avcodec_open2(video_streams_[i].ctx,video_streams_[i].codec,0) < 0) {
				log[log::error] << "Failed to open codec for video stream " <<i;
				return false;
			}
			video_streams_[i].format = libav::yuri_format_from_avcodec(video_streams_[i].ctx->codec_id);
			video_streams_[i].format_out = libav::yuri_pixelformat_from_av(video_streams_[i].ctx->pix_fmt);
			video_streams_[i].resolution = resolution_t{
						static_cast<dimension_t>(video_streams_[i].ctx->width),
						static_cast<dimension_t>(video_streams_[i].ctx->height)};

			log[log::info] << "Found video stream with format " << get_format_name_no_throw(video_streams_[i].format) <<
				" and resolution " << video_streams_[i].resolution <<
				". Decoding to " << get_format_name_no_throw(video_streams_[i].format_out);
		}
		for (size_t i=0;i<audio_streams_.size();++i) {
			audio_streams_[i].codec = avcodec_find_decoder(audio_streams_[i].ctx->codec_id);
			if (!audio_streams_[i].codec) {
				throw exception::InitializationFailed("Failed to find decoder");
			}
			if (avcodec_open2(audio_streams_[i].ctx,audio_streams_[i].codec,0) < 0) {
				throw exception::InitializationFailed("Failed to open codec!");
			}
			audio_streams_[i].format = libav::yuri_format_from_avcodec(audio_streams_[i].ctx->codec_id);
			audio_streams_[i].format_out = libav::yuri_audio_from_av(audio_streams_[i].ctx->sample_fmt);
			log[log::info] << "Found audio stream, format:" << audio_streams_[i].format << " to " << audio_streams_[i].format_out;
		}
	}


	for (auto i: irange(video_streams_.size())) {
		if (fps_>0) video_streams_[i].delta = 1_s/fps_;
		else if (fps_ == 0.0) {
			const auto& den = video_streams_[i].stream->avg_frame_rate.den;
			const auto& num = video_streams_[i].stream->avg_frame_rate.num;
			if (num) video_streams_[i].delta = 1_s*den/num;
			else {
				log[log::warning] << "No framerate specified for stream " << i << ", using default 25fps";
				video_streams_[i].delta = 1_s/25;
			}
			log[log::info] << "Delta " << i << " " << video_streams_[i].delta;
		}
	}

	next_times_.resize(video_streams_.size(),timestamp_t{});
	return true;
}


bool RawAVFile::push_ready_frames()
{
	bool ready = false;
	for (auto i: irange(video_streams_.size())) {
		if (frames_[i]) {
			if (fps_>=0) {
				timestamp_t curr_time;
				if (curr_time < next_times_[i]) {
					continue;
				} else {
					next_times_[i] = next_times_[i]+video_streams_[i].delta;
				}
			}
			push_frame(i,std::move(frames_[i]));

			frames_[i].reset();
			ready=true;
//				continue;
		} else {
			ready = true;
		}
	}
	if (video_streams_.empty()) {
		ready=true;
	}
	return ready;
}

bool RawAVFile::process_file_end()
{
	if (loop_) {
		if (next_filename_.empty() && fmtctx_) {
			log[log::debug] << "Seeking to the beginning";
			av_seek_frame(fmtctx_, 0, 0, AVSEEK_FLAG_BACKWARD);
			for (auto& s: video_streams_) {
				avcodec_flush_buffers(s.ctx);
			}
			for (auto& s: audio_streams_) {
				avcodec_flush_buffers(s.ctx);
			}
		} else {
			log[log::info] << "Opening: " << next_filename_;
			filename_ = std::move(next_filename_);
			next_filename_.clear();
			return open_file(filename_);
		}
		return true;
	}
	log[log::error] << "Failed to read next packet";
	request_end(core::yuri_exit_finished);
	return false;
}

bool RawAVFile::process_undecoded_frame(index_t idx, const AVPacket& packet)
{
	core::pCompressedVideoFrame f = core::CompressedVideoFrame::create_empty(video_streams_[idx].format,
			video_streams_[idx].resolution, packet.data, packet.size);
	frames_[idx] = f;
	log[log::debug] << "Pushing packet with size: " << f->size();
	duration_t dur = 1_s * packet.duration*video_streams_[idx].stream->avg_frame_rate.den/video_streams_[idx].stream->avg_frame_rate.num;
	if (!dur.value) dur = video_streams_[idx].delta;
	f->set_duration(dur);

	log[log::debug] << "Found packet!" /* (pts: " << pts << ", dts: " << dts <<*/ ", dur: " << dur;
	log[log::debug] << "num/den:" << video_streams_[idx].stream->avg_frame_rate.num << "/" << video_streams_[idx].stream->avg_frame_rate.den;
	log[log::debug] << "orig pts: " << packet.pts << ", dts: " << packet.dts << ", dur: " << packet.duration;
	return true;
}

bool RawAVFile::decode_video_frame(index_t idx, const AVPacket& packet, AVFrame* av_frame, bool& keep_packet)
{
	int whole_frame = 0;

	keep_packet = false;
	int decoded_size = avcodec_decode_video2(video_streams_[idx].ctx, av_frame, &whole_frame, &packet);
	if (!whole_frame) {
		log[log::verbose_debug] << "Didn't get whole frame";
		return false;
	}
	if (decoded_size < 0) {
		log[log::warning] << "Failed to decode frame";
		return false;
	}

	if (packet.size && decoded_size != packet.size) {
		keep_packet = true;
		log[log::debug] << "Used only " << decoded_size << " bytes out of " << packet.size;
	}

	auto f = libav::yuri_frame_from_av(*av_frame);
	if (!f) {
		log[log::warning] << "Failed to convert avframe, probably unsupported pixelformat";
		return false;
	}
	if (format_out_ != f->get_format()) {
		log[log::warning] << "Unexpected frame format! Expected '" << get_format_name_no_throw(format_out_)
		<< "', but got '" << get_format_name_no_throw(f->get_format()) << "'";
		format_out_ = f->get_format();
	}

	frames_[idx] = f;
	return true;
}

bool RawAVFile::decode_audio_frame(index_t idx, const AVPacket& packet, AVFrame* av_frame, bool& keep_packet)
{
#ifdef BROKEN_FFMPEG
// We are probably using BROKEN port of ffmpeg (libav) or VERY old ffmpeg.
	return false;
#else
	keep_packet = false;
	int whole_frame = 0;

	int decoded_size = avcodec_decode_audio4(audio_streams_[idx].ctx, av_frame, &whole_frame,&packet);
	if (!whole_frame) {
		return false;
	}
	if (decoded_size < 0) {
		log[log::warning] << "Failed to decode frame";
		return false;
	}

	if (decoded_size != packet.size) {
		keep_packet = true;
		log[log::debug] << "Used only " << decoded_size << " bytes out of " << packet.size;
	}
	size_t data_size = av_frame->nb_samples * av_frame_get_channels(av_frame) * 2 ;
	auto f = core::RawAudioFrame::create_empty(audio_streams_[idx].format_out, av_frame_get_channels(av_frame), av_frame->sample_rate, av_frame->data[0], data_size);
	if (!f) {
		log[log::warning] << "Failed to convert avframe, probably unsupported pixelformat";
		return false;
	}
	push_frame(idx + max_video_streams_, std::move(f));
	return true;
#endif
}

namespace {
int find_in_stream_group(int index, const std::vector<RawAVFile::stream_detail_t>& streams)
{
	for (auto i: irange(streams.size())) {
		if (index == streams[i].stream->index) {
			return i;
		}
	}
	return -1;
}

}
void RawAVFile::run()
{
	AVPacket packet;
	av_init_packet(&packet);
	AVPacket empty_packet;
	av_init_packet(&empty_packet);
	empty_packet.data = nullptr;
	empty_packet.size = 0;
	bool keep_packet = false;
	bool finishing = false;
	AVFrame *av_frame = avcodec_alloc_frame();

	next_times_.resize(video_streams_.size(),timestamp_t{});

	while (still_running()) {
		process_events();
		if (!fmtctx_) {
			if (next_filename_.empty()) {
				wait_for_events(10_ms);
				continue;
			}
		}

		if (reset_ || !fmtctx_) {
			log[log::info] << "RESET";
			if (!process_file_end()) {
				if (!loop_) break;
			}
			reset_ = false;
			if (!fmtctx_) {
				next_times_.clear();
				continue;
			}
		}

		if (!push_ready_frames()) {
			sleep(get_latency());
			continue;
		}

		if (!keep_packet && av_read_frame(fmtctx_,&packet)<0) {
			finishing = true;
		}


		if (finishing) {
			bool done = true;
			for (auto i: irange(video_streams_.size())) {
				if (!frames_[i]) {
					decode_video_frame(i, empty_packet, av_frame, keep_packet);
				}
				if (frames_[i]) done = false;
			}
			if (done) {
				finishing = false;
				reset_ = true;
			}
			continue;
		}

		auto idx = find_in_stream_group(packet.stream_index, video_streams_);
		if (idx>=0) {
			if (!decode_) {
				process_undecoded_frame(idx, packet);
			} else {
				if (!decode_video_frame(idx, packet, av_frame, keep_packet))
					continue;
			}
		} else {
			idx = find_in_stream_group(packet.stream_index, audio_streams_);
			if (idx<0) {
				continue;
			}

			if (!decode_audio_frame(idx, packet, av_frame, keep_packet)) {
				continue;
			}
		}
	}
	av_free(av_frame);
	av_free_packet(&packet);
	av_free_packet(&empty_packet);
}

bool RawAVFile::set_param(const core::Parameter &parameter)
{
	if (assign_parameters(parameter)
			(filename_,			"filename")
			(decode_, 			"decode")
			.parsed<std::string>
				(video_format_out_, "format", core::raw_format::parse_format)
			(fps_, 				"fps")
			(max_video_streams_,"max_video")
			(max_audio_streams_,"max_audio")
			(loop_, 			"loop")
			(allow_empty_,		"allow_empty"))
		return true;
	return IOThread::set_param(parameter);
}

bool RawAVFile::do_process_event(const std::string& event_name, const event::pBasicEvent& event)
{
	if (event->get_type() == event::event_type_t::bang_event) {
		if (event_name == "reset") {
			reset_ = true;
			return true;
		}
	}
	if (assign_events(event_name, event)
		(next_filename_, "filename")
		(reset_, "reset")) {
		return true;
	}

	return false;
}

} /* namespace video */
} /* namespace yuri */

