#include "LimitExitStrategy.h"
#include "TickTradingFramework.h"

// 涨停卖出策略
CLimitExitStrategy::CLimitExitStrategy(CTickTradingFramework* pFramework, std::string InstrumentID, std::string Parameters):
	CStrategy("LimitExit", pFramework, Parameters)
{
	AddInstrument(InstrumentID);
}

CLimitExitStrategy::~CLimitExitStrategy()
{
}

void CLimitExitStrategy::OnRtnOrder(TORASTOCKAPI::CTORATstpOrderField& Order)
{
}

void CLimitExitStrategy::OnRtnTrade(TORASTOCKAPI::CTORATstpTradeField& Trade)
{
}

void CLimitExitStrategy::OnRtnMarketData(TORALEV1API::CTORATstpMarketDataField& MarketData)
{
}
