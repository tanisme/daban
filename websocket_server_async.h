//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: WebSocket server, asynchronous
//
//------------------------------------------------------------------------------

#include "boost/asio.hpp"
#include "boost/beast/core.hpp"
#include "boost/beast/websocket.hpp"
#include "boost/asio/dispatch.hpp"
#include "boost/asio/strand.hpp"
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <unordered_map>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------

class CTickTradingFramework;
class session : public std::enable_shared_from_this<session>
{
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    std::string message_;

public:
    // Take ownership of the socket
    explicit
        session(tcp::socket&& socket, CTickTradingFramework* pFramework);

    // Get on the correct executor
    void
        run();

    // Start the asynchronous operation
    void
        on_run();

    void
        on_accept(beast::error_code ec);

    void
        do_read();

    void
        on_read(
            beast::error_code ec,
            std::size_t bytes_transferred);

    void
        on_write(
            beast::error_code ec,
            std::size_t bytes_transferred);

    void on_close(beast::error_code ec);

    std::unordered_map<std::string, bool> m_subInstrument;
    void SendConnectedEvent();
    void SendDisconnectedEvent();
    void SendRequestEvent(std::string& msg);
    void SendResponseEvent(std::string& msg);
    void send_message(const std::string& message);
    void OnHeartbeatSendTimeout(const boost::system::error_code& error);
    void OnHeartbeatRecvTimeout(const boost::system::error_code& error);
    CTickTradingFramework* m_pFramework;
    boost::asio::deadline_timer* m_pSendTimer = nullptr;
    boost::asio::deadline_timer* m_pRecvTimer = nullptr;

    std::vector<std::string> m_sendMsg;
    void send(std::string msg);
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener>
{
public:
    net::io_context ioc_;
    tcp::acceptor acceptor_;

public:

    listener(tcp::endpoint endpoint, CTickTradingFramework* pFramework);

    // Start accepting incoming connections
    void run();

private:
    void do_accept();

    void on_accept(beast::error_code ec, tcp::socket socket);

    CTickTradingFramework* m_pFramework;
};

