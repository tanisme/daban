#include "MDL2Impl.h"
#include "CApplication.h"
#include "LocalConfig.h"
#include <boost/bind.hpp>

namespace PROMD {

    MDL2Impl::MDL2Impl(CApplication *pApp, TTORATstpExchangeIDType exchangeID)
            : m_pApp(pApp), m_exchangeID(exchangeID) {
    }

    MDL2Impl::~MDL2Impl() {
        if (m_mdApi != nullptr) {
            m_mdApi->Join();
            m_mdApi->Release();
        }
    }

    bool MDL2Impl::Start() {
        m_mdApi = CTORATstpLev2MdApi::CreateTstpLev2MdApi();
        if (!m_mdApi) return false;

        m_mdApi->RegisterSpi(this);
        if (m_exchangeID == TORA_TSTP_EXD_SSE) {
            m_mdApi->RegisterFront((char *) LocalConfig::GetMe().GetSHMDAddr().c_str());
        } else if (m_exchangeID == TORA_TSTP_EXD_SZSE) {
            m_mdApi->RegisterFront((char *) LocalConfig::GetMe().GetSZMDAddr().c_str());
        } else {
            return false;
        }
        m_mdApi->Init();
        return true;
    }

    void MDL2Impl::OnFrontConnected() {
        printf("%s MDL2Impl::OnFrontConnected!!!\n", GetExchangeName(m_exchangeID));

        CTORATstpReqUserLoginField req = {0};
        m_mdApi->ReqUserLogin(&req, ++m_reqID);
    }

    void MDL2Impl::OnFrontDisconnected(int nReason) {
        printf("%s MDL2Impl::OnFrontDisconnected!!! Reason:%d\n", GetExchangeName(m_exchangeID), nReason);
        m_isLogined = false;
    }

    void MDL2Impl::OnRspUserLogin(CTORATstpRspUserLoginField *pRspUserLogin, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (!pRspUserLogin || !pRspInfo) return;
        if (pRspInfo->ErrorID > 0) {
            printf("%s MDL2Impl::OnRspUserLogin Failed!!! ErrMsg:%s\n", GetExchangeName(m_exchangeID), pRspInfo->ErrorMsg);
            return;
        }

        m_isLogined = true;
        m_pApp->m_ioc.post(boost::bind(&CApplication::MDOnRspUserLogin, m_pApp, m_exchangeID));
    }

    int MDL2Impl::ReqMarketData(TTORATstpSecurityIDType security, TTORATstpExchangeIDType exchangeID, int type, bool isSub) {
        char *security_arr[1];
        security_arr[0] = security;

        auto iter = m_subSecuritys.find(security);
        if (!isSub && iter != m_subSecuritys.end()) {
            m_subSecuritys.erase(security);
        }
        else {
            m_subSecuritys[security] = 1;
        }

        switch (type) {
            case 1:
                if (isSub) return m_mdApi->SubscribeMarketData(security_arr, 1, exchangeID);
                return m_mdApi->UnSubscribeMarketData(security_arr, 1, exchangeID);
                break;
            case 2:
                if (isSub) return m_mdApi->SubscribeTransaction(security_arr, 1, exchangeID);
                return m_mdApi->UnSubscribeTransaction(security_arr, 1, exchangeID);
                break;
            case 3:
                if (isSub) return m_mdApi->SubscribeOrderDetail(security_arr, 1, exchangeID);
                return m_mdApi->UnSubscribeOrderDetail(security_arr, 1, exchangeID);
                break;
            default:
                break;
        }
        return -1;
    }

    void MDL2Impl::OnRspSubMarketData(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (!pSpecificSecurity || !pRspInfo) return;
        if (pRspInfo->ErrorID > 0) {
            printf("%s MDL2Impl::OnRspSubMarketData Failed!!! ErrMsg:%s\n", GetExchangeName(m_exchangeID), pRspInfo->ErrorMsg);
            return;
        }

        printf("MDL2Impl::OnRspSubMarketData Success!!!\n");
    }

