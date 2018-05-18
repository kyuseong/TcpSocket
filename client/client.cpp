// TcpSocket.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"

#include "../net/NetService.h"
#include "../net/Logger.h"

using namespace Idea;

NetService* service;

class Client : public NetServiceDelegate
{
	virtual void handle_connected(int64 id, std::string address, const boost::system::error_code& error)
	{
		LIB_LOG(lv::trace) << "[Client] handle_connected - id:" << id << ",address:" << address << ",errorcode:" << error;

		buffer_ptr_t ptr = std::make_shared<std::vector<char>>();
		for(int i=0;i<100;++i)
			ptr->push_back(i);

		service->send_buffer(id, ptr);
		/*std::shared_ptr<ns::nspacket> packet_ptr = std::make_shared<ns::nspacket>();
		packet_ptr->set_id(1);
		packet_ptr->set_seq(1);
		packet_ptr->set_payload(boost::str(boost::format("client:%d") % id));
		service->send_packet(id, packet_ptr);*/
	}
	virtual void handle_closed(int64 id, const boost::system::error_code& error)
	{
		LIB_LOG(lv::trace) << "[Client] handle_closed - id:" << id << ",errorcode:" << error;
	}
	virtual bool handle_receive(int64 id, buffer_ptr_t buffer, int recvLength)
	{
		LIB_LOG(lv::trace) << "[Client] handle_receive - id:" << id;
		
		service->send_buffer(id, buffer);

		return false;
	}
/*
	virtual bool handle_receive_packet(int64 id, boost::shared_ptr<ns::nspacket> packet_ptr)
	{
		LIB_LOG(lv::trace) << "[Client] handle_receive_packet - id:" << id
			<< ",packet_id:" << packet_ptr->id() << ",payload:" << packet_ptr->payload();

		boost::shared_ptr<ns::nspacket> send_packet_ptr = boost::make_shared<ns::nspacket>();

		send_packet_ptr->set_id(1);
		send_packet_ptr->set_seq(1);
		send_packet_ptr->set_payload(packet_ptr->payload());

		service->send_packet(id, send_packet_ptr);
		return true;
	}*/
};

int _tmain(int argc, _TCHAR* argv[])
{
	server_config config;
	config.set_ip("127.0.0.1");
	config.set_port(10000);
	config.set_thread_pool_count(10);
	
	Client* server = new Client();
	service = new NetService(config, server);

	service->init();
	// service->run_io_worker();

	for (int i = 0; i<1; ++i) {
		int64 id;
		service->connect(id);
	}

	while (1) {

		service->poll();

		// std::this_thread::sleep_for(std::chrono::milliseconds(10));

	}

	service->term();

	return 0;
}