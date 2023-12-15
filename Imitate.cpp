#include <boost/filesystem.hpp>
#include "Imitate.h"
#include "CApplication.h"

namespace test {

    using namespace TORALEV2API;

    void Imitate::TestOrderBook(CApplication* pApp, std::string& srcDataDir) {
        printf("开始读取文件!!!\n");
        std::string file = srcDataDir;
        std::ifstream ifs(file, std::ios::in);
        if (!ifs.is_open()) {
            printf("xxfile.csv open failed!!! path%s\n", file.c_str());
            return;
        }

        int i = 0;
        std::string line;
        std::vector<std::string> res;
        while (std::getline(ifs, line)) {
            res.clear();
            trim(line);
            Stringsplit(line, ',', res);
            std::string SecurityID = res.at(2);
            if (++i % 1000000 == 0) {
                printf("已经读取行数:%d\n", i);
            }
            if (pApp->m_watchSecurity.find(SecurityID) == pApp->m_watchSecurity.end()) continue;
            std::string ExchangeID = res.at(3);

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
        printf("结束读取文件!!!\n");
        for (auto it : pApp->m_watchSecurity) {
            pApp->ShowOrderBook((char*)it.first.c_str());
        }
        ifs.close();
    }

}