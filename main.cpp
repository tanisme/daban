#include <fstream>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include "server.hpp"
#include "file_handler.hpp"
#include "CApplication.h"
#include "test.h"

int main() {
    std::string cfgfile = "daban.ini";
    std::string dbfile = "database.db";
    std::string httpurl = "", cpucore = "";
    std::string shmdaddr = "", szmdaddr = "", mdaddr = "", mdinterface = "";
    std::string tdaddr = "", tdaccount = "", tdpassword = "";
    int shmdnewversion = 0;
    bool isTest = true, strategyopen = false;
    std::string srcdatapath = "", watchsecurity = "";
    boost::program_options::options_description cfgdesc("Config file options");
    cfgdesc.add_options()
            ("daban.istest", boost::program_options::value<bool>(&isTest), "daban.istest")
            ("daban.strategyopen", boost::program_options::value<bool>(&strategyopen), "daban.strategyopen")
            ("daban.dbfile", boost::program_options::value<std::string>(&dbfile), "daban.dbfile")
            ("daban.cpucore", boost::program_options::value<std::string>(&cpucore), "daban.cpucore")
            ("daban.httpurl", boost::program_options::value<std::string>(&httpurl), "daban.httpurl")
            ("daban.tdaddr", boost::program_options::value<std::string>(&tdaddr), "daban.tdaddr")
            ("daban.tdaccount", boost::program_options::value<std::string>(&tdaccount), "daban.tdaccount")
            ("daban.tdpassword", boost::program_options::value<std::string>(&tdpassword), "daban.tdpassword")
            ("daban.shmdnewversion", boost::program_options::value<int>(&shmdnewversion), "daban.shmdnewversion")
            ("daban.watchsecurity", boost::program_options::value<std::string>(&watchsecurity), "daban.watchsecurity")
            ("ceshi.shmdaddr", boost::program_options::value<std::string>(&shmdaddr), "ceshi.shmdaddr")
            ("ceshi.szmdaddr", boost::program_options::value<std::string>(&szmdaddr), "ceshi.szmdaddr")
            ("shipan.mdaddr", boost::program_options::value<std::string>(&mdaddr), "shipan.mdaddr")
            ("shipan.mdinterface", boost::program_options::value<std::string>(&mdinterface), "shipan.mdinterface")
            ("csvfile.srcdatapath", boost::program_options::value<std::string>(&srcdatapath), "csvfile.srcdatapath");
    boost::program_options::variables_map vm;

    std::ifstream ifs;
    ifs.open(cfgfile.c_str());
    if (ifs.fail()) {
        printf("open cfgfile failed.[%s]\n", cfgfile.c_str());
        return -1;
    }
    store(parse_config_file(ifs, cfgdesc, true), vm);
    notify(vm);
    ifs.close();
    vm.clear();

    if (srcdatapath.length() > 0) {
        printf("-------------------该程序功能为生成订单簿-------------------\n");
        test::Imitate imitate;
        imitate.TestOrderBook(srcdatapath, watchsecurity);
        printf("-------------------生成所有合约订单簿完成-------------------\n");
        return 0;
    }

    boost::asio::io_context io_context;
    CApplication app(io_context);
    app.m_isTest = isTest;
    app.m_dbFile = dbfile;
    app.m_TDAddr = tdaddr;
    app.m_TDAccount = tdaccount;
    app.m_TDPassword = tdpassword;
    app.m_shMDNewVersion = shmdnewversion;
    app.m_shMDAddr = shmdaddr;
    app.m_szMDAddr = szmdaddr;
    app.m_mdAddr = mdaddr;
    app.m_mdInterface = mdinterface;
    app.m_cpucore = cpucore;
    app.m_strategyOpen = strategyopen;
    app.Init(watchsecurity);

    if (httpurl.length() > 0) {
        http::server4::server(io_context, std::string(httpurl, 0, httpurl.find(':')),
                              httpurl.substr(httpurl.find(':') + 1), http::server4::file_handler(&app))();
    }

    app.Start();
    io_context.run();

    return 0;
}
