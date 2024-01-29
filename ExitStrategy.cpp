#include "ExitStrategy.h"
#include "TickTradingFramework.h"

CExitStrategy::CExitStrategy(CTickTradingFramework* pFramework, std::string InstrumentID, std::string Parameters) :
	CStrategy("ExitStrategy", pFramework, Parameters)
{
	AddInstrument(InstrumentID);
}

CExitStrategy::~CExitStrategy()
{
}

void CExitStrategy::OnRtnOrder(TORASTOCKAPI::CTORATstpOrderField& Order)
{
}
void CExitStrategy::OnRtnTrade(TORASTOCKAPI::CTORATstpTradeField& Trade)
{
}

void CExitStrategy::OnRtnMarketData(TORALEV1API::CTORATstpMarketDataField& MarketData)
{
	// 如果价格上涨至快涨停的位置(还差两个点。。)，买入

	// 如果价格已经触到了涨停价，但是卖一档仍有少量挂单，买入

	// 如果已经板了(卖档为空，买一档已到涨停价),封单金额>X万元时，买入

	// 买入时，下单手数随机拆碎
}
