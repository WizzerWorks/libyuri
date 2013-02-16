/*!
 * @file 		LogProxy.h
 * @author 		Zdenek Travnicek
 * @date 		11.2.1013
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, 2013
 * 				Distributed under GNU Public License 3.0
 *
 */

#ifndef LOGPROXY_H_
#define LOGPROXY_H_
#include "yuri/io/types.h"
#include <ostream>
#include <sstream>
#include <boost/thread.hpp>

namespace yuri {
namespace log {
/*!
 * @brief 		Wrapper struct for std::basic_ostream providing locking
 */
template<
    class CharT,
    class Traits = std::char_traits<CharT>
>
struct guarded_stream {
	typedef std::basic_string<CharT, Traits> string_t;
	typedef std::basic_ostream<CharT, Traits> stream_t;
	typedef CharT char_t;
	/**
	 * @brief 		Writes a string to the contained ostream
	 * @param msg	String to write
	 */
	void write(const string_t msg) {
		boost::mutex::scoped_lock l(mutex_);
		str_ << msg;
	}
	guarded_stream(stream_t& str):str_(str) {}
	char_t widen(char c) { return str_.widen(c); }
private:
	stream_t& str_;
	boost::mutex mutex_;
};

/**
 * @brief Proxy for output stream
 *
 * LogProxy wraps an output stream and ensures that lines are written correctly
 * even when logging concurrently from several threads
 */
template<
    class CharT,
    class Traits = std::char_traits<CharT>
>
class EXPORT LogProxy {
private:
	typedef std::basic_ostream<char>& (*iomanip_t)(std::basic_ostream<char>&);
public:
	typedef guarded_stream<CharT, Traits> gstream_t;
	typedef std::basic_stringstream<CharT, Traits> sstream_t;
	/*!
	 * @param	str_	Reference to a @em guarded_stream to write the messages
	 * @param	dummy	All input is discarded, when dummy is set to true
	 */
	LogProxy(gstream_t& str_,bool dummy):stream_(str_),dummy_(dummy) {}
	/*!
	 * @brief			Copy constructor. Invalides the original LogProxy object
	 */
	LogProxy(const LogProxy& other):stream_(other.stream_),dummy_(other.dummy_) {
		if (!dummy_) {
			buffer_.str(other.buffer_.str());
			const_cast<LogProxy&>(other).dummy_=true;
		}
	}
	/*!
	 * @brief			Provides ostream like operator << for inserting messages
	 * @tparam	T		Type of message to insert
	 * @param	val_	Message to write
	 */
	template<typename T>
	LogProxy& operator<<(const T& val_)
	{
		if (!dummy_) {
			buffer_ << val_;
		}
		return *this;
	}
	/*!
	 * @brief 			Overloaded operator<< for manipulators
	 *
	 * We are storing internally to std::stringstream, which won't accept std::endl,
	 * so this method simply replaces std::endl with newlines.
	 * @param	manip	Manipulator for the stream
	 */
	LogProxy& operator<<(iomanip_t manip)
	{
		// We can't call endl on std::stringstream, so let's filter it out
		if (manip==static_cast<iomanip_t>(std::endl)) return *this << stream_.widen('\n');
		else return *this << manip;
	}

	~LogProxy() {
#if 0 && __cplusplus >=201103L
		const std::string msg = buffer_.str();
		if (!dummy_) std::async([&msg,this](){stream_.write(msg);});
#else
		if (!dummy_) stream_.write(buffer_.str());
#endif
	}
private:
	gstream_t& stream_;
	sstream_t buffer_;
	bool dummy_;
};

}
}


#endif /* LOGPROXY_H_ */