    void MDL2Impl::OnRspUnSubMarketData(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (!pSpecificSecurity || !pRspInfo) return;
        if (pRspInfo->ErrorID > 0) {
            printf("%s MDL2Impl::OnRspUnSubMarketData Failed!!! ErrMsg:%s\n", GetExchangeName(m_exchangeID), pRspInfo->ErrorMsg);
            return;
        }

        printf("MDL2Impl::OnRspUnSubMarketData Success!!!\n");
    }

    void MDL2Impl::OnRtnMarketData(CTORATstpLev2MarketDataField *pDepthMarketData, const int FirstLevelBuyNum, const int FirstLevelBuyOrderVolumes[], const int FirstLevelSellNum, const int FirstLevelSellOrderVolumes[]) {
        if (!pDepthMarketData) return;

        printf("%s MDL2Impl::OnRtnMarketData %s [%.3f %.3f %.3f %.3f %.3f] [%.3f %.3f %.3f %c]\n",
               PROMD::MDL2Impl::GetExchangeName(pDepthMarketData->ExchangeID),
               pDepthMarketData->SecurityID, pDepthMarketData->LastPrice, pDepthMarketData->OpenPrice,
               pDepthMarketData->HighestPrice,
               pDepthMarketData->LowestPrice, pDepthMarketData->ClosePrice, pDepthMarketData->UpperLimitPrice,
               pDepthMarketData->LowerLimitPrice,
               pDepthMarketData->PreClosePrice, pDepthMarketData->MDSecurityStat);
    }

    void MDL2Impl::OnRspSubTransaction(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (!pSpecificSecurity || !pRspInfo) return;
        if (pRspInfo->ErrorID > 0) {
            printf("%s MDL2Impl::OnRspSubTransaction Failed!!! ErrMsg:%s\n", GetExchangeName(m_exchangeID), pRspInfo->ErrorMsg);
            return;
        }

        printf("MDL2Impl::OnRspSubTransaction Success!!!\n");
    }

    void MDL2Impl::OnRspUnSubTransaction(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (!pSpecificSecurity || !pRspInfo) return;
        if (pRspInfo->ErrorID > 0) {
            printf("%s MDL2Impl::OnRspUnSubTransaction Failed!!! ErrMsg:%s\n", GetExchangeName(m_exchangeID), pRspInfo->ErrorMsg);
            return;
        }

        printf("MDL2Impl::OnRspUnSubTransaction Success!!!\n");
    }

    void MDL2Impl::OnRtnTransaction(CTORATstpLev2TransactionField *pTransaction) {
        if (!pTransaction) return;

        if (pTransaction->ExchangeID == TORA_TSTP_EXD_SSE) {
            ModifyOrder(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->BuyNo, TORA_TSTP_LSD_Buy);
            ModifyOrder(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->SellNo, TORA_TSTP_LSD_Sell);
            //FixOrder(pTransaction->SecurityID, pTransaction->TradePrice);
        } else if (pTransaction->ExchangeID == TORA_TSTP_EXD_SZSE) {
            if (pTransaction->ExecType == TORA_TSTP_ECT_Fill) {
                ModifyOrder(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->BuyNo, TORA_TSTP_LSD_Buy);
                ModifyOrder(pTransaction->SecurityID, pTransaction->TradeVolume, pTransaction->SellNo, TORA_TSTP_LSD_Sell);
                //FixOrder(pTransaction->SecurityID, pTransaction->TradePrice);
            } else if (pTransaction->ExecType == TORA_TSTP_ECT_Cancel) {
                if (pTransaction->BuyNo > 0) DeleteOrder(pTransaction->SecurityID, pTransaction->BuyNo);
                if (pTransaction->SellNo > 0) DeleteOrder(pTransaction->SecurityID, pTransaction->SellNo);
            }
        }
        //ShowOrderBook();
    }

    void MDL2Impl::OnRspSubOrderDetail(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (!pSpecificSecurity || !pRspInfo) return;
        if (pRspInfo->ErrorID > 0) {
            printf("%s MDL2Impl::OnRspSubOrderDetail Failed!!! ErrMsg:%s\n", GetExchangeName(m_exchangeID), pRspInfo->ErrorMsg);
            return;
        }

        printf("MDL2Impl::OnRspSubOrderDetail Success!!!\n");
    }

