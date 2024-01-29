#include "L2Impl.h"
#include "TickTradingFramework.h"
#ifndef _WIN32
#include <sys/time.h>
#endif
#include <iostream>

CMDL2Impl::CMDL2Impl(CTickTradingFramework* pFramework, std::string& hostAddr, std::string& interfaceAddr, std::string& MdCore, int nThreadCount) :
    m_pFramework(pFramework),
    m_hostAddr(hostAddr),
    m_MdCore(MdCore),
    m_interfaceAddr(interfaceAddr) {
    m_nThreadCount = nThreadCount;
}

CMDL2Impl::~CMDL2Impl() {
    if (m_pApi) {
        m_pApi->Join();
        m_pApi->Release();
    }
}

void CMDL2Impl::InitThreads()
{
    for (int i = 0; i < m_nThreadCount; i++) {
        m_matchThreads.push_back(new MatchThread(this, i));
    }
}

void CMDL2Impl::Start() {
    InitThreads();

    if (strstr(m_hostAddr.c_str(),"tcp://")) { // tcp
        m_pApi = CTORATstpLev2MdApi::CreateTstpLev2MdApi();
        m_pApi->RegisterSpi(this);
        m_pApi->RegisterFront((char*)m_hostAddr.c_str());
        m_pApi->Init();
    }
    else { // udp
        m_pApi = CTORATstpLev2MdApi::CreateTstpLev2MdApi(TORA_TSTP_MST_MCAST);
        m_pApi->RegisterSpi(this);
        m_pApi->RegisterMulticast((char*)m_hostAddr.c_str(), (char*)m_interfaceAddr.c_str(), nullptr);
        m_pApi->Init(m_MdCore.c_str());
    }
}

void CMDL2Impl::OnFrontConnected() {
    TTF_Message_t* Message = MessageAllocate();
    Message->MsgType = MsgTypeMdFrontConnected;
    m_pFramework->m_queue.enqueue(Message);

    CTORATstpReqUserLoginField Req = { 0 };
    m_pApi->ReqUserLogin(&Req, ++m_reqID);
}

void CMDL2Impl::OnFrontDisconnected(int nReason) {
    m_isInited = false;

    TTF_Message_t* Message = MessageAllocate();
    Message->MsgType = MsgTypeMdFrontDisconnected;
    auto pMessage = (MsgMdFrontDisconnected_t *)&Message->Message;
    pMessage->nReason = nReason;
    m_pFramework->m_queue.enqueue(Message);
}

void CMDL2Impl::OnRspUserLogin(CTORATstpRspUserLoginField* pRspUserLogin, CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
    if (!pRspUserLogin || !pRspInfo) return;
    if (pRspInfo->ErrorID > 0) {
        printf("MD::OnRspUserLogin Failed!!!ErrMsg: % s\n", pRspInfo->ErrorMsg);
        return;
    }
    m_isInited = true;

    TTF_Message_t* Message = MessageAllocate();
    Message->MsgType = MsgTypeRspMdUserLogin;
    auto pMessage = (MsgRspMdUserLogin_t *)&Message->Message;
    pMessage->nRequestID = nRequestID;
    if (pRspUserLogin) memcpy(&pMessage->RspUserLogin, pRspUserLogin, sizeof(pMessage->RspUserLogin));
    if (pRspInfo) memcpy(&pMessage->RspInfo, pRspInfo, sizeof(pMessage->RspInfo));
    m_pFramework->m_queue.enqueue(Message);
}

void CMDL2Impl::OnRtnOrderDetail(CTORATstpLev2OrderDetailField* pOrderDetail) {

#ifndef _WIN32
    struct timeval tv;
    gettimeofday(&tv, NULL);
    pOrderDetail->MainSeq = tv.tv_usec;
#endif
    TTF_Message_t* Message = MessageAllocate();
    Message->MsgType = MsgTypeRtnOrderDetail;
    auto pMessage = (MsgRtnOrderDetail_t *)&Message->Message;
    memcpy(&pMessage->OrderDetail, pOrderDetail, sizeof(pMessage->OrderDetail));
    int index = atol(pOrderDetail->SecurityID) % m_nThreadCount;
    //m_pvQueues[index]->enqueue(Message);
}

void CMDL2Impl::OnRtnTransaction(CTORATstpLev2TransactionField* pTransaction) {

#ifndef _WIN32
    struct timeval tv;
    gettimeofday(&tv, NULL);
    pTransaction->MainSeq = tv.tv_usec;
#endif
    TTF_Message_t* Message = MessageAllocate();
    Message->MsgType = MsgTypeRtnTransaction;
    auto pMessage = (MsgRtnTransaction_t *)&Message->Message;
    memcpy(&pMessage->Transaction, pTransaction, sizeof(pMessage->Transaction));
    int index = atol(pTransaction->SecurityID) % m_nThreadCount;
    //m_pvQueues[index]->enqueue(Message);
}

void CMDL2Impl::OnRtnNGTSTick(CTORATstpLev2NGTSTickField* pTick) {
#ifndef _WIN32
    struct timeval tv;
    gettimeofday(&tv, NULL);
    pTick->MainSeq = tv.tv_usec;
#endif

    TTF_Message_t* Message = MessageAllocate();
    Message->MsgType = MsgTypeRtnNGTSTick;
    auto pMessage = (MsgRtnNGTSTick_t *)&Message->Message;
    memcpy(&pMessage->NGTSTick, pTick, sizeof(pMessage->NGTSTick));
    int index = atol(pTick->SecurityID) % m_nThreadCount;
    //m_pvQueues[index]->enqueue(Message);
}

TTF_Message_t* CMDL2Impl::MessageAllocate()
{
    TTF_Message_t* Message = (TTF_Message_t*)(m_MessageArray + m_MessageCursor);
    if (m_MessageCursor == TTF_MESSAGE_ARRAY_SIZE - 1)
        m_MessageCursor = 0;
    else
        m_MessageCursor++;

    return Message;
}