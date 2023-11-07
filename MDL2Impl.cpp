#include "MDL2Impl.h"
#include "CApplication.h"

namespace PROMD {

    MDL2Impl::MDL2Impl(CApplication *pApp, TTORATstpExchangeIDType ExchangeID)
            : m_pApp(pApp), m_exchangeID(ExchangeID) {
    }

    MDL2Impl::~MDL2Impl() {
        if (m_pApi != nullptr) {
            m_pApi->Join();
            m_pApi->Release();
        }
    }

    bool MDL2Impl::Start(bool isTest) {
        if (isTest) { // tcp
            m_pApi = CTORATstpLev2MdApi::CreateTstpLev2MdApi();
            m_pApi->RegisterSpi(this);
            if (m_exchangeID == TORA_TSTP_EXD_SSE) {
                m_pApi->RegisterFront((char *) m_pApp->m_shMDAddr.c_str());
            } else if (m_exchangeID == TORA_TSTP_EXD_SZSE) {
                m_pApi->RegisterFront((char *) m_pApp->m_szMDAddr.c_str());
            } else {
                return false;
            }
        }
        else { // udp
            m_pApi = CTORATstpLev2MdApi::CreateTstpLev2MdApi(TORA_TSTP_MST_MCAST);
            m_pApi->RegisterSpi(this);
            m_pApi->RegisterMulticast((char *) m_pApp->m_mdAddr.c_str(), (char*)m_pApp->m_mdInterface.c_str(), nullptr);
        }
        printf("MD Start Bind cpucore %s %s %c\n", m_pApp->m_cpucore.c_str(), isTest?"tcp":"udp", m_exchangeID);
        m_pApi->Init(m_pApp->m_cpucore.c_str());
        return true;
    }

    int MDL2Impl::ReqMarketData(TTORATstpSecurityIDType SecurityID, TTORATstpExchangeIDType ExchangeID, int type, bool isSub) {
        char *security_arr[1];
        security_arr[0] = SecurityID;
        switch (type) {
            case 1:
                if (isSub) return m_pApi->SubscribeMarketData(security_arr, 1, ExchangeID);
                return m_pApi->UnSubscribeMarketData(security_arr, 1, ExchangeID);
            case 2:
                if (isSub) return m_pApi->SubscribeTransaction(security_arr, 1, ExchangeID);
                return m_pApi->UnSubscribeTransaction(security_arr, 1, ExchangeID);
            case 3:
                if (isSub) return m_pApi->SubscribeOrderDetail(security_arr, 1, ExchangeID);
                return m_pApi->UnSubscribeOrderDetail(security_arr, 1, ExchangeID);
            case 4:
                if (isSub) return m_pApi->SubscribeNGTSTick(security_arr, 1, ExchangeID);
                return m_pApi->UnSubscribeNGTSTick(security_arr, 1, ExchangeID);
        }
        return -1;
    }

    void MDL2Impl::ShowOrderBook(TTORATstpSecurityIDType SecurityID) {
        if (m_orderBuy.empty() && m_orderSell.empty()) return;

        std::stringstream stream;
        stream << "\n";
        stream << "--------- " << SecurityID << " " << GetTimeStr() << "---------\n";

        char buffer[512] = {0};
        int showPankouCount = 5;
        {
            auto iter = m_orderSell.find(SecurityID);
            if (iter != m_orderSell.end()) {
                auto size = (int) iter->second.size();
                if (showPankouCount < size) size = showPankouCount;
                for (auto i = size; i > 0; i--) {
                    memset(buffer, 0, sizeof(buffer));
                    TTORATstpLongVolumeType totalVolume = 0;
                    for (auto iter1: iter->second.at(i - 1).Orders) totalVolume += iter1->Volume;
                    sprintf(buffer, "S%d\t%.3f\t\t%d\t\t%lld\n", i, iter->second.at(i - 1).Price,
                            (int) iter->second.at(i - 1).Orders.size(), totalVolume);
                    stream << buffer;
                }
            }
        }

        {
            auto iter = m_orderBuy.find(SecurityID);
            if (iter != m_orderBuy.end()) {
                auto size = (int) iter->second.size();
                if (showPankouCount < size) size = showPankouCount;
                for (auto i = 1; i <= size; i++) {
                    memset(buffer, 0, sizeof(buffer));
                    TTORATstpLongVolumeType totalVolume = 0;
                    for (auto iter1: iter->second.at(i - 1).Orders) totalVolume += iter1->Volume;
                    sprintf(buffer, "B%d\t%.3f\t\t%d\t\t%lld\n", i, iter->second.at(i - 1).Price,
                            (int) iter->second.at(i - 1).Orders.size(), totalVolume);
                    stream << buffer;
                }
            }
        }
        printf("%s\n", stream.str().c_str());
    }

