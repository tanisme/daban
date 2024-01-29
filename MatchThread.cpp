//
// Created by tangang on 2024/1/29.
//

#include "MatchThread.h"
#include "L2Impl.h"
#include "TickTradingFramework.h"

MatchThread::MatchThread(CMDL2Impl* mdImpl, int threadIndex) :
    m_mdImpl(mdImpl),
    m_threadIndex(threadIndex) {
}

MatchThread::~MatchThread() {
    if (m_thread && m_thread->joinable()) {
        m_thread->join();
    }
}

void MatchThread::Start() {
    if (m_thread == nullptr) {
        m_thread = new std::thread(std::bind(&MatchThread::Run, this));
    }
}

void MatchThread::PushMessage(TTF_Message_t *message) {
    m_queue.enqueue(message);
}

void MatchThread::Run() {
    while (true) {
        TTF_Message_t * message = nullptr;
        if (m_queue.try_dequeue(message)) {
            switch (message->MsgType)
            {
                case MsgTypeRtnOrderDetail:
                {
                    auto pMsgRtnOrderDetail = (MsgRtnOrderDetail_t*)&message->Message;
                    MDOnRtnOrderDetail(pMsgRtnOrderDetail->OrderDetail);
                }
                    break;
                case MsgTypeRtnTransaction:
                {
                    auto pMsgRtnTransaction = (MsgRtnTransaction_t*)&message->Message;
                    MDOnRtnTransaction(pMsgRtnTransaction->Transaction);
                }
                    break;
                case MsgTypeRtnNGTSTick:
                {
                    auto pMsgRtnNGTSTick = (MsgRtnNGTSTick_t*)&message->Message;
                    MDOnRtnNGTSTick(pMsgRtnNGTSTick->NGTSTick);
                }
                    break;
                default:
                    break;
            };

        }
    }
}

void MatchThread::MDOnRtnOrderDetail(CTORATstpLev2OrderDetailField& OrderDetail)
{
    //current_time = OrderDetail.MainSeq;
    auto it = m_securityid_lastprice.find(OrderDetail.SecurityID);
    if (it == m_securityid_lastprice.end()) {
        m_securityid_lastprice[OrderDetail.SecurityID] = DBL_MAX;
    }

    int SecurityIDInt = atoi(OrderDetail.SecurityID);
    m_notifyCount[SecurityIDInt]++;

    if (OrderDetail.OrderType == TORA_TSTP_LOT_Market) return;
    if (OrderDetail.ExchangeID != TORA_TSTP_EXD_SZSE) return;
    if (OrderDetail.Side != TORA_TSTP_LSD_Buy && OrderDetail.Side != TORA_TSTP_LSD_Sell) return;
    if (OrderDetail.OrderType == TORA_TSTP_LOT_HomeBest) {
        AddHomebestOrder(SecurityIDInt, OrderDetail.OrderNO, OrderDetail.Volume, OrderDetail.Side);
        return;
    }
    InsertOrderN(SecurityIDInt, OrderDetail.OrderNO, OrderDetail.Price, OrderDetail.Volume, OrderDetail.Side);
    PostPrice(OrderDetail.SecurityID, OrderDetail.ExchangeID);
}

