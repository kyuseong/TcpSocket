// TcpSocket.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"

#include "../net/NetService.h"
#include "../net/Logger.h"

using namespace Idea;

NetService* service;

class Server : public NetServiceDelegate
{
	virtual void handle_connected(int64 id, std::string address, const boost::system::error_code& errorCode)
	{
		LIB_LOG(lv::trace) << "[Server] handle_connected - id:" << id << ",address:" << address << ",errorcode:" << errorCode;
	}

	virtual void handle_closed(int64 id, const boost::system::error_code& errorCode)
	{
		LIB_LOG(lv::trace) << "[Server] handle_closed - id:" << id << ",errorcode:" << errorCode;
	}

	virtual bool handle_receive(int64 id, buffer_ptr_t buffer, int recvLength)
	{
		LIB_LOG(lv::trace) << "[Server] handle_receive - id:" << id;


		service->send_buffer(id, buffer);

		return false;
	}
}; 

int _tmain(int argc, _TCHAR* argv[])
{
	//LOG_INIT(lv::trace);

	server_config config;

	config.set_ip("0.0.0.0");
	config.set_port(10000);
	config.set_thread_pool_count(10);

	Server* server = new Server();
	service = new NetService(config, server);

	service->start();
	service->start_acceptor();
	service->run();

	while (1)
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

	service->stop();

	return 0;
}
