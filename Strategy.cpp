#include "Strategy.h"
#include "TickTradingFramework.h"
#include "spdlog/spdlog.h"
#include <iostream>

extern std::shared_ptr<spdlog::logger> logger;

CStrategy::CStrategy(std::string StrategyID, CTickTradingFramework* pFramework, std::string Parameters):
	m_StrategyID(StrategyID),
	m_pFramework(pFramework),
	m_Parameters(Parameters)
{
}

CStrategy::~CStrategy()
{
	for(auto iter=m_vInstruments.begin();iter!=m_vInstruments.end();iter++)
		m_pFramework->UnSubscribe(iter->c_str(), this);
}

const std::string& CStrategy::GetStrategyID()
{
	return m_StrategyID;
}

void CStrategy::AddInstrument(std::string InstrumentID)
{
	m_vInstruments.push_back(InstrumentID);
	m_pFramework->Subscribe(InstrumentID.c_str(), this);
}

// 多开
int CStrategy::LongEntry(std::string InstrumentID, double Price, int Volume)
{
	auto pInstrument = m_pFramework->GetInstrument(InstrumentID);
	if (pInstrument == nullptr)
		return -1;

	std::map<std::string, TORASTOCKAPI::CTORATstpPositionField>::iterator iterPosition;

	if ((iterPosition = m_pFramework->m_mLongPositions.find(InstrumentID)) == m_pFramework->m_mLongPositions.end()) {
		m_pFramework->m_mLongPositions[InstrumentID] = TORASTOCKAPI::CTORATstpPositionField({ 0 });
		iterPosition = m_pFramework->m_mLongPositions.find(InstrumentID);
	}
	//iterPosition->second.LongFrozen += Volume;

	TORASTOCKAPI::CTORATstpOrderField Order = { 0 };
	strncpy(Order.InvestorID, m_pFramework->m_pTdImpl->m_UserID.c_str(), sizeof(Order.InvestorID) - 1);
	Order.ExchangeID = pInstrument->ExchangeID;
	switch (pInstrument->ExchangeID)
	{
	case TORASTOCKAPI::TORA_TSTP_EXD_SSE:
		strcpy(Order.ShareholderID, m_pFramework->m_ShareholderID_SSE);
		break;
	case TORASTOCKAPI::TORA_TSTP_EXD_SZSE:
		strcpy(Order.ShareholderID, m_pFramework->m_ShareholderID_SZSE);
		break;
	default:
		break;
	}
	strncpy(Order.SecurityID, InstrumentID.c_str(), sizeof(Order.SecurityID) - 1);
	Order.Direction = TORASTOCKAPI::TORA_TSTP_D_Buy;
	Order.VolumeTotalOriginal = Volume;
	Order.LimitPrice = Price;
	Order.OrderPriceType = TORASTOCKAPI::TORA_TSTP_OPT_LimitPrice;
	Order.FrontID = m_pFramework->m_nFrontID;
	Order.SessionID = m_pFramework->m_nSessionID;
	Order.OrderRef = m_pFramework->m_nMaxOrderRef++;
	Order.TimeCondition = TORASTOCKAPI::TORA_TSTP_TC_GFD;
	Order.VolumeCondition = TORASTOCKAPI::TORA_TSTP_VC_AV;
	Order.ForceCloseReason = TORASTOCKAPI::TORA_TSTP_FCC_NotForceClose;
	Order.Operway = TORASTOCKAPI::TORA_TSTP_OPERW_PCClient;
	strncpy(Order.SInfo, m_StrategyID.c_str(), sizeof(Order.SInfo) - 1); // 关联策略

	// 保存订单
	std::string key = std::to_string(Order.FrontID) + "." + std::to_string(Order.SessionID) + "." + std::to_string(Order.OrderRef);
	m_pFramework->m_mInputingOrders[key] = Order;

	// 下单
	TORASTOCKAPI::CTORATstpInputOrderField InputOrder = { 0 };
	strncpy(InputOrder.InvestorID, m_pFramework->m_pTdImpl->m_UserID.c_str(), sizeof(InputOrder.InvestorID) - 1);
	InputOrder.ExchangeID = pInstrument->ExchangeID;
	switch (pInstrument->ExchangeID)
	{
	case TORASTOCKAPI::TORA_TSTP_EXD_SSE:
		strcpy(InputOrder.ShareholderID, m_pFramework->m_ShareholderID_SSE);
		break;
	case TORASTOCKAPI::TORA_TSTP_EXD_SZSE:
		strcpy(InputOrder.ShareholderID, m_pFramework->m_ShareholderID_SZSE);
		break;
	default:
		break;
	}
	strncpy(InputOrder.SecurityID, InstrumentID.c_str(), sizeof(InputOrder.SecurityID) - 1);
	InputOrder.Direction = TORASTOCKAPI::TORA_TSTP_D_Buy;
	InputOrder.VolumeTotalOriginal = Volume;
	InputOrder.LimitPrice = Price;
	InputOrder.OrderPriceType = TORASTOCKAPI::TORA_TSTP_OPT_LimitPrice;
	InputOrder.OrderRef = Order.OrderRef;
	InputOrder.TimeCondition = TORASTOCKAPI::TORA_TSTP_TC_GFD;
	InputOrder.VolumeCondition = TORASTOCKAPI::TORA_TSTP_VC_AV;
	InputOrder.Operway = TORASTOCKAPI::TORA_TSTP_OPERW_PCClient;
	InputOrder.ForceCloseReason = TORASTOCKAPI::TORA_TSTP_FCC_NotForceClose;
	strncpy(InputOrder.SInfo, m_StrategyID.c_str(), sizeof(InputOrder.SInfo) - 1); // 关联策略
	//logger->info("ReqOrderInsert:{} - {} - {}", InstrumentID, Volume, Price);

#ifndef _WIN32
	struct timeval tv;
	gettimeofday(&tv, NULL);
#endif

	int r = 0;
	if ((r=m_pFramework->m_pTdImpl->m_pTdApi->ReqOrderInsert(&InputOrder, m_pFramework->current_time)) < 0) {
		logger->error("下单失败：r={}", r);

		// 释放资金与持仓冻结

		std::map<std::string, TORASTOCKAPI::CTORATstpPositionField>::iterator iterPosition;
		if ((iterPosition = m_pFramework->m_mLongPositions.find(InstrumentID)) == m_pFramework->m_mLongPositions.end()) {
			m_pFramework->m_mLongPositions[InstrumentID] = TORASTOCKAPI::CTORATstpPositionField({ 0 });
			iterPosition = m_pFramework->m_mLongPositions.find(InstrumentID);
		}
		//iterPosition->second.LongFrozen -= Volume;

		// 清除错单
		m_pFramework->m_mInputingOrders.erase(m_pFramework->m_mInputingOrders.find(key));

		return -1;
	}

#ifndef _WIN32
	if (tv.tv_usec > m_pFramework->current_time)
		std::cout << "time used of OrderInsert: " << tv.tv_usec - m_pFramework->current_time << std::endl;
	else
		std::cout << "time used of OrderInsert: " << 1000000 + tv.tv_usec - m_pFramework->current_time << std::endl;
#endif

	// 保存策略订单
	m_mInputingOrders[key] = Order;

	return 0;
}