void MatchThread::MDOnRtnTransaction(CTORATstpLev2TransactionField& Transaction)
{
    //current_time = Transaction.MainSeq;
    if (Transaction.ExecType == TORA_TSTP_ECT_Fill) {
        m_securityid_lastprice[Transaction.SecurityID] = Transaction.TradePrice;
    }

    int SecurityIDInt = atoi(Transaction.SecurityID);
    m_notifyCount[SecurityIDInt]++;

    if (Transaction.ExchangeID != TORA_TSTP_EXD_SZSE) return;
    if (Transaction.ExecType == TORA_TSTP_ECT_Fill) {
        auto homeBestBuyOrder = GetHomebestOrder(SecurityIDInt, Transaction.BuyNo);
        if (homeBestBuyOrder) {
            homeBestBuyOrder->Volume -= Transaction.TradeVolume;
            if (homeBestBuyOrder->Volume > 0) {
                InsertOrderN(SecurityIDInt, Transaction.BuyNo, Transaction.TradePrice, homeBestBuyOrder->Volume, homeBestBuyOrder->Side, true);
            }
            DelHomebestOrder(SecurityIDInt, Transaction.BuyNo);
        } else {
            auto priceIndex = ModifyOrderN(SecurityIDInt, Transaction.TradeVolume, Transaction.BuyNo, TORA_TSTP_LSD_Buy);
            DeleteOrderN(SecurityIDInt, priceIndex, Transaction.BuyNo, TORA_TSTP_LSD_Buy);
        }
        auto homeBestSellOrder = GetHomebestOrder(SecurityIDInt, Transaction.SellNo);
        if (homeBestSellOrder) {
            homeBestSellOrder->Volume -= Transaction.TradeVolume;
            if (homeBestSellOrder->Volume > 0) {
                InsertOrderN(SecurityIDInt, Transaction.SellNo, Transaction.TradePrice, homeBestSellOrder->Volume, homeBestSellOrder->Side, true);
            }
            DelHomebestOrder(SecurityIDInt, Transaction.SellNo);
        } else {
            auto priceIndex = ModifyOrderN(SecurityIDInt, Transaction.TradeVolume, Transaction.SellNo, TORA_TSTP_LSD_Sell);
            DeleteOrderN(SecurityIDInt, priceIndex, Transaction.SellNo, TORA_TSTP_LSD_Sell);
        }
    } else if (Transaction.ExecType == TORA_TSTP_ECT_Cancel) {
        if (Transaction.BuyNo > 0) {
            ModifyOrderN(SecurityIDInt, 0, Transaction.BuyNo, TORA_TSTP_LSD_Buy);
        } else if (Transaction.SellNo > 0) {
            ModifyOrderN(SecurityIDInt, 0, Transaction.SellNo, TORA_TSTP_LSD_Sell);
        }
    }
    PostPrice(Transaction.SecurityID, Transaction.ExchangeID);
}

void MatchThread::MDOnRtnNGTSTick(CTORATstpLev2NGTSTickField& Tick)
{
    //current_time = Tick.MainSeq;
    if (Tick.TickType == TORA_TSTP_LTT_Trade) {
        m_securityid_lastprice[Tick.SecurityID] = Tick.Price;
    }

    int SecurityIDInt = atoi(Tick.SecurityID);
    m_notifyCount[SecurityIDInt]++;

    if (Tick.ExchangeID != TORA_TSTP_EXD_SSE) return;
    if (Tick.TickType == TORA_TSTP_LTT_Add) {
        InsertOrderN(SecurityIDInt, Tick.Side==TORA_TSTP_LSD_Buy?Tick.BuyNo:Tick.SellNo, Tick.Price, Tick.Volume, Tick.Side);
    } else if (Tick.TickType == TORA_TSTP_LTT_Delete) {
        ModifyOrderN(SecurityIDInt, 0, Tick.Side==TORA_TSTP_LSD_Buy?Tick.BuyNo:Tick.SellNo, Tick.Side);
    } else if (Tick.TickType == TORA_TSTP_LTT_Trade) {
        auto buyPriceIndex = ModifyOrderN(SecurityIDInt, Tick.Volume, Tick.BuyNo, TORA_TSTP_LSD_Buy);
        DeleteOrderN(SecurityIDInt, buyPriceIndex, Tick.BuyNo, TORA_TSTP_LSD_Buy);
        auto sellPriceIndex = ModifyOrderN(SecurityIDInt, Tick.Volume, Tick.SellNo, TORA_TSTP_LSD_Sell);
        DeleteOrderN(SecurityIDInt, sellPriceIndex, Tick.SellNo, TORA_TSTP_LSD_Sell);
    }
    PostPrice(Tick.SecurityID, Tick.ExchangeID);
}

