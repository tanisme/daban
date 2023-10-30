#ifndef TEST_TDIMPL_H
#define TEST_TDIMPL_H

#include "defines.h"

class CApplication;
namespace PROTD {

    using namespace TORASTOCKAPI;
    class TDImpl : public CTORATstpTraderSpi {
    public:
        explicit TDImpl(CApplication *pApp);
        ~TDImpl();

        bool Start();
        bool IsLogined() const { return m_isLogined; }
        CTORATstpTraderApi *GetApi() const { return m_pApi; }

        int OrderInsert(TTORATstpSecurityIDType SecurityID, TTORATstpExchangeIDType ExchangeID, TTORATstpDirectionType Direction, TTORATstpVolumeType VolumeTotalOriginal, TTORATstpPriceType LimitPric);
        int OrderCancel();

    public:
        void OnFrontConnected() override;
        void OnFrontDisconnected(int nReason) override;
        void OnRspUserLogin(CTORATstpRspUserLoginField *pRspUserLoginField, CTORATstpRspInfoField *pRspInfoField, int nRequestID) override;
        void OnRspQryShareholderAccount(CTORATstpShareholderAccountField *pShareholderAccountField, CTORATstpRspInfoField *pRspInfoField, int nRequestID, bool bIsLast) override;
        void OnRspQrySecurity(CTORATstpSecurityField *pSecurity, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRspQryOrder(CTORATstpOrderField *pOrder, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRspQryTrade(CTORATstpTradeField *pTrade, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRspQryPosition(CTORATstpPositionField *pPosition, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRspQryTradingAccount(CTORATstpTradingAccountField *pTradingAccount, CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
        void OnRspOrderInsert(CTORATstpInputOrderField *pInputOrderField, CTORATstpRspInfoField *pRspInfoField, int nRequestID) override;
        void OnErrRtnOrderInsert(CTORATstpInputOrderField *pInputOrderField, CTORATstpRspInfoField *pRspInfoField, int nRequestID) override;
        void OnRtnOrder(CTORATstpOrderField *pOrder) override;
        void OnRtnTrade(CTORATstpTradeField *pTrade) override;
        void OnRspOrderAction(CTORATstpInputOrderActionField *pInputOrderActionField, CTORATstpRspInfoField *pRspInfoField, int nRequestID) override;
        void OnErrRtnOrderAction(CTORATstpInputOrderActionField *pInputOrderActionField, CTORATstpRspInfoField *pRspInfoField, int nRequestID) override;

    private:
        int m_reqID = 1;
        bool m_isLogined = false;
        CTORATstpTraderApi *m_pApi = nullptr;
        CApplication *m_pApp = nullptr;
        CTORATstpRspUserLoginField m_loginField = {0};
        std::unordered_map<TTORATstpExchangeIDType, CTORATstpShareholderAccountField> m_shareHolder;
    };

}

#endif //TEST_TDIMPL_H
