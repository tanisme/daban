#include <fstream>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include "CApplication.h"

int main() {
    std::string cfgfile = "daban.ini";
    bool istest = true, isshexchange = true, isshnewversion = false;
    std::string mdaddr = "", mdinterface = "";
    std::string tdaddr = "", tdaccount = "", tdpassword = "";
    std::string srcdatapath = "", watchsecurity = "";
    boost::program_options::options_description cfgdesc("Config file options");
    cfgdesc.add_options()
            ("daban.istest", boost::program_options::value<bool>(&istest), "daban.istest")
            ("daban.isshexchange", boost::program_options::value<bool>(&isshexchange), "daban.isshexchange")
            ("daban.isshnewversion", boost::program_options::value<bool>(&isshnewversion), "daban.isshnewversion")
            ("daban.tdaddr", boost::program_options::value<std::string>(&tdaddr), "daban.tdaddr")
            ("daban.tdaccount", boost::program_options::value<std::string>(&tdaccount), "daban.tdaccount")
            ("daban.tdpassword", boost::program_options::value<std::string>(&tdpassword), "daban.tdpassword")
            ("daban.watchsecurity", boost::program_options::value<std::string>(&watchsecurity), "daban.watchsecurity")
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

    boost::asio::io_context io_context;
    CApplication app(io_context);
    app.m_isTest = istest;
    app.m_isSHExchange = isshexchange;
    app.m_isSHNewversion = isshnewversion;
    app.m_TDAddr = tdaddr;
    app.m_TDAccount = tdaccount;
    app.m_TDPassword = tdpassword;
    app.m_MDAddr = mdaddr;
    app.m_MDInterface = mdinterface;
    app.m_dataDir = srcdatapath;
    app.Init(watchsecurity);

    app.Start();
    io_context.run();

    return 0;
}
