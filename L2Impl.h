#ifndef TEST_MDL2Impl_H
#define TEST_MDL2Impl_H

#include <thread>
#include <vector>
#include "concurrentqueue/concurrentqueue.h"
#include "defines.h"
#include "balibali.h"

using namespace TORALEV2API;

class CTickTradingFramework;
class CMDL2Impl : public CTORATstpLev2MdSpi {
public:
    explicit CMDL2Impl(CTickTradingFramework* pFramework, std::string& hostAddr, std::string& interfaceAddr, std::string& MdCore, int nThreadCount);
    ~CMDL2Impl();

    void ProcessMessages(int index);
    void ThreadRun(int index);
    void InitThreads();
    void Start();
    bool IsInited() const { return m_isInited; }
    CTORATstpLev2MdApi* GetApi() const { return m_pApi; }

public:
    void OnFrontConnected() override;
    void OnFrontDisconnected(int nReason) override;
    void OnRspUserLogin(CTORATstpRspUserLoginField* pRspUserLogin, CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRtnOrderDetail(CTORATstpLev2OrderDetailField* pOrderDetail) override;
    void OnRtnTransaction(CTORATstpLev2TransactionField* pTransaction) override;
    void OnRtnNGTSTick(CTORATstpLev2NGTSTickField* pTick) override;

private:
    std::string m_hostAddr = "";
    std::string m_interfaceAddr = "";
    std::string m_MdCore = "";

    int m_reqID = 1;
    bool m_isInited = false;
    CTORATstpLev2MdApi* m_pApi = nullptr;
    CTickTradingFramework* m_pFramework = nullptr;
    std::vector<std::thread*> m_pvThreads;
    std::vector<moodycamel::ConcurrentQueue<TTF_Message_t*>*> m_pvQueues;
    int m_nThreadCount;
};

#endif //TEST_CMDL2Impl_H
