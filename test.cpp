#include <boost/filesystem.hpp>
#include "test.h"

namespace test {

    using namespace TORALEV2API;

    void Imitate::SplitSecurityFileOrderQuot(std::string &dstDataDir, TTORATstpSecurityIDType SecurityID) {
        m_md->Clear();

        std::string file = dstDataDir + SecurityID + "_o.txt";
        std::ifstream ifs(file, std::ios::in);
        if (!ifs.is_open()) {
            printf("stOrder open failed!!! path%s\n", file.c_str());
            return;
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
            //std::string OrderType = res.at(6);                              //订单类别（只有深圳行情有效）
            //std::string MainSeq = res.at(7);                                //主序号
            //std::string SubSeq = res.at(8);                                 //子序号
            //std::string Info1 = res.at(9);                                     //附加信息1
            //std::string Info2 = res.at(10);                                    //附加信息2
            //std::string Info3 = res.at(11);                                    //附加信息3
            std::string OrderNO = res.at(12);                               //委托序号
            std::string OrderStatus = res.at(13);                           //订单状态
            //std::string BizIndex = res.at(14);                                 //业务序号（只有上海行情有效）

            //if (ExchangeID.c_str()[0] == PROMD::TORA_TSTP_EXD_SZSE || (ExchangeID.c_str()[0] == PROMD::TORA_TSTP_EXD_SSE && !m_shnewversion)) {
                m_md->OrderDetail(SecurityID, Side.c_str()[0], atoll(OrderNO.c_str()),
                                  atof(Price.c_str()), atoll(Volume.c_str()), ExchangeID.c_str()[0],
                                  OrderStatus.c_str()[0]);
            //} else {
                //m_md->NGTSTick(SecurityID, )
            //}
            i++;
        }
        ifs.close();
        printf("生成%s订单簿 处理完成所有订单 共%d条\n", SecurityID, i);
    }

