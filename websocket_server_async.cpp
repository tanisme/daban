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

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "websocket_server_async.h"
#include "TickTradingFramework.h"
#include "spdlog/spdlog.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

extern std::shared_ptr<spdlog::logger> logger;

//------------------------------------------------------------------------------
#define HEARTBEAT_TIME_OUT 120 // seconds

session::session(tcp::socket&& socket, CTickTradingFramework* pFramework)
    : ws_(std::move(socket)),
    m_pFramework(pFramework)
{
}

// Get on the correct executor
void session::run()
{
    // We need to be executing within a strand to perform async operations
    // on the I/O objects in this session. Although not strictly necessary
    // for single-threaded contexts, this example code is written to be
    // thread-safe by default.
    net::dispatch(ws_.get_executor(),
        beast::bind_front_handler(
            &session::on_run,
            shared_from_this()));
}

// Start the asynchronous operation
void session::on_run()
{
    // Set suggested timeout settings for the websocket
    ws_.set_option(
        websocket::stream_base::timeout::suggested(
            beast::role_type::server));

    // Set a decorator to change the Server of the handshake
    ws_.set_option(websocket::stream_base::decorator(
        [](websocket::response_type& res)
        {
            res.set(http::field::server,
                std::string(BOOST_BEAST_VERSION_STRING) +
                " websocket-server-async");
        }));
    // Accept the websocket handshake
    ws_.async_accept(
        beast::bind_front_handler(
            &session::on_accept,
            shared_from_this()));
}

void session::SendConnectedEvent()
{
    //TTF_Message_t* Message = m_pFramework->MessageAllocate();
    //Message->MsgType = MsgTypeClientConnected;
    //MsgClientConnected_t* pMsgClientConnected = (MsgClientConnected_t*)&Message->Message;
    //pMsgClientConnected->s = this;
    //m_pFramework->m_queue.enqueue(Message);
}

void session::SendDisconnectedEvent()
{
    //TTF_Message_t* Message = m_pFramework->MessageAllocate();
    //Message->MsgType = MsgTypeClientDisconnected;
    //MsgClientDisconnected_t* pMsgClientDisconnected = (MsgClientDisconnected_t*)&Message->Message;
    //pMsgClientDisconnected->s = this;
    //m_pFramework->m_queue.enqueue(Message);
}

void session::SendRequestEvent(std::string& msg)
{
    //TTF_Message_t* Message = m_pFramework->MessageAllocate();
    //Message->MsgType = MsgTypeClientRequest;
    //MsgClientRequest_t* pMsgClientRequest = (MsgClientRequest_t*)&Message->Message;
    //pMsgClientRequest->s = this;
    //strncpy(pMsgClientRequest->request, msg.c_str(), sizeof(pMsgClientRequest->request) - 1);
    //m_pFramework->m_queue.enqueue(Message);
}

void session::SendResponseEvent(std::string& msg)
{
    boost::beast::net::post(ws_.get_executor(),
        boost::beast::bind_front_handler(&session::send, shared_from_this(), msg));;
}

void session::on_accept(beast::error_code ec)
{
    if (ec) {
        std::cerr << "accept" << ": " << ec.message() << "\n";
        return;
    }

    logger->info("Connected.");

    SendConnectedEvent();
    //m_pFramework->OnRemoteConnected(this);

    // Heartbeat Send Timer
    m_pSendTimer = new boost::asio::deadline_timer(m_pFramework->m_pWebServer->ioc_, boost::posix_time::seconds(HEARTBEAT_TIME_OUT/2));
    m_pSendTimer->expires_from_now(boost::posix_time::seconds(HEARTBEAT_TIME_OUT/2));
    m_pSendTimer->async_wait(boost::bind(&session::OnHeartbeatSendTimeout, this, boost::asio::placeholders::error));

    // Heartbeat Recv Timer
    m_pRecvTimer = new boost::asio::deadline_timer(m_pFramework->m_pWebServer->ioc_, boost::posix_time::seconds(HEARTBEAT_TIME_OUT));
    m_pRecvTimer->expires_from_now(boost::posix_time::seconds(HEARTBEAT_TIME_OUT));
    m_pRecvTimer->async_wait(boost::bind(&session::OnHeartbeatRecvTimeout, this, boost::asio::placeholders::error));

    // Read a message
    do_read();
}

