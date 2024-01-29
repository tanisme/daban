#include "EntryStrategy.h"
#include "TickTradingFramework.h"
#include "spdlog/spdlog.h"

extern std::shared_ptr<spdlog::logger> logger;

CEntryStrategy::CEntryStrategy(CTickTradingFramework* pFramework, std::string InstrumentID, std::string Parameters):
	CStrategy("Entry", pFramework, Parameters)
{
	try {
		json parameters = json::parse(Parameters);
		stage1_enabled = parameters["stage1_enabled"];
		stage1_to_limit_percent = parameters["stage1_to_limit_percent"];
		stage1_to_limit_percent /= 100.0;
		stage1_to_limit_price = parameters["stage1_to_limit_price"];
		stage1_buy_amount = parameters["stage1_buy_amount"];
		stage1_buy_amount *= 10000.0;

		stage2_enabled = parameters["stage2_enabled"];
		stage2_ask_amount = parameters["stage2_ask_amount"];
		stage2_ask_amount *= 10000.0;
		stage2_buy_amount = parameters["stage2_buy_amount"];
		stage2_buy_amount *= 10000.0;

		stage3_enabled = parameters["stage3_enabled"];
		stage3_bid_amount = parameters["stage3_bid_amount"];
		stage3_bid_amount *= 10000.0;
		stage3_buy_amount = parameters["stage3_buy_amount"];
        stage3_buy_amount *= 10000.0;

		cancel_enabled = parameters["cancel_enabled"];
		cancel_after_milliseconds = 1000;
		cancel_after_amount = parameters["cancel_after_amount"];
		cancel_after_amount *= 10000.0;
		//cancel_after_volume = parameters["cancel_after_volume"];

		auto_split = parameters["auto_split"];
	}
	catch (...) {
		logger->error("Wrong parameter format.");
	}

	AddInstrument(InstrumentID);
}

CEntryStrategy::~CEntryStrategy()
{
	if (m_pTimer)
		delete m_pTimer;
}

// Timer
void CEntryStrategy::OnTime(const boost::system::error_code& error)
{
	if (error) {
		if (error == boost::asio::error::operation_aborted) {
			return;
		}
	}

	// 撤掉策略的所有委托
	for (auto iter = m_mInputingOrders.begin(); iter != m_mInputingOrders.end(); iter++) {
		OrderCancel(iter->second.ExchangeID, iter->second.FrontID, iter->second.SessionID, iter->second.OrderRef);
	}

	delete m_pTimer;
	m_pTimer = nullptr;
	//m_pTimer->expires_from_now(boost::posix_time::milliseconds(cancel_after_milliseconds));
	//m_pTimer->async_wait(boost::bind(&CEntryStrategy::OnTime, this, boost::asio::placeholders::error));
}

void CEntryStrategy::OnRspOrderInsert(TORASTOCKAPI::CTORATstpInputOrderField& InputOrder, TORASTOCKAPI::CTORATstpRspInfoField& RspInfo, int nRequestID)
{
	CStrategy::OnRspOrderInsert(InputOrder, RspInfo, nRequestID);

	// 如果没有订单了则取消定时器
	if (m_mInputingOrders.size() == 0) {
		if (m_pTimer) {
			m_pTimer->cancel();
		}
	}
}

void CEntryStrategy::OnRtnOrder(TORASTOCKAPI::CTORATstpOrderField& Order)
{
	CStrategy::OnRtnOrder(Order);

	// 如果没有订单了则取消定时器
	if (m_mInputingOrders.size() == 0) {
		if (m_pTimer) {
			m_pTimer->cancel();
		}
	}
}
void CEntryStrategy::OnRtnTrade(TORASTOCKAPI::CTORATstpTradeField& Trade)
{
	CStrategy::OnRtnTrade(Trade);

	// 如果没有订单了则取消定时器
	if (m_mInputingOrders.size() == 0) {
		if (m_pTimer) {
			m_pTimer->cancel();
		}
	}
}

