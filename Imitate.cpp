#include <boost/filesystem.hpp>
#include "Imitate.h"
#include "CApplication.h"
#include <iostream>
#include <ctime>
#include <iomanip>
#include <chrono>

namespace test {

    using namespace TORALEV2API;

    void Imitate::TestOrderBook(CApplication* pApp, std::string& srcDataDir) {

        using std::chrono::system_clock;
        system_clock::time_point now = system_clock::now();
        std::time_t now_c = system_clock::to_time_t(
                now - std::chrono::hours(24));
        std::cout << "One day ago, the time was "
                  << std::put_time(std::localtime(&now_c), "%F %T") << '\n';

        m_pApp = pApp;
        auto useAllDataFile = false;
        if (useAllDataFile) {
            auto createfile = false;
            std::string dstDataDir = srcDataDir + "/result/";
            boost::filesystem::path path(dstDataDir);
            if (!boost::filesystem::exists(dstDataDir)) {
                boost::filesystem::create_directories(dstDataDir);
            }

            {
                printf("-------------------------------------------------------------------\n");
                if (createfile) {
                    printf("开始分离文件 %s\n", GetTimeStr().c_str());
                    auto orderCnt = 0, tradeCnt = 0;
                    for (auto iter : pApp->m_watchSecurity) {
                        std::string orderFileName = dstDataDir + iter.second->SecurityID + "_o.txt";
                        std::string tradeFileName = dstDataDir + iter.second->SecurityID + "_t.txt";
                        FILE *opf = fopen(orderFileName.c_str(), "w+");
                        if (!opf) {
                            printf("打开订单%s文件失败\n", orderFileName.c_str());
                            return;
                        }
                        FILE *tpf = fopen(tradeFileName.c_str(), "w+");
                        if (!tpf) {
                            printf("打开成交%s文件失败\n", tradeFileName.c_str());
                            fclose(opf);
                            return;
                        }
                        m_watchOrderFILE[iter.second->SecurityID] = opf;
                        m_watchTradeFILE[iter.second->SecurityID] = tpf;
                    }
                    orderCnt = SplitSecurityFile(srcDataDir, true);
                    tradeCnt = SplitSecurityFile(srcDataDir, false);
                    for (auto iter = m_watchOrderFILE.begin(); iter != m_watchOrderFILE.end(); ++iter)
                        if (iter->second) fclose(iter->second);
                    for (auto iter = m_watchTradeFILE.begin(); iter != m_watchTradeFILE.end(); ++iter)
                        if (iter->second) fclose(iter->second);
                    printf("分离文件完成 订单数量:%d 成交数量:%d\n", orderCnt, tradeCnt);
                }

                auto bt = GetUs();
                printf("开始生成订单簿 %s\n", GetTimeStr().c_str());
                int cnt = 0;
                for (auto iter : pApp->m_watchSecurity) {
                    cnt += SplitSecurityFileOrderQuot(dstDataDir, (char*)iter.second->SecurityID);
                    cnt += SplitSecurityFileTradeQuot(dstDataDir, (char*)iter.second->SecurityID);
                }
                auto duration = GetUs() - bt;
                printf("生成订单簿完成 总数量:%d 耗时%d微秒 平均耗时:%.3f\n", (int)duration, cnt, duration*1.0/cnt);
                printf("-------------------------------------------------------------------\n");
            }
        } else {
            auto create_file = false;
            printf("开始读取文件!!!\n");
            std::string file = srcDataDir;
            std::ifstream ifs(file, std::ios::in);
            if (!ifs.is_open()) {
                printf("xxfile.csv open failed!!! path%s\n", file.c_str());
                return;
            }

            std::unordered_map<std::string, FILE*> file_fps;
            if (create_file) {
                for (auto& it : pApp->m_watchSecurity) {
                    std::string fileName = "./" + std::string(it.second->SecurityID) + ".txt";
                    FILE *fp = fopen(fileName.c_str(), "w+");
                    if (!fp) {
                        printf("打开%s文件失败\n", fileName.c_str());
                        return;
                    }
                    file_fps[it.second->SecurityID] = fp;
                }
            }
            int i = 0;
            std::string line;
            std::vector<std::string> res;
            while (std::getline(ifs, line)) {
                res.clear();
                trim(line);
                Stringsplit(line, ',', res);
                std::string SecurityID = res.at(2);
                //if (++i % 1000000 == 0) {
                    i++;
                    //printf("已经读取行数:%d %s\n", i, line.c_str());
                    //for (auto it : pApp->m_watchSecurity) {
                    //    if (it.second->ExchangeID == PROMD::TORA_TSTP_EXD_SZSE) {
                    //        pApp->ShowOrderBook((char*)it.second->SecurityID);
                    //    }
                    //}
                //}
                //if (i >= 65000000) {
                //    printf("%d %s\n", i, line.c_str());
                //}
                int SecurityIDInt = atoi(SecurityID.c_str());
                //if (SecurityIDInt != 2129) continue;
                if (pApp->m_watchSecurity.find(SecurityIDInt) == pApp->m_watchSecurity.end()) continue;
                if (create_file) {
                    auto it = file_fps.find(SecurityID);
                    FILE* fp = it->second;
                    fputs(line.c_str(), fp);
                    fputs("\n", fp);
                }
                std::string ExchangeID = res.at(3);
                if (ExchangeID.c_str()[0] == PROMD::TORA_TSTP_EXD_SSE) continue;
                std::string Time = res.at(1);
                //if (strncmp(Time.c_str(), "145700000", 9) >= 0) continue;
                if (res.at(0) == "O") {
                    std::string Price = res.at(4);
                    std::string Volume = res.at(5);
                    std::string Side = res.at(6);
                    std::string OrderType = res.at(7);
                    std::string OrderNo = res.at(13);
                    std::string OrderStatus = res.at(14);

                    PROMD::CTORATstpLev2OrderDetailField OrderDetail = {0};
                    strcpy(OrderDetail.SecurityID, SecurityID.c_str());
                    OrderDetail.ExchangeID = ExchangeID.c_str()[0];
                    OrderDetail.OrderType = OrderType.c_str()[0];
                    OrderDetail.Side = Side.c_str()[0];
                    OrderDetail.OrderNO = atoll(OrderNo.c_str());
                    OrderDetail.Price = atof(Price.c_str());
                    OrderDetail.Volume = atol(Volume.c_str());
                    OrderDetail.OrderStatus = OrderStatus.c_str()[0];
                    pApp->MDOnRtnOrderDetail(OrderDetail);
                } else if (res.at(0) == "T") {
                    std::string TradePrice = res.at(4);
                    std::string TradeVolume = res.at(5);
                    std::string ExecType = res.at(6);
                    std::string BuyNo = res.at(9);
                    std::string SellNo = res.at(10);

                    PROMD::CTORATstpLev2TransactionField Transaction = {0};
                    strcpy(Transaction.SecurityID, SecurityID.c_str());
                    Transaction.ExchangeID = ExchangeID.c_str()[0];
                    Transaction.TradeVolume = atoll(TradeVolume.c_str());
                    Transaction.ExecType = ExecType.c_str()[0];
                    Transaction.BuyNo = atoll(BuyNo.c_str());
                    Transaction.SellNo = atoll(SellNo.c_str());
                    Transaction.TradePrice = atof(TradePrice.c_str());
                    pApp->MDOnRtnTransaction(Transaction);
                }
            }

            for (auto& it : file_fps) {
                if (it.second) fclose(it.second);
            }
            printf("结束读取文件!!! 累计条数:%d\n", i);
            for (auto it : pApp->m_watchSecurity) {
                pApp->ShowOrderBook((char*)it.second->SecurityID);
            }
            ifs.close();
        }
    }

