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
        int SecurityIDInt = atoi(security->SecurityID);
        m_watchSecurity[SecurityIDInt] = security;
    }
}

void CApplication::Start() {
    if (m_dataDir.length() > 0) {
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
        ShowOrderBook((char*)iter.second->SecurityID);
    }

    m_timer.expires_from_now(boost::posix_time::milliseconds(6000));
    m_timer.async_wait(boost::bind(&CApplication::OnTime, this, boost::asio::placeholders::error));
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
    if (iter != m_orderNoPrice.end()) {
        auto it = iter->second.find(OrderNO);
        if (it != iter->second.end()) {
            printf("添加 %d 订单号 %lld 价格错误 已经存在该订单号价格 %.3f\n", SecurityIDInt, OrderNO, Price);
        }
    }
    m_orderNoPrice[SecurityIDInt][OrderNO] = Price;
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

void CApplication::AddHomebestOrder(int SecurityIDInt, PROMD::TTORATstpLongSequenceType OrderNO,
                                    PROMD::TTORATstpLongVolumeType Volume, PROMD::TTORATstpLSideType Side) {
    auto iter = m_homeBaseOrder.find(SecurityIDInt);
    if (iter != m_homeBaseOrder.end()) {
        auto it = iter->second.find(OrderNO);
        if (it != iter->second.end()) {
            printf("添加 %d 最优价单 %lld 错误 已经存在!!!\n", SecurityIDInt, OrderNO);
            it->second->Volume = Volume;
            it->second->Side = Side;
            return;
        }
    }
    auto homeBestOrder = m_pool.Malloc<stHomebestOrder>(sizeof(stHomebestOrder));
    homeBestOrder->SecurityIDInt = SecurityIDInt;
    homeBestOrder->OrderNO = OrderNO;
    homeBestOrder->Volume = Volume;
    homeBestOrder->Side = Side;
    m_homeBaseOrder[SecurityIDInt][OrderNO] = homeBestOrder;
}

stHomebestOrder* CApplication::GetHomebestOrder(int SecurityIDInt, PROMD::TTORATstpLongSequenceType OrderNO) {
    auto iter = m_homeBaseOrder.find(SecurityIDInt);
    if (iter == m_homeBaseOrder.end()) return nullptr;
    if (iter->second.find(OrderNO) == iter->second.end()) return nullptr;
    return iter->second[OrderNO];
}

void CApplication::DelHomebestOrder(int SecurityIDInt, PROMD::TTORATstpLongSequenceType OrderNO) {
    auto iter = m_homeBaseOrder.find(SecurityIDInt);
    if (iter == m_homeBaseOrder.end()) return;
    if (iter->second.find(OrderNO) == iter->second.end()) return;
    auto ptr = iter->second[OrderNO];
    iter->second.erase(OrderNO);
    m_pool.Free<stHomebestOrder>(ptr, sizeof(stHomebestOrder));
}

int CApplication::GetPriceIndex(int SecurityIDInt, PROMD::TTORATstpPriceType Price, PROMD::TTORATstpTradeBSFlagType Side) {
    auto priceIndex = -1;
    auto iter = m_marketSecurity.find(SecurityIDInt);
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

void CApplication::AddOrderPriceIndex(int SecurityIDInt, int idx, PROMD::TTORATstpTradeBSFlagType Side) {
    auto& orderIndex = Side==PROMD::TORA_TSTP_LSD_Buy?m_orderBuyNIndex:m_orderSellNIndex;
    auto iter = orderIndex.find(SecurityIDInt);
    if (iter == orderIndex.end()) {
        orderIndex[SecurityIDInt][idx] = 1;
    } else {
        auto it = iter->second.find(idx);
        if (it == iter->second.end()) {
            orderIndex[SecurityIDInt][idx] = 1;
        } else {
            it->second++;
        }
    }
}

void CApplication::ModOrderPriceIndex(int SecurityIDInt, int idx, PROMD::TTORATstpTradeBSFlagType Side) {
    auto& orderIndex = Side==PROMD::TORA_TSTP_LSD_Buy?m_orderBuyNIndex:m_orderSellNIndex;
    auto iter = orderIndex.find(SecurityIDInt);
    if (iter == orderIndex.end()) return;
    auto it = iter->second.find(idx);
    if (it == iter->second.end()) return;
    it->second -= 1;
    if (it->second <= 0) {
        orderIndex[SecurityIDInt].erase(idx);
    }
    if (orderIndex[SecurityIDInt].empty()) {
        orderIndex.erase(SecurityIDInt);
    }
}

void CApplication::InsertOrderN(int SecurityIDInt, PROMD::TTORATstpLongSequenceType OrderNO, PROMD::TTORATstpPriceType Price,
                                PROMD::TTORATstpLongVolumeType Volume, PROMD::TTORATstpLSideType Side) {
    auto priceIndex = GetPriceIndex(SecurityIDInt, Price, Side);
    if (priceIndex < 0) return;

    AddOrderNoPrice(SecurityIDInt, OrderNO, Price);
    AddOrderPriceIndex(SecurityIDInt, priceIndex, Side);

    auto order = m_pool.Malloc<stOrder>(sizeof(stOrder));
    order->OrderNo = OrderNO;
    order->Volume = Volume;
    auto& mapOrder = Side == PROMD::TORA_TSTP_LSD_Buy?m_orderBuyN:m_orderSellN;
    mapOrder.at(SecurityIDInt).at(priceIndex).Orders.emplace_back(order);
}

double CApplication::ModifyOrderN(int SecurityIDInt, PROMD::TTORATstpLongVolumeType Volume,
                                PROMD::TTORATstpLongSequenceType OrderNo, PROMD::TTORATstpTradeBSFlagType Side) {
    auto Price = GetOrderNoPrice(SecurityIDInt, OrderNo);
    if (Price < 0.000001) return Price;
    auto priceIndex = GetPriceIndex(SecurityIDInt, Price, Side);
    if (priceIndex < 0) return Price;

    auto& mapOrder = Side == PROMD::TORA_TSTP_LSD_Buy?m_orderBuyN:m_orderSellN;
    auto iter = mapOrder.at(SecurityIDInt).at(priceIndex).Orders.begin();
    for (;iter != mapOrder.at(SecurityIDInt).at(priceIndex).Orders.end(); ++iter) {
        if ((*iter)->OrderNo == OrderNo) {
            break;
        }
    }

    if (Volume > 0) {
        (*iter)->Volume -= Volume;
    }
    if (Volume == 0 || (*iter)->Volume <= 0) {
        auto order = (*iter);
        DelOrderNoPrice(SecurityIDInt, order->OrderNo);
        ModOrderPriceIndex(SecurityIDInt, priceIndex, Side);
        mapOrder.at(SecurityIDInt).at(priceIndex).Orders.erase(iter);
        m_pool.Free<stOrder>(order, sizeof(stOrder));
    }

    //std::vector<PROMD::TTORATstpLongSequenceType> OrderNos;
    //for (auto it : mapOrder.at(SecurityIDInt).at(priceIndex).Orders) {
    //    OrderNos.emplace_back(it->OrderNo);
    //}
    //int idx = FindOrderNo(OrderNos, OrderNo);
    //int idx = -1;
    //for (int i = 0; i < mapOrder.at(SecurityIDInt).at(priceIndex).Orders.size(); ++i) {
    //    if (mapOrder.at(SecurityIDInt).at(priceIndex).Orders.at(i)->OrderNo == OrderNo) {
    //        idx = i;
    //        break;
    //    }
    //}

    //if (idx < 0) return Price;
    //if (Volume > 0) {
    //    mapOrder.at(SecurityIDInt).at(priceIndex).Orders.at(idx)->Volume -= Volume;
    //}
    //if (Volume == 0 || mapOrder.at(SecurityIDInt).at(priceIndex).Orders.at(idx)->Volume <= 0) {
    //    auto order = mapOrder.at(SecurityIDInt).at(priceIndex).Orders.at(idx);
    //    DelOrderNoPrice(SecurityIDInt, order->OrderNo);
    //    ModOrderPriceIndex(SecurityIDInt, priceIndex, Side);
    //    mapOrder.at(SecurityIDInt).at(priceIndex).Orders.erase(mapOrder.at(SecurityIDInt).at(priceIndex).Orders.begin() + idx);
    //    m_pool.Free<stOrder>(order, sizeof(stOrder));
    //}
    return Price;
}

void CApplication::FixOrderN(int SecurityIDInt, PROMD::TTORATstpLongSequenceType OrderNO, PROMD::TTORATstpPriceType Price, PROMD::TTORATstpLSideType Side) {

}

/***************************************TD***************************************/
void CApplication::TDOnRspQrySecurity(PROTD::CTORATstpSecurityField &Security) {
    int SecurityIDInt = atoi(Security.SecurityID);
    auto iter = m_marketSecurity.find(SecurityIDInt);
    if (iter == m_marketSecurity.end()) {
        auto security = m_pool.Malloc<stSecurity>(sizeof(stSecurity));
        if (security) {
            strcpy(security->SecurityID, Security.SecurityID);
            security->ExchangeID = Security.ExchangeID;
            security->UpperLimitPrice = Security.UpperLimitPrice;
            security->LowerLimitPrice = Security.LowerLimitPrice;
            security->TotalIndex = int((Security.UpperLimitPrice - Security.LowerLimitPrice) / 0.01) + 1;
            m_marketSecurity[SecurityIDInt] = security;
        }
        if (m_watchSecurity.find(SecurityIDInt) != m_watchSecurity.end()) {
            strcpy(m_watchSecurity[SecurityIDInt]->SecurityID, Security.SecurityID);
            m_watchSecurity[SecurityIDInt]->ExchangeID = Security.ExchangeID;
        }
    }
}

void CApplication::TDOnInitFinished() {
    printf("CApplication::TDOnInitFinished\n");

    if (m_MD) return;
    InitOrderMap();
    PROMD::TTORATstpExchangeIDType ExchangeID = PROMD::TORA_TSTP_EXD_SZSE;
    if (m_isSHExchange) ExchangeID = PROMD::TORA_TSTP_EXD_SSE;
    m_MD = new PROMD::MDL2Impl(this, ExchangeID);
    m_MD->Start(m_isTest);
}

/***************************************MD***************************************/
void CApplication::InitOrderMap() {
    m_orderBuyN.resize(999999);
    m_orderSellN.resize(999999);

    for (auto& it : m_marketSecurity) {
        auto SecurityIDInt = atoi(it.second->SecurityID);
        m_orderBuyN.at(SecurityIDInt).resize(it.second->TotalIndex);
        m_orderSellN.at(SecurityIDInt).resize(it.second->TotalIndex);
        for (auto i = 0; i < it.second->TotalIndex; ++i) {
            m_orderBuyN.at(SecurityIDInt).at(i).Price = it.second->UpperLimitPrice - i * 0.01;
            m_orderSellN.at(SecurityIDInt).at(i).Price = it.second->LowerLimitPrice + i * 0.01;
        }
    }
}

void CApplication::MDOnInitFinished(PROMD::TTORATstpExchangeIDType ExchangeID) {
    auto cnt = 0;
    for (auto &iter: m_marketSecurity) {
        if (ExchangeID == iter.second->ExchangeID) {
            cnt++;
            PROMD::TTORATstpSecurityIDType SecurityID = {0};
            strncpy(SecurityID, iter.second->SecurityID, sizeof(SecurityID));
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

    int SecurityIDInt = atoi(OrderDetail.SecurityID);
    if (OrderDetail.ExchangeID == PROMD::TORA_TSTP_EXD_SZSE) {
        if (OrderDetail.OrderType == PROMD::TORA_TSTP_LOT_HomeBest) {
            AddHomebestOrder(SecurityIDInt, OrderDetail.OrderNO, OrderDetail.Volume, OrderDetail.Side);
            return;
        }
        InsertOrderN(SecurityIDInt, OrderDetail.OrderNO, OrderDetail.Price, OrderDetail.Volume, OrderDetail.Side);
    }
    else if (OrderDetail.ExchangeID == PROMD::TORA_TSTP_EXD_SSE) {
        if (OrderDetail.OrderStatus == PROMD::TORA_TSTP_LOS_Add) {
            InsertOrderN(SecurityIDInt, OrderDetail.OrderNO, OrderDetail.Price, OrderDetail.Volume, OrderDetail.Side);
        } else if (OrderDetail.OrderStatus == PROMD::TORA_TSTP_LOS_Delete) {
            ModifyOrderN(SecurityIDInt, 0, OrderDetail.OrderNO, OrderDetail.Side);
        }
    }
}

void CApplication::MDOnRtnTransaction(PROMD::CTORATstpLev2TransactionField &Transaction) {
    int SecurityIDInt = atoi(Transaction.SecurityID);
    if (Transaction.ExchangeID == PROMD::TORA_TSTP_EXD_SZSE) {
        if (Transaction.ExecType == PROMD::TORA_TSTP_ECT_Fill) {
            {
                auto homeBestBuyOrder = GetHomebestOrder(SecurityIDInt, Transaction.BuyNo);
                if (homeBestBuyOrder) {
                    homeBestBuyOrder->Volume -= Transaction.TradeVolume;
                    if (homeBestBuyOrder->Volume > 0) {
                        InsertOrderN(SecurityIDInt, homeBestBuyOrder->OrderNO, Transaction.TradePrice, homeBestBuyOrder->Volume, homeBestBuyOrder->Side);
                    }
                    DelHomebestOrder(SecurityIDInt, homeBestBuyOrder->OrderNO);
                } else {
                    auto BuyPrice = ModifyOrderN(SecurityIDInt, Transaction.TradeVolume, Transaction.BuyNo, PROMD::TORA_TSTP_LSD_Buy);
                    FixOrderN(SecurityIDInt, Transaction.BuyNo, BuyPrice, PROMD::TORA_TSTP_LSD_Buy);
                }
            }
            {
                auto homeBestSellOrder = GetHomebestOrder(SecurityIDInt, Transaction.SellNo);
                if (homeBestSellOrder) {
                    homeBestSellOrder->Volume -= Transaction.TradeVolume;
                    if (homeBestSellOrder->Volume > 0) {
                        InsertOrderN(SecurityIDInt, homeBestSellOrder->OrderNO, Transaction.TradePrice, homeBestSellOrder->Volume, homeBestSellOrder->Side);
                    }
                    DelHomebestOrder(SecurityIDInt, homeBestSellOrder->OrderNO);
                } else {
                    auto SellPrice = ModifyOrderN(SecurityIDInt, Transaction.TradeVolume, Transaction.SellNo, PROMD::TORA_TSTP_LSD_Sell);
                    FixOrderN(SecurityIDInt, Transaction.SellNo, SellPrice, PROMD::TORA_TSTP_LSD_Sell);
                }
            }
        } else if (Transaction.ExecType == PROMD::TORA_TSTP_ECT_Cancel) {
            if (Transaction.BuyNo > 0) {
                ModifyOrderN(SecurityIDInt, 0, Transaction.BuyNo, PROMD::TORA_TSTP_LSD_Buy);
            } else if (Transaction.SellNo > 0) {
                ModifyOrderN(SecurityIDInt, 0, Transaction.SellNo, PROMD::TORA_TSTP_LSD_Sell);
            }
        }
    }
    else if (Transaction.ExchangeID == PROMD::TORA_TSTP_EXD_SSE) {
        auto BuyPrice = ModifyOrderN(SecurityIDInt, Transaction.TradeVolume, Transaction.BuyNo, PROMD::TORA_TSTP_LSD_Buy);
        FixOrderN(SecurityIDInt, Transaction.BuyNo, BuyPrice, PROMD::TORA_TSTP_LSD_Buy);
        auto SellPrice = ModifyOrderN(SecurityIDInt, Transaction.TradeVolume, Transaction.SellNo, PROMD::TORA_TSTP_LSD_Sell);
        FixOrderN(SecurityIDInt, Transaction.BuyNo, BuyPrice, PROMD::TORA_TSTP_LSD_Buy);
    }
}

void CApplication::MDOnRtnNGTSTick(PROMD::CTORATstpLev2NGTSTickField &Tick) {
    if (Tick.ExchangeID != PROMD::TORA_TSTP_EXD_SSE) return;
    if (Tick.Side != PROMD::TORA_TSTP_LSD_Buy && Tick.Side != PROMD::TORA_TSTP_LSD_Sell) return;

    int SecurityIDInt = atoi(Tick.SecurityID);
    if (Tick.TickType == PROMD::TORA_TSTP_LTT_Add) {
        InsertOrderN(SecurityIDInt, Tick.Side==PROMD::TORA_TSTP_LSD_Buy?Tick.BuyNo:Tick.SellNo, Tick.Price, Tick.Volume, Tick.Side);
    } else if (Tick.TickType == PROMD::TORA_TSTP_LTT_Delete) {
        ModifyOrderN(SecurityIDInt, 0, Tick.Side==PROMD::TORA_TSTP_LSD_Buy?Tick.BuyNo:Tick.SellNo, Tick.Side);
    } else if (Tick.TickType == PROMD::TORA_TSTP_LTT_Trade) {
        auto BuyPrice = ModifyOrderN(SecurityIDInt, Tick.Volume, Tick.BuyNo, PROMD::TORA_TSTP_LSD_Buy);
        FixOrderN(SecurityIDInt, Tick.BuyNo, BuyPrice, PROMD::TORA_TSTP_LSD_Buy);
        auto SellPrice = ModifyOrderN(SecurityIDInt, Tick.Volume, Tick.SellNo, PROMD::TORA_TSTP_LSD_Sell);
        FixOrderN(SecurityIDInt, Tick.SellNo, SellPrice, PROMD::TORA_TSTP_LSD_Sell);
    }
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

