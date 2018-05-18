#pragma once

#include <memory>
#include <deque>
#include <utility>
#include <string>
#include <memory>

#include <boost/thread/mutex.hpp>
#include <chrono>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/atomic.hpp>

#include "NetServiceDelegate.h"

namespace Idea {

class NetService;
class TcpConnection;
class NetServiceDelegate;
class TcpConnection;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using TcpConnectionWeakPtr = std::weak_ptr<TcpConnection>;

// tcp connection
class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
	// �̺�Ʈ�� ó���ؾ��ϴ� Listener
	NetServiceDelegate* event_listener_;	
//	MessageHandler* message_handler_;
	// ������ id
	int64 id_;
	// 
	boost::asio::io_service& io_service_;
	// io strand (connect/send/receive/timer)
	boost::asio::io_service::strand strand_;
	// socket
	boost::asio::ip::tcp::socket socket_;
	// ��������
	server_config config_;
	// receive buffer - payload����
	int receive_payload_length_;
	// receive buffer - payload����
	buffer_ptr_t receive_payload_buffer_;
	// send buffer queue
	std::deque<buffer_ptr_t> send_que;
	// ������ receive �� �ð� (timeout üũ�� ����� )
	std::chrono::steady_clock::time_point last_receive_time_;
	// timeout üũ�� timer
	boost::asio::basic_waitable_timer<std::chrono::steady_clock> timer_;
	// last error
	boost::system::error_code last_error_;
	mutable std::atomic<int32> send_seq_;
	mutable std::atomic<int32> receive_seq_;	
public:
	explicit TcpConnection(boost::asio::io_service& io_service, NetServiceDelegate* event_listener, /*MessageHandler* message_handler, */const server_config& config);
	virtual ~TcpConnection();

	// ���� ��û�ϱ�(target �� server_config �� �ִ� ip/port ��)
	bool connect();
	// ���� ����
	void close(bool gracefully = true);

	int64 id();
	void id(int64 id);

	boost::asio::ip::tcp::socket& socket();
	std::string remote_address();
	// send
	// void send_packet(std::shared_ptr<ns::nspacket> packet);
	// send
	void send_buffer(buffer_ptr_t buffer);
	// send message
	// void send_message(const ::google::protobuf::Message& message);
	// accpeted �Ǿ���
	void accpeted(const boost::system::error_code& error);
private:
	std::size_t send_que_size();
	std::chrono::steady_clock::time_point last_receive_time();

//	void send_packet_lock(std::shared_ptr<ns::nspacket> packet_ptr);
	void send_buffer_lock(buffer_ptr_t buffer);
		
	void post_receive();
	void post_send();
	void post_timer();

	void reset_last_receive_time();

	void completed_connected(const boost::system::error_code& error);
	void completed_timer(const boost::system::error_code& error);
	void completed_closed(const boost::system::error_code& error);
	void completed_received_header(const boost::system::error_code& error);
	void completed_received_payload(const boost::system::error_code& error);
	void completed_sent(size_t posted_count, const boost::system::error_code& error);

	bool check_receive_timeout();
	void set_last_error(boost::system::error_code error);

};

}