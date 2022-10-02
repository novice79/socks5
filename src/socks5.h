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
	std::shared_ptr<Util> util_;
public:
	Session(tcp::socket in_socket, boost::asio::io_context& io, std::shared_ptr<Util> util)
		:	in_socket_(std::move(in_socket)), 
			out_socket_(io), util_(util)
	{
		client_ep_ = in_socket_.remote_endpoint();
	}
	~Session();
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
	void read_client_tcp();
	void read_server_tcp();
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
	// for log usage
	tcp::endpoint client_ep_, target_ep_;
};

class Socks5
{
	std::shared_ptr<Util> util_;
public:
	// arg use for get executable path, and to write log there
	Socks5(int port, std::string arg0 = "")
		: acceptor_(io_, tcp::endpoint(tcp::v4(), port)), util_(new Util(arg0))
	{	
		
	}
	void run()
	{
		co_spawn(io_, do_accept(),  detached);
		LOGD("socks5 proxy server listen on: %d", acceptor_.local_endpoint().port() )
		io_.run();
	}
private:
	awaitable<void> do_accept()
	{
		for (;;)
		{
			std::make_shared<Session>(
				co_await acceptor_.async_accept(use_awaitable),
				io_,
				util_
			)->start();
		}
	}
	tcp::acceptor acceptor_;
	static boost::asio::io_context io_;
};
