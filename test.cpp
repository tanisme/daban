//
// Created by tangang on 2023/10/21.
//

#include <vector>
#include <unordered_map>
#include <fstream>
#include <boost/filesystem.hpp>
#include "test.h"
#include <iostream>
#include <stdio.h>
#include <thread>
#include <sstream>

namespace test {

    using namespace TORALEV2API;

    struct Order {
        TTORATstpLongSequenceType OrderNo;
        TTORATstpLongVolumeType Volume;
    };

    struct PriceOrders {
        TTORATstpPriceType Price;
        std::vector<Order> Orders;
    };

    typedef std::unordered_map<std::string, std::vector<PriceOrders>> MapOrder;
    MapOrder m_orderBuy;
    MapOrder m_orderSell;
    std::unordered_map<std::string, std::map<TTORATstpLongSequenceType, std::vector<Order>> > m_unFindBuyTrades;
    std::unordered_map<std::string, std::map<TTORATstpLongSequenceType, std::vector<Order>> > m_unFindSellTrades;

    void Stringsplit(const std::string &str, const char split, std::vector<std::string> &res) {
        res.clear();
        if (str == "") return;
        //在字符串末尾也加入分隔符，方便截取最后一段
        std::string strs = str + split;
        size_t pos = strs.find(split);

        // 若找不到内容则字符串搜索函数返回 npos
        while (pos != strs.npos) {
            std::string temp = strs.substr(0, pos);
            res.push_back(temp);
            //去掉已分割的字符串,在剩下的字符串中进行分割
            strs = strs.substr(pos + 1, strs.size());
            pos = strs.find(split);
        }
    }

    void ShowOrderBook(TTORATstpSecurityIDType SecurityID) {
        if (m_orderBuy.empty() && m_orderSell.empty()) return;

        auto t = time(nullptr);
        auto* now = localtime(&t);
        char time[32] = {0};
        sprintf(time, "%04d-%02d-%02d %02d:%02d:%02d", now->tm_year+1900, now->tm_mon+1,
                now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);

        std::stringstream stream;
        stream << "\n";
        stream << "--------- " << SecurityID << " " << time << "---------\n";

        char buffer[4096] = {0};
        {
            int showCount = 5;
            auto iter1 = m_orderSell.find(SecurityID);
            if (iter1 != m_orderSell.end()) {
                auto size = (int) iter1->second.size();
                if (showCount < size) size = showCount;
                for (auto i = size; i > 0; i--) {
                    memset(buffer, 0, sizeof(buffer));
                    long long int totalVolume = 0;
                    for (auto iter2: iter1->second.at(i - 1).Orders) {
                        totalVolume += iter2.Volume;
                    }
                    sprintf(buffer, "S%d\t%.3f\t\t%d\t\t%lld\n", i, iter1->second.at(i - 1).Price,
                            (int) iter1->second.at(i - 1).Orders.size(), totalVolume);
                    stream << buffer;
                }
            }
        }

        {
            int showCount = 5;
            auto iter1 = m_orderBuy.find(SecurityID);
            if (iter1 != m_orderBuy.end()) {
                auto size = (int) iter1->second.size();
                if (showCount < size) size = showCount;
                for (auto i = 1; i <= size; i++) {
                    memset(buffer, 0, sizeof(buffer));
                    long long int totalVolume = 0;
                    for (auto iter2: iter1->second.at(i - 1).Orders) {
                        totalVolume += iter2.Volume;
                    }
                    sprintf(buffer, "B%d\t%.3f\t\t%d\t\t%lld\n", i, iter1->second.at(i - 1).Price,
                            (int) iter1->second.at(i - 1).Orders.size(), totalVolume);
                    stream << buffer;
                }
            }
        }
        printf("%s\n", stream.str().c_str());
    }

    void AddUnFindTrade(TTORATstpSecurityIDType securityID, TTORATstpLongVolumeType tradeVolume, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType side) {
        Order order = {0};
        order.Volume = tradeVolume;
        order.OrderNo = OrderNo;

        std::unordered_map<std::string, std::map<TTORATstpLongSequenceType, std::vector<Order>> >& unFindTrades = side==TORA_TSTP_LSD_Buy?m_unFindBuyTrades:m_unFindSellTrades;
        if (unFindTrades.find(securityID) == unFindTrades.end()) {
            unFindTrades[securityID] = std::map<TTORATstpLongSequenceType, std::vector<Order>>();
        }
        if (unFindTrades[securityID].find(OrderNo) == unFindTrades[securityID].end()) {
            unFindTrades[securityID][OrderNo] = std::vector<Order>();
        }
        unFindTrades[securityID][OrderNo].emplace_back(order);
    }

