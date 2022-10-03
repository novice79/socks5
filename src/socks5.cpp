

#include "socks5.h"
#include <thread>
#include <boost/asio/experimental/as_tuple.hpp>

boost::asio::io_context Socks5::io_(std::thread::hardware_concurrency());
using namespace boost;
using namespace std;
using asio::experimental::as_tuple;
using namespace std::literals::chrono_literals;

Session::~Session()
{
    LOGT("Session destroyed of client %1% ", lexical_cast<std::string>(client_ep_));
}

awaitable<void> Session::delay(std::chrono::seconds d)
{
    asio::steady_timer timer(co_await this_coro::executor);
    timer.expires_after(d);
    co_await timer.async_wait(use_awaitable);
}
awaitable<bool> Session::read_req()
{
    /*
    +----+-----+-------+------+----------+----------+
    |VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
    +----+-----+-------+------+----------+----------+
    | 1  |  1  | X'00' |  1   | Variable |    2     |
    +----+-----+-------+------+----------+----------+
    */
    auto [ec, len] = co_await asio::async_read(
        in_socket_,
        asio::buffer(in_buf_, 4),
        as_tuple(use_awaitable));
    if (ec)
        co_return false;
    if (0x05 != in_buf_[0] || 0x0 != in_buf_[2])
        co_return false;
    cmd_ = in_buf_[1];
    a_type_ = in_buf_[3];
    LOGT("---> %1%", util_->byte2str(in_buf_, 4))
    switch (a_type_)
    {
    case 0x01: // IP V4 addres, with a length of 4 octets
        std::tie(ec, len) = co_await asio::async_read(
            in_socket_,
            asio::buffer(in_buf_ + 4, 4 + 2),
            as_tuple(use_awaitable));
        if (ec)
            co_return false;
        LOGT("---> %1% [Session::read_req read ip&port]", util_->byte2str(in_buf_, 10))
        remote_host_ = asio::ip::address_v4(ntohl(*((uint32_t *)&in_buf_[4]))).to_string();
        remote_port_ = std::to_string(ntohs(*((uint16_t *)&in_buf_[8])));
        LOGT("Session::read_req remote_host_=%1%; remote_port_=%2%",
             remote_host_, remote_port_)
        break;
    case 0x03: // DOMAINNAME, The first
               // octet of the address field contains the number of octets of name that
               // follow, there is no terminating NUL octet
        std::tie(ec, len) = co_await asio::async_read(
            in_socket_,
            asio::buffer(in_buf_ + 4, 1),
            as_tuple(use_awaitable));
        if (ec)
            co_return false;
        LOGT("---> %1% [Session::read_req read domain len]", util_->byte2str(in_buf_, 5))
        std::tie(ec, len) = co_await asio::async_read(
            in_socket_,
            asio::buffer(in_buf_ + 5, in_buf_[4] + 2),
            as_tuple(use_awaitable));
        if (ec)
            co_return false;
        LOGT("---> %1% [Session::read_req read domain&port]", util_->byte2str(in_buf_, 5 + in_buf_[4] + 2))
        remote_host_ = std::string((const char *)&in_buf_[5], in_buf_[4]);
        remote_port_ = std::to_string(ntohs(*((uint16_t *)&in_buf_[5 + in_buf_[4]])));
        LOGT("Session::read_req remote_host_=%1%; remote_port_=%2%",
             remote_host_, remote_port_)
        break;
    case 0x04: // IP V6 address, with a length of 16 octets.
        std::tie(ec, len) = co_await asio::async_read(
            in_socket_,
            asio::buffer(in_buf_ + 4, 16 + 2),
            as_tuple(use_awaitable));
        if (ec)
            co_return false;
        asio::ip::address_v6::bytes_type a_data;
        std::copy(&in_buf_[4], &in_buf_[4] + a_data.size(), a_data.data());
        remote_host_ = asio::ip::address_v6(a_data).to_string();
        remote_port_ = std::to_string(ntohs(*((uint16_t *)&in_buf_[4 + 16])));
        break;
    default:
        co_return false;
    }
    co_return true;
}
// log packet format:
// ---> : client to proxy server
// <--- : proxy to client
// ===> : proxy to target server
// <=== : target to proxy
awaitable<void> Session::handshake()
{
    auto self(shared_from_this());
    /*
    +----+----------+----------+
    |VER | NMETHODS | METHODS  |
    +----+----------+----------+
    | 1  |    1     | 1 to 255 |
    +----+----------+----------+
    */
    auto [ec, len] = co_await asio::async_read(
        in_socket_,
        asio::buffer(in_buf_, 2),
        as_tuple(use_awaitable));
    if (ec || in_buf_[0] != 0x05)
        co_return;
    uint8_t nm = in_buf_[1];
    std::tie(ec, len) = co_await asio::async_read(
        in_socket_,
        asio::buffer(in_buf_ + 2, nm),
        as_tuple(use_awaitable));
    if (ec)
        co_return;
    // Only 0x00 - 'NO AUTHENTICATION REQUIRED' is now support_ed
    in_buf_[1] = 0xFF;
    for (uint8_t i = 0; i < nm; ++i)
        if (in_buf_[2 + i] == 0x00)
        {
            in_buf_[1] = 0x00;
            break;
        }
    /*
    +----+--------+
    |VER | METHOD |
    +----+--------+
    | 1  |   1    |
    +----+--------+
    */
    std::tie(ec, len) = co_await asio::async_write(
        in_socket_,
        boost::asio::buffer(in_buf_, 2),
        as_tuple(use_awaitable));
    LOGT("<--- %1%", util_->byte2str(in_buf_, 2))
    if (ec || in_buf_[1] == 0xFF)
    {
        // co_await delay(3s);
        co_return;
    }
    if (!co_await read_req())
        co_return;
    if (!co_await handle_req())
        co_return;
    switch (cmd_)
    {
    case 0x01: // CONNECT
    {
        std::bind(&Session::read_client_tcp, self)();
        std::bind(&Session::read_server_tcp, self)();
        // read_client_tcp();
        // read_server_tcp();
    }
    break;
    case 0x02: // BIND
    {
        co_spawn(
            in_socket_.get_executor(),
            std::bind(&Session::listener, shared_from_this()),
            detached);
    }
    break;
    case 0x03: // UDP
    {
        co_spawn(
            in_socket_.get_executor(),
            std::bind(&Session::cross_udp, shared_from_this()),
            detached);
    }
    break;
    default:
        break;
    }
}
void Session::read_client_tcp()
{
    auto self(shared_from_this());
    in_socket_.async_read_some(
        asio::buffer(in_buf_),
        [this, self](const boost::system::error_code &ec, std::size_t n)
        {
            if (!ec)
            {
                LOGT("---> %1% [n=%2%]", util_->byte2str(in_buf_, n), n)
                asio::async_write(
                    out_socket_,
                    asio::buffer(in_buf_, n),
                    [this, self](const boost::system::error_code &ec, std::size_t n)
                    {
                        if (!ec)
                        {
                            LOGT("===> %1% [n=%2%]", util_->byte2str(in_buf_, n), n)
                            std::bind(&Session::read_client_tcp, self)();
                        }
                        else
                        {
                            LOGT("write to %1% failed [%2%:%3%]",
                                 boost::lexical_cast<std::string>(target_ep_),
                                 boost::lexical_cast<std::string>(ec),
                                 ec.message())
                        }
                    });
            }
            else
            {
                LOGT("read from %1% failed [%2%:%3%]",
                     boost::lexical_cast<std::string>(client_ep_),
                     boost::lexical_cast<std::string>(ec),
                     ec.message())
                if(out_socket_.is_open())
                {
                    LOGT("close target socket")
                    out_socket_.close();
                }
            }
        });
}
void Session::read_server_tcp()
{
    auto self(shared_from_this());
    out_socket_.async_read_some(
        asio::buffer(out_buf_),
        [this, self](const boost::system::error_code &ec, std::size_t n)
        {
            if (!ec)
            {
                LOGT("<=== %1% [n=%2%]", util_->byte2str(out_buf_, n), n)
                asio::async_write(
                    in_socket_,
                    asio::buffer(out_buf_, n),
                    [this, self](const boost::system::error_code &ec, std::size_t n)
                    {
                        if (!ec)
                        {
                            LOGT("<--- %1% [n=%2%]", util_->byte2str(out_buf_, n), n)
                            std::bind(&Session::read_server_tcp, self)();
                        }
                        else
                        {
                            LOGT("write to %1% failed [%2%:%3%]",
                                 boost::lexical_cast<std::string>(client_ep_),
                                 boost::lexical_cast<std::string>(ec),
                                 ec.message())
                        }
                    });
            }
            else
            {
                LOGT("read from target %1% failed [%2%:%3%]",
                     boost::lexical_cast<std::string>(target_ep_),
                     boost::lexical_cast<std::string>(ec),
                     ec.message())
                if(in_socket_.is_open())
                {
                    LOGT("close client socket")
                    in_socket_.close();
                }
                
            }
        });
}

