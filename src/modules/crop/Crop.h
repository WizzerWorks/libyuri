/*!
 * @file 		Crop.h
 * @author 		Zdenek Travnicek
 * @date 		17.11.2010
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2010 - 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef CROP_H_
#define CROP_H_

#include "yuri/core/thread/SpecializedIOFilter.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/event/BasicEventConsumer.h"

namespace yuri {

namespace io {

class Crop: public core::SpecializedIOFilter<core::RawVideoFrame>, public event::BasicEventConsumer {
	using base_type = core::SpecializedIOFilter<core::RawVideoFrame>;
public:
	Crop(log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters);
	virtual ~Crop() noexcept;
	IOTHREAD_GENERATOR_DECLARATION
	static core::Parameters configure();
	virtual bool set_param(const core::Parameter &parameter) override;
protected:
	virtual core::pFrame do_special_single_step(core::pRawVideoFrame frame) override;
	virtual bool do_process_event(const std::string& event_name, const event::pBasicEvent& event) override;
	geometry_t geometry_;
};

}

}

#endif /* CROP_H_ */
