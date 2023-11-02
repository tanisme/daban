#include "MDL2Impl.h"
#include "CApplication.h"
#include <boost/bind.hpp>

#include <sstream>

namespace PROMD {

    MDL2Impl::MDL2Impl(CApplication *pApp, TTORATstpExchangeIDType exchangeID)
            : m_pApp(pApp), m_exchangeID(exchangeID) {
    }

    MDL2Impl::~MDL2Impl() {
        if (m_pApi != nullptr) {
            m_pApi->Join();
            m_pApi->Release();
        }
    }

    bool MDL2Impl::Start(bool useTcp) {
        if (useTcp) {
            m_pApi = CTORATstpLev2MdApi::CreateTstpLev2MdApi();
            if (!m_pApi) return false;
            m_pApi->RegisterSpi(this);
            if (m_exchangeID == TORA_TSTP_EXD_SSE) {
                m_pApi->RegisterFront((char *) m_pApp->m_shMDAddr.c_str());
            } else if (m_exchangeID == TORA_TSTP_EXD_SZSE) {
                m_pApi->RegisterFront((char *) m_pApp->m_szMDAddr.c_str());
            } else {
                return false;
            }
        }
        else {
            m_pApi = CTORATstpLev2MdApi::CreateTstpLev2MdApi(TORA_TSTP_MST_MCAST);
            m_pApi->RegisterSpi(this);
            m_pApi->RegisterMulticast((char *) m_pApp->m_mdAddr.c_str(), (char*)m_pApp->m_mdInterface.c_str(), nullptr);
        }
        m_pApi->Init(m_pApp->m_cpucore.c_str());
        return true;
    }

    void MDL2Impl::OnFrontConnected() {
        printf("%s MD::OnFrontConnected!!!\n", GetExchangeName(m_exchangeID));
        CTORATstpReqUserLoginField req = {0};
        m_pApi->ReqUserLogin(&req, ++m_reqID);
    }

    void MDL2Impl::OnFrontDisconnected(int nReason) {
        printf("%s MD::OnFrontDisconnected!!! Reason:%d\n", GetExchangeName(m_exchangeID), nReason);
        m_isLogined = false;
    }

    void MDL2Impl::OnRspUserLogin(CTORATstpRspUserLoginField *pRspUserLogin, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (!pRspUserLogin || !pRspInfo) return;
        if (pRspInfo->ErrorID > 0) {
            printf("%s MD::OnRspUserLogin Failed!!! ErrMsg:%s\n", GetExchangeName(m_exchangeID), pRspInfo->ErrorMsg);
            return;
        }
        m_isLogined = true;
        m_pApp->m_ioc.post(boost::bind(&CApplication::MDOnRspUserLogin, m_pApp, m_exchangeID));
    }

    int MDL2Impl::ReqMarketData(TTORATstpSecurityIDType securityID, TTORATstpExchangeIDType exchangeID, int type, bool isSub) {
        char *security_arr[1];
        security_arr[0] = securityID;

        switch (type) {
            case 1:
                if (isSub) return m_pApi->SubscribeMarketData(security_arr, 1, exchangeID);
                return m_pApi->UnSubscribeMarketData(security_arr, 1, exchangeID);
                break;
            case 2:
                if (isSub) return m_pApi->SubscribeTransaction(security_arr, 1, exchangeID);
                return m_pApi->UnSubscribeTransaction(security_arr, 1, exchangeID);
                break;
            case 3:
                if (isSub) return m_pApi->SubscribeOrderDetail(security_arr, 1, exchangeID);
                return m_pApi->UnSubscribeOrderDetail(security_arr, 1, exchangeID);
                break;
            case 4:
                if (isSub) return m_pApi->SubscribeNGTSTick(security_arr, 1, exchangeID);
                return m_pApi->UnSubscribeNGTSTick(security_arr, 1, exchangeID);
                break;
            default:
                break;
        }
        return -1;
    }

    void MDL2Impl::OnRspSubMarketData(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (!pSpecificSecurity || !pRspInfo) return;
        if (pRspInfo->ErrorID > 0) {
            printf("%s MD::OnRspSubMarketData Failed!!! ErrMsg:%s\n", GetExchangeName(m_exchangeID), pRspInfo->ErrorMsg);
            return;
        }
    }

    void MDL2Impl::OnRspUnSubMarketData(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (!pSpecificSecurity || !pRspInfo) return;
        if (pRspInfo->ErrorID > 0) {
            printf("%s MD::OnRspUnSubMarketData Failed!!! ErrMsg:%s\n", GetExchangeName(m_exchangeID), pRspInfo->ErrorMsg);
            return;
        }
    }

