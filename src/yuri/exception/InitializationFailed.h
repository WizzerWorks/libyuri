/*!
 * @file 		InitializationFailed.h
 * @author 		Zdenek Travnicek
 * @date 		28.7.2010
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2010 - 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef INITIALIZATIONFAILED_H_
#define INITIALIZATIONFAILED_H_

#include "Exception.h"
#include <string>
namespace yuri {

namespace exception {

class InitializationFailed: public yuri::exception::Exception {
public:
	EXPORT InitializationFailed(std::string reason = "Failed to initialize object");
	EXPORT virtual ~InitializationFailed() throw();
};

}

}

#endif /* INITIALIZATIONFAILED_H_ */
