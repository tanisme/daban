#include "CApplication.h"

CApplication::CApplication(boost::asio::io_context& ioc)
        : m_ioc(ioc)
        , m_timer(m_ioc, boost::posix_time::milliseconds(1000)) {
}

CApplication::~CApplication() {
    if (m_MD) {
        m_MD->Api()->Join();
        m_MD->Api()->Release();
        delete m_MD;
    }

    if (m_TD) {
        m_TD->Api()->Join();
        m_TD->Api()->Release();
        delete m_TD;
    }
}

void CApplication::Init(std::string& watchSecurity) {
    trim(watchSecurity);
    std::vector<std::string> vtSecurity;
    Stringsplit(watchSecurity, ',', vtSecurity);
    for (auto& iter : vtSecurity) {
        auto security = m_pool.Malloc<stSecurity>(sizeof(stSecurity));
        strcpy(security->SecurityID, iter.c_str());
        m_watchSecurity[security->SecurityID] = security;
    }
}

void CApplication::Start() {
    if (m_dataDir.length() > 0) {
        InitOrderMapN(true);
        m_imitate.TestOrderBook(this, m_dataDir);
    } else {
        if (!m_TD) {
            m_TD = new PROTD::TDImpl(this);
            m_TD->Start();
        }
        m_timer.expires_from_now(boost::posix_time::milliseconds(5000));
        m_timer.async_wait(boost::bind(&CApplication::OnTime, this, boost::asio::placeholders::error));
    }
}

void CApplication::OnTime(const boost::system::error_code& error) {
    if (error) {
        if (error == boost::asio::error::operation_aborted) {
            return;
        }
    }

    for (auto& iter : m_watchSecurity) {
        ShowOrderBook((char*)iter.first.c_str());
    }

    m_timer.expires_from_now(boost::posix_time::milliseconds(6000));
    m_timer.async_wait(boost::bind(&CApplication::OnTime, this, boost::asio::placeholders::error));
}

/***************************************MD***************************************/
void CApplication::MDOnInitFinished(PROMD::TTORATstpExchangeIDType ExchangeID) {
    auto cnt = 0;
    for (auto &iter: m_marketSecurity) {
        if (ExchangeID == iter.second->ExchangeID) {
            cnt++;
            PROMD::TTORATstpSecurityIDType SecurityID = {0};
            strncpy(SecurityID, iter.first.c_str(), sizeof(SecurityID));
            char *security_arr[1];
            security_arr[0] = SecurityID;
            if (iter.second->ExchangeID == PROMD::TORA_TSTP_EXD_SSE && m_isSHNewversion) {
                m_MD->Api()->SubscribeNGTSTick(security_arr, 1, ExchangeID);
            } else {
                m_MD->Api()->SubscribeOrderDetail(security_arr, 1, ExchangeID);
                m_MD->Api()->SubscribeTransaction(security_arr, 1, ExchangeID);
            }
        }
    }
    printf("CApplication::MDOnInitFinished %s subscribe:%d total:%d\n",
           PROMD::MDL2Impl::GetExchangeName(ExchangeID), cnt, (int)m_marketSecurity.size());
}

void CApplication::MDOnRtnOrderDetail(PROMD::CTORATstpLev2OrderDetailField &OrderDetail) {
    if (OrderDetail.OrderType == PROMD::TORA_TSTP_LOT_Market) return;
    if (OrderDetail.Side != PROMD::TORA_TSTP_LSD_Buy && OrderDetail.Side != PROMD::TORA_TSTP_LSD_Sell) return;

    if (OrderDetail.ExchangeID == PROMD::TORA_TSTP_EXD_SZSE) {
        if (OrderDetail.OrderType == PROMD::TORA_TSTP_LOT_HomeBest) {
            AddHomebestOrder(OrderDetail.SecurityID, OrderDetail.OrderNO, OrderDetail.Price, OrderDetail.Volume, OrderDetail.Side, OrderDetail.ExchangeID);
            return;
        }
        InsertOrderN(OrderDetail.SecurityID, OrderDetail.OrderNO, OrderDetail.Price, OrderDetail.Volume, OrderDetail.Side);
    }
    else if (OrderDetail.ExchangeID == PROMD::TORA_TSTP_EXD_SSE) {
        if (OrderDetail.OrderStatus == PROMD::TORA_TSTP_LOS_Add) {
            InsertOrderN(OrderDetail.SecurityID, OrderDetail.OrderNO, OrderDetail.Price, OrderDetail.Volume, OrderDetail.Side);
        } else if (OrderDetail.OrderStatus == PROMD::TORA_TSTP_LOS_Delete) {
            ModifyOrderN(OrderDetail.SecurityID, 0, OrderDetail.OrderNO, OrderDetail.Side);
        }
    }
}