awaitable<std::tuple<bool, udp::resolver::results_type, uint8_t>> Session::resolve_udp_target_addr()
{
    string udp_r_host, udp_r_port;
    uint8_t skip_n{0};
    system::error_code ec;
    udp::resolver::results_type eps;
    switch (in_buf_[3])
    {
    case 0x01: // IP V4 addres, with a length of 4 octets
        udp_r_host = asio::ip::address_v4(ntohl(*((uint32_t *)&in_buf_[4]))).to_string();
        udp_r_port = std::to_string(ntohs(*((uint16_t *)&in_buf_[8])));
        skip_n = 10;
        break;
    case 0x03: // DOMAINNAME, The first
               // octet of the address field contains the number of octets of name that
               // follow, there is no terminating NUL octet
        udp_r_host = std::string((const char *)&in_buf_[5], in_buf_[4]);
        udp_r_port = std::to_string(ntohs(*((uint16_t *)&in_buf_[5 + in_buf_[4]])));
        skip_n = 5 + in_buf_[4] + 2;
        break;
    case 0x04: // IP V6 address, with a length of 16 octets.
        asio::ip::address_v6::bytes_type a_data;
        std::copy(&in_buf_[4], &in_buf_[4] + a_data.size(), a_data.data());
        udp_r_host = asio::ip::address_v6(a_data).to_string();
        udp_r_port = std::to_string(ntohs(*((uint16_t *)&in_buf_[20])));
        skip_n = 22;
        break;
    default:
        co_return std::make_tuple(false, eps, skip_n);
    }
    udp::resolver resolver{in_socket_.get_executor()};
    std::tie(ec, eps) = co_await resolver.async_resolve(
        udp_r_host,
        udp_r_port,
        as_tuple(use_awaitable));
    if (ec)
        co_return std::make_tuple(false, eps, skip_n);
    co_return std::make_tuple(true, eps, skip_n);
}
awaitable<void> Session::cross_udp()
{
    /*
    +----+------+------+----------+----------+----------+
    |RSV | FRAG | ATYP | DST.ADDR | DST.PORT |   DATA   |
    +----+------+------+----------+----------+----------+
    | 2  |  1   |  1   | Variable |    2     | Variable |
    +----+------+------+----------+----------+----------+
    */
    auto self(shared_from_this());
    udp::endpoint ep;
    for (;;)
    {
        auto [ec, n] = co_await udp_sock_->async_receive_from(asio::buffer(in_buf_), ep, as_tuple(use_awaitable));
        if (ec)
            break;
        if (ep.address().to_string() == in_socket_.remote_endpoint().address().to_string())
        {
            cli_udp_ep_ = ep;
            auto [r, eps, skip_n] = co_await resolve_udp_target_addr();
            if (r && !eps.empty())
            {
                std::tie(ec, n) = co_await udp_sock_->async_send_to(
                    asio::buffer(in_buf_ + skip_n, n - skip_n),
                    eps->endpoint(),
                    as_tuple(use_awaitable));
            }
        }
        else
        {
            uint32_t dst_addr = htonl(ep.address().to_v4().to_ulong());
            uint16_t dst_port = htons(ep.port());
            memset(out_buf_, 0, 3);
            out_buf_[3] = 0x01;
            memcpy(&out_buf_[4], &dst_addr, 4);
            memcpy(&out_buf_[8], &dst_port, 2);
            memcpy(&out_buf_[10], in_buf_, n);
            std::tie(ec, n) = co_await udp_sock_->async_send_to(
                asio::buffer(out_buf_, 10 + n),
                cli_udp_ep_,
                as_tuple(use_awaitable));
        }
    }
}
awaitable<bool> Session::handle_req()
{
    /*
    +----+-----+-------+------+----------+----------+
    |VER | REP |  RSV  | ATYP | BND.ADDR | BND.PORT |
    +----+-----+-------+------+----------+----------+
    | 1  |  1  | X'00' |  1   | Variable |    2     |
    +----+-----+-------+------+----------+----------+
    */
    auto executor = co_await this_coro::executor;
    uint32_t bind_addr{0};
    uint16_t bind_port{0};
    uint8_t reply_buf[10] = {0x05, 0x00, 0x00, 0x01};
    switch (cmd_)
    {
    case 0x01: // CONNECT
    {
        tcp::resolver resolver{in_socket_.get_executor()};
        auto [ec, eps] = co_await resolver.async_resolve(
            remote_host_,
            remote_port_,
            as_tuple(use_awaitable));
        if (ec)
            co_return false;
        auto [rec, ep] = co_await asio::async_connect(
            out_socket_,
            eps,
            as_tuple(use_awaitable));
        if (rec)
        {
            LOGT("connect to %1%:%2% failed", remote_host_, remote_port_)
            co_return false;
        }
        bind_addr = htonl(out_socket_.local_endpoint().address().to_v4().to_ulong());
        bind_port = htons(out_socket_.local_endpoint().port());

        target_ep_ = *eps;
        LOGT("connect to %1% succeed, bind local ep=%2%:%3%",
             boost::lexical_cast<std::string>(target_ep_),
             out_socket_.local_endpoint().address().to_v4(),
             out_socket_.local_endpoint().port())
    }
    break;
    case 0x02: // BIND
    {
        acceptor_.reset(new tcp::acceptor{executor, {tcp::v4(), 0}});
        bind_addr = htonl(acceptor_->local_endpoint().address().to_v4().to_ulong());
        bind_port = htons(acceptor_->local_endpoint().port());
    }
    break;
    case 0x03: // UDP
    {
        udp_sock_.reset(new udp::socket{executor, {udp::v4()}});
        bind_addr = htonl(udp_sock_->local_endpoint().address().to_v4().to_ulong());
        bind_port = htons(udp_sock_->local_endpoint().port());
    }
    break;
    default:
        co_return false;
    }
    memcpy(&reply_buf[4], &bind_addr, 4);
    memcpy(&reply_buf[8], &bind_port, 2);
    auto [ec, len] = co_await asio::async_write(
        in_socket_,
        asio::buffer(reply_buf, sizeof(reply_buf)),
        as_tuple(use_awaitable));
    if (ec)
        co_return false;
    LOGT("<--- %1%", util_->byte2str(reply_buf, sizeof(reply_buf)))
    co_return true;
}
awaitable<void> Session::listener()
{
    auto self(shared_from_this());
    auto executor = co_await this_coro::executor;
    for (;;)
    {
        auto [ec, socket] = co_await acceptor_->async_accept(as_tuple(use_awaitable));
        if (ec)
            break;
        uint32_t bind_addr{0};
        uint16_t bind_port{0};
        uint8_t reply_buf[10] = {0x05, 0x00, 0x00, 0x01};
        bind_addr = htonl(socket.remote_endpoint().address().to_v4().to_ulong());
        bind_port = htons(socket.remote_endpoint().port());
        memcpy(&reply_buf[4], &bind_addr, 4);
        memcpy(&reply_buf[8], &bind_port, 2);
        auto [rec, len] = co_await asio::async_write(
            in_socket_,
            asio::buffer(reply_buf, sizeof(reply_buf)),
            as_tuple(use_awaitable));
        if (rec)
            break;
        co_spawn(executor, notify(std::move(socket)), detached);
    }
}
awaitable<void> Session::notify(tcp::socket socket)
{
    uint8_t data[1024] = {0};
    for (;;)
    {
        auto [ec, n] = co_await socket.async_read_some(
            boost::asio::buffer(data, sizeof(data)),
            as_tuple(use_awaitable));
        if (ec)
            break;
        co_await async_write(in_socket_, boost::asio::buffer(data, n), use_awaitable);
    }
}
