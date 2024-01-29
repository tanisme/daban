//
// Created by tangang on 2024/1/29.
//

#ifndef BALIBALI_HFT_MATCHTHREAD_H
#define BALIBALI_HFT_MATCHTHREAD_H

#include <thread>
#include <atomic>
#include <memory>
#include <unordered_map>
#include <set>

#include "balibali.h"
#include "defines.h"
#include "MemoryPool.h"
#include "concurrentqueue/concurrentqueue.h"

using namespace TORALEV2API;
class CMDL2Impl;
class MatchThread
{
public:
    MatchThread(CMDL2Impl* mdImpl, int threadIndex);
    ~MatchThread();

    void Start();
    void Run();

    void PushMessage(TTF_Message_t* message);

private:
    void MDOnRtnOrderDetail(CTORATstpLev2OrderDetailField& OrderDetail);
    void MDOnRtnTransaction(CTORATstpLev2TransactionField& Transaction);
    void MDOnRtnNGTSTick(CTORATstpLev2NGTSTickField& Tick);

    int m_threadIndex = -1;
    CMDL2Impl* m_mdImpl = nullptr;
    std::thread* m_thread = nullptr;
    moodycamel::ConcurrentQueue<TTF_Message_t*> m_queue;

    void InitOrderMap();

    int GetHTLPriceIndex(int SecurityIDInt, TTORATstpPriceType Price, TTORATstpTradeBSFlagType Side);
    void AddOrderPriceIndex(int SecutityIDInt, int idx, TTORATstpTradeBSFlagType Side);
    void DelOrderPriceIndex(int SecurityIDInt, int idx, TTORATstpTradeBSFlagType Side);

    double GetOrderNoToPrice(int SecurityIDInt, TTORATstpLongSequenceType OrderNO);
    void AddOrderNoToPrice(int SecurityIDInt, TTORATstpLongSequenceType OrderNO, TTORATstpPriceType Price);
    void DelOrderNoPrice(int SecurityIDInt, TTORATstpLongSequenceType OrderNO);

    void InsertOrderN(int SecurityIDInt, TTORATstpLongSequenceType OrderNO, TTORATstpPriceType Price, TTORATstpLongVolumeType Volume, TTORATstpLSideType Side, bool findPos = false);
    int ModifyOrderN(int SecurityIDInt, TTORATstpLongVolumeType Volume, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType Side);
    void DeleteOrderN(int SecurityIDInt, int priceIndex, TTORATstpLongSequenceType OrderNo, TTORATstpTradeBSFlagType Side);

    void AddHomebestOrder(int SecurityIDInt, TTORATstpLongSequenceType OrderNO, TTORATstpLongVolumeType Volume, TTORATstpLSideType Side);
    stHomebestOrder* GetHomebestOrder(int SecurityIDInt, TTORATstpLongSequenceType OrderNO);
    void DelHomebestOrder(int SecurityIDInt, TTORATstpLongSequenceType OrderNO);

    int FindPriceIndexOrderNo(std::vector<stOrder*>& orderNOs, TTORATstpLongSequenceType OrderNo);
    void PostPrice(TTORATstpSecurityIDType SecurityID, TTORATstpExchangeIDType ExchangeID);
    //void OnRtnMarketData(CTORATstpMarketDataField& MarketDataField);

    std::unordered_map<int, int> m_notifyCount;
    MapOrderN m_orderBuyN;
    MapOrderN m_orderSellN;
    MemoryPool m_pool;
    std::vector<std::string> m_vExchanges;

    bool IsTradingExchange(TTORATstpExchangeIDType ExchangeID);

    std::unordered_map<std::string, double> m_securityid_lastprice;
    std::unordered_map<int, std::set<int>> m_buyPriceIndex;
    std::unordered_map<int, std::set<int>> m_sellPriceIndex;
    std::unordered_map<int, std::unordered_map<TTORATstpLongSequenceType, TTORATstpPriceType>> m_orderNoToPrice;
    std::unordered_map<int, std::unordered_map<TTORATstpLongSequenceType, stHomebestOrder*>> m_homeBaseOrder;

};

#endif //BALIBALI_HFT_MATCHTHREAD_H