void CApplication::MDOnRtnTransaction(PROMD::CTORATstpLev2TransactionField &Transaction) {
    if (Transaction.ExchangeID == PROMD::TORA_TSTP_EXD_SZSE) {
        if (Transaction.ExecType == PROMD::TORA_TSTP_ECT_Fill) {
            auto homeBestBuyOrder = GetHomebestOrder(Transaction.SecurityID, Transaction.BuyNo);
            if (homeBestBuyOrder) {
                homeBestBuyOrder->Volume -= Transaction.TradeVolume;
                if (homeBestBuyOrder->Volume > 0) {
                    InsertOrderN(homeBestBuyOrder->SecurityID, homeBestBuyOrder->OrderNO, Transaction.TradePrice, homeBestBuyOrder->Volume, homeBestBuyOrder->Side);
                } else {
                    m_pool.Free<stHomebestOrder>(homeBestBuyOrder, sizeof(stHomebestOrder));
                }
            } else {
                ModifyOrderN(Transaction.SecurityID, Transaction.TradeVolume, Transaction.BuyNo, PROMD::TORA_TSTP_LSD_Buy);
            }

            auto homeBestSellOrder = GetHomebestOrder(Transaction.SecurityID, Transaction.SellNo);
            if (homeBestSellOrder) {
                homeBestSellOrder->Volume -= Transaction.TradeVolume;
                if (homeBestSellOrder->Volume > 0) {
                    InsertOrderN(homeBestSellOrder->SecurityID, homeBestSellOrder->OrderNO, Transaction.TradePrice, homeBestSellOrder->Volume, homeBestSellOrder->Side);
                } else {
                    m_pool.Free<stHomebestOrder>(homeBestSellOrder, sizeof(stHomebestOrder));
                }
            } else {
                ModifyOrderN(Transaction.SecurityID, Transaction.TradeVolume, Transaction.SellNo, PROMD::TORA_TSTP_LSD_Sell);
            }
        } else if (Transaction.ExecType == PROMD::TORA_TSTP_ECT_Cancel) {
            if (Transaction.BuyNo > 0) {
                ModifyOrderN(Transaction.SecurityID, 0, Transaction.BuyNo, PROMD::TORA_TSTP_LSD_Buy);
            } else if (Transaction.SellNo > 0) {
                ModifyOrderN(Transaction.SecurityID, 0, Transaction.SellNo, PROMD::TORA_TSTP_LSD_Sell);
            }
        }
    }
    else if (Transaction.ExchangeID == PROMD::TORA_TSTP_EXD_SSE) {
        ModifyOrderN(Transaction.SecurityID, Transaction.TradeVolume, Transaction.BuyNo, PROMD::TORA_TSTP_LSD_Buy);
        ModifyOrderN(Transaction.SecurityID, Transaction.TradeVolume, Transaction.SellNo, PROMD::TORA_TSTP_LSD_Sell);
    }
}

void CApplication::MDOnRtnNGTSTick(PROMD::CTORATstpLev2NGTSTickField &Tick) {
    if (Tick.ExchangeID != PROMD::TORA_TSTP_EXD_SSE) return;
    if (Tick.Side != PROMD::TORA_TSTP_LSD_Buy && Tick.Side != PROMD::TORA_TSTP_LSD_Sell) return;

    if (Tick.TickType == PROMD::TORA_TSTP_LTT_Add) {
        InsertOrderN(Tick.SecurityID, Tick.Side==PROMD::TORA_TSTP_LSD_Buy?Tick.BuyNo:Tick.SellNo, Tick.Price, Tick.Volume, Tick.Side);
    } else if (Tick.TickType == PROMD::TORA_TSTP_LTT_Delete) {
        ModifyOrderN(Tick.SecurityID, 0, Tick.Side==PROMD::TORA_TSTP_LSD_Buy?Tick.BuyNo:Tick.SellNo, Tick.Side);
    } else if (Tick.TickType == PROMD::TORA_TSTP_LTT_Trade) {
        ModifyOrderN(Tick.SecurityID, Tick.Volume, Tick.BuyNo, PROMD::TORA_TSTP_LSD_Buy);
        ModifyOrderN(Tick.SecurityID, Tick.Volume, Tick.SellNo, PROMD::TORA_TSTP_LSD_Sell);
    }
}

