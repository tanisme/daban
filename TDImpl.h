#pragma once

#include <string>
#include "TORA/TORATstpTraderApi.h"

class CTickTradingFramework;
class CTdImpl : public TORASTOCKAPI::CTORATstpTraderSpi
{
public:
	CTdImpl(CTickTradingFramework* pFramework, std::string Host, std::string User, std::string Password, std::string UserProductInfo, const std::string LIP, const std::string MAC, const std::string HD) :
		m_pFramework(pFramework),
		m_Server(Host),
		m_UserID(User),
		m_Password(Password)
	{
		m_LIP = LIP;
		m_MAC = MAC;
		m_HD = HD;
		m_pTdApi = TORASTOCKAPI::CTORATstpTraderApi::CreateTstpTraderApi();
		m_pTdApi->RegisterSpi(this);
	}

	~CTdImpl()
	{
	}

	void Run();

	void OnFrontConnected();

	void OnFrontDisconnected(int nReason);


	///用户登录应答
	void OnRspUserLogin(TORASTOCKAPI::CTORATstpRspUserLoginField* pRspUserLoginField, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID);

	//查询股东账户
	void OnRspQryShareholderAccount(TORASTOCKAPI::CTORATstpShareholderAccountField* pShareholderAccount, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///合约查询应答
	void OnRspQrySecurity(TORASTOCKAPI::CTORATstpSecurityField* pSecurity, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	//订单查询
	void OnRspQryOrder(TORASTOCKAPI::CTORATstpOrderField* pOrder, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	//成交查询
	void OnRspQryTrade(TORASTOCKAPI::CTORATstpTradeField* pTrade, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	//持仓查询
	void OnRspQryPosition(TORASTOCKAPI::CTORATstpPositionField* pPosition, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	//资金查询
	void OnRspQryTradingAccount(TORASTOCKAPI::CTORATstpTradingAccountField* pTradingAccount, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	//订单回报
	void OnRtnOrder(TORASTOCKAPI::CTORATstpOrderField* pOrder);

	//成交回报
	void OnRtnTrade(TORASTOCKAPI::CTORATstpTradeField* pTrade);

	///报单录入响应
	void OnRspOrderInsert(TORASTOCKAPI::CTORATstpInputOrderField* pInputOrderField, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfoField, int nRequestID);

	///报单错误回报
	void OnErrRtnOrderInsert(TORASTOCKAPI::CTORATstpInputOrderField* pInputOrderField, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfoField, int nRequestID);

	///撤单响应
	void OnRspOrderAction(TORASTOCKAPI::CTORATstpInputOrderActionField* pInputOrderActionField, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfoField, int nRequestID);

	///撤单错误回报
	void OnErrRtnOrderAction(TORASTOCKAPI::CTORATstpInputOrderActionField* pInputOrderActionField, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfoField, int nRequestID);

public:
	std::string m_Server;
	std::string m_UserID;
	std::string m_Password;
	std::string m_UserProductInfo;
	std::string m_LIP, m_MAC, m_HD;

	TORASTOCKAPI::CTORATstpTraderApi* m_pTdApi;
	CTickTradingFramework* m_pFramework;
};