    void HandleUnFindTrade(TTORATstpSecurityIDType securityID, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType side) {
        std::unordered_map<std::string, std::map<TTORATstpLongSequenceType, std::vector<Order>> >& unFindTrades = side==TORA_TSTP_LSD_Buy?m_unFindBuyTrades:m_unFindSellTrades;
        auto iter = unFindTrades.find(securityID);
        if (iter == unFindTrades.end() || iter->second.empty()) return;
        auto iter1 = iter->second.find(OrderNo);
        if (iter1 == iter->second.end() || iter1->second.empty()) return;

        for (auto iter2 = iter1->second.begin(); iter2 != iter1->second.end();) {
            if (ModifyOrder(securityID, iter2->Volume, OrderNo, side)) {
                iter2 = iter1->second.erase(iter2);
            } else {
                ++iter2;
            }
        }
        if (iter1->second.empty()) {
            iter->second.erase(iter1);
        }
        if (iter->second.empty()) {
            unFindTrades.erase(iter);
        }
    }
// 测试打印 实盘屏蔽
    void GenOrderBook(TTORATstpSecurityIDType securityID) {

        auto t = time(nullptr);
        auto* now = localtime(&t);
        char time[32] = {0};
        sprintf(time, "%04d-%02d-%02d %02d:%02d:%02d", now->tm_year+1900, now->tm_mon+1,
                now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);

        std::stringstream stream;
        stream << "\n";
        stream << "--------- " << securityID << " " << time << "---------\n";

        char buffer[4096] = {0};
        {
            int showCount = 5;
            auto iter1 = m_orderSell.find(securityID);
            if (iter1 != m_orderSell.end()) {
                auto size = (int) iter1->second.size();
                if (showCount < size) size = showCount;
                for (auto i = size; i > 0; i--) {
                    memset(buffer, 0, sizeof(buffer));
                    long long int totalVolume = 0;
                    for (auto iter2: iter1->second.at(i - 1).Orders) {
                        totalVolume += iter2.Volume;
                    }
                    sprintf(buffer, "S%d\t%.3f\t\t%d\t\t%lld\n", i, iter1->second.at(i - 1).Price,
                            (int) iter1->second.at(i - 1).Orders.size(), totalVolume);
                    stream << buffer;
                }
            }
        }

        {
            int showCount = 5;
            auto iter1 = m_orderBuy.find(securityID);
            if (iter1 != m_orderBuy.end()) {
                auto size = (int) iter1->second.size();
                if (showCount < size) size = showCount;
                for (auto i = 1; i <= size; i++) {
                    memset(buffer, 0, sizeof(buffer));
                    long long int totalVolume = 0;
                    for (auto iter2: iter1->second.at(i - 1).Orders) {
                        totalVolume += iter2.Volume;
                    }
                    sprintf(buffer, "B%d\t%.3f\t\t%d\t\t%lld\n", i, iter1->second.at(i - 1).Price,
                            (int) iter1->second.at(i - 1).Orders.size(), totalVolume);
                    stream << buffer;
                }
            }
        }
    }

    void OnRtnOrderDetail(TTORATstpSecurityIDType SecurityID, TTORATstpTradeBSFlagType Side, TTORATstpLongSequenceType OrderNO, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpExchangeIDType ExchangeID, TTORATstpLOrderStatusType OrderStatus) {
        if (Side != TORA_TSTP_LSD_Buy && Side != TORA_TSTP_LSD_Sell) return;

        if (ExchangeID == TORA_TSTP_EXD_SSE) {
            if (OrderStatus == TORA_TSTP_LOS_Add) {
                InsertOrder(SecurityID, OrderNO, Price, Volume, Side);
            } else if (OrderStatus == TORA_TSTP_LOS_Delete) {
                ModifyOrder(SecurityID, 0, OrderNO, Side);
            }
        } else if (ExchangeID == TORA_TSTP_EXD_SZSE) {
            InsertOrder(SecurityID, OrderNO, Price, Volume, Side);
        }
    }