    void MDL2Impl::OnRtnMarketData(CTORATstpLev2MarketDataField *pDepthMarketData, const int FirstLevelBuyNum, const int FirstLevelBuyOrderVolumes[], const int FirstLevelSellNum, const int FirstLevelSellOrderVolumes[]) {
        if (!pDepthMarketData) return;
        printf("%s MD::OnRtnMarketData %s [%.3f %.3f %.3f %.3f %.3f] [%.3f %.3f %.3f %c]\n",
               PROMD::MDL2Impl::GetExchangeName(pDepthMarketData->ExchangeID),
               pDepthMarketData->SecurityID, pDepthMarketData->LastPrice, pDepthMarketData->OpenPrice,
               pDepthMarketData->HighestPrice, pDepthMarketData->LowestPrice, pDepthMarketData->ClosePrice,
               pDepthMarketData->UpperLimitPrice, pDepthMarketData->LowerLimitPrice, pDepthMarketData->PreClosePrice, pDepthMarketData->MDSecurityStat);
    }

    void MDL2Impl::OnRspSubTransaction(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (!pSpecificSecurity || !pRspInfo) return;
        if (pRspInfo->ErrorID > 0) {
            printf("%s MD::OnRspSubTransaction Failed!!! ErrMsg:%s\n", GetExchangeName(m_exchangeID), pRspInfo->ErrorMsg);
            return;
        }
    }

    void MDL2Impl::OnRspUnSubTransaction(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (!pSpecificSecurity || !pRspInfo) return;
        if (pRspInfo->ErrorID > 0) {
            printf("%s MD::OnRspUnSubTransaction Failed!!! ErrMsg:%s\n", GetExchangeName(m_exchangeID), pRspInfo->ErrorMsg);
            return;
        }
    }

    void MDL2Impl::OnRtnTransaction(CTORATstpLev2TransactionField *pTransaction) {
        if (!pTransaction) return;
        if (pTransaction->ExchangeID == TORA_TSTP_EXD_SSE) {
            if (!ModifyOrder(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->BuyNo, TORA_TSTP_LSD_Buy))
                AddUnFindTrade(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->BuyNo, TORA_TSTP_LSD_Buy);
            if (!ModifyOrder(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->SellNo, TORA_TSTP_LSD_Sell))
                AddUnFindTrade(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->SellNo, TORA_TSTP_LSD_Sell);
        } else if (pTransaction->ExchangeID == TORA_TSTP_EXD_SZSE) {
            if (pTransaction->ExecType == TORA_TSTP_ECT_Fill) {
                if (!ModifyOrder(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->BuyNo, TORA_TSTP_LSD_Buy)) {
                    //AddUnFindTrade(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->BuyNo, TORA_TSTP_LSD_Buy);
                }
                if (!ModifyOrder(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->SellNo, TORA_TSTP_LSD_Sell)) {
                    //AddUnFindTrade(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->SellNo, TORA_TSTP_LSD_Sell);
                }
            } else if (pTransaction->ExecType == TORA_TSTP_ECT_Cancel) {
                if (pTransaction->BuyNo > 0) DeleteOrder(pTransaction->SecurityID, pTransaction->BuyNo);
                if (pTransaction->SellNo > 0) DeleteOrder(pTransaction->SecurityID, pTransaction->SellNo);
            }
        }
        //PostPrice(pTransaction->SecurityID, pTransaction->TradePrice);
        //GenOrderBook(pTransaction->SecurityID);
    }

    void MDL2Impl::OnRspSubOrderDetail(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (!pSpecificSecurity || !pRspInfo) return;
        if (pRspInfo->ErrorID > 0) {
            printf("%s MD::OnRspSubOrderDetail Failed!!! ErrMsg:%s\n", GetExchangeName(m_exchangeID), pRspInfo->ErrorMsg);
            return;
        }
    }

    void MDL2Impl::OnRspUnSubOrderDetail(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (!pSpecificSecurity || !pRspInfo) return;
        if (pRspInfo->ErrorID > 0) {
            printf("%s MD::OnRspUnSubOrderDetail Failed!!! ErrMsg:%s\n", GetExchangeName(m_exchangeID), pRspInfo->ErrorMsg);
            return;
        }
    }