// 平多
int CStrategy::ExitLong(std::string InstrumentID, double Price, int Volume)
{
	auto pInstrument = m_pFramework->GetInstrument(InstrumentID);
	if (pInstrument == nullptr)
		return -1;

	std::map<std::string, TORASTOCKAPI::CTORATstpPositionField>::iterator iterPosition;

	if ((iterPosition = m_pFramework->m_mLongPositions.find(InstrumentID)) == m_pFramework->m_mLongPositions.end()) {
		m_pFramework->m_mLongPositions[InstrumentID] = TORASTOCKAPI::CTORATstpPositionField({ 0 });
		iterPosition = m_pFramework->m_mLongPositions.find(InstrumentID);
	}
	//iterPosition->second.ShortFrozen += Volume;

	TORASTOCKAPI::CTORATstpOrderField Order = { 0 };
	strncpy(Order.InvestorID, m_pFramework->m_pTdImpl->m_UserID.c_str(), sizeof(Order.InvestorID) - 1);
	Order.ExchangeID = pInstrument->ExchangeID;
	switch (pInstrument->ExchangeID)
	{
	case TORASTOCKAPI::TORA_TSTP_EXD_SSE:
		strcpy(Order.ShareholderID, m_pFramework->m_ShareholderID_SSE);
		break;
	case TORASTOCKAPI::TORA_TSTP_EXD_SZSE:
		strcpy(Order.ShareholderID, m_pFramework->m_ShareholderID_SZSE);
		break;
	default:
		break;
	}
	strncpy(Order.SecurityID, InstrumentID.c_str(), sizeof(Order.SecurityID) - 1);
	Order.Direction = TORASTOCKAPI::TORA_TSTP_D_Sell;
	Order.VolumeTotalOriginal = Volume;
	Order.LimitPrice = Price;
	Order.OrderPriceType = TORASTOCKAPI::TORA_TSTP_OPT_LimitPrice;
	Order.FrontID = m_pFramework->m_nFrontID;
	Order.SessionID = m_pFramework->m_nSessionID;
	Order.OrderRef = m_pFramework->m_nMaxOrderRef++;
	Order.TimeCondition = TORASTOCKAPI::TORA_TSTP_TC_GFD;
	Order.VolumeCondition = TORASTOCKAPI::TORA_TSTP_VC_AV;
	Order.ForceCloseReason = TORASTOCKAPI::TORA_TSTP_FCC_NotForceClose;
	Order.Operway = TORASTOCKAPI::TORA_TSTP_OPERW_PCClient;
	strncpy(Order.SInfo, m_StrategyID.c_str(), sizeof(Order.SInfo) - 1); // 关联策略

	// 保存订单
	std::string key = std::to_string(Order.FrontID) + "." + std::to_string(Order.SessionID) + "." + std::to_string(Order.OrderRef);
	m_pFramework->m_mInputingOrders[key] = Order;

	// 下单
	TORASTOCKAPI::CTORATstpInputOrderField InputOrder = { 0 };
	strncpy(InputOrder.InvestorID, m_pFramework->m_pTdImpl->m_UserID.c_str(), sizeof(InputOrder.InvestorID) - 1);
	InputOrder.ExchangeID = pInstrument->ExchangeID;
	switch (pInstrument->ExchangeID)
	{
	case TORASTOCKAPI::TORA_TSTP_EXD_SSE:
		strcpy(InputOrder.ShareholderID, m_pFramework->m_ShareholderID_SSE);
		break;
	case TORASTOCKAPI::TORA_TSTP_EXD_SZSE:
		strcpy(InputOrder.ShareholderID, m_pFramework->m_ShareholderID_SZSE);
		break;
	default:
		break;
	}
	strncpy(InputOrder.SecurityID, InstrumentID.c_str(), sizeof(InputOrder.SecurityID) - 1);
	InputOrder.Direction = TORASTOCKAPI::TORA_TSTP_D_Sell;
	InputOrder.VolumeTotalOriginal = Volume;
	InputOrder.LimitPrice = Price;
	InputOrder.OrderPriceType = TORASTOCKAPI::TORA_TSTP_OPT_LimitPrice;
	InputOrder.OrderRef = Order.OrderRef;
	InputOrder.TimeCondition = TORASTOCKAPI::TORA_TSTP_TC_GFD;
	InputOrder.VolumeCondition = TORASTOCKAPI::TORA_TSTP_VC_AV;
	InputOrder.Operway = TORASTOCKAPI::TORA_TSTP_OPERW_PCClient;
	InputOrder.ForceCloseReason = TORASTOCKAPI::TORA_TSTP_FCC_NotForceClose;
	strncpy(InputOrder.SInfo, m_StrategyID.c_str(), sizeof(InputOrder.SInfo) - 1); // 关联策略

	if (m_pFramework->m_pTdImpl->m_pTdApi->ReqOrderInsert(&InputOrder, atol(InstrumentID.c_str())) < 0) {
		// 释放资金与持仓冻结

		std::map<std::string, TORASTOCKAPI::CTORATstpPositionField>::iterator iterPosition;
		if ((iterPosition = m_pFramework->m_mLongPositions.find(InstrumentID)) == m_pFramework->m_mLongPositions.end()) {
			m_pFramework->m_mLongPositions[InstrumentID] = TORASTOCKAPI::CTORATstpPositionField({ 0 });
			iterPosition = m_pFramework->m_mLongPositions.find(InstrumentID);
		}
		//iterPosition->second.ShortFrozen -= Volume;

		// 清除错单
		m_pFramework->m_mInputingOrders.erase(m_pFramework->m_mInputingOrders.find(key));

		return -1;
	}

	// 保存策略订单
	m_mInputingOrders[key] = Order;

	return 0;
}