void session::do_read()
{
    // Read a message into our buffer
    ws_.async_read(
        buffer_,
        beast::bind_front_handler(
            &session::on_read,
            shared_from_this()));
}

void session::on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    // This indicates that the session was closed
    if (ec == websocket::error::closed) {
        logger->warn("Disconnected.");
        if (m_pSendTimer) {
            m_pSendTimer->cancel();
            m_pSendTimer = nullptr;
        }
        if (m_pRecvTimer) {
            m_pRecvTimer->cancel();
            m_pRecvTimer = nullptr;
        }
        SendDisconnectedEvent();
        //m_pFramework->OnRemoteDisconnected(this);
        return;
    }

    if (ec) {
        std::cerr << "read" << ": " << ec.message() << "\n";
        logger->warn("Disconnected.");
        SendDisconnectedEvent();
        //m_pFramework->OnRemoteDisconnected(this);
        if (m_pSendTimer) {
            m_pSendTimer->cancel();
            m_pSendTimer = nullptr;
        }
        if (m_pRecvTimer) {
            m_pRecvTimer->cancel();
            m_pRecvTimer = nullptr;
        }
        return;
    }

    m_pRecvTimer->expires_from_now(boost::posix_time::seconds(HEARTBEAT_TIME_OUT));
    m_pRecvTimer->async_wait(boost::bind(&session::OnHeartbeatRecvTimeout, this, boost::asio::placeholders::error));

    std::string req((char*)buffer_.data().data(),buffer_.size());
    
    logger->debug("recved:"+req);

    SendRequestEvent(req);

	//message_ = "";
 //   if (!m_pFramework->OnRemoteMessage(this, req, message_)) {
	//	logger->warn("Recv err msg , close it.");
	//	if (m_pRecvTimer) {
	//		m_pRecvTimer->cancel();
	//		delete m_pRecvTimer;
	//		m_pRecvTimer = nullptr;
	//	}
	//	if (m_pSendTimer) {
	//		m_pSendTimer->cancel();
	//		delete m_pSendTimer;
	//		m_pSendTimer = nullptr;
	//	}
	//	ws_.async_close(websocket::close_code::going_away,
	//		beast::bind_front_handler(
	//			&session::on_close,
	//			shared_from_this()));
 //       return;
 //   }

    // Clear the buffer
    buffer_.consume(buffer_.size());

    // Send response
    //if (message_.length() > 0) {
    //    send_message(message_);
    //}

    do_read();
}

void session::on_write(
        beast::error_code ec,
        std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if (ec) {
        std::cerr << "write" << ": " << ec.message() << "\n";
        logger->warn("Disconnected.");
        SendDisconnectedEvent();
        //m_pFramework->OnRemoteDisconnected(this);
        if (m_pSendTimer) {
            m_pSendTimer->cancel();
            m_pSendTimer = nullptr;
        }
        if (m_pRecvTimer) {
            m_pRecvTimer->cancel();
            m_pRecvTimer = nullptr;
        }
        return;
    }

    m_sendMsg.erase(m_sendMsg.begin());
    if (m_sendMsg.empty()) return;
    ws_.async_write(
            net::buffer(*m_sendMsg.begin()),
            beast::bind_front_handler(
                    &session::on_write,
                    shared_from_this()));
}

void session::on_close(beast::error_code ec)
{
    logger->warn("Disconnected.");
    SendDisconnectedEvent();
    //m_pFramework->OnRemoteDisconnected(this);
    if (m_pSendTimer) {
        m_pSendTimer->cancel();
        m_pSendTimer = nullptr;
    }
    if (m_pRecvTimer) {
        m_pRecvTimer->cancel();
        m_pRecvTimer = nullptr;
    }
}

