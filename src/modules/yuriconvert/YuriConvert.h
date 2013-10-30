/*!
 * @file 		YuriConvertor.h
 * @author 		Zdenek Travnicek
 * @date 		13.8.2010
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, 2010 - 2013
 * 				Distributed under GNU Public License 3.0
 *
 */

#ifndef YURICONVERTOR_H_
#define YURICONVERTOR_H_

#include "yuri/core/thread/SpecializedIOFilter.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/core/thread/ConverterThread.h"
namespace yuri {

namespace video {

enum colorimetry_t {
	YURI_COLORIMETRY_REC709,
	YURI_COLORIMETRY_REC601,
	YURI_COLORIMETRY_REC2020
};

/// @bug Conversion from limited range YUV to RGB does not work properly

class YuriConvertor: public core::SpecializedIOFilter<core::RawVideoFrame>, public core::ConverterThread {
public:
	YuriConvertor(log::Log &log_, core::pwThreadBase parent, const core::Parameters& parameters);
	virtual ~YuriConvertor() noexcept;
	IOTHREAD_GENERATOR_DECLARATION
	static core::Parameters configure();
	colorimetry_t get_colorimetry() const { return colorimetry_; }
	bool get_full_range() const { return full_range_; }
private:
	bool set_param(const core::Parameter &p);
	core::pFrame do_special_single_step(const core::pRawVideoFrame& frame);
	virtual core::pFrame do_convert_frame(core::pFrame input_frame, format_t target_format) override;
	colorimetry_t colorimetry_;
	bool full_range_;
	yuri::format_t format_;
};

}

}

#endif /* YURICONVERTOR_H_ */