// 撤单
int CStrategy::OrderCancel(TORASTOCKAPI::TTORATstpExchangeIDType ExchangeID, const char* OrderSysID)
{
	TORASTOCKAPI::CTORATstpInputOrderActionField Req = { 0 };

	Req.ActionFlag = TORASTOCKAPI::TORA_TSTP_AF_Delete;
	Req.OrderActionRef = m_pFramework->m_nMaxOrderRef++;
	Req.ExchangeID = ExchangeID;
	strncpy(Req.OrderSysID, OrderSysID, sizeof(Req.OrderSysID) - 1);

	if (m_pFramework->m_pTdImpl->m_pTdApi->ReqOrderAction(&Req, 0) < 0)
		return -1;

	return 0;
}

// 撤单
int CStrategy::OrderCancel(TORASTOCKAPI::TTORATstpExchangeIDType ExchangeID, TORASTOCKAPI::TTORATstpFrontIDType FrontID, TORASTOCKAPI::TTORATstpSessionIDType SessionID, TORASTOCKAPI::TTORATstpOrderRefType OrderRef)
{
	TORASTOCKAPI::CTORATstpInputOrderActionField Req = { 0 };

	Req.ActionFlag = TORASTOCKAPI::TORA_TSTP_AF_Delete;
	Req.OrderActionRef = m_pFramework->m_nMaxOrderRef++;
	Req.ExchangeID = ExchangeID;
	Req.FrontID = FrontID;
	Req.SessionID = SessionID;
	Req.OrderRef = OrderRef;

	if (m_pFramework->m_pTdImpl->m_pTdApi->ReqOrderAction(&Req, 0) < 0)
		return -1;

	return 0;
}

