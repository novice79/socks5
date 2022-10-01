#pragma once
#include "util.h"

using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;
using boost::asio::steady_timer;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;
namespace this_coro = boost::asio::this_coro;
#define BUFFER_SIZE 2048
class Session : public std::enable_shared_from_this<Session>
{
public:
	Session(tcp::socket in_socket, boost::asio::io_context& io)
		:	in_socket_(std::move(in_socket)), 
			out_socket_(io)
	{
	}
	void start()
	{
		co_spawn(
			in_socket_.get_executor(), 
			std::bind( &Session::handshake, shared_from_this() ), 
			detached
		);
	}

private:	
	awaitable<void> delay(std::chrono::seconds d = std::chrono::seconds(2));
	awaitable<void> handshake();
	awaitable<bool> read_req();
	awaitable<bool> handle_req();
	awaitable<void> cross_two_tcp();
	awaitable<void> cross_udp();
	awaitable<std::tuple<bool, udp::resolver::results_type, uint8_t>> resolve_udp_target_addr();
	awaitable<void> listener();
	awaitable<void> notify(tcp::socket socket);
	udp::endpoint cli_udp_ep_;
	tcp::socket in_socket_;
	tcp::socket out_socket_;
	std::shared_ptr<udp::socket> udp_sock_;
	std::shared_ptr<tcp::acceptor> acceptor_;
	std::string remote_host_, remote_port_;
	uint8_t in_buf_[BUFFER_SIZE];
	uint8_t out_buf_[BUFFER_SIZE];
	uint8_t a_type_, cmd_;
};

class Socks5
{
	std::shared_ptr<Util> util_;
public:
	Socks5(int port)
		: acceptor_(io_, tcp::endpoint(tcp::v4(), port)), util_(new Util())
	{	
		
	}
	void run()
	{
		co_spawn(io_, do_accept(),  detached);
		LOGD("listen on %d", acceptor_.local_endpoint().port() )
		io_.run();
	}
private:
	awaitable<void> do_accept()
	{
		for (;;)
		{
			std::make_shared<Session>(
				co_await acceptor_.async_accept(use_awaitable),
				io_
			)->start();
		}
	}
	tcp::acceptor acceptor_;
	static boost::asio::io_context io_;
};
