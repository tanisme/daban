﻿#include <fstream>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include "CApplication.h"
#include "Imitate.h"

////////////////////////////////////////////////////////////////////////////////////////
//#include <clickhouse/client.h>
//#include <clickhouse/block.h>
//#include <clickhouse/error_codes.h>
//#include <clickhouse/types/type_parser.h>
//#include <clickhouse/base/socket.h>
//#include <iostream>
//#if defined(_MSC_VER)
//#   pragma warning(disable : 4996)
//#endif
//
//using namespace clickhouse;
//using namespace std;
//
//inline void PrintBlock(const Block& block) {
//    for (Block::Iterator bi(block); bi.IsValid(); bi.Next()) {
//        std::cout << bi.Name() << " ";
//    }
//    //std::cout << std::endl << block;
//}
//
//inline void ArrayExample(Client& client) {
//    Block b;
//
//    /// Create a table.
//    client.Execute("CREATE TEMPORARY TABLE IF NOT EXISTS test_array (arr Array(UInt64))");
//
//    auto arr = std::make_shared<ColumnArray>(std::make_shared<ColumnUInt64>());
//
//    auto id = std::make_shared<ColumnUInt64>();
//    id->Append(1);
//    arr->AppendAsColumn(id);
//
//    id->Append(3);
//    arr->AppendAsColumn(id);
//
//    id->Append(7);
//    arr->AppendAsColumn(id);
//
//    id->Append(9);
//    arr->AppendAsColumn(id);
//
//    b.AppendColumn("arr", arr);
//    client.Insert("test_array", b);
//
//    client.Select("SELECT arr FROM test_array", [](const Block& block)
//                  {
//                      for (size_t c = 0; c < block.GetRowCount(); ++c) {
//                          auto col = block[0]->As<ColumnArray>()->GetAsColumn(c);
//                          for (size_t i = 0; i < col->Size(); ++i) {
//                              std::cerr << (int)(*col->As<ColumnUInt64>())[i] << " ";
//                          }
//                          std::cerr << std::endl;
//                      }
//                  }
//    );
//
//    /// Delete table.
//    client.Execute("DROP TEMPORARY TABLE test_array");
//}
//
//inline void MultiArrayExample(Client& client) {
//    Block b;
//
//    /// Create a table.
//    client.Execute("CREATE TEMPORARY TABLE IF NOT EXISTS test_multiarray (arr Array(Array(UInt64)))");
//
//    auto arr = std::make_shared<ColumnArray>(std::make_shared<ColumnUInt64>());
//
//    auto id = std::make_shared<ColumnUInt64>();
//    id->Append(17);
//    arr->AppendAsColumn(id);
//
//    auto a2 = std::make_shared<ColumnArray>(std::make_shared<ColumnArray>(std::make_shared<ColumnUInt64>()));
//    a2->AppendAsColumn(arr);
//    b.AppendColumn("arr", a2);
//    client.Insert("test_multiarray", b);
//
//    client.Select("SELECT arr FROM test_multiarray", [](const Block& block)
//                  {
//                      for (size_t c = 0; c < block.GetRowCount(); ++c) {
//                          auto col = block[0]->As<ColumnArray>()->GetAsColumn(c);
//                          cout << "[";
//                          for (size_t i = 0; i < col->Size(); ++i) {
//                              auto col2 = col->As<ColumnArray>()->GetAsColumn(i);
//                              for (size_t j = 0; j < col2->Size(); ++j) {
//                                  cout << (int)(*col2->As<ColumnUInt64>())[j];
//                                  if (j + 1 != col2->Size()) {
//                                      cout << " ";
//                                  }
//                              }
//                          }
//                          std::cout << "]" << std::endl;
//                      }
//                  }
//    );
//
//    /// Delete table.
//    client.Execute("DROP TEMPORARY TABLE test_multiarray");
//}
//
//inline void DateExample(Client& client) {
//    Block b;
//
//    /// Create a table.
//    client.Execute("CREATE TEMPORARY TABLE IF NOT EXISTS test_date (d DateTime, dz DateTime('Europe/Moscow'))");
//
//    auto d = std::make_shared<ColumnDateTime>();
//    auto dz = std::make_shared<ColumnDateTime>();
//    d->Append(std::time(nullptr));
//    dz->Append(std::time(nullptr));
//    b.AppendColumn("d", d);
//    b.AppendColumn("dz", dz);
//    client.Insert("test_date", b);
//
//    client.Select("SELECT d, dz FROM test_date", [](const Block& block)
//                  {
//                      for (size_t c = 0; c < block.GetRowCount(); ++c) {
//
//                          auto print_value = [&](const auto& col) {
//                              std::time_t t = col->At(c);
//                              std::cerr << std::asctime(std::localtime(&t));
//                              std::cerr << col->Timezone() << std::endl;
//                          };
//
//                          print_value(block[0]->As<ColumnDateTime>());
//                          print_value(block[1]->As<ColumnDateTime>());
//                      }
//                  }
//    );
//
//    /// Delete table.
//    client.Execute("DROP TEMPORARY TABLE test_date");
//}
//
//inline void DateTime64Example(Client& client) {
//    Block b;
//
//    /// Create a table.
//    client.Execute("CREATE TEMPORARY TABLE IF NOT EXISTS test_datetime64 (dt64 DateTime64(6))");
//
//    auto d = std::make_shared<ColumnDateTime64>(6);
//    d->Append(std::time(nullptr) * 1000000 + 123456);
//
//    b.AppendColumn("dt64", d);
//    client.Insert("test_datetime64", b);
//
//    client.Select("SELECT dt64 FROM test_datetime64", [](const Block& block)
//                  {
//                      for (size_t c = 0; c < block.GetRowCount(); ++c) {
//                          auto col = block[0]->As<ColumnDateTime64>();
//                          uint64_t t = col->As<ColumnDateTime64>()->At(c);
//
//                          std::time_t ct = t / 1000000;
//                          uint64_t us = t % 1000000;
//                          std::cerr << "ctime: " << std::asctime(std::localtime(&ct));
//                          std::cerr << "us: " << us << std::endl;
//                      }
//                  }
//    );
//
//    /// Delete table.
//    client.Execute("DROP TEMPORARY TABLE test_datetime64");
//}
//
//inline void DecimalExample(Client& client) {
//    Block b;
//
//    /// Create a table.
//    client.Execute("CREATE TEMPORARY TABLE IF NOT EXISTS test_decimal (d Decimal64(4))");
//
//    auto d = std::make_shared<ColumnDecimal>(18, 4);
//    d->Append(21111);
//    b.AppendColumn("d", d);
//    client.Insert("test_decimal", b);
//
//    client.Select("SELECT d FROM test_decimal", [](const Block& block)
//                  {
//                      for (size_t c = 0; c < block.GetRowCount(); ++c) {
//                          auto col = block[0]->As<ColumnDecimal>();
//                          cout << (int)col->At(c) << endl;
//                      }
//                  }
//    );
//
//    client.Select("SELECT toDecimal32(2, 4) AS x", [](const Block& block)
//                  {
//                      for (size_t c = 0; c < block.GetRowCount(); ++c) {
//                          auto col = block[0]->As<ColumnDecimal>();
//                          cout << (int)col->At(c) << endl;
//                      }
//                  }
//    );
//
//    /// Delete table.
//    client.Execute("DROP TEMPORARY TABLE test_decimal");
//}
//
//inline void GenericExample(Client& client) {
//    /// Create a table.
//    client.Execute("CREATE TEMPORARY TABLE IF NOT EXISTS test_client (id UInt64, name String)");
//
//    /// Insert some values.
//    {
//        Block block;
//
//        auto id = std::make_shared<ColumnUInt64>();
//        id->Append(1);
//        id->Append(7);
//
//        auto name = std::make_shared<ColumnString>();
//        name->Append("one");
//        name->Append("seven");
//
//        block.AppendColumn("id"  , id);
//        block.AppendColumn("name", name);
//
//        client.Insert("test_client", block);
//    }
//
//    /// Select values inserted in the previous step.
//    client.Select("SELECT id, name FROM test_client", [](const Block& block)
//                  {
//                      //std::cout << PrettyPrintBlock{block} << std::endl;
//                  }
//    );
//
//    /// Delete table.
//    client.Execute("DROP TEMPORARY TABLE test_client");
//}
//
//inline void NullableExample(Client& client) {
//    /// Create a table.
//    client.Execute("CREATE TEMPORARY TABLE IF NOT EXISTS test_client (id Nullable(UInt64), date Nullable(Date))");
//
//    /// Insert some values.
//    {
//        Block block;
//
//        {
//            auto id = std::make_shared<ColumnUInt64>();
//            id->Append(1);
//            id->Append(2);
//
//            auto nulls = std::make_shared<ColumnUInt8>();
//            nulls->Append(0);
//            nulls->Append(0);
//
//            block.AppendColumn("id", std::make_shared<ColumnNullable>(id, nulls));
//        }
//
//        {
//            auto date = std::make_shared<ColumnDate>();
//            date->Append(std::time(nullptr));
//            date->Append(std::time(nullptr));
//
//            auto nulls = std::make_shared<ColumnUInt8>();
//            nulls->Append(0);
//            nulls->Append(1);
//
//            block.AppendColumn("date", std::make_shared<ColumnNullable>(date, nulls));
//        }
//
//        client.Insert("test_client", block);
//    }
//
//    /// Select values inserted in the previous step.
//    client.Select("SELECT id, date FROM test_client", [](const Block& block)
//                  {
//                      for (size_t c = 0; c < block.GetRowCount(); ++c) {
//                          auto col_id   = block[0]->As<ColumnNullable>();
//                          auto col_date = block[1]->As<ColumnNullable>();
//
//                          if (col_id->IsNull(c)) {
//                              std::cerr << "\\N ";
//                          } else {
//                              std::cerr << col_id->Nested()->As<ColumnUInt64>()->At(c)
//                                        << " ";
//                          }
//
//                          if (col_date->IsNull(c)) {
//                              std::cerr << "\\N\n";
//                          } else {
//                              std::time_t t = col_date->Nested()->As<ColumnDate>()->At(c);
//                              std::cerr << std::asctime(std::localtime(&t))
//                                        << "\n";
//                          }
//                      }
//                  }
//    );
//
//    /// Delete table.
//    client.Execute("DROP TEMPORARY TABLE test_client");
//}
//
//inline void NumbersExample(Client& client) {
//    size_t num = 0;
//
//    client.Select("SELECT number, number FROM system.numbers LIMIT 100000", [&num](const Block& block)
//                  {
//                      if (Block::Iterator(block).IsValid()) {
//                          auto col = block[0]->As<ColumnUInt64>();
//
//                          for (size_t i = 0; i < col->Size(); ++i) {
//                              if (col->At(i) < num) {
//                                  throw std::runtime_error("invalid sequence of numbers");
//                              }
//
//                              num = col->At(i);
//                          }
//                      }
//                  }
//    );
//}
//
//inline void CancelableExample(Client& client) {
//    /// Create a table.
//    client.Execute("CREATE TEMPORARY TABLE IF NOT EXISTS test_client (x UInt64)");
//
//    /// Insert a few blocks.
//    for (unsigned j = 0; j < 10; j++) {
//        Block b;
//
//        auto x = std::make_shared<ColumnUInt64>();
//        for (uint64_t i = 0; i < 1000; i++) {
//            x->Append(i);
//        }
//
//        b.AppendColumn("x", x);
//        client.Insert("test_client", b);
//    }
//
//    /// Send a query which is canceled after receiving the first block (note:
//    /// due to the low number of rows in this test, this will not actually have
//    /// any effect, it just tests for errors)
//    client.SelectCancelable("SELECT * FROM test_client", [](const Block&)
//                            {
//                                return false;
//                            }
//    );
//
//    /// Delete table.
//    client.Execute("DROP TEMPORARY TABLE test_client");
//}
//
//inline void ExecptionExample(Client& client) {
//    /// Create a table.
//    client.Execute("CREATE TEMPORARY TABLE IF NOT EXISTS test_exceptions (id UInt64, name String)");
//    /// Expect failing on table creation.
//    try {
//        client.Execute("CREATE TEMPORARY TABLE test_exceptions (id UInt64, name String)");
//    } catch (const ServerException& e) {
//        if (e.GetCode() == ErrorCodes::TABLE_ALREADY_EXISTS) {
//            // OK
//        } else {
//            throw;
//        }
//    }
//
//    /// Delete table.
//    client.Execute("DROP TEMPORARY TABLE test_exceptions");
//}
//
//inline void EnumExample(Client& client) {
//    /// Create a table.
//    client.Execute("CREATE TEMPORARY TABLE IF NOT EXISTS test_enums (id UInt64, e Enum8('One' = 1, 'Two' = 2))");
//
//    /// Insert some values.
//    {
//        Block block;
//
//        auto id = std::make_shared<ColumnUInt64>();
//        id->Append(1);
//        id->Append(2);
//
//        auto e = std::make_shared<ColumnEnum8>(Type::CreateEnum8({{"One", 1}, {"Two", 2}}));
//        e->Append(1);
//        e->Append("Two");
//
//        block.AppendColumn("id", id);
//        block.AppendColumn("e", e);
//
//        client.Insert("test_enums", block);
//    }
//
//    /// Select values inserted in the previous step.
//    client.Select("SELECT id, e FROM test_enums", [](const Block& block)
//                  {
//                      for (Block::Iterator bi(block); bi.IsValid(); bi.Next()) {
//                          std::cout << bi.Name() << " ";
//                      }
//                      std::cout << std::endl;
//
//                      for (size_t i = 0; i < block.GetRowCount(); ++i) {
//                          std::cout << (*block[0]->As<ColumnUInt64>())[i] << " "
//                                    << (*block[1]->As<ColumnEnum8>()).NameAt(i) << "\n";
//                      }
//                  }
//    );
//
//    /// Delete table.
//    client.Execute("DROP TEMPORARY TABLE test_enums");
//}
//
//inline void SelectNull(Client& client) {
//    client.Select("SELECT NULL", []([[maybe_unused]] const Block& block)
//                  {
//                      assert(block.GetRowCount() < 2);
//                      (void)(block);
//                  }
//    );
//}
//
//inline void ShowTables(Client& client) {
//    /// Select values inserted in the previous step.
//    client.Select("SHOW TABLES", [](const Block& block)
//                  {
//                      for (size_t i = 0; i < block.GetRowCount(); ++i) {
//                          std::cout << (*block[0]->As<ColumnString>())[i] << "\n";
//                      }
//                  }
//    );
//}
//
//inline void IPExample(Client &client) {
//    /// Create a table.
//    //client.Execute("CREATE TEMPORARY TABLE IF NOT EXISTS test_ips (id UInt64, v4 IPv4, v6 IPv6)");
//    client.Execute("CREATE TABLE IF NOT EXISTS test_ips (id UInt64, v4 IPv4, v6 IPv6)");
//
//    /// Insert some values.
//    {
//        Block block;
//
//        auto id = std::make_shared<ColumnUInt64>();
//        id->Append(1);
//        id->Append(2);
//        id->Append(3);
//
//        auto v4 = std::make_shared<ColumnIPv4>();
//        v4->Append("127.0.0.1");
//        v4->Append(3585395774);
//        v4->Append(0);
//
//        auto v6 = std::make_shared<ColumnIPv6>();
//        v6->Append("::1");
//        v6->Append("aa::ff");
//        v6->Append("fe80::86ba:ef31:f2d8:7e8b");
//
//        block.AppendColumn("id", id);
//        block.AppendColumn("v4", v4);
//        block.AppendColumn("v6", v6);
//
//        client.Insert("test_ips", block);
//    }
//
//    /// Select values inserted in the previous step.
//    client.Select("SELECT id, v4, v6 FROM test_ips", [](const Block& block)
//                  {
//                      for (Block::Iterator bi(block); bi.IsValid(); bi.Next()) {
//                          std::cout << bi.Name() << " ";
//                      }
//                      std::cout << std::endl;
//
//                      for (size_t i = 0; i < block.GetRowCount(); ++i) {
//                          std::cout << (*block[0]->As<ColumnUInt64>())[i] << " "
//                                    << (*block[1]->As<ColumnIPv4>()).AsString(i) << " (" << (*block[1]->As<ColumnIPv4>())[i].s_addr << ") "
//                                    << (*block[2]->As<ColumnIPv6>()).AsString(i) << "\n";
//                      }
//                  }
//    );
//
//    /// Delete table.
//    //client.Execute("DROP TEMPORARY TABLE test_ips");
//}
//
//static void RunTests(Client& client) {
//    //ArrayExample(client);
//    //CancelableExample(client);
//    //DateExample(client);
//    //DateTime64Example(client);
//    //DecimalExample(client);
//    //EnumExample(client);
//    //ExecptionExample(client);
//    //GenericExample(client);
//    IPExample(client);
//    //MultiArrayExample(client);
//    //NullableExample(client);
//    //NumbersExample(client);
//    //SelectNull(client);
//    ShowTables(client);
//}
//
//void test_clickhouse() {
//
//    try {
//        const auto localHostEndpoint = ClientOptions()
//                .SetHost(   "124.221.16.239")
//                .SetPort(   9000)
//                .SetEndpoints({   {"124.221.16.239", 9000}
//                //                      ,{"localhost"}
//                //                      ,{"noalocalhost", 9000}
//                              })
//                .SetUser(           "default")
//                .SetPassword(       "")
//                .SetDefaultDatabase("tests");
//
//        {
//            Client client(ClientOptions(localHostEndpoint)
//                                  .SetPingBeforeQuery(true));
//            RunTests(client);
//            std::cout << "current endpoint : " <<  client.GetCurrentEndpoint().value().host << "\n";
//        }
//
//        {
//            Client client(ClientOptions(localHostEndpoint)
//                                  .SetPingBeforeQuery(true)
//                                  .SetCompressionMethod(CompressionMethod::LZ4));
//            RunTests(client);
//        }
//    } catch (const std::exception& e) {
//        std::cerr << "exception : " << e.what() << std::endl;
//    }
//
//}