void MatchThread::InitOrderMap() {
    m_orderBuyN.resize(999999);
    m_orderSellN.resize(999999);

    for (size_t i = 0; i < TTF_INSTRUMENT_ARRAY_SIZE; i++) {
        TORASTOCKAPI::CTORATstpSecurityField* pInstrument = m_mdImpl->GetFrame()->m_pInstruments + i;
        if (pInstrument->SecurityID[0] == '\0')
            continue;
        //if (!IsTradingExchange(pInstrument->ExchangeID))
        //    continue;
        auto SecurityIDInt = atoi(pInstrument->SecurityID);
        auto TotalIndex = GetTotalIndex(pInstrument->UpperLimitPrice, pInstrument->LowerLimitPrice);
        m_orderBuyN.at(SecurityIDInt).resize(TotalIndex);
        m_orderSellN.at(SecurityIDInt).resize(TotalIndex);
        for (auto i = 0; i < TotalIndex; ++i) {
            m_orderBuyN.at(SecurityIDInt).at(i).Price = pInstrument->UpperLimitPrice - i * 0.01;
            m_orderSellN.at(SecurityIDInt).at(i).Price = pInstrument->LowerLimitPrice + i * 0.01;
        }
    }
}

double MatchThread::GetOrderNoToPrice(int SecurityIDInt, TTORATstpLongSequenceType OrderNO) {
    auto Price = 0.0;
    auto iter = m_orderNoToPrice.find(SecurityIDInt);
    if (iter != m_orderNoToPrice.end()) {
        auto it = iter->second.find(OrderNO);
        if (it != iter->second.end()) {
            Price = it->second;
        }
    }
    return Price;
}

void MatchThread::AddOrderNoToPrice(int SecurityIDInt, TTORATstpLongSequenceType OrderNO, TTORATstpPriceType Price) {
    m_orderNoToPrice[SecurityIDInt][OrderNO] = Price;
}

void MatchThread::DelOrderNoPrice(int SecurityIDInt, TTORATstpLongSequenceType OrderNO) {
    auto iter = m_orderNoToPrice.find(SecurityIDInt);
    if (iter == m_orderNoToPrice.end()) return;
    auto it = iter->second.find(OrderNO);
    if (it == iter->second.end()) return;
    iter->second.erase(it);
}

void MatchThread::AddHomebestOrder(int SecurityIDInt, TTORATstpLongSequenceType OrderNO,
                                             TTORATstpLongVolumeType Volume, TTORATstpLSideType Side) {
    auto iter = m_homeBaseOrder.find(SecurityIDInt);
    if (iter != m_homeBaseOrder.end()) {
        auto it = iter->second.find(OrderNO);
        if (it != iter->second.end()) {
            it->second->Volume = Volume;
            it->second->Side = Side;
            return;
        }
    }
    auto homeBestOrder = m_pool.Malloc<stHomebestOrder>(sizeof(stHomebestOrder));
    homeBestOrder->Volume = Volume;
    homeBestOrder->Side = Side;
    m_homeBaseOrder[SecurityIDInt][OrderNO] = homeBestOrder;
}

stHomebestOrder* MatchThread::GetHomebestOrder(int SecurityIDInt, TTORATstpLongSequenceType OrderNO) {
    auto iter = m_homeBaseOrder.find(SecurityIDInt);
    if (iter == m_homeBaseOrder.end()) return nullptr;
    auto it = iter->second.find(OrderNO);
    if (it == iter->second.end()) return nullptr;
    return it->second;
}

void MatchThread::DelHomebestOrder(int SecurityIDInt, TTORATstpLongSequenceType OrderNO) {
    auto iter = m_homeBaseOrder.find(SecurityIDInt);
    if (iter == m_homeBaseOrder.end()) return;
    auto it = iter->second.find(OrderNO);
    if (it == iter->second.end()) return;
    auto ptr = it->second;
    iter->second.erase(it);
    m_pool.Free<stHomebestOrder>(ptr, sizeof(stHomebestOrder));
}

