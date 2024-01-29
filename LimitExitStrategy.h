/* 进场策略 */

#pragma once
#include "TORA/TORATstpUserApiStruct.h"
#include "TORA/TORATstpXMdApiStruct.h"
#include <string>
#include "Strategy.h"


// 行情事件、时间事件(boost.timer）
class CLimitExitStrategy :public CStrategy
{
public:
	CLimitExitStrategy(CTickTradingFramework* pFramework, std::string InstrumentID, std::string Parameters);
	~CLimitExitStrategy();

	void OnRtnOrder(TORASTOCKAPI::CTORATstpOrderField& Order);
	void OnRtnTrade(TORASTOCKAPI::CTORATstpTradeField& Trade);
	void OnRtnMarketData(TORALEV1API::CTORATstpMarketDataField& MarketData);
};
