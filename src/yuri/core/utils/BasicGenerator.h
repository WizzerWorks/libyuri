/*
 * BasicGenerator.h
 *
 *  Created on: 25.11.2012
 *      Author: neneko
 */

#ifndef BASICGENERATOR_H_
#define BASICGENERATOR_H_
#include "yuri/core/utils/new_types.h"
#include <map>
#include <stdexcept>
#include <vector>
#include <algorithm>

#ifdef YURI_MODULE_IN_TREE
	#define MODULE_REGISTRATION_BEGIN(name)
	#define MODULE_REGISTRATION_END(name)
#else
	#define MODULE_REGISTRATION_BEGIN(name) extern "C" {\
		const char* yuri2_8_module_get_name() { \
			return name; \
		}\
		int yuri2_8_module_register() {
	#define MODULE_REGISTRATION_END(name)  \
			return 0;\
		}\
	}
#endif

namespace yuri {
namespace core {
namespace generator {
template<class T, class KeyType, class Param>
class DefaultErrorPolicy {
protected:
	DefaultErrorPolicy() = default;
	virtual ~DefaultErrorPolicy() noexcept {};
	typedef shared_ptr<T> ptr_type;
	typedef Param param_type;
	ptr_type key_not_found(const KeyType&) const {
		throw(std::runtime_error("Key not found"));
	}
	param_type cfg_key_not_found(const KeyType&) const {
		throw(std::runtime_error("Key not found"));
	}
//	bool generator_already_registered(const KeyType&)
//	{
//		return false;
//	}

};
template<class T, class KeyType, class Param>
class FatalErrorPolicy {
	FatalErrorPolicy() = default;
	virtual ~FatalErrorPolicy() noexcept {};
	typedef shared_ptr<T> ptr_type;
	typedef Param param_type;
	ptr_type key_not_found(const KeyType&) const {
		throw(std::runtime_error("Key not found"));
	}
	param_type cfg_key_not_found(const KeyType&) const {
		throw(std::runtime_error("Key not found"));
	}
};

}
template<
	class T,
	class KeyType,
	class ParamType,
	template<class, class, class> class ErrorPolicy = generator::DefaultErrorPolicy,
	class GeneratorType = function<shared_ptr<T> (const ParamType& params)>,
	class ConfiguratorType = function<ParamType()>
	>
class BasicGenerator: public ErrorPolicy<T, KeyType, ParamType>
{
public:
	typedef shared_ptr<T> ptr_type;
	typedef KeyType key_type;
	typedef ParamType param_type;
	typedef GeneratorType generator_type;
	typedef ConfiguratorType configurator_type;
	bool register_generator(const key_type& key, generator_type, configurator_type = configurator_type());
	bool unregister_generator(const key_type& key, generator_type, configurator_type = configurator_type());
	template<class... Args>
	ptr_type generate(const key_type& key, Args&&... args) const;
	param_type configure(const key_type& key) const;
	std::vector<key_type> list_keys() const;
	bool is_registered(const key_type& key) const;
	BasicGenerator() {}
	virtual ~BasicGenerator() noexcept {}
private:
	std::map<key_type, std::pair<generator_type, configurator_type>> generators_;
//	std::map<key_type, configurator_type> configurators_;
};

template<class T, class KeyType, class ParamType, template<class, class, class> class ErrorPolicy, class GeneratorType, class ConfiguratorType>
template<class... Args>
typename BasicGenerator<T, KeyType, ParamType, ErrorPolicy, GeneratorType, ConfiguratorType>::ptr_type
	BasicGenerator<T, KeyType, ParamType, ErrorPolicy, GeneratorType, ConfiguratorType>::generate
		(const typename BasicGenerator<T, KeyType, ParamType, ErrorPolicy, GeneratorType, ConfiguratorType>::key_type& key, Args&&... args)
		const
{
	auto it = generators_.find(key);
	if (it == generators_.end()) return ErrorPolicy<T, KeyType, ParamType>::key_not_found(key);
	return it->second.first(std::forward<Args>(args)...);
}

template<class T, class KeyType, class ParamType, template<class, class, class> class ErrorPolicy, class GeneratorType, class ConfiguratorType>
bool
	BasicGenerator<T, KeyType, ParamType, ErrorPolicy, GeneratorType, ConfiguratorType>::register_generator
		(const typename BasicGenerator<T, KeyType, ParamType, ErrorPolicy, GeneratorType, ConfiguratorType>::key_type& key,
		 typename BasicGenerator<T, KeyType, ParamType, ErrorPolicy, GeneratorType, ConfiguratorType>::generator_type generator,
		 typename BasicGenerator<T, KeyType, ParamType, ErrorPolicy, GeneratorType, ConfiguratorType>::configurator_type configurator)
{
	return generators_.insert(std::make_pair(key, std::make_pair(generator,configurator))).second;// &&
//			(configurators_.insert(std::make_pair(key, configurator))).second;
}

template<class T, class KeyType, class ParamType, template<class, class, class> class ErrorPolicy, class GeneratorType, class ConfiguratorType>
std::vector<typename BasicGenerator<T, KeyType, ParamType, ErrorPolicy, GeneratorType, ConfiguratorType>::key_type>
	BasicGenerator<T, KeyType, ParamType, ErrorPolicy, GeneratorType, ConfiguratorType>::list_keys()
		const
{
	std::vector<key_type> keys(generators_.size());
	std::transform(generators_.begin(),generators_.end(),keys.begin(),
			[](const std::pair<key_type, std::pair<generator_type, configurator_type>> &p){return p.first;});
	return keys;
}

template<class T, class KeyType, class ParamType, template<class, class, class> class ErrorPolicy, class GeneratorType, class ConfiguratorType>
typename BasicGenerator<T, KeyType, ParamType, ErrorPolicy, GeneratorType, ConfiguratorType>::param_type
	BasicGenerator<T, KeyType, ParamType, ErrorPolicy, GeneratorType, ConfiguratorType>::configure
		(const typename BasicGenerator<T, KeyType, ParamType, ErrorPolicy, GeneratorType, ConfiguratorType>::key_type& key)
		const
{
	auto it = generators_.find(key);
	if (it == generators_.end()) return ErrorPolicy<T, KeyType, ParamType>::cfg_key_not_found(key);
	return it->second.second();
}
template<class T, class KeyType, class ParamType, template<class, class, class> class ErrorPolicy, class GeneratorType, class ConfiguratorType>
bool BasicGenerator<T, KeyType, ParamType, ErrorPolicy, GeneratorType, ConfiguratorType>::is_registered(const key_type& key) const
{
	return generators_.find(key) != generators_.end();
}
} /* namespace core */
} /* namespace yuri */
#endif /* BASICGENERATOR_H_ */