int MatchThread::GetHTLPriceIndex(int SecurityIDInt, TTORATstpPriceType Price, TTORATstpTradeBSFlagType Side) {
    TORASTOCKAPI::CTORATstpSecurityField* pInstrument = m_mdImpl->GetFrame()->m_pInstruments + SecurityIDInt;

    auto tempIndex = -2.0;
    if (Side == TORA_TSTP_LSD_Buy) {
        auto diffPrice = pInstrument->UpperLimitPrice - Price;
        tempIndex = diffPrice / 0.01;
    } else {
        auto diffPrice = Price - pInstrument->LowerLimitPrice;
        tempIndex = diffPrice / 0.01;
    }

    auto priceIndex = int(tempIndex);
    if (priceIndex < 0) return -1;
    if (tempIndex - priceIndex >= 0.5) priceIndex++;
    auto TotalIndex = GetTotalIndex(pInstrument->UpperLimitPrice, pInstrument->LowerLimitPrice);
    if (priceIndex >= TotalIndex || priceIndex < 0) {
        priceIndex = -1;
    }
    return priceIndex;
}

void MatchThread::AddOrderPriceIndex(int SecurityIDInt, int priceIndex, TTORATstpTradeBSFlagType Side) {
    auto& orderIndex = Side==TORA_TSTP_LSD_Buy?m_buyPriceIndex:m_sellPriceIndex;
    orderIndex[SecurityIDInt].insert(priceIndex);
}

void MatchThread::DelOrderPriceIndex(int SecurityIDInt, int priceIndex, TTORATstpTradeBSFlagType Side) {
    auto& orderIndex = Side==TORA_TSTP_LSD_Buy?m_buyPriceIndex:m_sellPriceIndex;
    auto iter = orderIndex.find(SecurityIDInt);
    if (iter == orderIndex.end()) return;
    auto it = iter->second.find(priceIndex);
    if (it == iter->second.end()) return;
    iter->second.erase(it);
    if (iter->second.empty()) {
        orderIndex.erase(iter);
    }
}

void MatchThread::InsertOrderN(int SecurityIDInt, TTORATstpLongSequenceType OrderNO, TTORATstpPriceType Price,
                                         TTORATstpLongVolumeType Volume, TTORATstpLSideType Side, bool findPos) {
    auto priceIndex = GetHTLPriceIndex(SecurityIDInt, Price, Side);
    if (priceIndex < 0) {
        return;
    }

    AddOrderNoToPrice(SecurityIDInt, OrderNO, Price);
    AddOrderPriceIndex(SecurityIDInt, priceIndex, Side);

    auto order = m_pool.Malloc<stOrder>(sizeof(stOrder));
    order->OrderNo = OrderNO;
    order->Volume = Volume;
    auto& mapOrder = Side==TORA_TSTP_LSD_Buy?m_orderBuyN:m_orderSellN;
    if (!findPos) {
        mapOrder.at(SecurityIDInt).at(priceIndex).Orders.emplace_back(order);
        return;
    }

    auto size = (int)mapOrder.at(SecurityIDInt).at(priceIndex).Orders.size();
    if (size == 0) {
        mapOrder.at(SecurityIDInt).at(priceIndex).Orders.emplace_back(order);
        return;
    }

    for (auto i = 0 ; i < size; ++i) {
        auto curOrder = mapOrder.at(SecurityIDInt).at(priceIndex).Orders.at(i);
        if (curOrder->OrderNo > OrderNO) {
            mapOrder.at(SecurityIDInt).at(priceIndex).Orders.insert(mapOrder.at(SecurityIDInt).at(priceIndex).Orders.begin() + i, order);
            break;
        }
    }
}

