#include "TrendT0Strategy.h"
#include "TickTradingFramework.h"

// 趋势T0策略
CTrendT0Strategy::CTrendT0Strategy(CTickTradingFramework* pFramework, std::string InstrumentID, std::string Parameters):
	CStrategy("TrendT0", pFramework, Parameters)
{
	AddInstrument(InstrumentID);
}

CTrendT0Strategy::~CTrendT0Strategy()
{
}

void CTrendT0Strategy::OnRtnOrder(TORASTOCKAPI::CTORATstpOrderField& Order)
{
}

void CTrendT0Strategy::OnRtnTrade(TORASTOCKAPI::CTORATstpTradeField& Trade)
{
}

void CTrendT0Strategy::OnRtnMarketData(TORALEV1API::CTORATstpMarketDataField& MarketData)
{
}
