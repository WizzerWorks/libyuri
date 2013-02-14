#include "ASIOUDPSocket.h"

namespace yuri
{

namespace io
{

ASIOUDPSocket::ASIOUDPSocket(Log &_log,pThreadBase parent,yuri::ushort_t port)
		throw (InitializationFailed):
		SocketBase(_log,parent),socket(0)
{
	log.setLabel("[ASIO UDP] ");
	try {
		socket=new udp::socket(io_service);
		socket->open(udp::v4());
		socket->set_option(boost::asio::socket_base::reuse_address(true));
		socket->bind(udp::endpoint(udp::v4(), port));
	}
	catch (std::exception &e) {
		log[error] << "EEEEEERRRRRRRRRROOOOOOOOOORRRRRRRRRRRRRRRRRRR!!"<<std::endl;
		throw InitializationFailed(std::string("Failed to initialize UDP socket: ")+e.what());
	}
	try {boost::asio::socket_base::send_buffer_size opt;
		boost::asio::socket_base::send_buffer_size opt2(10485760);
		boost::asio::socket_base::receive_buffer_size opt3(10485760);
		socket->get_option(opt);
		log[debug] << "Send size: " << opt.value() << std::endl;
		socket->set_option(opt2);
		socket->set_option(opt3);
		socket->get_option(opt);
		log[debug] << "Send size: " << opt.value() << std::endl;
		log[debug] << "Receive size: " << opt3.value() << std::endl;

	}
	catch (std::exception &e) {
		log[warning] << "Failed to increase buffer sizes....!!"<<std::endl;
		//throw InitializationFailed(std::string("Failed to initialize UDP socket: ")+e.what());
	}
}

ASIOUDPSocket::~ASIOUDPSocket()
{
}

yuri::size_t ASIOUDPSocket::read(yuri::ubyte_t * data,yuri::size_t size)
{
	yuri::size_t recvd =
		socket->receive_from(boost::asio::buffer(data,size), remote_endpoint, 0);
	//log[debug] << "Received data from " << remote_endpoint.address().to_string()
	//	<< std::endl;
	return recvd;

}
yuri::size_t ASIOUDPSocket::write(yuri::ubyte_t * data,yuri::size_t size)
{
	return socket->send_to(boost::asio::buffer(data,size), remote_endpoint);
}

bool ASIOUDPSocket::bind_local(std::string addr,yuri::ushort_t port)
{
	if (addr=="") return bind_local(port);
	return true;
}
bool ASIOUDPSocket::bind_local(yuri::ushort_t /*port*/)
{
	return true;
}

void ASIOUDPSocket::set_port(yuri::ushort_t port)
{
	this->port=port;
	//socket=udp::socket(io_service, udp::endpoint(udp::v4(), port));
}

bool ASIOUDPSocket::data_available()
{
	return socket->available();
}

int ASIOUDPSocket::get_fd()
{
	return (int) socket->native();
}

bool ASIOUDPSocket::set_endpoint(std::string address, yuri::size_t port)
{
	udp::resolver resolver(io_service);
	udp::resolver::query query(udp::v4(), address, boost::lexical_cast<std::string>(port));
	log[debug] << "Resolving " << address << std::endl;
	remote_endpoint = *resolver.resolve(query);
	log[debug] << "Resolved to " << remote_endpoint.address().to_string() << std::endl;
	if (remote_endpoint.address().is_v4()) {

		boost::asio::ip::address_v4 v4addr = boost::asio::ip::address_v4::from_string(remote_endpoint.address().to_string());
		if (v4addr.is_multicast()) {
			boost::asio::ip::multicast::join_group option(remote_endpoint.address());
			socket->set_option(option);
		}
	}
	return true;
}

void ASIOUDPSocket::disable_checksums(bool disable_checksums)
{
//ifdef __CYGWIN32__ || __WIN32__
#ifdef __linux__
	socket_no_check sck(disable_checksums);
	socket->set_option(sck);		
#endif
}

}

}
// End of File
