/*!
 * @file 		PipeGenerator.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date 		9.9.2013
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
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
		std::function<std::shared_ptr<T> (const std::string&, const log::Log&, const Parameters&)>>
{
public:
	BasicPipeGenerator(){}
	~BasicPipeGenerator() noexcept {}
};

using PipeGenerator = yuri::utils::Singleton<BasicPipeGenerator<Pipe, std::string>>;

#define REGISTER_PIPE(name, type) namespace { bool reg_ ## type = yuri::core::PipeGenerator::get_instance().register_generator(name,type::generate, type::configure); }


}
}

SINGLETON_DECLARE_HELPER(yuri::core::PipeGenerator)

#endif /* PIPEGENERATOR_H_ */