double CApplication::GetOrderNoPrice(int SecurityIDInt, PROMD::TTORATstpLongSequenceType OrderNO) {
    auto Price = 0.0;
    auto iter = m_orderNoPrice.find(SecurityIDInt);
    if (iter != m_orderNoPrice.end()) {
        auto it = iter->second.find(OrderNO);
        if (it != iter->second.end()) {
            Price = it->second;
        }
    }
    return Price;
}

void CApplication::AddOrderNoPrice(int SecurityIDInt, PROMD::TTORATstpLongSequenceType OrderNO, PROMD::TTORATstpPriceType Price) {
    auto iter = m_orderNoPrice.find(SecurityIDInt);
    if (iter == m_orderNoPrice.end()) {
        m_orderNoPrice[SecurityIDInt][OrderNO] = Price;
    } else {
        auto it = iter->second.find(OrderNO);
        if (it != iter->second.end()) {
            printf("添加 %d 订单号 %lld 价格错误 已经存在该订单号价格 %.3f\n", SecurityIDInt, OrderNO, Price);
        }
        m_orderNoPrice[SecurityIDInt][OrderNO] = Price;
    }
}

void CApplication::DelOrderNoPrice(int SecurityIDInt, PROMD::TTORATstpLongSequenceType OrderNO) {
    auto iter = m_orderNoPrice.find(SecurityIDInt);
    if (iter != m_orderNoPrice.end()) {
        auto it = iter->second.find(OrderNO);
        if (it != iter->second.end()) {
            m_orderNoPrice[SecurityIDInt].erase(OrderNO);
        }
    }
}

void CApplication::AddHomebestOrder(PROMD::TTORATstpSecurityIDType SecurityID, PROMD::TTORATstpLongSequenceType OrderNO,
                                    PROMD::TTORATstpPriceType Price, PROMD::TTORATstpLongVolumeType Volume,
                                    PROMD::TTORATstpLSideType Side, PROMD::TTORATstpExchangeIDType ExchangeID) {
    auto homeBestOrder = m_pool.Malloc<stHomebestOrder>(sizeof(stHomebestOrder));
    strcpy(homeBestOrder->SecurityID, SecurityID);
    homeBestOrder->OrderNO = OrderNO;
    homeBestOrder->Price = Price;
    homeBestOrder->Volume = Volume;
    homeBestOrder->Side = Side;
    homeBestOrder->ExchangeID = ExchangeID;
    m_homeBaseOrder[homeBestOrder->SecurityID][homeBestOrder->OrderNO] = homeBestOrder;
}

stHomebestOrder* CApplication::GetHomebestOrder(PROMD::TTORATstpSecurityIDType SecurityID, PROMD::TTORATstpLongSequenceType OrderNO) {
    auto iter = m_homeBaseOrder.find(SecurityID);
    if (iter == m_homeBaseOrder.end()) return nullptr;
    if (iter->second.find(OrderNO) == iter->second.end()) return nullptr;
    return iter->second[OrderNO];
}

void CApplication::ShowOrderBook(PROMD::TTORATstpSecurityIDType SecurityID) {
    //if (m_orderBuyV.find(SecurityID) == m_orderBuyV.end() &&
    //    m_orderSellV.find(SecurityID) == m_orderSellV.end()) return;

    //std::stringstream stream;
    //stream << "\n";
    //stream << "-----------" << SecurityID << " " << GetTimeStr() << "-----------\n";

    //char buffer[512] = {0};
    //{
    //    std::vector<std::string> temp;
    //    auto iter = m_orderSellV.find(SecurityID);
    //    if (iter != m_orderSellV.end()) {
    //        int i = 1;
    //        for (auto iter1: iter->second) {
    //            memset(buffer, 0, sizeof(buffer));
    //            auto totalVolume = 0;
    //            for (auto iter2 : iter1.Orders) totalVolume += iter2->Volume;
    //            int Volume = (int)totalVolume / 100;
    //            if (totalVolume % 100 >= 50) Volume++;
    //            sprintf(buffer, "S%d\t%.2f\t%d\t\t%d\n", i, iter1.Price, Volume, (int)iter1.Orders.size());
    //            temp.emplace_back(buffer);
    //            if (i++ >= 5) break;
    //        }
    //        for (auto it = temp.rbegin(); it != temp.rend(); ++it) stream << (*it);
    //    }
    //}

    //{
    //    std::vector<std::string> temp;
    //    auto iter = m_orderBuyV.find(SecurityID);
    //    if (iter != m_orderBuyV.end()) {
    //        int i = 1;
    //        for (auto iter1: iter->second) {
    //            memset(buffer, 0, sizeof(buffer));
    //            auto totalVolume = 0;
    //            for (auto iter2 : iter1.Orders) totalVolume += iter2->Volume;
    //            int Volume = (int)totalVolume / 100;
    //            if (totalVolume % 100 >= 50) Volume++;
    //            sprintf(buffer, "B%d\t%.2f\t%d\t\t%d\n", i, iter1.Price, Volume, (int)iter1.Orders.size());
    //            temp.emplace_back(buffer);
    //            if (i++ >= 5) break;
    //        }
    //        for (auto it = temp.begin(); it != temp.end(); ++it) stream << (*it);
    //    }
    //}
    //printf("%s\n", stream.str().c_str());
}