int MatchThread::ModifyOrderN(int SecurityIDInt, TTORATstpLongVolumeType Volume,
                                        TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType Side) {
    auto Price = GetOrderNoToPrice(SecurityIDInt, OrderNo);
    if (Price < 0.000001) {
        return -1;
    }

    auto priceIndex = GetHTLPriceIndex(SecurityIDInt, Price, Side);
    if (priceIndex < 0) {
        return -1;
    }

    auto& sidePriceIndex = Side==TORA_TSTP_LSD_Buy?m_buyPriceIndex:m_sellPriceIndex;
    auto priceIndexIter = sidePriceIndex.find(SecurityIDInt);
    if (priceIndexIter == sidePriceIndex.end()) {
        return priceIndex;
    }

    if (priceIndexIter->second.find(priceIndex) == priceIndexIter->second.end()) {
        return priceIndex;
    }

    auto& mapOrder = Side==TORA_TSTP_LSD_Buy?m_orderBuyN:m_orderSellN;
    auto idx = FindPriceIndexOrderNo(mapOrder.at(SecurityIDInt).at(priceIndex).Orders, OrderNo);
    if (idx < 0) {
        return priceIndex;
    }

    auto iter = mapOrder.at(SecurityIDInt).at(priceIndex).Orders.begin() + idx;
    if (iter == mapOrder.at(SecurityIDInt).at(priceIndex).Orders.end()) {
        return priceIndex;
    }

    if (Volume > 0) {
        (*iter)->Volume -= Volume;
    }

    auto rmPriceIndex = false;
    if (Volume == 0 || (*iter)->Volume <= 0) {
        DelOrderNoPrice(SecurityIDInt, OrderNo);
        auto order = *iter;
        mapOrder.at(SecurityIDInt).at(priceIndex).Orders.erase(iter);
        m_pool.Free<stOrder>(order, sizeof(stOrder));
        rmPriceIndex = mapOrder.at(SecurityIDInt).at(priceIndex).Orders.empty();
    }

    if (rmPriceIndex) {
        DelOrderPriceIndex(SecurityIDInt, priceIndex, Side);
    }
    return priceIndex;
}

void MatchThread::DeleteOrderN(int SecurityIDInt, int priceIndex,
                                         TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType Side) {
    if (priceIndex < 0) return;
    auto& sidePriceIndex = Side==TORA_TSTP_LSD_Buy?m_buyPriceIndex:m_sellPriceIndex;
    auto priceIndexIter = sidePriceIndex.find(SecurityIDInt);
    if (priceIndexIter == sidePriceIndex.end()) return;

    std::vector<int> toRemovePriceIndex;
    auto& mapOrder = Side==TORA_TSTP_LSD_Buy?m_orderBuyN:m_orderSellN;
    for (auto it = priceIndexIter->second.begin(); it != priceIndexIter->second.end(); ++it) {
        auto curPriceIndex = *it;
        if (curPriceIndex == priceIndex) {
            auto iter = mapOrder.at(SecurityIDInt).at(curPriceIndex).Orders.begin();
            for (;iter != mapOrder.at(SecurityIDInt).at(curPriceIndex).Orders.end();) {
                auto order = (*iter);
                if (order->OrderNo < OrderNo) {
                    DelOrderNoPrice(SecurityIDInt, order->OrderNo);
                    iter = mapOrder.at(SecurityIDInt).at(curPriceIndex).Orders.erase(iter);
                    m_pool.Free<stOrder>(order, sizeof(stOrder));
                } else {
                    break;
                }
            }
            if (mapOrder.at(SecurityIDInt).at(curPriceIndex).Orders.empty()) {
                toRemovePriceIndex.push_back(curPriceIndex);
            }
        } else {
            if (curPriceIndex < priceIndex) {
                auto iter = mapOrder.at(SecurityIDInt).at(curPriceIndex).Orders.begin();
                for (;iter != mapOrder.at(SecurityIDInt).at(curPriceIndex).Orders.end(); ++iter) {
                    auto order = (*iter);
                    DelOrderNoPrice(SecurityIDInt, order->OrderNo);
                    m_pool.Free<stOrder>(order, sizeof(stOrder));
                }
                mapOrder.at(SecurityIDInt).at(curPriceIndex).Orders.clear();
                toRemovePriceIndex.push_back(curPriceIndex);
            } else {
                break;
            }
        }
    }

    for (auto it : toRemovePriceIndex) {
        DelOrderPriceIndex(SecurityIDInt, it, Side);
    }
}