    void MDL2Impl::OnRspUnSubOrderDetail(CTORATstpSpecificSecurityField *pSpecificSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (!pSpecificSecurity || !pRspInfo) return;
        if (pRspInfo->ErrorID > 0) {
            printf("%s MDL2Impl::OnRspUnSubOrderDetail Failed!!! ErrMsg:%s\n", GetExchangeName(m_exchangeID), pRspInfo->ErrorMsg);
            return;
        }

        printf("%s MDL2Impl::OnRspUnSubOrderDetail Success!!! %s\n", GetExchangeName(pSpecificSecurity->ExchangeID), pSpecificSecurity->SecurityID);
    }

    void MDL2Impl::OnRtnOrderDetail(CTORATstpLev2OrderDetailField *pOrderDetail) {
        if (!pOrderDetail ||
            (pOrderDetail->Side != TORA_TSTP_LSD_Buy && pOrderDetail->Side != TORA_TSTP_LSD_Sell))
            return;

        if (pOrderDetail->ExchangeID == TORA_TSTP_EXD_SSE) {
            if (pOrderDetail->OrderStatus == TORA_TSTP_LOS_Add) {
                InsertOrder(pOrderDetail->SecurityID, pOrderDetail->OrderNO, pOrderDetail->Price, pOrderDetail->Volume, pOrderDetail->Side);
            } else if (pOrderDetail->OrderStatus == TORA_TSTP_LOS_Delete) {
                DeleteOrder(pOrderDetail->SecurityID, pOrderDetail->OrderNO);
            }
        } else if (pOrderDetail->ExchangeID == TORA_TSTP_EXD_SZSE) {
            InsertOrder(pOrderDetail->SecurityID, pOrderDetail->OrderNO, pOrderDetail->Price, pOrderDetail->Volume, pOrderDetail->Side);
        }
    }