void session::send_message(const std::string& message)
{
    boost::beast::net::post(ws_.get_executor(),
                            boost::beast::bind_front_handler(&session::send, shared_from_this(), message));
}

void session::send(std::string message)
{
    m_sendMsg.push_back(message);
    if (m_sendMsg.size() > 1) return;

    ws_.async_write(boost::beast::net::buffer(*m_sendMsg.begin()),
                    boost::beast::bind_front_handler(&session::on_write, shared_from_this()));
}

// OnHeartbeatSendTimeout Timer
void session::OnHeartbeatSendTimeout(const boost::system::error_code& error)
{
    if (error) {
        if (error == boost::asio::error::operation_aborted) {
            return;
        }
    }

    // ��������
    send_message("{\"MsgType\": \"ping\"}");

    if (m_pSendTimer) {
		m_pSendTimer->expires_from_now(boost::posix_time::seconds(HEARTBEAT_TIME_OUT/2));
		m_pSendTimer->async_wait(boost::bind(&session::OnHeartbeatSendTimeout, this, boost::asio::placeholders::error));
    }
}

// OnHeartbeatRecvTimeout Timer
void session::OnHeartbeatRecvTimeout(const boost::system::error_code& error)
{
    if (error) {
        if (error == boost::asio::error::operation_aborted) {
            return;
        }
    }

    logger->warn("Connection heartbeat timeout.");
    if (m_pRecvTimer) {
        m_pRecvTimer->cancel();
		delete m_pRecvTimer;
		m_pRecvTimer = nullptr;
    }
    if (m_pSendTimer) {
        m_pSendTimer->cancel();
		delete m_pSendTimer;
		m_pSendTimer = nullptr;
    }
    ws_.async_close(websocket::close_code::going_away,
        beast::bind_front_handler(
            &session::on_close,
            shared_from_this()));
}


//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions

listener::listener(tcp::endpoint endpoint, CTickTradingFramework* pFramework)
    :acceptor_(ioc_)
    , m_pFramework(pFramework)
{
    beast::error_code ec;

    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if (ec)
    {
        std::cerr << "open" << ": " << ec.message() << "\n";
        return;
    }

    // Allow address reuse
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec)
    {
        std::cerr << "set_option" << ": " << ec.message() << "\n";
        return;
    }

    // Bind to the server address
    acceptor_.bind(endpoint, ec);
    if (ec)
    {
        std::cerr << "bind" << ": " << ec.message() << "\n";
        return;
    }

    // Start listening for connections
    acceptor_.listen(
        net::socket_base::max_listen_connections, ec);
    if (ec)
    {
        std::cerr << "listen" << ": " << ec.message() << "\n";
        return;
    }
}

// Start accepting incoming connections
void listener::run()
{
    do_accept();
    std::thread *t = new std::thread(boost::bind(&net::io_context::run, &ioc_));
//#ifndef _WIN32
//    auto bindcore = atoi(webbindcore.c_str());
//    cpu_set_t cpuset;
//    CPU_ZERO(&cpuset);
//    CPU_SET(bindcore, &cpuset);
//    int rc = pthread_setaffinity_np(t->native_handle(), sizeof(cpu_set_t), &cpuset);
//    if (rc != 0) {
//        printf("websocket thread bind failed[%d]\n", bindcore);
//    } else {
//        printf("websocket thread bind success[%d]\n", bindcore);
//    }
//#endif
}

void listener::do_accept()
{
    // The new connection gets its own strand
    acceptor_.async_accept(
        net::make_strand(ioc_),
        beast::bind_front_handler(
            &listener::on_accept,
            shared_from_this()));
}

void listener::on_accept(beast::error_code ec, tcp::socket socket)
{
    if (ec)
    {
        std::cerr << "accept" << ": " << ec.message() << "\n";
    }
    else
    {
        // Create the session and run it
        std::make_shared<session>(std::move(socket), m_pFramework)->run();
    }

    // Accept another connection
    do_accept();
}