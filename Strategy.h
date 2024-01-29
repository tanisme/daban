/* 策略抽象类 */

#pragma once

#include "TORA/TORATstpUserApiStruct.h"
#include "TORA/TORATstpTraderApi.h"
#include "TORA/TORATstpXMdApiStruct.h"
#include <string>
#include <vector>
#include <map>

// 行情事件、时间事件(boost.timer）
class CTickTradingFramework;
class CStrategy
{
public:
	CStrategy(std::string StrategyID, CTickTradingFramework* pFramework, std::string Parameters);
	virtual ~CStrategy();

	const std::string& GetStrategyID();
	void AddInstrument(std::string InstrumentID);
	int LongEntry(std::string InstrumentID, double Price, int Volume); // 多开
	int ExitLong(std::string InstrumentID, double Price, int Volume); // 平多
	int OrderCancel(TORASTOCKAPI::TTORATstpExchangeIDType ExchangeID, const char* OrderSysID); // 撤单
	int OrderCancel(TORASTOCKAPI::TTORATstpExchangeIDType ExchangeID, TORASTOCKAPI::TTORATstpFrontIDType FrontID, TORASTOCKAPI::TTORATstpSessionIDType SessionID, TORASTOCKAPI::TTORATstpOrderRefType OrderRef);

	virtual void OnRspOrderInsert(TORASTOCKAPI::CTORATstpInputOrderField& InputOrder, TORASTOCKAPI::CTORATstpRspInfoField& RspInfo, int nRequestID);
	virtual void OnRtnOrder(TORASTOCKAPI::CTORATstpOrderField& Order);
	virtual void OnRtnTrade(TORASTOCKAPI::CTORATstpTradeField& Trade);
	virtual void OnRtnMarketData(TORALEV1API::CTORATstpMarketDataField& MarketData);

	std::string	m_StrategyID; ///策略ID
	std::string m_Parameters;
	std::vector<std::string> m_vInstruments; // 策略合约列表
	TORASTOCKAPI::CTORATstpTradingAccountField m_TradingAccount; // 策略资金
	std::map<std::string, TORASTOCKAPI::CTORATstpOrderField> m_mInputingOrders; // 等待交易所响应的报单，Key: FrontID.SessionID.OrderRef
	CTickTradingFramework* m_pFramework;
};

