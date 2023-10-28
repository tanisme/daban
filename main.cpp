#include <fstream>

#include "CApplication.h"
#include <iostream>
#include <boost/asio.hpp>
#include "server.hpp"
#include "file_handler.hpp"
#include <boost/program_options.hpp>
#include "LocalConfig.h"

int main() {
    std::string cfgfile = "daban.ini";
    std::string dbfile = "database.db";
    std::string httpurl = "";
    std::string shmdaddr = "", szmdaddr = "";
    std::string tdaddr = "", tdaccount = "", tdpassword = "";
    boost::program_options::options_description cfgdesc("Config file options");
    cfgdesc.add_options()
            ("daban.dbfile", boost::program_options::value<std::string>(&dbfile), "daban.dbfile")
            ("daban.httpurl", boost::program_options::value<std::string>(&httpurl), "daban.httpurl")
            ("daban.shmdaddr", boost::program_options::value<std::string>(&shmdaddr), "daban.shmdaddr")
            ("daban.szmdaddr", boost::program_options::value<std::string>(&szmdaddr), "daban.szmdaddr")
            ("daban.tdaddr", boost::program_options::value<std::string>(&tdaddr), "daban.tdaddr")
            ("daban.tdaccount", boost::program_options::value<std::string>(&tdaccount), "daban.tdaccount")
            ("daban.tdpassword", boost::program_options::value<std::string>(&tdpassword), "daban.tdpassword");
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

    LocalConfig::GetMe().SetDBFile(dbfile);
    LocalConfig::GetMe().SetSHMDAddr(shmdaddr);
    LocalConfig::GetMe().SetSZMDAddr(szmdaddr);
    LocalConfig::GetMe().SetTDAddr(tdaddr);
    LocalConfig::GetMe().SetTDAccount(tdaccount);
    LocalConfig::GetMe().SetTDPassword(tdpassword);
    LocalConfig::GetMe().SetHttpUrl(httpurl);
    LocalConfig::GetMe().Init();

    boost::asio::io_context io_context;

    CApplication app(io_context);
    if (httpurl.length() > 0) {
        http::server4::server(io_context, std::string(httpurl, 0, httpurl.find(':')),
                              httpurl.substr(httpurl.find(':') + 1), http::server4::file_handler(&app))();
    }

    app.Start();

    io_context.run();

    return 0;
}
