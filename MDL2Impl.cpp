#include "MDL2Impl.h"
#include "CApplication.h"

namespace PROMD {

    MDL2Impl::MDL2Impl(CApplication *pApp, TTORATstpExchangeIDType ExchangeID)
            : m_pApp(pApp), m_exchangeID(ExchangeID) {
    }

    MDL2Impl::~MDL2Impl() {
        if (m_pApi) {
            m_pApi->Join();
            m_pApi->Release();
        }
    }

    void MDL2Impl::Start() {
        if (m_pApp->m_isTest) { // tcp
            m_pApi = CTORATstpLev2MdApi::CreateTstpLev2MdApi();
            m_pApi->RegisterSpi(this);
            char addr[128] = {0};
            if (m_exchangeID == TORA_TSTP_EXD_SSE) {
                strcpy(addr, "tcp://210.14.72.17:16900");
            } else if (m_exchangeID == TORA_TSTP_EXD_SZSE) {
                strcpy(addr, "tcp://210.14.72.17:6900");
            }
            m_pApi->RegisterFront(addr);
            m_pApi->Init();
        } else { // udp
            m_pApi = CTORATstpLev2MdApi::CreateTstpLev2MdApi(TORA_TSTP_MST_MCAST);
            m_pApi->RegisterSpi(this);
            m_pApi->RegisterMulticast((char *)m_pApp->m_MDAddr.c_str(), (char*)m_pApp->m_MDInterface.c_str(), nullptr);
            m_pApi->Init();
        }
    }

    const char *MDL2Impl::GetExchangeName(TTORATstpExchangeIDType ExchangeID) {
        switch (ExchangeID) {
            case TORA_TSTP_EXD_SSE: return "SH";
            case TORA_TSTP_EXD_SZSE: return "SZ";
            case TORA_TSTP_EXD_HK: return "HK";
            case TORA_TSTP_EXD_COMM: return "COM";
        }
        return "UNKNOW";
    }

    void MDL2Impl::OnFrontConnected() {
        printf("%s MD::OnFrontConnected!!!\n", GetExchangeName(m_exchangeID));
        CTORATstpReqUserLoginField Req = {0};
        m_pApi->ReqUserLogin(&Req, ++m_reqID);
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
        m_pApp->m_ioc.post(boost::bind(&CApplication::MDOnInitFinished, m_pApp, m_exchangeID));
    }

    void MDL2Impl::OnRtnOrderDetail(CTORATstpLev2OrderDetailField *pOrderDetail) {
        if (!pOrderDetail || pOrderDetail->OrderType == TORA_TSTP_LOT_Market) return;
        m_pApp->m_ioc.post(boost::bind(&CApplication::MDOnRtnOrderDetail, m_pApp, *pOrderDetail));
    }

    void MDL2Impl::OnRtnTransaction(CTORATstpLev2TransactionField *pTransaction) {
        if (!pTransaction) return;
        m_pApp->m_ioc.post(boost::bind(&CApplication::MDOnRtnTransaction, m_pApp, *pTransaction));
    }

    void MDL2Impl::OnRtnNGTSTick(CTORATstpLev2NGTSTickField *pTick) {
        if (!pTick) return;
        m_pApp->m_ioc.post(boost::bind(&CApplication::MDOnRtnNGTSTick, m_pApp, *pTick));
    }

}