    int Imitate::SplitSecurityFileOrderQuot(std::string &dstDataDir, TTORATstpSecurityIDType SecurityID) {
        std::string file = dstDataDir + SecurityID + "_o.txt";
        std::ifstream ifs(file, std::ios::in);
        if (!ifs.is_open()) {
            printf("stOrder open failed!!! path%s\n", file.c_str());
            return 0;
        }

        int i = 1;
        std::string line;
        std::vector<std::string> res;
        while (std::getline(ifs, line)) {
            res.clear();
            Stringsplit(line, ',', res);

            std::string ExchangeID = res.at(0);                             //交易所代码
            //std::string SecurityID = res.at(1);                                //证券代码
            if (strcmp(SecurityID, res.at(1).c_str())) continue;
            std::string OrderTime = res.at(2);                              //时间戳
            std::string Price = res.at(3).substr(0, 8);         //委托价格
            std::string Volume = res.at(4);                                 //委托数量
            std::string Side = res.at(5);                                   //委托方向
            std::string OrderType = res.at(6);                              //订单类别（只有深圳行情有效）
            std::string OrderNO = res.at(12);                               //委托序号
            std::string OrderStatus = res.at(13);                           //订单状态

            PROMD::CTORATstpLev2OrderDetailField OrderDetail = {0};
            strcpy(OrderDetail.SecurityID, SecurityID);
            OrderDetail.Side = Side.c_str()[0];
            OrderDetail.OrderNO = atoll(OrderNO.c_str());
            OrderDetail.Price = atof(Price.c_str());
            OrderDetail.Volume = atoll(Volume.c_str());
            OrderDetail.ExchangeID = ExchangeID.c_str()[0];
            OrderDetail.OrderStatus = OrderStatus.c_str()[0];
            OrderDetail.OrderType = OrderType.c_str()[0];
            m_pApp->MDOnRtnOrderDetail(OrderDetail);
            i++;
        }
        ifs.close();
        printf("生成%s订单簿 处理完成所有订单 共%d条\n", SecurityID, i);
        return i;
    }