    void OnRtnTransaction(TTORATstpSecurityIDType SecurityID, TTORATstpExchangeIDType ExchangeID, TTORATstpLongVolumeType TradeVolume, TTORATstpExecTypeType ExecType, TTORATstpLongSequenceType BuyNo, TTORATstpLongSequenceType SellNo, TTORATstpPriceType TradePrice) {
        if (ExchangeID == TORA_TSTP_EXD_SSE) {
            if (!ModifyOrder(SecurityID, TradeVolume, BuyNo, TORA_TSTP_LSD_Buy)) {
                //AddUnFindTrade(SecurityID, TradeVolume, BuyNo, TORA_TSTP_LSD_Buy);
            }
            if (!ModifyOrder(SecurityID, TradeVolume, SellNo, TORA_TSTP_LSD_Sell)) {
                //AddUnFindTrade(SecurityID, TradeVolume, SellNo, TORA_TSTP_LSD_Sell);
            }
        } else if (ExchangeID == TORA_TSTP_EXD_SZSE) {
            if (ExecType == TORA_TSTP_ECT_Fill) {
                ModifyOrder(SecurityID, TradeVolume, BuyNo, TORA_TSTP_LSD_Buy);
                ModifyOrder(SecurityID, TradeVolume, SellNo, TORA_TSTP_LSD_Sell);
            } else if (ExecType == TORA_TSTP_ECT_Cancel) {
                if (BuyNo > 0) ModifyOrder(SecurityID, 0, BuyNo, TORA_TSTP_LSD_Buy);
                if (SellNo > 0) ModifyOrder(SecurityID, 0, SellNo, TORA_TSTP_LSD_Sell);
            }
        }
    }

    void OnRtnNGTSTick(TTORATstpSecurityIDType SecurityID, TTORATstpLTickTypeType TickType, TTORATstpLongSequenceType BuyNo, TTORATstpLongSequenceType SellNo, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpLSideType	Side) {
        if (TickType == TORA_TSTP_LTT_Add) {
            if (Side == TORA_TSTP_LSD_Buy) {
                InsertOrder(SecurityID, BuyNo, Price, Volume, Side);
            } else if (Side == TORA_TSTP_LSD_Sell){
                InsertOrder(SecurityID, SellNo, Price, Volume, Side);
            }
        } else if (TickType == TORA_TSTP_LTT_Delete) {
            if (Side == TORA_TSTP_LSD_Buy) {
                ModifyOrder(SecurityID, 0, BuyNo, TORA_TSTP_LSD_Buy);
            } else if (Side == TORA_TSTP_LSD_Sell){
                ModifyOrder(SecurityID, 0, SellNo, TORA_TSTP_LSD_Buy);
            }
        } else if (TickType == TORA_TSTP_LTT_Trade) {
            ModifyOrder(SecurityID, Volume, BuyNo, TORA_TSTP_LSD_Buy);
            ModifyOrder(SecurityID, Volume, SellNo, TORA_TSTP_LSD_Sell);
        }
    }

