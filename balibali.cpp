// balibali.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <fstream>
#include <stdarg.h>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include "TickTradingFramework.h"

#define BALIBALI_VERSION_NO "1.0"

std::shared_ptr<spdlog::logger> logger;

void log_init(std::string level)
{
    try {
#ifdef _DEBUG
        // debug版使用同步日志，同时打印到屏幕
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::from_str(level));
        console_sink->set_pattern("%^[%Y-%m-%d %H:%M:%S:%e] [%t] [%l] %v%$");

        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("log/balibali.log");
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S:%e] [%t] [%l] %v");
        file_sink->set_level(spdlog::level::from_str(level));

        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(console_sink);
        sinks.push_back(file_sink);

        logger = std::make_shared<spdlog::logger>("multi_logger", begin(sinks), end(sinks));
        logger->set_level(spdlog::level::from_str(level));
#else
        // release版使用异步日志，不打印到屏幕，日志队列满了也不会影响系统运行
        spdlog::init_thread_pool(32768, 1); // queue with 32k items and 1 backing thread.
        //logger = spdlog::create_async_nb<spdlog::sinks::daily_file_sink_mt>("async_file_logger", "log/balibali.log" ,0 ,0); // 每天一个日志文件
        logger = spdlog::create_async_nb<spdlog::sinks::basic_file_sink_mt>("async_file_logger", "log/balibali.log");
        //logger = spdlog::basic_logger_mt<spdlog::async_factory>("async_file_logger", "log/balibali.log");
        logger->set_pattern("[%Y-%m-%d %H:%M:%S:%e] [%t] [%l] %v");
        logger->set_level(spdlog::level::from_str(level));
#endif
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        std::cout << "spdlog initialization faild" << ex.what() << std::endl;
    }
}


int main(int argc, char** argv)
{
    std::string WebServer, TdHost, TdNameServer, LIP, MAC, HD, MdHost, User, Password, UserProductInfo, L2InterfaceAddress;
    std::string Admin = "admin";
    std::string AdminPassword;
    std::string log_level = "debug";
    std::string WsCore = "", TdCore = "", MdCore = "";
    std::string Exchanges = "SSE,SZSE";

    using namespace boost::program_options;

    options_description desc("Usage");
    desc.add_options()
        ("help,h", "display usage")
        ("version,v", "display version")
        ;

    options_description cfgdesc("Config file options");
    cfgdesc.add_options()
        ("balibali.Exchanges", value<std::string>(&Exchanges), "balibali.Exchanges")
        ("balibali.WsCore", value<std::string>(&WsCore), "balibali.WsCore")
        ("balibali.TdCore", value<std::string>(&TdCore), "balibali.TdCore")
        ("balibali.MdCore", value<std::string>(&MdCore), "balibali.MdCore")
        ("balibali.WebServer", value<std::string>(&WebServer), "balibali.WebServer")
        ("balibali.TradeFront", value<std::string>(&TdHost), "balibali.TradeFront")
        ("balibali.TradeNameServer", value<std::string>(&TdNameServer), "balibali.TradeNameServer")
        ("balibali.UserProductInfo", value<std::string>(&UserProductInfo), "balibali.UserProductInfo")
        ("balibali.LIP", value<std::string>(&LIP), "balibali.LIP")
        ("balibali.MAC", value<std::string>(&MAC), "balibali.MAC")
        ("balibali.HD", value<std::string>(&HD), "balibali.HD")
        ("balibali.L2MarketFront", value<std::string>(&MdHost), "balibali.L2MarketFront")
        ("balibali.user", value<std::string>(&User), "balibali.user")
        ("balibali.password", value<std::string>(&Password), "balibali.password")
        ("balibali.Admin", value<std::string>(&Admin), "balibali.Admin")
        ("balibali.AdminPassword", value<std::string>(&AdminPassword), "balibali.AdminPassword")
        ("balibali.L2InterfaceAddress", value<std::string>(&L2InterfaceAddress), "balibali.L2InterfaceAddress")
        ("balibali.log_level", value<std::string>(&log_level), "balibali.log_level")
        ;
    variables_map vm;

#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif	

    // read config from command line
    store(parse_command_line(argc, argv, desc), vm);
    notify(vm);

    if (vm.count("help")) {
        std::cout << desc;
        return 0;
    }
    if (vm.count("version")) {
        std::cout << BALIBALI_VERSION_NO << "@" << __DATE__ << " " << __TIME__ << std::endl;
        return 0;
    }

    vm.clear();

    // read config from file
    std::ifstream ifs;
    ifs.open("balibali.ini");
    if (ifs.fail()) {
        std::cout << "open ini file failed." << std::endl;
        return -1;
    }
    store(parse_config_file(ifs, cfgdesc, true), vm);
    notify(vm);
    ifs.close();

    vm.clear();

    log_init(log_level);

    CTickTradingFramework Framework(WebServer, Admin, AdminPassword, TdHost, MdHost, User, Password, UserProductInfo, LIP, MAC, HD, L2InterfaceAddress, Exchanges, WsCore, TdCore, MdCore);

	// 运行
	Framework.Run();
}