void CEntryStrategy::OnRtnMarketData(TORALEV1API::CTORATstpMarketDataField& MarketData)
{
	if (m_Triggered) // 只允许触发一次
		return;
	auto iterSecurity = m_pFramework->GetInstrument(MarketData.SecurityID);
	
	// 如果价格上涨至快涨停的位置(还差两个点。。)，买入
	if (stage1_enabled)
	{
		if ((stage1_to_limit_percent>0.000001 && MarketData.LastPrice+MarketData.LastPrice*stage1_to_limit_percent > iterSecurity->UpperLimitPrice-0.000001)
			|| (stage1_to_limit_price>0.000001 && MarketData.LastPrice + stage1_to_limit_price  > iterSecurity->UpperLimitPrice - 0.000001))
		{
			double price = iterSecurity->LowerLimitPrice; // 下单价格
			int volume = stage1_buy_amount / price; // 下单数量
			volume /= 100;
			volume *= 100;
			m_Triggered = true;
			if (LongEntry(MarketData.SecurityID, price, volume) < 0)
				return;
			//if (cancel_after_milliseconds) {
			//	m_pTimer = new boost::asio::deadline_timer(m_pFramework->m_io_service, boost::posix_time::milliseconds(cancel_after_milliseconds));
			//	m_pTimer->expires_from_now(boost::posix_time::milliseconds(cancel_after_milliseconds));
			//	m_pTimer->async_wait(boost::bind(&CEntryStrategy::OnTime, this, boost::asio::placeholders::error));
			//}
			return;
		}
	}

	// 如果价格已经触到了涨停价，但是卖一档仍有少量挂单，买入
	if (stage2_enabled)
	{
		if (MarketData.LastPrice > iterSecurity->UpperLimitPrice - 0.000001 && MarketData.AskPrice1 * MarketData.AskVolume1 < stage2_ask_amount)
		{
			double price = iterSecurity->LowerLimitPrice; // 下单价格
			int volume = stage2_buy_amount / price; // 下单数量
			volume /= 100;
			volume *= 100;
			m_Triggered = true;
			if (LongEntry(MarketData.SecurityID, price, volume) < 0)
				return;
			//if (cancel_after_milliseconds) {
			//	m_pTimer = new boost::asio::deadline_timer(m_pFramework->m_io_service, boost::posix_time::milliseconds(cancel_after_milliseconds));
			//	m_pTimer->expires_from_now(boost::posix_time::milliseconds(cancel_after_milliseconds));
			//	m_pTimer->async_wait(boost::bind(&CEntryStrategy::OnTime, this, boost::asio::placeholders::error));
			//}
			return;
		}
	}

	// 如果已经板了(卖档为空，买一档已到涨停价),封单金额>X万元时，买入
	if (stage3_enabled)
	{
		if (MarketData.BidPrice1 > iterSecurity->UpperLimitPrice - 0.0000001 && MarketData.BidPrice1 * MarketData.BidVolume1 < stage3_bid_amount)
		{
			double price = iterSecurity->LowerLimitPrice; // 下单价格
			int volume = stage3_buy_amount / price; // 下单数量
			volume /= 100;
			volume *= 100;
			m_Triggered = true;
			if (LongEntry(MarketData.SecurityID, price, volume) < 0)
				return;
			//if (cancel_after_milliseconds) {
			//	m_pTimer = new boost::asio::deadline_timer(m_pFramework->m_io_service, boost::posix_time::milliseconds(cancel_after_milliseconds));
			//	m_pTimer->expires_from_now(boost::posix_time::milliseconds(cancel_after_milliseconds));
			//	m_pTimer->async_wait(boost::bind(&CEntryStrategy::OnTime, this, boost::asio::placeholders::error));
			//}
			return;
		}
	}
	// 买入时，下单手数随机拆碎
}