    void InsertOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType OrderNO, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpLSideType Side) {
        Order order = {0};
        order.OrderNo = OrderNO;
        order.Volume = Volume;

        PriceOrders priceOrder = {0};
        priceOrder.Orders.emplace_back(order);
        priceOrder.Price = Price;

        MapOrder &mapOrder = Side == TORA_TSTP_LSD_Buy ? m_orderBuy : m_orderSell;
        auto iter = mapOrder.find(SecurityID);
        if (iter == mapOrder.end()) {
            mapOrder[SecurityID] = std::vector<PriceOrders>();
            mapOrder[SecurityID].emplace_back(priceOrder);
        } else if (iter->second.empty()) {
            iter->second.emplace_back(priceOrder);
        } else {
            auto size = (int) iter->second.size();
            for (auto i = 0; i < size; i++) {
                auto nextPrice = iter->second.at(i).Price;
                auto lastPrice = iter->second.at(size - 1).Price;
                if (Side == TORA_TSTP_LSD_Buy) {
                    if (Price > nextPrice + 0.000001) {
                        iter->second.insert(iter->second.begin() + i, priceOrder);
                        break;
                    } else if (Price > nextPrice - 0.000001) {
                        iter->second.at(i).Orders.emplace_back(order);
                        break;
                    }
                    if (i == size - 1 && Price < lastPrice - 0.000001) {
                        iter->second.emplace_back(priceOrder);
                    }
                } else if (Side == TORA_TSTP_LSD_Sell) {
                    if (Price < nextPrice - 0.000001) {
                        iter->second.insert(iter->second.begin() + i, priceOrder);
                        break;
                    } else if (Price < nextPrice + 0.000001) {
                        iter->second.at(i).Orders.emplace_back(order);
                        break;
                    }
                    if (i == size - 1 && Price > lastPrice + 0.000001) {
                        iter->second.emplace_back(priceOrder);
                    }
                }
            }
        }
    }

    bool ModifyOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongVolumeType TradeVolume, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType Side) {
        MapOrder &mapOrder = Side == TORA_TSTP_LSD_Buy ? m_orderBuy : m_orderSell;
        auto iter = mapOrder.find(SecurityID);
        if (iter != mapOrder.end()) {
            auto needReset = false, founded = false;
            for (auto i = 0; i < (int) iter->second.size(); i++) {
                for (auto j = 0; j < (int) iter->second.at(i).Orders.size(); j++) {
                    if (iter->second.at(i).Orders.at(j).OrderNo == OrderNo) {
                        if (TradeVolume == 0) {
                            iter->second.at(i).Orders.at(j).Volume = 0;
                            needReset = true;
                        } else {
                            iter->second.at(i).Orders.at(j).Volume -= TradeVolume;
                            if (iter->second.at(i).Orders.at(j).Volume <= 0) {
                                needReset = true;
                            }
                        }
                        founded = true;
                    }
                }
            }
            if (needReset) {
                ResetOrder(SecurityID, Side);
            }
            if (founded) {
                return true;
            }
        }
        return false;
    }

    void ResetOrder(TTORATstpSecurityIDType SecurityID, TTORATstpTradeBSFlagType Side) {
        MapOrder &mapOrder = Side == TORA_TSTP_LSD_Buy ? m_orderBuy : m_orderSell;
        auto iter = mapOrder.find(SecurityID);
        if (iter == mapOrder.end()) return;

        for (auto iterPriceOrder = iter->second.begin(); iterPriceOrder != iter->second.end();) {
            auto bFlag1 = false;
            auto bFlag2 = false;
            for (auto iterOrder = iterPriceOrder->Orders.begin(); iterOrder != iterPriceOrder->Orders.end();) {
                if (iterOrder->Volume <= 0) {
                    iterOrder = iterPriceOrder->Orders.erase(iterOrder);
                    bFlag1 = false;
                } else {
                    ++iterOrder;
                    bFlag1 = true;
                    bFlag2 = true;
                }
            }
            if (!bFlag1 && !bFlag2) {
                iterPriceOrder = iter->second.erase(iterPriceOrder);
            } else {
                ++iterPriceOrder;
            }
        }
    }

    void BigFileOrderQuot(std::string &srcDataDir, TTORATstpSecurityIDType SecurityID) {
        m_orderBuy.clear();m_orderSell.clear();
        std::string file = srcDataDir + "/OrderDetail.csv";
        std::ifstream ifs(file, std::ios::in);
        if (!ifs.is_open()) {
            printf("OrderDetail.csv open failed!!! path%s\n", file.c_str());
            return;
        }

        long long int i = 1;
        std::string line;
        std::vector<std::string> res;
        while (std::getline(ifs, line)) {
            res.clear();
            Stringsplit(line, ',', res);

            if ((int) res.size() != 15) {
                //printf("读订单 有错误数据 列数:%d 内容:%s\n", (int) res.size(), line.c_str());
            } else {
                //交易所代码
                std::string ExchangeID = res.at(0);
                //证券代码
                //std::string SecurityID = res.at(1);
                if (strcmp(SecurityID, res.at(1).c_str())) continue;
                //时间戳
                std::string OrderTime = res.at(2);
                //委托价格
                std::string Price = res.at(3).substr(0, 8);
                //委托数量
                std::string Volume = res.at(4);
                //委托方向
                std::string Side = res.at(5);
                //订单类别（只有深圳行情有效）
                std::string OrderType = res.at(6);
                //主序号
                std::string MainSeq = res.at(7);
                //子序号
                std::string SubSeq = res.at(8);
                //附加信息1
                std::string Info1 = res.at(9);
                //附加信息2
                std::string Info2 = res.at(10);
                //附加信息3
                std::string Info3 = res.at(11);
                //委托序号
                std::string OrderNO = res.at(12);
                //订单状态
                std::string OrderStatus = res.at(13);
                //业务序号（只有上海行情有效）
                std::string BizIndex = res.at(14);

                OnRtnOrderDetail(SecurityID, Side.c_str()[0], atoll(OrderNO.c_str()), atof(Price.c_str()),
                                 atoll(Volume.c_str()),
                                 ExchangeID.c_str()[0],
                                 OrderStatus.c_str()[0]);
                if (i++%1000 == 0) {
                    //printf("已处理大文件中 %s %lld 条订单\n", SecurityID, i);
                }
            }
        }
        //printf("处理完成所有订单 共 %lld 条\n", i);
        //ShowOrderBook(SecurityID);
    }

    void BigFileTradeQuot(std::string &srcDataDir, TTORATstpSecurityIDType SecurityID) {
        std::string file = srcDataDir + "/Transaction.csv";
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

            if ((int) res.size() != 15) {
                //printf("读成交 有错误数据 列数:%d 内容:%s\n", (int) res.size(), line.c_str());
            } else {
                //交易所代码
                std::string ExchangeID = res.at(0);
                //证券代码
                //std::string SecurityID = res.at(1);
                if (strcmp(SecurityID, res.at(1).c_str())) continue;
                //时间戳
                std::string TradeTime = res.at(2);
                //成交价格
                std::string TradePrice = res.at(3).substr(0, 8);
                //成交数量
                std::string TradeVolume = res.at(4);
                //成交类别（只有深圳行情有效）
                std::string ExecType = res.at(5);
                //主序号
                std::string MainSeq = res.at(6);
                //子序号
                std::string SubSeq = res.at(7);
                //买方委托序号
                std::string BuyNo = res.at(8);
                //卖方委托序号
                std::string SellNo = res.at(9);
                //附加信息1
                std::string Info1 = res.at(10);
                //附加信息2
                std::string Info2 = res.at(11);
                //附加信息3
                std::string Info3 = res.at(12);
                //内外盘标志（只有上海行情有效）
                std::string TradeBSFlag = res.at(13);
                //业务序号（只有上海行情有效）
                std::string BizIndex = res.at(14);

                OnRtnTransaction(SecurityID, ExchangeID.c_str()[0], atoll(TradeVolume.c_str()), ExecType.c_str()[0],
                                 atoll(BuyNo.c_str()), atoll(SellNo.c_str()), atof(TradePrice.c_str()));
                if (i++%1000 == 0) {
                    //printf("已处理大文件中 %s %lld 条成交\n", SecurityID, i);
                    //ShowOrderBook(SecurityID);
                }
            }
        }
        printf("=====================================================================\n");
        printf("Big处理完成 %s 所有成交 共 %lld 条\n", SecurityID, i);
        ShowOrderBook(SecurityID);
        printf("=====================================================================\n");
    }

    void SplitSecurityFileOrderQuot(std::string &dstDataDir, TTORATstpSecurityIDType SecurityID) {
        m_orderBuy.clear();m_orderSell.clear();
        std::string file = dstDataDir +"/"+ SecurityID + "_o.txt";
        std::ifstream ifs(file, std::ios::in);
        if (!ifs.is_open()) {
            printf("Order open failed!!! path%s\n", file.c_str());
            return;
        }

        long long int i = 1;
        std::string line;
        std::vector<std::string> res;
        while (std::getline(ifs, line)) {
            res.clear();
            Stringsplit(line, ',', res);

            if ((int) res.size() != 15) {
                printf("读订单 有错误数据 列数:%d 内容:%s\n", (int) res.size(), line.c_str());
            } else {
                //交易所代码
                std::string ExchangeID = res.at(0);
                //证券代码
                //std::string SecurityID = res.at(1);
                if (strcmp(SecurityID, res.at(1).c_str())) continue;
                //时间戳
                std::string OrderTime = res.at(2);
                //委托价格
                std::string Price = res.at(3).substr(0, 8);
                //委托数量
                std::string Volume = res.at(4);
                //委托方向
                std::string Side = res.at(5);
                //订单类别（只有深圳行情有效）
                std::string OrderType = res.at(6);
                //主序号
                std::string MainSeq = res.at(7);
                //子序号
                std::string SubSeq = res.at(8);
                //附加信息1
                std::string Info1 = res.at(9);
                //附加信息2
                std::string Info2 = res.at(10);
                //附加信息3
                std::string Info3 = res.at(11);
                //委托序号
                std::string OrderNO = res.at(12);
                //订单状态
                std::string OrderStatus = res.at(13);
                //业务序号（只有上海行情有效）
                std::string BizIndex = res.at(14);

                OnRtnOrderDetail(SecurityID, Side.c_str()[0], atoll(OrderNO.c_str()), atof(Price.c_str()),
                                 atoll(Volume.c_str()),
                                 ExchangeID.c_str()[0],
                                 OrderStatus.c_str()[0]);
                i++;
                if (i % 1000 == 0) {
                    printf("生成订单簿 已处理 %s %lld 条订单\n", SecurityID, i);
                }
            }
        }
        printf("生成订单簿 处理完成所有订单 共 %lld 条\n", i);
        ShowOrderBook(SecurityID);
    }

    void SplitSecurityFileTradeQuot(std::string &dstDataDir, TTORATstpSecurityIDType SecurityID) {
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

            if ((int) res.size() != 15) {
                printf("读成交 有错误数据 列数:%d 内容:%s\n", (int) res.size(), line.c_str());
            } else {
                //交易所代码
                std::string ExchangeID = res.at(0);
                //证券代码
                //std::string SecurityID = res.at(1);
                if (strcmp(SecurityID, res.at(1).c_str())) continue;
                //时间戳
                std::string TradeTime = res.at(2);
                //成交价格
                std::string TradePrice = res.at(3).substr(0, 8);
                //成交数量
                std::string TradeVolume = res.at(4);
                //成交类别（只有深圳行情有效）
                std::string ExecType = res.at(5);
                //主序号
                std::string MainSeq = res.at(6);
                //子序号
                std::string SubSeq = res.at(7);
                //买方委托序号
                std::string BuyNo = res.at(8);
                //卖方委托序号
                std::string SellNo = res.at(9);
                //附加信息1
                std::string Info1 = res.at(10);
                //附加信息2
                std::string Info2 = res.at(11);
                //附加信息3
                std::string Info3 = res.at(12);
                //内外盘标志（只有上海行情有效）
                std::string TradeBSFlag = res.at(13);
                //业务序号（只有上海行情有效）
                std::string BizIndex = res.at(14);

                OnRtnTransaction(SecurityID, ExchangeID.c_str()[0], atoll(TradeVolume.c_str()), ExecType.c_str()[0],
                                 atoll(BuyNo.c_str()), atoll(SellNo.c_str()), atof(TradePrice.c_str()));
                i++;
                if (i % 1000 == 0) {
                    printf("生成订单簿 已处理 %s %lld 条成交\n", SecurityID, i);
                }
            }
        }
        printf("生成订单簿 处理完成 %s 所有成交 共 %lld 条\n", SecurityID, i);
        ShowOrderBook(SecurityID);
        printf("=====================================================================\n");
    }

    void SplitSecurityFile(std::string srcDataDir, std::string dstDataDir, bool isOrder, TTORATstpSecurityIDType SecurityID) {
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

        boost::filesystem::path path(dstDataDir);
        if (!boost::filesystem::exists(dstDataDir)) {
            boost::filesystem::create_directories(dstDataDir);
        }

        long long int i = 0;
        std::string line;
        std::vector<std::string> res;

        std::string dstDataFileName = dstDataDir + "/" + SecurityID;
        if (isOrder) dstDataFileName = dstDataFileName + "_o.txt";
        else dstDataFileName = dstDataFileName + "_t.txt";
        FILE *pf = fopen(dstDataFileName.c_str(), "w+");
        if (!pf) {
            printf("写入%s文件 打开目标合约文件失败\n", isOrder ? "订单" : "成交");
            return;
        }

        while (std::getline(ifs, line)) {
            res.clear();
            Stringsplit(line, ',', res);

            if ((int) res.size() != 15) {
                printf("读取%s文件 有错误数据 列数:%d 内容:%s\n", isOrder ? "订单" : "成交", (int) res.size(), line.c_str());
            } else {
                if (strcmp(SecurityID, res.at(1).c_str())) continue;
                fputs(line.c_str(), pf);
                fputs("\n", pf);
            }
            line.clear();
            i++;
            if (i % 1000 == 0) {
                printf("已拆分 %s %lld 条 %s\n", SecurityID, i, isOrder ? "订单" : "成交");
            }
        }

        fclose(pf);
        printf("累计拆分写入 %s %s 文件 条数 %lld\n", SecurityID, isOrder ? "订单" : "成交", i);
    }

    void trim(std::string &s)
    {
        int index = 0;
        if(!s.empty())
        {
            while( (index = s.find(' ',index)) != std::string::npos)
            {
                s.erase(index,1);
            }
        }
    }
    void ParseSecurityFile(TTORATstpSecurityIDType CalSecurityID) {
        std::string srcDataFile = "I:/tanisme/workspace/gitcode/data/1.csv";
        std::ifstream ifs(srcDataFile, std::ios::in);
        if (!ifs.is_open()) {
            printf("srcDataFile csv open failed!!! path%s\n", srcDataFile.c_str());
            return;
        }

        std::string line;
        std::vector<std::string> res;

        long long int i = 1;
        while (std::getline(ifs, line)) {
            res.clear();
            trim(line);
            Stringsplit(line, ',', res);

            //printf("处理第 %lld 行数据\n", i);
            if ((int) res.size() != 16) {
                printf("有错误数据 列数:%d 内容:%s\n", (int) res.size(), line.c_str());
            } else {

                //证券代码
                std::string SecurityID = res.at(2);
                if (strcmp(SecurityID.c_str(), CalSecurityID) != 0) {
                    continue;
                }
                std::string Time = res.at(1);
                //交易所代码
                std::string ExchangeID = res.at(3);

                std::string OoT = res.at(0);
                if (OoT == "O") {
                    //委托价格
                    std::string Price = res.at(4);
                    //委托数量
                    std::string Volume = res.at(5);
                    //委托方向
                    std::string Side = res.at(6);
                    //订单类别（只有深圳行情有效）
                    std::string OrderType = res.at(7);
                    //委托序号
                    std::string OrderNO = res.at(13);
                    //订单状态
                    std::string OrderStatus = res.at(14);

                    OnRtnOrderDetail(CalSecurityID, Side.c_str()[0], atoll(OrderNO.c_str()), atof(Price.c_str()),
                                     atoll(Volume.c_str()),
                                     ExchangeID.c_str()[0],
                                     OrderStatus.c_str()[0]);
                } else if (OoT == "T") {
                    //成交价格
                    std::string TradePrice = res.at(4);
                    //成交数量
                    std::string TradeVolume = res.at(5);
                    //成交类别（只有深圳行情有效）
                    std::string ExecType = res.at(6);
                    //买方委托序号
                    std::string BuyNo = res.at(9);
                    //卖方委托序号
                    std::string SellNo = res.at(10);
                    OnRtnTransaction(CalSecurityID, ExchangeID.c_str()[0], atoll(TradeVolume.c_str()), ExecType.c_str()[0],
                                     atoll(BuyNo.c_str()), atoll(SellNo.c_str()), atof(TradePrice.c_str()));
                }
                line.clear();
                //ShowFixOrderBook(CalSecurityID);
                ++i;
                if (i%5000 == 0) printf("已经处理 %lld 条数据\n", i);
            }
        }
    }

    bool TestOrderBook(std::string& srcDataDir, std::string& watchsecurity) {
        //std::string srcDataDir = "I:/tanisme/workspace/gitcode/data";
        //std::string dstDataDir = "I:/tanisme/workspace/gitcode/result";
        //std::string srcDataDir = "D:/BakFiles/15386406/FileRecv/20231018_A";
        std::string dstDataDir = srcDataDir + "/result";

        std::vector<std::string> Securityes;
        Stringsplit(watchsecurity, ',', Securityes);

        for (auto iter : Securityes)
        {
            char Security[31] = {0};
            strcpy(Security, iter.c_str());
            SplitSecurityFile(srcDataDir, dstDataDir, true, Security);
            SplitSecurityFile(srcDataDir, dstDataDir, false, Security);
            SplitSecurityFileOrderQuot(dstDataDir, Security);
            SplitSecurityFileTradeQuot(dstDataDir, Security);
            //BigFileOrderQuot(srcDataDir, Security);
            //BigFileTradeQuot(srcDataDir, Security);
        }

        //{
        //    char Security[] = "002151";
        //    BigFileOrderQuot(srcDataDir, Security);
        //    BigFileTradeQuot(srcDataDir, Security);
        //}

        //{
        //    char Security[] = "002151";
        //    ParseSecurityFile(Security);
        //    ShowOrderBook(Security);
        //}
        return true;
    }


}