void CApplication::InitOrderMapN(bool isTest) {
    m_orderBuyN.resize(999999);
    m_orderSellN.resize(999999);

    auto& Security = m_marketSecurity;
    if (isTest) {
        Security = m_watchSecurity;
    }
    for (auto it : Security) {
        auto SecurityIDInt = atoi(it.second->SecurityID);
        m_orderBuyN.at(SecurityIDInt).resize(it.second->TotalIndex);
        for (auto i = 0; i < it.second->TotalIndex; ++i) {
            m_orderBuyN.at(SecurityIDInt).at(i).Price = it.second->UpperLimitPrice - i * 0.01;
        }
        m_orderSellN.at(SecurityIDInt).resize(it.second->TotalIndex);
        for (auto i = 0; i < it.second->TotalIndex; ++i) {
            m_orderSellN.at(SecurityIDInt).at(i).Price = it.second->LowerLimitPrice + i * 0.01;
        }
    }
}

int CApplication::GetPriceIndexN(PROMD::TTORATstpSecurityIDType SecurityID, PROMD::TTORATstpPriceType Price, PROMD::TTORATstpTradeBSFlagType Side) {
    auto priceIndex = -1;
    auto iter = m_marketSecurity.find(SecurityID);
    if (iter == m_marketSecurity.end()) return priceIndex;

    if (Side == PROMD::TORA_TSTP_LSD_Buy) {
        priceIndex = int((iter->second->UpperLimitPrice - Price) / 0.01);
    } else {
        priceIndex = int((Price - iter->second->LowerLimitPrice) / 0.01);
    }
    if (priceIndex >= iter->second->TotalIndex || priceIndex < 0) {
        priceIndex = -1;
    }
    return priceIndex;
}

void CApplication::InsertOrderN(PROMD::TTORATstpSecurityIDType SecurityID, PROMD::TTORATstpLongSequenceType OrderNO,
                                PROMD::TTORATstpPriceType Price, PROMD::TTORATstpLongVolumeType Volume,
                                PROMD::TTORATstpLSideType Side) {
    int SecurityIDInt = atoi(SecurityID);

    auto priceIndex = GetPriceIndexN(SecurityID, Price, Side);
    if (priceIndex < 0) return;
    AddOrderNoPrice(SecurityIDInt, OrderNO, Price);
    AddOrderIndexN(SecurityIDInt, priceIndex, Side);

    auto order = m_pool.Malloc<stOrder>(sizeof(stOrder));
    order->OrderNo = OrderNO;
    order->Volume = Volume;
    auto& mapOrder = Side == PROMD::TORA_TSTP_LSD_Buy?m_orderBuyN:m_orderSellN;
    mapOrder.at(SecurityIDInt).at(priceIndex).Orders.emplace_back(order);
}

