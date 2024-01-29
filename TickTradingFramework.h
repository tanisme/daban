#pragma once

#include <string>
#include <map>
#include <set>
#include <vector>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <thread>
#include "TORA/TORATstpTraderApi.h"
#include "TORA/TORATstpXMdApi.h"
#include "TORA/TORATstpLev2MdApi.h"
#include "TdImpl.h"
#include "L2Impl.h"
#include "Strategy.h"
#include <mutex>
#include <functional>
#include <algorithm>
#include <memory>
#include "nlohmann/json.hpp"
#include "soci/soci.h"
#include "MemoryPool.h"
#include "concurrentqueue/concurrentqueue.h"
#include <unordered_set>
#include "balibali.h"

using json = nlohmann::json;


// 用于存储无锁队列消息内容的空间，消费者从无锁队列拿到消息指针（指向消息数组中的一项），处理完后也不对消息数组有任何修改。
// 生产者逐条写入消息，并将消息指针推到无锁队列，如果消费者性能不够，未及时处理的消息将会被生产者覆盖，所以要有足够的缓冲空间以应对性能的抖动。
// 一般3秒钟足够了，如果3秒钟还没处理完，那就是设计问题了，留个10万笔消息的空间吧。10w笔x1000字节=100MB
#define TTF_MESSAGE_ARRAY_SIZE 100000 // 消息数量

// 沪深合约号为固定6位数，最大合约号为999999，直接以数据下标定位合约信息，合约号600000的信息为Instruments[600000]
#define TTF_INSTRUMENT_ARRAY_SIZE 1000000

class CStrategy;
class listener;
class session;
class CTickTradingFramework
{
public:
	CTickTradingFramework(std::string WebServer, std::string Admin, std::string AdminPassword, std::string TdHost, std::string MdHost, std::string User, std::string Password, std::string UserProductInfo, const std::string LIP, const std::string MAC, const std::string HD, std::string level2_interface_address, std::string Exchanges, std::string FrameCore, std::string TdCore, std::string MdCore);
	~CTickTradingFramework();

	void SetParameters(const std::string& LIP, const std::string& MAC, const std::string& HD);
	void Run();

	int DBConnect();

	// 行情订阅管理（引用计数，计数为0时退订）
	void Subscribe(const std::string& InstrumentID, CStrategy* Strategy);
	void UnSubscribe(const std::string& InstrumentID, CStrategy* Strategy);

	// Td
	void OnTdFrontConnected();
	void OnTdFrontDisconnected(int nReason);
	void OnRspTdUserLogin(TORASTOCKAPI::CTORATstpRspUserLoginField& RspUserLogin, TORASTOCKAPI::CTORATstpRspInfoField& RspInfo, int nRequestID);
	void OnRspQryShareholderAccount(TORASTOCKAPI::CTORATstpShareholderAccountField& ShareholderAccount, TORASTOCKAPI::CTORATstpRspInfoField& RspInfo, int nRequestID, bool bIsLast);
	void OnRspQrySecurity(TORASTOCKAPI::CTORATstpSecurityField& Security, TORASTOCKAPI::CTORATstpRspInfoField& RspInfo, int nRequestID, bool bIsLast);
	void OnRspQryOrder(TORASTOCKAPI::CTORATstpOrderField& Order, TORASTOCKAPI::CTORATstpRspInfoField& RspInfo, int nRequestID, bool bIsLast);
	void OnRspQryTrade(TORASTOCKAPI::CTORATstpTradeField& Trade, TORASTOCKAPI::CTORATstpRspInfoField& RspInfo, int nRequestID, bool bIsLast);
	void OnRtnOrder(TORASTOCKAPI::CTORATstpOrderField& Order);
	void OnRtnTrade(TORASTOCKAPI::CTORATstpTradeField& Trade);
	void OnRspOrderInsert(TORASTOCKAPI::CTORATstpInputOrderField& InputOrderField, TORASTOCKAPI::CTORATstpRspInfoField& RspInfoField, int nRequestID);
	void OnRspQryTradingAccount(TORASTOCKAPI::CTORATstpTradingAccountField& TradingAccount, TORASTOCKAPI::CTORATstpRspInfoField& RspInfo, int nRequestID, bool bIsLast);
	void OnRspQryPosition(TORASTOCKAPI::CTORATstpPositionField& Position, TORASTOCKAPI::CTORATstpRspInfoField& RspInfo, int nRequestID, bool bIsLast);
	///报单错误回报
	void OnErrRtnOrderInsert(TORASTOCKAPI::CTORATstpInputOrderField& InputOrderField, TORASTOCKAPI::CTORATstpRspInfoField& RspInfoField, int nRequestID);
	///撤单响应
	void OnRspOrderAction(TORASTOCKAPI::CTORATstpInputOrderActionField& InputOrderActionField, TORASTOCKAPI::CTORATstpRspInfoField& RspInfoField, int nRequestID);
	///撤单错误回报
	void OnErrRtnOrderAction(TORASTOCKAPI::CTORATstpInputOrderActionField& InputOrderActionField, TORASTOCKAPI::CTORATstpRspInfoField& RspInfoField, int nRequestID);
	///市场状态回报
	void OnRtnMarketStatus(TORASTOCKAPI::CTORATstpMarketStatusField& MarketStatus);

