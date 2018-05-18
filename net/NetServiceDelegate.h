#pragma once

#include <boost/asio.hpp>
#include <boost/function.hpp>

#include "types.h"
//
//#include "..\protocol\output\config.pb.h"
//#include "..\protocol\output\nspacket.pb.h"

namespace Idea {
	
class NetServiceDelegate 
{
public:
	NetServiceDelegate() {};
	virtual ~NetServiceDelegate() {};

	virtual void handle_connected( int64 id, std::string address, const boost::system::error_code& error) = 0;
	virtual void handle_closed( int64 id, const boost::system::error_code& error) = 0;
	virtual bool handle_receive(int64 id, buffer_ptr_t buffer, int recvLength) = 0;
	//virtual bool handle_receive_packet( int64 id, std::shared_ptr<ns::nspacket> packet_ptr) = 0;
};


enum nspacket_define {
	MESSAGE_LENGTH_SIZE = 4,
	MESSAGE_PAYLOAD = 16380
};

struct server_config
{
	::std::string ip_;
	int32 port_ = 8888;
	int32 thread_pool_count_ = 5;
	int32 receive_timeout_ = 60;			// 리시브 timeout 
	int32 receive_timeout_intval_ = 60;		// 리시브 timeout 체크 간격
	int32 max_send_queue_size_ = 50;		// 보내기 큐 최대 사이즈
	int32 zombie_intval_ = 60;				// 좀비 클라이언트 체크 간격

	inline void set_ip(const ::std::string& value) { ip_ = value; }
	inline void set_port(int32 value) { port_ = value; }
	inline const std::string& ip() const{ 	return ip_;	}
	inline int32 port() const { return port_; }

	inline int32 thread_pool_count() const {
		return thread_pool_count_;
	}
	inline void set_thread_pool_count(int32 value) {
		thread_pool_count_ = value;
	}

	inline int32 server_config::receive_timeout() const {
		return receive_timeout_;
	}
	inline void server_config::set_receive_timeout(int32 value) {
		receive_timeout_ = value;
	}
	inline int32 server_config::max_send_queue_size() const {
		return max_send_queue_size_;
	}
	inline void server_config::set_max_send_queue_size(int32 value) {
		max_send_queue_size_ = value;
	}
	inline int32 receive_timeout_intval() const
	{
		return receive_timeout_intval_;
	}
	inline void set_receive_timeout_intval(int32 value)
	{
		receive_timeout_intval_ = value;
	}


	inline int32 zombie_intval() const {
		return zombie_intval_;
	}
	inline void set_zombie_intval(int32 value) {
		zombie_intval_ = value;
	}

};

/*

class IMessageProc
{
public:
	virtual bool proc(int64 id, std::shared_ptr<ns::nspacket> packet) = 0;
};

template <typename MsgT>
class Proc
{
public:
	typedef boost::function<void (int64, std::shared_ptr<MsgT>)> Type;
};

template <typename MsgT>
class MessageProc : public IMessageProc
{
	boost::function<void (int64, std::shared_ptr<MsgT>)> func_;
public:
	MessageProc(boost::function<void (int64, std::shared_ptr<MsgT>)> func)
		: func_(func)
	{}

	virtual bool proc(int64 id, std::shared_ptr<ns::nspacket> packet)
	{
		std::shared_ptr<MsgT> message = boost::make_shared<MsgT>();
		if (!message->ParseFromString( packet->payload()))
		{
			return false;;
		}
		
		func_(id, message);

		return true;
	}
};

class MessageHandler
{
private:
	std::map<int, std::shared_ptr<IMessageProc>> proc_map_;

public:
	template <typename T>
	bool register_handler( boost::function<void (int64, std::shared_ptr<T>)> func )
	{
		T message;
		if (proc_map_.end() != proc_map_.find(message.id()))
			return false;

		auto msg_proc = boost::make_shared<MessageProc<T>>(func);
		auto result = proc_map_.insert( std::make_pair( message.id(), msg_proc));
		return result.second;
	}
	
	void clear()
	{
		proc_map_.clear();
	}

	bool process( int64 id, std::shared_ptr<ns::nspacket> message)
	{
		auto proc = proc_map_.find(message->id());
		if ( proc_map_.end() == proc)
		{
			return false;
		}

		return proc->second->proc(id, message);
	}
};
*/
}