double CApplication::ModifyOrderN(PROMD::TTORATstpSecurityIDType SecurityID, PROMD::TTORATstpLongVolumeType Volume,
                                PROMD::TTORATstpLongSequenceType OrderNo, PROMD::TTORATstpTradeBSFlagType Side) {
    int SecurityIDInt = atoi(SecurityID);
    auto Price = GetOrderNoPrice(SecurityIDInt, OrderNo);
    if (Price < 0.000001) return Price;
    auto priceIndex = GetPriceIndexN(SecurityID, Price, Side);
    if (priceIndex < 0) return Price;

    std::vector<PROMD::TTORATstpLongSequenceType> OrderNos;
    auto& mapOrder = Side == PROMD::TORA_TSTP_LSD_Buy?m_orderBuyN:m_orderSellN;
    for (auto& it : mapOrder.at(SecurityIDInt).at(priceIndex).Orders) {
        OrderNos.emplace_back(it->OrderNo);
    }
    int idx = FindOrderNo(OrderNos, OrderNo);
    if (idx < 0) return Price;

    if (Volume > 0) {
        mapOrder.at(SecurityIDInt).at(priceIndex).Orders.at(idx)->Volume -= Volume;
    }
    if (Volume == 0 || mapOrder.at(SecurityIDInt).at(priceIndex).Orders.at(idx)->Volume <= 0) {
        auto order = mapOrder.at(SecurityIDInt).at(priceIndex).Orders.at(idx);
        DelOrderNoPrice(SecurityIDInt, order->OrderNo);
        mapOrder.at(SecurityIDInt).at(priceIndex).Orders.erase(mapOrder.at(SecurityIDInt).at(priceIndex).Orders.begin() + idx);
        m_pool.Free<stOrder>(order, sizeof(stOrder));

        if (Side == PROMD::TORA_TSTP_LSD_Buy) {
        } else {
        }
    }

    return Price;
}

void CApplication::FixOrderN(PROMD::TTORATstpSecurityIDType SecurityID, PROMD::TTORATstpLongSequenceType OrderNO,
                             PROMD::TTORATstpPriceType Price, PROMD::TTORATstpLongVolumeType Volume,
                             PROMD::TTORATstpLSideType Side) {

}

void CApplication::AddOrderIndexN(int SecurityIDInt, int idx, PROMD::TTORATstpTradeBSFlagType Side) {
    auto& orderIndex = Side == PROMD::TORA_TSTP_LSD_Buy?m_orderBuyNIndex:m_orderSellNIndex;
    auto iter = orderIndex.find(SecurityIDInt);
    if (iter == orderIndex.end()) {
        orderIndex[SecurityIDInt][idx] = 1;
    } else {
        auto it = orderIndex[SecurityIDInt].find(idx);
        if (it == orderIndex[SecurityIDInt].end()) {
            orderIndex[SecurityIDInt][idx] = 1;
        } else {
            orderIndex[SecurityIDInt][idx] += 1;
        }
    }
}

void CApplication::ModOrderIndexN(int SecurityIDInt, int idx, PROMD::TTORATstpTradeBSFlagType Side) {
    auto& orderIndex = Side == PROMD::TORA_TSTP_LSD_Buy?m_orderBuyNIndex:m_orderSellNIndex;
    auto iter = orderIndex.find(SecurityIDInt);
    if (iter == orderIndex.end()) return;
    auto it = orderIndex[SecurityIDInt].find(idx);
    if (it == orderIndex[SecurityIDInt].end()) return;
    orderIndex[SecurityIDInt][idx] -= 1;
    if (orderIndex[SecurityIDInt][idx] <= 0) {
        orderIndex[SecurityIDInt].erase(idx);
    }
    if (orderIndex[SecurityIDInt].empty()) {
        orderIndex.erase(SecurityIDInt);
    }
}

/***************************************TD***************************************/
void CApplication::TDOnRspQrySecurity(PROTD::CTORATstpSecurityField &Security) {
    auto iter = m_marketSecurity.find(Security.SecurityID);
    if (iter == m_marketSecurity.end()) {
        auto security = m_pool.Malloc<stSecurity>(sizeof(stSecurity));
        if (security) {
            strcpy(security->SecurityID, Security.SecurityID);
            security->ExchangeID = Security.ExchangeID;
            security->UpperLimitPrice = Security.UpperLimitPrice;
            security->LowerLimitPrice = Security.LowerLimitPrice;
            security->TotalIndex = int((Security.UpperLimitPrice - Security.LowerLimitPrice) / 0.01) + 1;
            m_marketSecurity[security->SecurityID] = security;
        }
    }

    if (m_watchSecurity.find(Security.SecurityID) != m_watchSecurity.end()) {
        m_watchSecurity[Security.SecurityID]->ExchangeID = Security.ExchangeID;
    }
}

void CApplication::TDOnInitFinished() {
    printf("CApplication::TDOnInitFinished\n");

    if (m_MD) return;
    InitOrderMapN();
    PROMD::TTORATstpExchangeIDType ExchangeID = PROMD::TORA_TSTP_EXD_SZSE;
    if (m_isSHExchange) ExchangeID = PROMD::TORA_TSTP_EXD_SSE;
    m_MD = new PROMD::MDL2Impl(this, ExchangeID);
    m_MD->Start(m_isTest);
}