    // MD
	void MDOnFrontConnected();
	void MDOnFrontDisconnected(int nReason);
	void MDOnRspUserLogin(CTORATstpRspUserLoginField& RspUserLoginField);

    void OnRtnMarketData(TORALEV1API::CTORATstpMarketDataField& MarketDataField);

	// Timer
	void OnTime(const boost::system::error_code& error);

	// Control
	void OnRemoteConnected(session* s);
	void OnRemoteDisconnected(session* s);
	bool OnRemoteMessage(session* s, const std::string& request);
	void OnRemotePing(const json& request, std::string& response);
	void OnRemoteLogin(const json& request, std::string& response);
	void OnRemoteSubscribeMarketData(session* s, const json& request, std::string& response);
	void OnRemoteUnSubscribeMarketData(session* s, const json& request, std::string& response);
	void OnRemoteQueryStratety(const json& request, std::string& response);
	void OnRemoteAddStratety(const json& request, std::string& response);
	void OnRemoteDeleteStratety(const json& request, std::string& response);
	void OnRemoteModifyStratety(const json& request, std::string& response);
	void OnRemoteStartStopStratety(const json& request, std::string& response);
    void OnRemoteQryInstrument(const json& request, std::string& response);
    void OnRemoteQryInvestorPosition(const json& request, std::string& response);
    void OnRemoteQryTradingAccount(const json& request, std::string& response);

	// 小函数
	TORASTOCKAPI::CTORATstpSecurityField* GetInstrument(std::string InstrumentID);
	TORASTOCKAPI::CTORATstpTradingAccountField& GetTradingAccount();


	// 策略管理
	void AddStrategy(std::string StrategyID, std::string InstrumentIDs, CStrategy* pStrategy);
	void DelStrategy(std::string StrategyID, std::string InstrumentIDs);
	bool StrategyExists(const std::string StrategyID, const std::string InstrumentID);

public:
	CTdImpl* m_pTdImpl;
	CMDL2Impl* m_pMdL2Impl;
	std::shared_ptr<listener> m_pWebServer;
	std::string m_Admin;
	std::string m_AdminPassword;
	soci::session* m_db; // soci session

	std::map<std::string, size_t> m_mSubscribeRef; // 行情订阅引用计数，Key: InstrumentID
	std::map<std::string, CStrategy*> m_mStrategies; // 策略列表，Key：StrategyID.InstrumentID
	std::map<std::string, std::vector<CStrategy*> > m_mRelatedStrategies; // 与合约关联的策略列表, Key: InstrumentID
	TORASTOCKAPI::CTORATstpSecurityField* m_pInstruments; // 合约表，Key: InstrumentID
	std::map<std::string, TORASTOCKAPI::CTORATstpOrderField> m_mOrders; // 订单表，Key: ExchangeID.OrderSysID
	std::map<std::string, TORASTOCKAPI::CTORATstpTradeField> m_mTrades; // 成交表，key: ExchangeID.TradeID.OrderSysID
	std::map<std::string, TORASTOCKAPI::CTORATstpPositionField> m_mLongPositions; // 多头, Key: InstrumentID
	TORASTOCKAPI::CTORATstpTradingAccountField m_TradingAccount; // 资金
	std::map<std::string, TORASTOCKAPI::CTORATstpOrderField> m_mInputingOrders; // 等待交易所响应的报单，Key: FrontID.SessionID.OrderRef
	TORASTOCKAPI::TTORATstpDateType	m_TradingDay; // 交易日
	CWorkingStatus m_WorkingStatus; // 框架运行状态（初始化、运行中）
	TORASTOCKAPI::TTORATstpFrontIDType	m_nFrontID; // 前置编号
	TORASTOCKAPI::TTORATstpSessionIDType	m_nSessionID; // 会话编号
	TORASTOCKAPI::TTORATstpOrderRefType m_nMaxOrderRef; // 报单引用
	TORASTOCKAPI::TTORATstpShareholderIDType m_ShareholderID_SSE;
	TORASTOCKAPI::TTORATstpShareholderIDType m_ShareholderID_SZSE;
	std::string m_LIP, m_MAC, m_HD, m_UserProductInfo;
	moodycamel::ConcurrentQueue<TTF_Message_t*> m_queue;
	std::unordered_map<std::string, double> m_securityid_lastprice;
//#ifndef _WIN32
	int current_time; // micro_seconds
//#endif

    std::unordered_set<session*> m_websessions;
    void addsession(session* ses);
    void delsession(session* ses);

private:
    void HandleQueueMessage();
    MemoryPool m_pool;
    std::vector<std::string> m_vExchanges;
	bool IsTradingExchange(TTORATstpExchangeIDType ExchangeID);
};