int main() {
    std::string cfgfile = "daban.ini";
    bool istest = true, isstrategyopen = false, isshnewversion = false, createfile = false, isuseunlockqueue = false;
    int matchoore = 1;
    std::string dbfile = "database.db";
    std::string shcpucore = "", szcpucore = "", currentexchangeid = "";
    std::string testshmdaddr = "", testszmdaddr = "", shmdaddr = "", shmdinterface = "", szmdaddr = "", szmdinterface = "";
    std::string tdaddr = "", tdaccount = "", tdpassword = "";
    std::string srcdatapath = "", watchsecurity = "";
    boost::program_options::options_description cfgdesc("Config file options");
    cfgdesc.add_options()
            ("daban.istest", boost::program_options::value<bool>(&istest), "daban.istest")
            ("daban.isstrategyopen", boost::program_options::value<bool>(&isstrategyopen), "daban.isstrategyopen")
            ("daban.isshnewversion", boost::program_options::value<bool>(&isshnewversion), "daban.isshnewversion")
            ("daban.isuseunlockqueue", boost::program_options::value<bool>(&isuseunlockqueue), "daban.isuseunlockqueue")
            ("daban.matchoore", boost::program_options::value<int>(&matchoore), "daban.matchoore")
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
            ("csvfile.createfile", boost::program_options::value<bool>(&createfile), "csvfile.createfile")
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
        imitate.TestOrderBook(srcdatapath, watchsecurity, createfile);
        printf("-------------------生成所有合约订单簿完成-------------------\n");
        return 0;
    }

    boost::asio::io_context io_context;
    CApplication app(io_context);
    app.m_isTest = istest;
    app.m_isUseUnlockQueue = isuseunlockqueue;
    app.m_isStrategyOpen = isstrategyopen;
    app.m_isSHNewversion = isshnewversion;
    app.m_matchCore = matchoore;
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
