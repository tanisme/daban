/* 进场策略 */

#pragma once
#include "TORA/TORATstpUserApiStruct.h"
#include "TORA/TORATstpXMdApiStruct.h"
#include <string>
#include "boost/asio.hpp"
#include "Strategy.h"


// 行情事件、时间事件(boost.timer）
class CEntryStrategy :public CStrategy
{
public:
	CEntryStrategy(CTickTradingFramework* pFramework, std::string InstrumentID, std::string Parameters);
	~CEntryStrategy();

	// Timer
	void OnTime(const boost::system::error_code& error);

	void OnRspOrderInsert(TORASTOCKAPI::CTORATstpInputOrderField& InputOrder, TORASTOCKAPI::CTORATstpRspInfoField& RspInfo, int nRequestID);
	void OnRtnOrder(TORASTOCKAPI::CTORATstpOrderField& Order);
	void OnRtnTrade(TORASTOCKAPI::CTORATstpTradeField& Trade);
	void OnRtnMarketData(TORALEV1API::CTORATstpMarketDataField& MarketData);

	bool stage1_enabled = false;
	double stage1_to_limit_percent = 0.0;
	double stage1_to_limit_price = 0.0;
	double stage1_buy_amount = 0;

	bool stage2_enabled = false;
	double stage2_ask_amount = 0.0;
	double stage2_buy_amount = 0.0;

	bool stage3_enabled = false;
	double stage3_bid_amount = 0.0;
	double stage3_buy_amount = 0.0;

	bool cancel_enabled = false;
	long cancel_after_milliseconds = 1000;
	double cancel_after_amount = 1000.0;
	long cancel_after_volume = 0;

	bool auto_split = false;

	boost::asio::deadline_timer* m_pTimer = nullptr;
	bool m_Triggered = false;
};
