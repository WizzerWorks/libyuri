/*!
 * @file 		AlsaOutput.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		22.10.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef ALSAOUTPUT_H_
#define ALSAOUTPUT_H_

#include "yuri/core/thread/SpecializedIOFilter.h"
#include "yuri/core/frame/RawAudioFrame.h"
#include <alsa/asoundlib.h>

namespace yuri {
namespace alsa_output {

class AlsaOutput: public core::SpecializedIOFilter<core::RawAudioFrame>
{
public:
	IOTHREAD_GENERATOR_DECLARATION
	static core::Parameters configure();
	AlsaOutput(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters);
	virtual ~AlsaOutput() noexcept;
private:
	
	virtual core::pFrame do_special_single_step(const core::pRawAudioFrame& frame) override;
	virtual bool set_param(const core::Parameter& param) override;
	bool is_different_format(const core::pRawAudioFrame& frame);

	bool init_alsa(const core::pRawAudioFrame& frame);

	bool error_call(int, const std::string&);
	format_t format_;
	std::string device_name_;
	size_t channels_;
	unsigned int sampling_rate_;


	snd_pcm_t			*handle_;

};

} /* namespace alsa_output */
} /* namespace yuri */
#endif /* ALSAOUTPUT_H_ */