    int Imitate::SplitSecurityFileTradeQuot(std::string &dstDataDir, TTORATstpSecurityIDType SecurityID) {
        std::string file = dstDataDir + SecurityID + "_t.txt";
        std::ifstream ifs(file, std::ios::in);
        if (!ifs.is_open()) {
            printf("Transaction.csv open failed!!! path%s\n", file.c_str());
            return 0;
        }

        int i = 0;
        std::string line;
        std::vector<std::string> res;
        while (std::getline(ifs, line)) {
            res.clear();
            Stringsplit(line, ',', res);

            std::string ExchangeID = res.at(0);                                 //交易所代码
            //std::string SecurityID = res.at(1);                                    //证券代码
            if (strcmp(SecurityID, res.at(1).c_str())) continue;
            std::string TradeTime = res.at(2);                                  //时间戳
            std::string TradePrice = res.at(3).substr(0, 8);        //成交价格
            std::string TradeVolume = res.at(4);                                //成交数量
            std::string ExecType = res.at(5);                                   //成交类别（只有深圳行情有效）
            std::string BuyNo = res.at(8);                                      //买方委托序号
            std::string SellNo = res.at(9);                                     //卖方委托序号

            PROMD::CTORATstpLev2TransactionField Transaction = {0};
            strcpy(Transaction.SecurityID, SecurityID);
            Transaction.ExchangeID = ExchangeID.c_str()[0];
            Transaction.TradeVolume = atoll(TradeVolume.c_str());
            Transaction.ExecType = ExecType.c_str()[0];
            Transaction.BuyNo = atoll(BuyNo.c_str());
            Transaction.SellNo = atoll(SellNo.c_str());
            Transaction.TradePrice = atof(TradePrice.c_str());
            m_pApp->MDOnRtnTransaction(Transaction);
            i++;
        }
        ifs.close();
        printf("生成%s订单簿 处理完成所有成交 共%d条\n", SecurityID, i);
        m_pApp->ShowOrderBook(SecurityID);
        return i;
    }

    int Imitate::SplitSecurityFile(std::string srcDataDir, bool isOrder) {
        if (isOrder) {
            srcDataDir += "/OrderDetail.csv";
        } else {
            srcDataDir += "/Transaction.csv";
        }

        std::ifstream ifs(srcDataDir, std::ios::in);
        if (!ifs.is_open()) {
            printf("srcDataFile csv open failed!!! path%s\n", srcDataDir.c_str());
            return 0;
        }

        auto cnt = 0;
        std::string line;
        std::vector<std::string> res;
        std::unordered_map<std::string, int> writeCount;

        while (std::getline(ifs, line)) {
            res.clear();
            Stringsplit(line, ',', res);

            if ((int) res.size() != 15) {
                //printf("读取%s文件 有错误数据 列数:%d 内容:%s\n", isOrder ? "订单" : "成交", (int) res.size(), line.c_str());
            } else {
                std::string SecurityID = res.at(1);
                if (SecurityID.c_str()[0] == '0' || SecurityID.c_str()[0] == '6') cnt++;
                FILE *pf = nullptr;
                if (isOrder) {
                    auto iter = m_watchOrderFILE.find(SecurityID);
                    if (iter != m_watchOrderFILE.end()) pf = iter->second;
                } else {
                    auto iter = m_watchTradeFILE.find(SecurityID);
                    if (iter != m_watchTradeFILE.end()) pf = iter->second;
                }
                if (!pf) continue;

                fputs(line.c_str(), pf);
                fputs("\n", pf);

                auto iter = writeCount.find(SecurityID);
                if (iter == writeCount.end()) writeCount[SecurityID] = 0;
                writeCount[SecurityID]++;
                if (writeCount[SecurityID] % 5000 == 0) {
                    printf("已写入%s%s数据%d条\n", SecurityID.c_str(), isOrder ? "订单" : "成交", writeCount[SecurityID]);
                }
            }
            line.clear();
        }
        return cnt;
    }


}