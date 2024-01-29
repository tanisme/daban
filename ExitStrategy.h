/* 出场策略 */

#pragma once

#include "TORA/TORATstpUserApiStruct.h"
#include "TORA/TORATstpXMdApiStruct.h"
#include "Strategy.h"
#include <string>


// 行情事件、时间事件(boost.timer）
class CExitStrategy:public CStrategy
{
public:
	CExitStrategy(CTickTradingFramework* pFramework, std::string InstrumentID, std::string Parameters);
	~CExitStrategy();

	void OnRtnOrder(TORASTOCKAPI::CTORATstpOrderField& Order);
	void OnRtnTrade(TORASTOCKAPI::CTORATstpTradeField& Trade);
	void OnRtnMarketData(TORALEV1API::CTORATstpMarketDataField& MarketData);
};