    void MDL2Impl::ShowUnfindOrder(TTORATstpSecurityIDType SecurityID) {
        char buffer[512] = {0};
        {
            auto iter = m_unFindOrders.find(SecurityID);
            if (iter != m_unFindOrders.end()) {
                sprintf(buffer, "%s %s UnfindOrderCount:%d", GetTimeStr().c_str(), SecurityID, (int)iter->second.size());
                printf("%s\n", buffer);
            }
        }
    }

    const char *MDL2Impl::GetExchangeName(TTORATstpExchangeIDType ExchangeID) {
        switch (ExchangeID) {
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

    /***********************API*********************************/
    void MDL2Impl::OnFrontConnected() {
        printf("%s MD::OnFrontConnected!!!\n", GetExchangeName(m_exchangeID));
        CTORATstpReqUserLoginField req = {0};
        m_pApi->ReqUserLogin(&req, ++m_reqID);
    }

    void MDL2Impl::OnFrontDisconnected(int nReason) {
        printf("%s MD::OnFrontDisconnected!!! Reason:%d\n", GetExchangeName(m_exchangeID), nReason);
        m_isInited = false;
    }

    void MDL2Impl::OnRspUserLogin(CTORATstpRspUserLoginField *pRspUserLogin, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (!pRspUserLogin || !pRspInfo) return;
        if (pRspInfo->ErrorID > 0) {
            printf("%s MD::OnRspUserLogin Failed!!! ErrMsg:%s\n", GetExchangeName(m_exchangeID), pRspInfo->ErrorMsg);
            return;
        }
        m_isInited = true;
        m_pApp->m_ioc.post(boost::bind(&CApplication::MDOnInited, m_pApp, m_exchangeID));
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

    void MDL2Impl::OnRtnMarketData(CTORATstpLev2MarketDataField *pDepthMarketData, const int FirstLevelBuyNum, const int FirstLevelBuyOrderVolumes[], const int FirstLevelSellNum, const int FirstLevelSellOrderVolumes[]) {
        if (!pDepthMarketData) return;
        printf("%s MD::OnRtnMarketData %s [%.3f %.3f %.3f %.3f %.3f] [%.3f %.3f %.3f %c]\n",
               PROMD::MDL2Impl::GetExchangeName(pDepthMarketData->ExchangeID),
               pDepthMarketData->SecurityID, pDepthMarketData->LastPrice, pDepthMarketData->OpenPrice,
               pDepthMarketData->HighestPrice, pDepthMarketData->LowestPrice, pDepthMarketData->ClosePrice,
               pDepthMarketData->UpperLimitPrice, pDepthMarketData->LowerLimitPrice, pDepthMarketData->PreClosePrice, pDepthMarketData->MDSecurityStat);
    }

    /************************************HandleOrderBook***************************************/
    void MDL2Impl::OnRtnOrderDetail(CTORATstpLev2OrderDetailField *pOrderDetail) {
        if (!pOrderDetail) return;
        if (pOrderDetail->Side != TORA_TSTP_LSD_Buy && pOrderDetail->Side != TORA_TSTP_LSD_Sell) return;

        if (pOrderDetail->ExchangeID == TORA_TSTP_EXD_SSE) {
            if (pOrderDetail->OrderStatus == TORA_TSTP_LOS_Add) {
                InsertOrder(pOrderDetail->SecurityID, pOrderDetail->OrderNO, pOrderDetail->Price, pOrderDetail->Volume, pOrderDetail->Side);
                HandleUnFindOrder(pOrderDetail->SecurityID, pOrderDetail->OrderNO, pOrderDetail->Side);
            } else if (pOrderDetail->OrderStatus == TORA_TSTP_LOS_Delete) {
                auto ret = ModifyOrder(pOrderDetail->SecurityID, 0, pOrderDetail->OrderNO, pOrderDetail->Side);
                if (!ret) {
                    AddUnFindOrder(pOrderDetail->SecurityID, 0, pOrderDetail->OrderNO, pOrderDetail->Side, 1) ;
                }
            }
        } else if (pOrderDetail->ExchangeID == TORA_TSTP_EXD_SZSE) {
            InsertOrder(pOrderDetail->SecurityID, pOrderDetail->OrderNO, pOrderDetail->Price, pOrderDetail->Volume, pOrderDetail->Side);
            HandleUnFindOrder(pOrderDetail->SecurityID, pOrderDetail->OrderNO, pOrderDetail->Side);
        }
        PostPrice(pOrderDetail->SecurityID, 0);
    }

    void MDL2Impl::OnRtnTransaction(CTORATstpLev2TransactionField *pTransaction) {
        if (!pTransaction) return;

        if (pTransaction->ExchangeID == TORA_TSTP_EXD_SSE) {
            auto ret = ModifyOrder(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->BuyNo, TORA_TSTP_LSD_Buy);
            if (!ret) {
                AddUnFindOrder(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->BuyNo, TORA_TSTP_LSD_Buy);
            }
            ret = ModifyOrder(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->SellNo, TORA_TSTP_LSD_Sell);
            if (!ret) {
                AddUnFindOrder(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->SellNo, TORA_TSTP_LSD_Sell);
            }
        } else if (pTransaction->ExchangeID == TORA_TSTP_EXD_SZSE) {
            if (pTransaction->ExecType == TORA_TSTP_ECT_Fill) {

                auto ret = ModifyOrder(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->BuyNo, TORA_TSTP_LSD_Buy);
                if (!ret) {
                    AddUnFindOrder(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->BuyNo, TORA_TSTP_LSD_Buy);
                }
                ret = ModifyOrder(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->SellNo, TORA_TSTP_LSD_Sell);
                if (!ret) {
                    AddUnFindOrder(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->SellNo, TORA_TSTP_LSD_Sell);
                }
            } else if (pTransaction->ExecType == TORA_TSTP_ECT_Cancel) {
                if (pTransaction->BuyNo > 0) {
                    auto ret = ModifyOrder(pTransaction->SecurityID, 0, pTransaction->BuyNo, TORA_TSTP_LSD_Buy);
                    if (!ret) {
                        AddUnFindOrder(pTransaction->SecurityID, 0, pTransaction->BuyNo, TORA_TSTP_LSD_Buy, 1);
                    }
                }
                if (pTransaction->SellNo > 0) {
                    auto ret = ModifyOrder(pTransaction->SecurityID, 0, pTransaction->SellNo, TORA_TSTP_LSD_Sell);
                    if (!ret) {
                        AddUnFindOrder(pTransaction->SecurityID, 0, pTransaction->SellNo, TORA_TSTP_LSD_Sell, 1);
                    }
                }
            }
        }
        PostPrice(pTransaction->SecurityID, pTransaction->TradePrice);
    }

    void MDL2Impl::OnRtnNGTSTick(CTORATstpLev2NGTSTickField *pTick) {
        if (!pTick) return;
        if (pTick->Side != TORA_TSTP_LSD_Buy && pTick->Side != TORA_TSTP_LSD_Sell) return;

        if (pTick->TickType == TORA_TSTP_LTT_Add) {
            TTORATstpLongSequenceType OrderNo;
            if (pTick->Side == TORA_TSTP_LSD_Buy)
                OrderNo = pTick->BuyNo;
            else
                OrderNo = pTick->SellNo;
            InsertOrder(pTick->SecurityID, OrderNo, pTick->Price, pTick->Volume, pTick->Side);
            HandleUnFindOrder(pTick->SecurityID, OrderNo, pTick->Side);
        } else if (pTick->TickType == TORA_TSTP_LTT_Delete) {
            TTORATstpLongSequenceType OrderNo;
            if (pTick->Side == TORA_TSTP_LSD_Buy)
                OrderNo = pTick->BuyNo;
            else
                OrderNo = pTick->SellNo;
            auto ret = ModifyOrder(pTick->SecurityID, 0, OrderNo, pTick->Side);
            if (!ret) {
                AddUnFindOrder(pTick->SecurityID, 0, OrderNo, pTick->Side, 1);
            }
        } else if (pTick->TickType == TORA_TSTP_LTT_Trade) {
            auto ret = ModifyOrder(pTick->SecurityID, pTick->Volume, pTick->BuyNo, TORA_TSTP_LSD_Buy);
            if (!ret) {
                AddUnFindOrder(pTick->SecurityID, pTick->Volume, pTick->BuyNo, TORA_TSTP_LSD_Buy);
            }
            ret = ModifyOrder(pTick->SecurityID, pTick->Volume, pTick->SellNo, TORA_TSTP_LSD_Sell);
            if (!ret)  {
                AddUnFindOrder(pTick->SecurityID, pTick->Volume, pTick->SellNo, TORA_TSTP_LSD_Sell);
            }
        }
        PostPrice(pTick->SecurityID, pTick->Price);
    }

    void MDL2Impl::InsertOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType OrderNO, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpLSideType Side) {
        Order* order = m_pApp->m_pool.Malloc<Order>(sizeof(Order));
        order->OrderNo = OrderNO;
        order->Volume = Volume;

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

    bool MDL2Impl::ModifyOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongVolumeType Volume, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType Side) {
        MapOrder &mapOrder = Side == TORA_TSTP_LSD_Buy ? m_orderBuy : m_orderSell;
        auto iter = mapOrder.find(SecurityID);
        if (iter != mapOrder.end()) {
            auto needReset = false, founded = false;
            for (auto i = 0; i < (int) iter->second.size(); i++) {
                for (auto j = 0; j < (int) iter->second.at(i).Orders.size(); j++) {
                    if (iter->second.at(i).Orders.at(j)->OrderNo == OrderNo) {
                        if (Volume == 0) {
                            iter->second.at(i).Orders.at(j)->Volume = 0;
                            needReset = true;
                        } else {
                            iter->second.at(i).Orders.at(j)->Volume -= Volume;
                            if (iter->second.at(i).Orders.at(j)->Volume <= 0) {
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

    void MDL2Impl::ResetOrder(TTORATstpSecurityIDType SecurityID, TTORATstpTradeBSFlagType Side) {
        MapOrder &mapOrder = Side == TORA_TSTP_LSD_Buy ? m_orderBuy : m_orderSell;
        auto iter = mapOrder.find(SecurityID);
        if (iter == mapOrder.end()) return;

        for (auto iterPriceOrder = iter->second.begin(); iterPriceOrder != iter->second.end();) {
            for (auto iterOrder = iterPriceOrder->Orders.begin(); iterOrder != iterPriceOrder->Orders.end();) {
                if ((*iterOrder)->Volume <= 0) {
                    Order* order = *iterOrder;
                    iterOrder = iterPriceOrder->Orders.erase(iterOrder);
                    m_pApp->m_pool.Free<Order>(order, sizeof(Order));
                } else {
                    ++iterOrder;
                }
            }
            if (iterPriceOrder->Orders.empty()) {
                iterPriceOrder = iter->second.erase(iterPriceOrder);
            } else {
                ++iterPriceOrder;
            }
        }
    }

    void MDL2Impl::FixOrder(TTORATstpSecurityIDType SecurityID, TTORATstpPriceType Price) {
        //{
        //    auto iter = m_orderSell.find(SecurityID);
        //    if (iter != m_orderSell.end()) {
        //        auto size = (int) iter->second.size();
        //        if (showPankouCount < size) size = showPankouCount;
        //        for (auto i = size; i > 0; i--) {
        //            memset(buffer, 0, sizeof(buffer));
        //            TTORATstpLongVolumeType totalVolume = 0;
        //            for (auto iter1: iter->second.at(i - 1).Orders) totalVolume += iter1->Volume;
        //            sprintf(buffer, "S%d\t%.3f\t\t%d\t\t%lld\n", i, iter->second.at(i - 1).Price,
        //                    (int) iter->second.at(i - 1).Orders.size(), totalVolume);
        //            stream << buffer;
        //        }
        //    }
        //}

        //{
        //    auto iter = m_orderBuy.find(SecurityID);
        //    if (iter != m_orderBuy.end()) {
        //        auto size = (int) iter->second.size();
        //        if (showPankouCount < size) size = showPankouCount;
        //        for (auto i = 1; i <= size; i++) {
        //            memset(buffer, 0, sizeof(buffer));
        //            TTORATstpLongVolumeType totalVolume = 0;
        //            for (auto iter1: iter->second.at(i - 1).Orders) totalVolume += iter1->Volume;
        //            sprintf(buffer, "B%d\t%.3f\t\t%d\t\t%lld\n", i, iter->second.at(i - 1).Price,
        //                    (int) iter->second.at(i - 1).Orders.size(), totalVolume);
        //            stream << buffer;
        //        }
        //    }
        //}


    }

    void MDL2Impl::AddUnFindOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongVolumeType Volume, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType Side, int type) {
        auto order = m_pApp->m_pool.Malloc<stUnfindOrder>(sizeof(stUnfindOrder));
        strcpy(order->SecurityID, SecurityID);
        order->OrderNo = OrderNo;
        order->Volume = Volume;
        order->Side = Side;
        order->Time = GetNowTick();
        order->Type = type; // 0-trade 1-delete

        auto iter = m_unFindOrders.find(SecurityID);
        if (iter == m_unFindOrders.end()) {
            m_unFindOrders[SecurityID] = std::unordered_map<TTORATstpLongSequenceType, std::vector<stUnfindOrder*> >();
        }
        m_unFindOrders[SecurityID][OrderNo].emplace_back(order);
    }

    void MDL2Impl::HandleUnFindOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType Side) {
        auto nowTick = GetNowTick();

        {
            auto iter = m_unFindOrders.find(SecurityID);
            if (iter == m_unFindOrders.end()) return;

            auto iter1 = iter->second.find(OrderNo);
            if (iter1 != iter->second.end()) {
                for (auto iter2 = iter1->second.begin(); iter2 != iter1->second.end();) {
                    if ((*iter2)->Side == Side && (*iter2)->Type == 0) {
                        if (ModifyOrder(SecurityID, (*iter2)->Volume, OrderNo, Side)) {
                            //printf("处理%s未找到的成交单成功 OrderNo:%lld Volume:%lld Side:%c\n", SecurityID, OrderNo, (*iter2)->Volume, Side);
                            auto order = (*iter2);
                            iter2 = iter1->second.erase(iter2);
                            m_pApp->m_pool.Free<stUnfindOrder>(order, sizeof(stUnfindOrder));
                        } else {
                            ++iter2;
                        }
                    }
                    else {
                        ++iter2;
                    }
                }
                if (iter1->second.empty()) {
                    iter->second.erase(iter1);
                }
            }
            if (iter->second.empty()) {
                m_unFindOrders.erase(iter);
            }
        }

        {
            auto iter = m_unFindOrders.find(SecurityID);
            if (iter == m_unFindOrders.end()) return;

            auto iter1 = iter->second.find(OrderNo);
            if (iter1 != iter->second.end()) {
                for (auto iter2 = iter1->second.begin(); iter2 != iter1->second.end();) {
                    if ((*iter2)->Side == Side && (*iter2)->Type == 1) {
                        if (ModifyOrder(SecurityID, (*iter2)->Volume, OrderNo, Side)) {
                            //printf("处理%s未找到的撤单成功 OrderNo:%lld Volume:%lld Side:%c\n", SecurityID, OrderNo, (*iter2)->Volume, Side);
                            auto order = (*iter2);
                            iter2 = iter1->second.erase(iter2);
                            m_pApp->m_pool.Free<stUnfindOrder>(order, sizeof(stUnfindOrder));
                        } else {
                            ++iter2;
                        }
                    }
                    else {
                        ++iter2;
                    }
                }
                if (iter1->second.empty()) {
                    iter->second.erase(iter1);
                }
            }
            if (iter->second.empty()) {
                m_unFindOrders.erase(iter);
            }
        }

        {
            auto iter = m_unFindOrders.find(SecurityID);
            if (iter == m_unFindOrders.end()) return;

            for (auto iter1 = iter->second.begin(); iter1 != iter->second.end();) {
                for (auto iter2 = iter1->second.begin(); iter2 != iter1->second.end();) {
                    if ((*iter2)->Time + 30 < nowTick) {
                        //printf("删除订单%d-%d=%d\n", nowTick, (*iter2)->Time, nowTick-(*iter2)->Time);
                        auto order = (*iter2);
                        iter2 = iter1->second.erase(iter2);
                        m_pApp->m_pool.Free<stUnfindOrder>(order, sizeof(stUnfindOrder));
                    } else {
                        ++iter2;
                    }
                }
                if (iter1->second.empty()) {
                    iter1 = iter->second.erase(iter1);
                }
                else {
                    ++iter1;
                }
            }
            if (iter->second.empty()) {
                m_unFindOrders.erase(iter);
            }
        }
    }

    void MDL2Impl::PostPrice(TTORATstpSecurityIDType SecurityID, TTORATstpPriceType tradePrice) {
        if (!m_pApp->m_strategyOpen) return;

        TTORATstpPriceType BidPrice1 = 0.0;
        TTORATstpLongVolumeType BidVolume1 = 0;
        TTORATstpPriceType AskPrice1 = 0.0;
        TTORATstpLongVolumeType AskVolume1 = 0;
        TTORATstpPriceType TradePrice = tradePrice;

        {
            auto iter = m_orderSell.find(SecurityID);
            if (iter != m_orderSell.end() && !iter->second.empty()) {
                auto size = (int) iter->second.begin()->Orders.size();
                long long int totalVolume = 0;
                for (auto i = 0; i < size; ++i) {
                    totalVolume += iter->second.begin()->Orders.at(i)->Volume;
                }
                AskPrice1 = iter->second.begin()->Price;
                AskVolume1 = totalVolume;
            }
        }

        {
            auto iter = m_orderBuy.find(SecurityID);
            if (iter != m_orderBuy.end() && !iter->second.empty()) {
                auto size = (int) iter->second.begin()->Orders.size();
                long long int totalVolume = 0;
                for (auto i = 0; i < size; ++i) {
                    totalVolume += iter->second.begin()->Orders.at(i)->Volume;
                }
                BidPrice1 = iter->second.begin()->Price;
                BidVolume1 = totalVolume;
            }
        }

        stPostPrice p = {0};
        strcpy(p.SecurityID, SecurityID);
        p.BidVolume1 = BidVolume1;
        p.BidPrice1 = BidPrice1;
        p.AskPrice1 = AskPrice1;
        p.AskVolume1 = AskVolume1;
        p.TradePrice = TradePrice;
        m_pApp->m_ioc.post(boost::bind(&CApplication::MDPostPrice, m_pApp, p));
    }

}