    void MDL2Impl::InsertOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType OrderNO, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpLSideType Side) {
        Order order = {0};
        order.OrderNo = OrderNO;
        order.Volume = Volume;

        PriceOrders priceOrder = {0};
        priceOrder.Orders.emplace_back(order);
        priceOrder.Price = Price;

        MapOrder &mapOrder = Side == TORA_TSTP_LSD_Buy ? m_OrderBuy : m_OrderSell;
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
        //ShowOrderBook();
    }

    void MDL2Impl::DeleteOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongSequenceType OrderNO) {
        ModifyOrder(SecurityID, 0, OrderNO, TORA_TSTP_LSD_Buy);
        ModifyOrder(SecurityID, 0, OrderNO, TORA_TSTP_LSD_Sell);
        //ShowOrderBook();
    }

    void MDL2Impl::ModifyOrder(TTORATstpSecurityIDType SecurityID, TTORATstpLongVolumeType TradeVolume, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType Side) {
        MapOrder &mapOrder = Side == TORA_TSTP_LSD_Buy ? m_OrderBuy : m_OrderSell;
        auto iter = mapOrder.find(SecurityID);
        if (iter == mapOrder.end()) return;

        auto needReset = false;
        for (auto i = 0; i < (int) iter->second.size(); i++) {
            for (auto j = 0; j < (int) iter->second.at(i).Orders.size(); j++) {
                if (iter->second.at(i).Orders.at(j).OrderNo == OrderNo) {
                    if (TradeVolume == 0) {
                        iter->second.at(i).Orders.at(j).Volume = 0;
                        needReset = true;
                    } else {
                        iter->second.at(i).Orders.at(j).Volume -= TradeVolume;
                        needReset = true;
                    }
                }
            }
        }
        if (needReset) ResetOrder(SecurityID, Side);
    }

    void MDL2Impl::ResetOrder(TTORATstpSecurityIDType SecurityID, TTORATstpTradeBSFlagType Side) {
        MapOrder &mapOrder = Side == TORA_TSTP_LSD_Buy ? m_OrderBuy : m_OrderSell;
        auto iter = mapOrder.find(SecurityID);
        if (iter == mapOrder.end()) return;

        for (auto iterPriceOrder = iter->second.begin(); iterPriceOrder != iter->second.end();) {
            auto bFlag1 = false;
            auto bFlag2 = false;
            for (auto iterOrder = iterPriceOrder->Orders.begin();
                 iterOrder != iterPriceOrder->Orders.end();) {
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

    void MDL2Impl::ShowOrderBook() {
        if (m_OrderBuy.empty() && m_OrderSell.empty()) return;

        for (auto &iter: m_subSecuritys) {
            printf("\n");
            printf("--------------------- %s ---------------------\n", iter.first.c_str());

            {
                int showCount = 5;
                auto iter1 = m_OrderSell.find(iter.first);
                if (iter1 != m_OrderSell.end()) {
                    auto size = (int) iter1->second.size();
                    if (showCount < size) size = showCount;
                    for (auto i = size; i > 0; i--) {
                        long long int totalVolume = 0;
                        for (auto iter2: iter1->second.at(i - 1).Orders) {
                            totalVolume += iter2.Volume;
                        }
                        printf("S%d\t%.3f\t\t%d\t\t%lld\n", i, iter1->second.at(i - 1).Price,
                               (int) iter1->second.at(i - 1).Orders.size(), totalVolume);
                    }
                }
            }

            {
                int showCount = 5;
                auto iter1 = m_OrderBuy.find(iter.first);
                if (iter1 != m_OrderBuy.end()) {
                    auto size = (int) iter1->second.size();
                    if (showCount < size) size = showCount;
                    for (auto i = 1; i <= size; i++) {
                        long long int totalVolume = 0;
                        for (auto iter2: iter1->second.at(i - 1).Orders) {
                            totalVolume += iter2.Volume;
                        }
                        printf("B%d\t%.3f\t\t%d\t\t%lld\n", i, iter1->second.at(i - 1).Price,
                               (int) iter1->second.at(i - 1).Orders.size(), totalVolume);
                    }
                }
            }
        }
    }

    void MDL2Impl::FixOrder(TTORATstpSecurityIDType SecurityID, TORALEV2API::TTORATstpPriceType Price) {

        {
            auto iter1 = m_OrderSell.find(SecurityID);
            if (iter1 != m_OrderSell.end()) {
                auto size = (int) iter1->second.size();
                for (auto i = size; i > 0; i--) {
                    if (iter1->second.at(i - 1).Price + 0.000001 < Price) {
                        for (auto iter2: iter1->second.at(i - 1).Orders) {
                            iter2.Volume = 0;
                        }
                    }
                }
                ResetOrder(SecurityID, TORA_TSTP_LSD_Sell);
            }
        }

        {
            auto iter1 = m_OrderBuy.find(SecurityID);
            if (iter1 != m_OrderBuy.end()) {
                auto size = (int) iter1->second.size();
                for (auto i = size; i > 0; i--) {
                    if (iter1->second.at(i - 1).Price > Price + 0.000001) {
                        for (auto iter2: iter1->second.at(i - 1).Orders) {
                            iter2.Volume = 0;
                        }
                    }
                }
                ResetOrder(SecurityID, TORA_TSTP_LSD_Buy);
            }
        }
    }

    const char *MDL2Impl::GetExchangeName(TTORATstpExchangeIDType exchangeID) {
        switch (exchangeID) {
            case TORA_TSTP_EXD_SSE:
                return "SH";
            case TORA_TSTP_EXD_SZSE:
                return "SZ";
            case TORA_TSTP_EXD_HK:
                return "HK";
            default:
                break;
        }
        return "UNKNOW";
    }

    const char *MDL2Impl::GetSide(TTORATstpLSideType Side) {
        switch (Side) {
            case TORA_TSTP_LSD_Buy:
                return "B";
            case TORA_TSTP_LSD_Sell:
                return "S";
            case TORA_TSTP_LSD_Borrow:
                return "R";
            case TORA_TSTP_LSD_Lend:
                return "C";
        }
        return "UNKNOW";
    }

}