    void MDL2Impl::OnRtnOrderDetail(CTORATstpLev2OrderDetailField *pOrderDetail) {
        if (!pOrderDetail || (pOrderDetail->Side != TORA_TSTP_LSD_Buy && pOrderDetail->Side != TORA_TSTP_LSD_Sell)) return;
        if (pOrderDetail->ExchangeID == TORA_TSTP_EXD_SSE) {
            if (pOrderDetail->OrderStatus == TORA_TSTP_LOS_Add) {
                InsertOrder(pOrderDetail->SecurityID, pOrderDetail->OrderNO, pOrderDetail->Price, pOrderDetail->Volume, pOrderDetail->Side);
                HandleUnFindTrade(pOrderDetail->SecurityID, pOrderDetail->OrderNO, pOrderDetail->Side);
            } else if (pOrderDetail->OrderStatus == TORA_TSTP_LOS_Delete) {
                DeleteOrder(pOrderDetail->SecurityID, pOrderDetail->OrderNO);
            }
        } else if (pOrderDetail->ExchangeID == TORA_TSTP_EXD_SZSE) {
            InsertOrder(pOrderDetail->SecurityID, pOrderDetail->OrderNO, pOrderDetail->Price, pOrderDetail->Volume, pOrderDetail->Side);
            //HandleUnFindTrade(pOrderDetail->SecurityID, pOrderDetail->OrderNO, pOrderDetail->Side);
        }
        //PostPrice(pOrderDetail->SecurityID, 0);
        //GenOrderBook(pOrderDetail->SecurityID);
    }

    void MDL2Impl::OnRspSubNGTSTick(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (!pSpecificSecurity || !pRspInfo) return;
        if (pRspInfo->ErrorID > 0) {
            printf("%s MD::OnRspSubNGTSTick Failed!!! ErrMsg:%s\n", GetExchangeName(m_exchangeID), pRspInfo->ErrorMsg);
            return;
        }
    }

    void MDL2Impl::OnRspUnSubNGTSTick(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (!pSpecificSecurity || !pRspInfo) return;
        if (pRspInfo->ErrorID > 0) {
            printf("%s MD::OnRspUnSubNGTSTick Failed!!! ErrMsg:%s\n", GetExchangeName(m_exchangeID), pRspInfo->ErrorMsg);
            return;
        }
    }

    void MDL2Impl::OnRtnNGTSTick(CTORATstpLev2NGTSTickField *pTick) {
        if (!pTick) return;
        if (pTick->TickType == TORA_TSTP_LTT_Add) {
            if (pTick->Side == TORA_TSTP_LSD_Buy) {
                InsertOrder(pTick->SecurityID, pTick->BuyNo, pTick->Price, pTick->Volume, pTick->Side);
            } else if (pTick->Side == TORA_TSTP_LSD_Sell){
                InsertOrder(pTick->SecurityID, pTick->SellNo, pTick->Price, pTick->Volume, pTick->Side);
            }
        } else if (pTick->TickType == TORA_TSTP_LTT_Delete) {
            if (pTick->Side == TORA_TSTP_LSD_Buy) {
                ModifyOrder(pTick->SecurityID, 0, pTick->BuyNo, TORA_TSTP_LSD_Buy);
            } else if (pTick->Side == TORA_TSTP_LSD_Sell){
                ModifyOrder(pTick->SecurityID, 0, pTick->SellNo, TORA_TSTP_LSD_Buy);
            }
        } else if (pTick->TickType == TORA_TSTP_LTT_Trade) {
            ModifyOrder(pTick->SecurityID, pTick->Volume, pTick->BuyNo, TORA_TSTP_LSD_Buy);
            ModifyOrder(pTick->SecurityID, pTick->Volume, pTick->SellNo, TORA_TSTP_LSD_Sell);
        }
        //PostPrice(pTick->SecurityID, pTick->Price);
    }

