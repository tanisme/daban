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


// ���ڴ洢����������Ϣ���ݵĿռ䣬�����ߴ����������õ���Ϣָ�루ָ����Ϣ�����е�һ����������Ҳ������Ϣ�������κ��޸ġ�
// ����������д����Ϣ��������Ϣָ���Ƶ��������У�������������ܲ�����δ��ʱ�������Ϣ���ᱻ�����߸��ǣ�����Ҫ���㹻�Ļ���ռ���Ӧ�����ܵĶ�����
// һ��3�����㹻�ˣ����3���ӻ�û�����꣬�Ǿ�����������ˣ�����10�����Ϣ�Ŀռ�ɡ�10w��x1000�ֽ�=100MB
#define TTF_MESSAGE_ARRAY_SIZE 100000 // ��Ϣ����

// �����Լ��Ϊ�̶�6λ��������Լ��Ϊ999999��ֱ���������±궨λ��Լ��Ϣ����Լ��600000����ϢΪInstruments[600000]
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

	// ���鶩�Ĺ������ü���������Ϊ0ʱ�˶���
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
	///��������ر�
	void OnErrRtnOrderInsert(TORASTOCKAPI::CTORATstpInputOrderField& InputOrderField, TORASTOCKAPI::CTORATstpRspInfoField& RspInfoField, int nRequestID);
	///������Ӧ
	void OnRspOrderAction(TORASTOCKAPI::CTORATstpInputOrderActionField& InputOrderActionField, TORASTOCKAPI::CTORATstpRspInfoField& RspInfoField, int nRequestID);
	///��������ر�
	void OnErrRtnOrderAction(TORASTOCKAPI::CTORATstpInputOrderActionField& InputOrderActionField, TORASTOCKAPI::CTORATstpRspInfoField& RspInfoField, int nRequestID);
	///�г�״̬�ر�
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

	// С����
	TORASTOCKAPI::CTORATstpSecurityField* GetInstrument(std::string InstrumentID);
	TORASTOCKAPI::CTORATstpTradingAccountField& GetTradingAccount();


	// ���Թ���
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

	std::map<std::string, size_t> m_mSubscribeRef; // ���鶩�����ü�����Key: InstrumentID
	std::map<std::string, CStrategy*> m_mStrategies; // �����б�Key��StrategyID.InstrumentID
	std::map<std::string, std::vector<CStrategy*> > m_mRelatedStrategies; // ���Լ�����Ĳ����б�, Key: InstrumentID
	TORASTOCKAPI::CTORATstpSecurityField* m_pInstruments; // ��Լ��Key: InstrumentID
	std::map<std::string, TORASTOCKAPI::CTORATstpOrderField> m_mOrders; // ������Key: ExchangeID.OrderSysID
	std::map<std::string, TORASTOCKAPI::CTORATstpTradeField> m_mTrades; // �ɽ���key: ExchangeID.TradeID.OrderSysID
	std::map<std::string, TORASTOCKAPI::CTORATstpPositionField> m_mLongPositions; // ��ͷ, Key: InstrumentID
	TORASTOCKAPI::CTORATstpTradingAccountField m_TradingAccount; // �ʽ�
	std::map<std::string, TORASTOCKAPI::CTORATstpOrderField> m_mInputingOrders; // �ȴ���������Ӧ�ı�����Key: FrontID.SessionID.OrderRef
	TORASTOCKAPI::TTORATstpDateType	m_TradingDay; // ������
	CWorkingStatus m_WorkingStatus; // �������״̬����ʼ���������У�
	TORASTOCKAPI::TTORATstpFrontIDType	m_nFrontID; // ǰ�ñ��
	TORASTOCKAPI::TTORATstpSessionIDType	m_nSessionID; // �Ự���
	TORASTOCKAPI::TTORATstpOrderRefType m_nMaxOrderRef; // ��������
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