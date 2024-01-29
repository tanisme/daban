/* 进场策略 */

#pragma once
#include "TORA/TORATstpUserApiStruct.h"
#include "TORA/TORATstpXMdApiStruct.h"
#include <string>
#include "Strategy.h"


// 行情事件、时间事件(boost.timer）
class CLimitT0Strategy :public CStrategy
{
public:
	CLimitT0Strategy(CTickTradingFramework* pFramework, std::string InstrumentID, std::string Parameters);
	~CLimitT0Strategy();

	void OnRtnOrder(TORASTOCKAPI::CTORATstpOrderField& Order);
	void OnRtnTrade(TORASTOCKAPI::CTORATstpTradeField& Trade);
	void OnRtnMarketData(TORALEV1API::CTORATstpMarketDataField& MarketData);
};