    void Imitate::SplitSecurityFileTradeQuot(std::string &dstDataDir, TTORATstpSecurityIDType SecurityID) {
        std::string file = dstDataDir +"/"+ SecurityID + "_t.txt";
        std::ifstream ifs(file, std::ios::in);
        if (!ifs.is_open()) {
            printf("Transaction.csv open failed!!! path%s\n", file.c_str());
            return;
        }

        long long int i = 0;
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
            //std::string MainSeq = res.at(6);                                    //主序号
            //std::string SubSeq = res.at(7);                                     //子序号
            std::string BuyNo = res.at(8);                                      //买方委托序号
            std::string SellNo = res.at(9);                                     //卖方委托序号
            //std::string Info1 = res.at(10);                                     //附加信息1
            //std::string Info2 = res.at(11);                                     //附加信息2
            //std::string Info3 = res.at(12);                                     //附加信息3
            //std::string TradeBSFlag = res.at(13);                               //内外盘标志（只有上海行情有效）
            //std::string BizIndex = res.at(14);                                  //业务序号（只有上海行情有效）
            m_md->Transaction(SecurityID, ExchangeID.c_str()[0], atoll(TradeVolume.c_str()),
                              ExecType.c_str()[0], atoll(BuyNo.c_str()), atoll(SellNo.c_str()),
                              atof(TradePrice.c_str()), atoi(TradeTime.c_str()));
            i++;
        }
        ifs.close();
        printf("生成%s订单簿 处理完成所有成交 共%lld条\n", SecurityID, i);
        m_md->ShowOrderBook(SecurityID);
        m_md->ShowUnfindOrder(SecurityID);
    }

    void Imitate::SplitSecurityFile(std::string srcDataDir, bool isOrder) {
        if (isOrder) {
            srcDataDir += "/OrderDetail.csv";
        } else {
            srcDataDir += "/Transaction.csv";
        }

        std::ifstream ifs(srcDataDir, std::ios::in);
        if (!ifs.is_open()) {
            printf("srcDataFile csv open failed!!! path%s\n", srcDataDir.c_str());
            return;
        }

        std::string line;
        std::vector<std::string> res;
        std::unordered_map<std::string, int> writeCount;

        while (std::getline(ifs, line)) {
            res.clear();
            Stringsplit(line, ',', res);

            if ((int) res.size() != 15) {
                printf("读取%s文件 有错误数据 列数:%d 内容:%s\n", isOrder ? "订单" : "成交", (int) res.size(), line.c_str());
            } else {
                std::string SecurityID = res.at(1);
                FILE *pf = nullptr;
                if (isOrder) {
                    auto iter = m_watchOrderFILE.find(SecurityID);
                    if (iter != m_watchOrderFILE.end()) pf = iter->second;
                } else {
                    auto iter = m_watchTradeFILE.find(SecurityID);
                    if (iter != m_watchTradeFILE.end()) pf = iter->second;
                }
                if (!pf) {
                    continue;
                }

                fputs(line.c_str(), pf);
                fputs("\n", pf);

                auto iter = writeCount.find(SecurityID);
                if (iter == writeCount.end()) {
                    writeCount[SecurityID] = 0;
                }
                writeCount[SecurityID]++;
                if (writeCount[SecurityID] % 5000 == 0) {
                    printf("已写入%s%s数据%d条\n", SecurityID.c_str(), isOrder ? "订单" : "成交", writeCount[SecurityID]);
                }
            }
            line.clear();
        }
    }

    void Imitate::LDParseSecurityFile(TTORATstpSecurityIDType CalSecurityID) {
        //std::string srcDataFile = "I:/tanisme/workspace/gitcode/data/1.csv";
        //std::ifstream ifs(srcDataFile, std::ios::in);
        //if (!ifs.is_open()) {
        //    printf("srcDataFile csv open failed!!! path%s\n", srcDataFile.c_str());
        //    return;
        //}

        //std::string line;
        //std::vector<std::string> res;

        //long long int i = 1;
        //while (std::getline(ifs, line)) {
        //    res.clear();
        //    trim(line);
        //    Stringsplit(line, ',', res);

        //    //printf("处理第 %lld 行数据\n", i);
        //    if ((int) res.size() != 16) {
        //        //printf("有错误数据 列数:%d 内容:%s\n", (int) res.size(), line.c_str());
        //    } else {

        //        //证券代码
        //        std::string SecurityID = res.at(2);
        //        if (strcmp(SecurityID.c_str(), CalSecurityID) != 0) {
        //            continue;
        //        }
        //        std::string Time = res.at(1);
        //        //交易所代码
        //        std::string ExchangeID = res.at(3);

        //        std::string OoT = res.at(0);
        //        if (OoT == "O") {
        //            //委托价格
        //            std::string Price = res.at(4);
        //            //委托数量
        //            std::string Volume = res.at(5);
        //            //委托方向
        //            std::string Side = res.at(6);
        //            //订单类别（只有深圳行情有效）
        //            std::string OrderType = res.at(7);
        //            //委托序号
        //            std::string OrderNO = res.at(13);
        //            //订单状态
        //            std::string OrderStatus = res.at(14);

        //            OnRtnOrderDetail(CalSecurityID, Side.c_str()[0], atoll(OrderNO.c_str()), atof(Price.c_str()),
        //                             atoll(Volume.c_str()),
        //                             ExchangeID.c_str()[0],
        //                             OrderStatus.c_str()[0]);
        //        } else if (OoT == "T") {
        //            //成交价格
        //            std::string TradePrice = res.at(4);
        //            //成交数量
        //            std::string TradeVolume = res.at(5);
        //            //成交类别（只有深圳行情有效）
        //            std::string ExecType = res.at(6);
        //            //买方委托序号
        //            std::string BuyNo = res.at(9);
        //            //卖方委托序号
        //            std::string SellNo = res.at(10);
        //            OnRtnTransaction(CalSecurityID, ExchangeID.c_str()[0], atoll(TradeVolume.c_str()), ExecType.c_str()[0],
        //                             atoll(BuyNo.c_str()), atoll(SellNo.c_str()), atof(TradePrice.c_str()));
        //        }
        //        line.clear();
        //        //ShowFixOrderBook(CalSecurityID);
        //        ++i;
        //        if (i%5000 == 0) printf("已经处理 %lld 条数据\n", i);
        //    }
        //}
    }

    bool Imitate::TestOrderBook(std::string& srcDataDir, std::string& watchsecurity, bool shnewversion, bool createfile) {
        m_md = new PROMD::MDL2Impl(nullptr, PROMD::TORA_TSTP_EXD_COMM);
        m_shnewversion = shnewversion;

        trim(watchsecurity);
        if (watchsecurity.length() <= 0) {
            printf("配置文件中watchsecurity未配置\n");
            return false;
        }

        std::vector<std::string> tempSecurity;
        Stringsplit(watchsecurity, ',', tempSecurity);

        std::string dstDataDir = srcDataDir + "/result/";
        boost::filesystem::path path(dstDataDir);
        if (!boost::filesystem::exists(dstDataDir)) {
            boost::filesystem::create_directories(dstDataDir);
        }

        {
            auto bt = time(nullptr);
            printf("-------------------------------------------------------------------\n");
            printf("开始生成订单簿 %s\n", GetTimeStr().c_str());

            if (createfile) {
                for (auto iter : tempSecurity) {
                    std::string orderFileName = dstDataDir + iter + "_o.txt";
                    std::string tradeFileName = dstDataDir + iter + "_t.txt";
                    FILE *opf = fopen(orderFileName.c_str(), "w+");
                    if (!opf) {
                        printf("打开订单%s文件失败\n", orderFileName.c_str());
                        return false;
                    }
                    FILE *tpf = fopen(tradeFileName.c_str(), "w+");
                    if (!tpf) {
                        printf("打开成交%s文件失败\n", tradeFileName.c_str());
                        fclose(opf);
                        return false;
                    }
                    m_watchOrderFILE[iter] = opf;
                    m_watchTradeFILE[iter] = tpf;
                }
                SplitSecurityFile(srcDataDir, true);
                SplitSecurityFile(srcDataDir, false);
                for (auto iter = m_watchOrderFILE.begin(); iter != m_watchOrderFILE.end(); ++iter) if (iter->second) fclose(iter->second);
                for (auto iter = m_watchTradeFILE.begin(); iter != m_watchTradeFILE.end(); ++iter) if (iter->second) fclose(iter->second);
            }

            for (auto iter : tempSecurity) {
                SplitSecurityFileOrderQuot(dstDataDir, (char*)iter.c_str());
                SplitSecurityFileTradeQuot(dstDataDir, (char*)iter.c_str());
            }
            auto et = time(nullptr);
            printf("生成订单簿完成 耗时 %d分%d秒\n", (int)((et-bt)/60), (int)((et-bt)%60));
            printf("-------------------------------------------------------------------\n");
        }

        //{
        //    char Security[] = "002151";
        //    ParseSecurityFile(Security);
        //    ShowOrderBook(Security);
        //}
        if (m_md) {
            m_md->GetApi()->Join();
            m_md->GetApi()->Release();
            delete m_md;
        }
        return true;
    }

}