    void MDL2Impl::InsertOrder(TTORATstpSecurityIDType securityID, TTORATstpLongSequenceType OrderNO, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpLSideType Side) {
        Order order = {0};
        order.OrderNo = OrderNO;
        order.Volume = Volume;

        PriceOrders priceOrder = {0};
        priceOrder.Orders.emplace_back(order);
        priceOrder.Price = Price;

        MapOrder &mapOrder = Side == TORA_TSTP_LSD_Buy ? m_orderBuy : m_orderSell;
        auto iter = mapOrder.find(securityID);
        if (iter == mapOrder.end()) {
            mapOrder[securityID] = std::vector<PriceOrders>();
            mapOrder[securityID].emplace_back(priceOrder);
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

    void MDL2Impl::DeleteOrder(TTORATstpSecurityIDType securityID, TTORATstpLongSequenceType OrderNO) {
        ModifyOrder(securityID, 0, OrderNO, TORA_TSTP_LSD_Buy);
        ModifyOrder(securityID, 0, OrderNO, TORA_TSTP_LSD_Sell);
    }

    bool MDL2Impl::ModifyOrder(TTORATstpSecurityIDType securityID, TTORATstpLongVolumeType TradeVolume, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType Side) {
        MapOrder &mapOrder = Side == TORA_TSTP_LSD_Buy ? m_orderBuy : m_orderSell;
        auto iter = mapOrder.find(securityID);
        if (iter != mapOrder.end()) {
            auto needReset = false;
            for (auto i = 0; i < (int) iter->second.size(); i++) {
                for (auto j = 0; j < (int) iter->second.at(i).Orders.size(); j++) {
                    if (iter->second.at(i).Orders.at(j).OrderNo == OrderNo) {
                        if (TradeVolume == 0) {
                            iter->second.at(i).Orders.at(j).Volume = 0;
                        } else {
                            iter->second.at(i).Orders.at(j).Volume -= TradeVolume;
                        }
                        needReset = true;
                    }
                }
            }
            if (needReset) {
                ResetOrder(securityID, Side);
                return true;
            }
        }
        return false;
    }

    void MDL2Impl::ResetOrder(TTORATstpSecurityIDType securityID, TTORATstpTradeBSFlagType Side) {
        MapOrder &mapOrder = Side == TORA_TSTP_LSD_Buy ? m_orderBuy : m_orderSell;
        auto iter = mapOrder.find(securityID);
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

    void MDL2Impl::AddUnFindTrade(TTORATstpSecurityIDType securityID, TTORATstpLongVolumeType tradeVolume, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType side) {
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

    void MDL2Impl::HandleUnFindTrade(TTORATstpSecurityIDType securityID, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType side) {
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
    void MDL2Impl::ShowFixOrderBook(TTORATstpSecurityIDType securityID) {
        if (m_orderBookStr.find(securityID) != m_orderBookStr.end())
            printf(m_orderBookStr[securityID].c_str());
    }

    // 测试打印 实盘屏蔽
    void MDL2Impl::GenOrderBook(TTORATstpSecurityIDType securityID) {
        if (m_orderBuy.empty() && m_orderSell.empty()) return;

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
        printf("%s\n", stream.str().c_str());
        //m_orderBookStr[securityID] = std::move(stream.str());
    }

    void MDL2Impl::PostPrice(TTORATstpSecurityIDType securityID, TTORATstpPriceType tradePrice) {
        TTORATstpPriceType BidPrice1 = 0.0;
        TTORATstpLongVolumeType BidVolume1 = 0;
        TTORATstpPriceType AskPrice1 = 0.0;
        TTORATstpLongVolumeType AskVolume1 = 0;
        TTORATstpPriceType TradePrice = tradePrice;

        {
            auto iter = m_orderSell.find(securityID);
            if (iter != m_orderSell.end() && !iter->second.empty()) {
                auto size = (int) iter->second.begin()->Orders.size();
                long long int totalVolume = 0;
                for (auto i = 0; i < size; ++i) {
                    totalVolume += iter->second.begin()->Orders.at(i).Volume;
                }
                AskPrice1 = iter->second.begin()->Price;
                AskVolume1 = totalVolume;
            }
        }

        {
            auto iter = m_orderBuy.find(securityID);
            if (iter != m_orderBuy.end() && !iter->second.empty()) {
                auto size = (int) iter->second.begin()->Orders.size();
                long long int totalVolume = 0;
                for (auto i = 0; i < size; ++i) {
                    totalVolume += iter->second.begin()->Orders.at(i).Volume;
                }
                BidPrice1 = iter->second.begin()->Price;
                BidVolume1 = totalVolume;
            }
        }

        auto iter = m_postMDL2.find(securityID);
        if (iter == m_postMDL2.end()) {
            stPostPrice p = {0};
            strcpy(p.SecurityID, securityID);
            p.BidVolume1 = BidVolume1;
            p.BidPrice1 = BidPrice1;
            p.AskPrice1 = AskPrice1;
            p.AskVolume1 = AskVolume1;
            p.TradePrice = TradePrice;
            m_postMDL2[securityID] = p;
        } else {
            iter->second.BidVolume1 = BidVolume1;
            iter->second.BidPrice1 = BidPrice1;
            iter->second.AskPrice1 = AskPrice1;
            iter->second.TradePrice = TradePrice;
            iter->second.AskVolume1 = AskVolume1;
        }

        m_pApp->m_ioc.post(boost::bind(&CApplication::MDPostPrice, m_pApp, m_postMDL2[securityID]));
        //printf("MDPostPrice %s %lld %.3f %.3f %lld\n", SecurityID, AskVolume1, AskPrice1, BidPrice1, BidVolume1);
    }

    const char *MDL2Impl::GetExchangeName(TTORATstpExchangeIDType exchangeID) {
        switch (exchangeID) {
            case TORA_TSTP_EXD_SSE:
                return "SH";
            case TORA_TSTP_EXD_SZSE:
                return "SZ";
            case TORA_TSTP_EXD_HK:
                return "HK";
            case TORA_TSTP_EXD_COMM:
                return "COM";
            default:
                break;
        }
        return "UNKNOW";
    }

}