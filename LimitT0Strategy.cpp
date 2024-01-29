#include "LimitT0Strategy.h"
#include "TickTradingFramework.h"

// 涨停T0策略
CLimitT0Strategy::CLimitT0Strategy(CTickTradingFramework* pFramework, std::string InstrumentID, std::string Parameters):
	CStrategy("LimitT0", pFramework, Parameters)
{
	AddInstrument(InstrumentID);
}

CLimitT0Strategy::~CLimitT0Strategy()
{
}

void CLimitT0Strategy::OnRtnOrder(TORASTOCKAPI::CTORATstpOrderField& Order)
{
}

void CLimitT0Strategy::OnRtnTrade(TORASTOCKAPI::CTORATstpTradeField& Trade)
{
}

void CLimitT0Strategy::OnRtnMarketData(TORALEV1API::CTORATstpMarketDataField& MarketData)
{
}

