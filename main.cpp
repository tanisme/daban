#include <fstream>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include "CApplication.h"
#include "Imitate.h"

int main() {
    std::string cfgfile = "daban.ini";
    int version = 2, useQueueVersion = 0;
    bool istest = true, isstrategyopen = false, isshmdnewversion = true;
    std::string dbfile = "database.db";
    std::string shcpucore = "", szcpucore = "", currentexchangeid = "";
    std::string testshmdaddr = "", testszmdaddr = "", shmdaddr = "", shmdinterface = "", szmdaddr = "", szmdinterface = "";
    std::string tdaddr = "", tdaccount = "", tdpassword = "";
    std::string srcdatapath = "", watchsecurity = "";
    boost::program_options::options_description cfgdesc("Config file options");
    cfgdesc.add_options()
            ("daban.version", boost::program_options::value<int>(&version), "daban.version")
            ("daban.istest", boost::program_options::value<bool>(&istest), "daban.istest")
            ("daban.isstrategyopen", boost::program_options::value<bool>(&isstrategyopen), "daban.isstrategyopen")
            ("daban.usequeueversion", boost::program_options::value<int>(&useQueueVersion), "daban.usequeueversion")
            ("daban.isshmdnewversion", boost::program_options::value<bool>(&isshmdnewversion), "daban.isshmdnewversion")
            ("daban.currentexchangeid", boost::program_options::value<std::string>(&currentexchangeid), "daban.currentexchangeid")
            ("daban.dbfile", boost::program_options::value<std::string>(&dbfile), "daban.dbfile")
            ("daban.tdaddr", boost::program_options::value<std::string>(&tdaddr), "daban.tdaddr")
            ("daban.tdaccount", boost::program_options::value<std::string>(&tdaccount), "daban.tdaccount")
            ("daban.tdpassword", boost::program_options::value<std::string>(&tdpassword), "daban.tdpassword")
            ("daban.watchsecurity", boost::program_options::value<std::string>(&watchsecurity), "daban.watchsecurity")
            ("daban.shcpucore", boost::program_options::value<std::string>(&shcpucore), "daban.shcpucore")
            ("daban.szcpucore", boost::program_options::value<std::string>(&szcpucore), "daban.szcpucore")
            ("ceshi.testshmdaddr", boost::program_options::value<std::string>(&testshmdaddr), "ceshi.testshmdaddr")
            ("ceshi.testszmdaddr", boost::program_options::value<std::string>(&testszmdaddr), "ceshi.testszmdaddr")
            ("shipan.shmdaddr", boost::program_options::value<std::string>(&shmdaddr), "shipan.shmdaddr")
            ("shipan.shmdinterface", boost::program_options::value<std::string>(&shmdinterface), "shipan.shmdinterface")
            ("shipan.szmdaddr", boost::program_options::value<std::string>(&szmdaddr), "shipan.szmdaddr")
            ("shipan.szmdinterface", boost::program_options::value<std::string>(&szmdinterface), "shipan.szmdinterface")
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
        imitate.TestOrderBook(srcdatapath, watchsecurity, false);
        printf("-------------------生成所有合约订单簿完成-------------------\n");
        return 0;
    }

    boost::asio::io_context io_context;
    CApplication app(io_context);
    app.m_version = version;
    app.m_isTest = istest;
    app.m_isUseQueue = useQueueVersion;
    app.m_isStrategyOpen = isstrategyopen;
    app.m_isSHMDNewVersion = isshmdnewversion;
    app.m_dbFile = dbfile;
    app.m_TDAddr = tdaddr;
    app.m_TDAccount = tdaccount;
    app.m_TDPassword = tdpassword;
    app.m_shCpucore = shcpucore;
    app.m_szCpucore = szcpucore;
    app.m_testSHMDAddr = testshmdaddr;
    app.m_testSZMDAddr = testszmdaddr;
    app.m_shMDAddr = shmdaddr;
    app.m_shMDInterface = shmdinterface;
    app.m_szMDAddr = szmdaddr;
    app.m_szMDInterface = szmdinterface;
    app.Init(watchsecurity, currentexchangeid);

    app.Start();
    io_context.run();

    return 0;
}