void MatchThread::PostPrice(TTORATstpSecurityIDType SecurityID, TTORATstpExchangeIDType ExchangeID) {
    //auto itprice = m_securityid_lastprice.find(SecurityID);
    //if (itprice == m_securityid_lastprice.end() || itprice->second > 999999999.0) return;

    //TORALEV1API::CTORATstpMarketDataField field = {};
    //strcpy(field.SecurityID, SecurityID);
    //field.ExchangeID = ExchangeID;
    //field.LastPrice = itprice->second;

    //int maxidx = 5;
    //int SecurityIDInt = atoi(SecurityID);
    //{
    //    auto iter = m_sellPriceIndex.find(SecurityIDInt);
    //    if (iter != m_sellPriceIndex.end()) {
    //        int idx = 1;
    //        for (auto it : iter->second) {
    //            auto priceIndex = it;
    //            const auto& orders = m_orderSellN.at(SecurityIDInt).at(priceIndex);
    //            auto totalVolume = 0ll;
    //            for (auto iter1 : orders.Orders) {
    //                totalVolume += iter1->Volume;
    //            }
    //            switch (idx) {
    //                case 1:{
    //                    field.AskPrice1 = orders.Price;
    //                    field.AskVolume1 = totalVolume;
    //                } break;
    //                case 2:{
    //                    field.AskPrice2 = orders.Price;
    //                    field.AskVolume2 = totalVolume;
    //                } break;
    //                case 3:{
    //                    field.AskPrice3 = orders.Price;
    //                    field.AskVolume3 = totalVolume;
    //                } break;
    //                case 4:{
    //                    field.AskPrice4 = orders.Price;
    //                    field.AskVolume4 = totalVolume;
    //                } break;
    //                case 5:{
    //                    field.AskPrice5 = orders.Price;
    //                    field.AskVolume5 = totalVolume;
    //                } break;
    //            }
    //            if (idx++ >= maxidx)
    //                break;
    //        }
    //    }
    //}
    //{
    //    auto iter = m_buyPriceIndex.find(SecurityIDInt);
    //    if (iter != m_buyPriceIndex.end()) {
    //        int idx = 1;
    //        for (auto it : iter->second) {
    //            auto priceIndex = it;
    //            const auto& orders = m_orderBuyN.at(SecurityIDInt).at(priceIndex);
    //            auto totalVolume = 0ll;
    //            for (auto iter1 : orders.Orders) {
    //                totalVolume += iter1->Volume;
    //            }
    //            switch (idx) {
    //                case 1:{
    //                    field.BidPrice1 = orders.Price;
    //                    field.BidVolume1 = totalVolume;
    //                } break;
    //                case 2:{
    //                    field.BidPrice2 = orders.Price;
    //                    field.BidVolume2 = totalVolume;
    //                } break;
    //                case 3:{
    //                    field.BidPrice3 = orders.Price;
    //                    field.BidVolume3 = totalVolume;
    //                } break;
    //                case 4:{
    //                    field.BidPrice4 = orders.Price;
    //                    field.BidVolume4 = totalVolume;
    //                } break;
    //                case 5:{
    //                    field.BidPrice5 = orders.Price;
    //                    field.BidVolume5 = totalVolume;
    //                } break;
    //            }
    //            if (idx++ >= maxidx)
    //                break;
    //        }
    //    }
    //}

    //OnRtnMarketData(field);
}

//void MatchThread::OnRtnMarketData(TORALEV1API::CTORATstpMarketDataField& MarketData)
//{
//    //// 找到与合约关联的策略，逐一回调
//    //auto iter = m_mRelatedStrategies.find(MarketData.SecurityID);
//    //if (iter != m_mRelatedStrategies.end()) {
//    //    for (auto iterStrategy = iter->second.begin(); iterStrategy != iter->second.end(); iterStrategy++)
//    //        (*iterStrategy)->OnRtnMarketData(MarketData);
//    //}
//}

int MatchThread::FindPriceIndexOrderNo(std::vector<stOrder *> &orderNOs,
                                                 TORALEV2API::TTORATstpLongSequenceType OrderNo) {
    int left = 0;
    int right = (int)orderNOs.size() - 1;
    while (left <= right) {
        int middle = left + ((right - left) / 2);
        if (orderNOs.at(middle)->OrderNo > OrderNo) {
            right = middle - 1;
        } else if (orderNOs.at(middle)->OrderNo < OrderNo) {
            left = middle + 1;
        } else {
            return middle;
        }
    }
    return -1;
}