#pragma once

#include <unordered_map>
#include <iostream>

#include <boost/thread/thread.hpp>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/atomic.hpp>
#include <boost/noncopyable.hpp>

#include "TcpConnection.h"

namespace Idea 
{

/*
	net service


*/
class NetService : private boost::noncopyable
{
	boost::asio::io_service io_service_;
	boost::thread_group io_thread_pool_;
	std::mutex mtx_;
	std::shared_ptr<boost::asio::io_service::work> work_;

	boost::asio::ip::tcp::endpoint endpoint_;
	boost::asio::ip::tcp::acceptor acceptor_;
	
	boost::asio::io_service::strand strand_;
	boost::asio::basic_waitable_timer<boost::chrono::steady_clock> timer_;
	
	NetServiceDelegate* event_listener_;
	std::unordered_map<int64, TcpConnectionWeakPtr> connection_map_;
	server_config config_;

	mutable boost::atomic<int64> top_id_;

public:
	explicit NetService(const server_config& config, NetServiceDelegate* listener);
	virtual ~NetService(void);

	// �ʱ�ȭ
	bool init();
	// io worker ����
	void run_io_worker();
	// io ���� ó���� polling ��
	void poll();
	// ���� ��Ű��
	void term();
	// ��� ���� ����
	void close_all();

	// ���� �ϱ�( �ٸ� �������� )
	bool connect(int64& id);
	// ���� ����
	bool close(int64 id);
	// ���� ������ üũ
	bool is_connected(int64 id);

	// acceptor ���۽�Ű��
	bool start_acceptor();
	// acceptor �����Ű��
	void stop_acceptor();

	//void send_message(int64 id, const ::google::protobuf::Message& message);
	//void send_packet(int64 id, std::shared_ptr<ns::nspacket> packet);

	// buffer �� �ش��ϴ� ������ �����Ѵ�.
	void send_buffer(int64 id, buffer_ptr_t buffer);
	
	/*template <typename T>
	void register_message_handler( boost::function<void (int64, std::shared_ptr<T>)> func )
	{
		message_handler_.register_handler<T>(func);
	}
	void clear_message_handler()
	{
		message_handler_.clear();
	}*/

private:
	void post_accept();
	void post_timer();

	TcpConnectionPtr get_connection(int64 id);

	void completed_accept(TcpConnectionPtr connection, const boost::system::error_code& error);
	void completed_timer(const boost::system::error_code& error);

	void check_zombie_client();

};

}