void CStrategy::OnRspOrderInsert(TORASTOCKAPI::CTORATstpInputOrderField& InputOrder, TORASTOCKAPI::CTORATstpRspInfoField& RspInfo, int nRequestID)
{
	// 清除错单
	std::string key = std::to_string(m_pFramework->m_nFrontID) + "." + std::to_string(m_pFramework->m_nSessionID) + "." + std::to_string(InputOrder.OrderRef);
	auto iterInputingOrder = m_mInputingOrders.find(key);
	if (iterInputingOrder == m_mInputingOrders.end())
		return;
	m_mInputingOrders.erase(iterInputingOrder);
}

void CStrategy::OnRtnOrder(TORASTOCKAPI::CTORATstpOrderField& Order)
{
	// 清除已撤订单
	std::string key = std::to_string(Order.FrontID) + "." + std::to_string(Order.SessionID) + "." + std::to_string(Order.OrderRef);
	auto iterInputingOrder = m_mInputingOrders.find(key);
	if (iterInputingOrder == m_mInputingOrders.end())
		return;
	strcpy(iterInputingOrder->second.OrderSysID, Order.OrderSysID);
	if (Order.OrderStatus == TORASTOCKAPI::TORA_TSTP_OST_PartTradeCanceled
		|| Order.OrderStatus == TORASTOCKAPI::TORA_TSTP_OST_AllCanceled
		|| Order.OrderStatus == TORASTOCKAPI::TORA_TSTP_OST_Rejected) {
		m_mInputingOrders.erase(iterInputingOrder);
	}
}

void CStrategy::OnRtnTrade(TORASTOCKAPI::CTORATstpTradeField& Trade)
{
	// 清除全部成交订单
	std::string key = std::string(1, Trade.ExchangeID) + "." + Trade.OrderSysID;
	auto Order = m_pFramework->m_mOrders[key];
	key = std::to_string(Order.FrontID) + "." + std::to_string(Order.SessionID) + "." + std::to_string(Order.OrderRef);
	auto iterInputingOrder = m_mInputingOrders.find(key);
	if (iterInputingOrder == m_mInputingOrders.end())
		return;
	iterInputingOrder->second.VolumeTraded += Trade.Volume;
	if (iterInputingOrder->second.VolumeTotalOriginal == iterInputingOrder->second.VolumeTraded) {
		iterInputingOrder->second.OrderStatus = TORASTOCKAPI::TORA_TSTP_OST_AllTraded;
		m_mInputingOrders.erase(iterInputingOrder);
	}
	else {
		iterInputingOrder->second.OrderStatus = TORASTOCKAPI::TORA_TSTP_OST_PartTraded;
	}
}

void CStrategy::OnRtnMarketData(TORALEV1API::CTORATstpMarketDataField& MarketData)
{

}
