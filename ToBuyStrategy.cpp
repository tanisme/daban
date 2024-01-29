#include "ToBuyStrategy.h"
#include "TickTradingFramework.h"

// 一字板策略
CToBuyStrategy::CToBuyStrategy(CTickTradingFramework* pFramework, std::string InstrumentID, std::string Parameters):
	CStrategy("ToBuy", pFramework, Parameters)
{
	AddInstrument(InstrumentID);
}

CToBuyStrategy::~CToBuyStrategy()
{
}

void CToBuyStrategy::OnRtnOrder(TORASTOCKAPI::CTORATstpOrderField& Order)
{
}

void CToBuyStrategy::OnRtnTrade(TORASTOCKAPI::CTORATstpTradeField& Trade)
{
}

void CToBuyStrategy::OnRtnMarketData(TORALEV1API::CTORATstpMarketDataField& MarketData)
{
}
