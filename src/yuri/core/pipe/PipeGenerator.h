/*
 * PipeGenerator.h
 *
 *  Created on: 9.9.2013
 *      Author: neneko
 */

#ifndef PIPEGENERATOR_H_
#define PIPEGENERATOR_H_
#include "yuri/core/pipe/Pipe.h"
#include "yuri/core/utils/BasicGenerator.h"
#include "yuri/core/utils/Singleton.h"
#include "yuri/core/parameter/Parameters.h"
#include <string>
namespace yuri {
namespace core {

template<
	class T,
	class KeyType>
class BasicPipeGenerator: public BasicGenerator<T, KeyType,
		Parameters,
		generator::DefaultErrorPolicy,
		function<shared_ptr<T> (const std::string&, const log::Log&, const Parameters&)>>
{
public:
	BasicPipeGenerator(){}
	~BasicPipeGenerator() noexcept {}
};

// TODO: Pipe generator
typedef utils::Singleton<BasicPipeGenerator<Pipe, std::string>> PipeGenerator;

#define REGISTER_PIPE(name, type) namespace { bool reg_ ## type = yuri::core::PipeGenerator::get_instance().register_generator(name,type::generate, type::configure); }


}
}


#endif /* PIPEGENERATOR_H_ */
