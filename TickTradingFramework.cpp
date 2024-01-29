// TickTradingFrameworkHFT.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <thread>
#include <condition_variable>
#include <vector>
#include "spdlog/spdlog.h"
#include "boost/algorithm/string.hpp"
#include "TickTradingFramework.h"
#include "websocket_server_async.h"
#include "EntryStrategy.h"
#include "ExitStrategy.h"
#include "ToBuyStrategy.h"
#include "LimitExitStrategy.h"
#include "LimitT0Strategy.h"
#include "TrendT0Strategy.h"

extern std::shared_ptr<spdlog::logger> logger;

CTickTradingFramework::CTickTradingFramework(std::string WebServer, std::string Admin, std::string AdminPassword, std::string TdHost, std::string MdHost, std::string User, std::string Password, std::string UserProductInfo, const std::string LIP, const std::string MAC, const std::string HD, std::string level2_interface_address, std::string Exchanges, std::string WsCore, std::string TdCore, std::string MdCore):
	//m_timer(m_io_service, boost::posix_time::milliseconds(1000)),
	m_Admin(Admin),
	m_AdminPassword(AdminPassword)
{
	m_pInstruments = (TORASTOCKAPI::CTORATstpSecurityField*)calloc(TTF_INSTRUMENT_ARRAY_SIZE, sizeof(TORASTOCKAPI::CTORATstpSecurityField));
	boost::split(m_vExchanges, Exchanges, boost::is_any_of(","), boost::token_compress_off);
	std::vector<std::string> fields;
	boost::split(fields, WebServer, boost::is_any_of(":"), boost::token_compress_off);
	m_pWebServer = std::make_shared<listener>(boost::asio::ip::tcp::endpoint{ boost::asio::ip::make_address(fields[0].c_str()), (unsigned short)atol(fields[1].c_str()) }, this);
	m_pTdImpl = new CTdImpl(this, TdHost, User, Password, UserProductInfo, LIP, MAC,HD);
	m_pMdL2Impl = new CMDL2Impl(this, MdHost, level2_interface_address, MdCore, 5);
	memset(m_TradingDay, 0x00, sizeof(m_TradingDay));
	memset(&m_TradingAccount, 0x00, sizeof(m_TradingAccount));
	m_nMaxOrderRef = 1;
	m_nFrontID = 0;
	m_nSessionID = 0;
	m_WorkingStatus = WORKING_STATUS_INITING;
	m_UserProductInfo = UserProductInfo;
	m_LIP = LIP;
	m_MAC = MAC;
	m_HD = HD;
	current_time = 0;
}

CTickTradingFramework::~CTickTradingFramework()
{

}

void CTickTradingFramework::SetParameters(const std::string& LIP, const std::string& MAC, const std::string& HD)
{
	m_LIP = LIP;
	m_MAC = MAC;
	m_HD = HD;
}

void CTickTradingFramework::HandleQueueMessage()
{
    // 从队列中获取消息（消息指针）
    TTF_Message_t* pMessage = nullptr;
    while (true)
    {
		if (!m_queue.try_dequeue(pMessage))
			continue;
        switch (pMessage->MsgType)
        {
            case MsgTypeTdFrontConnected:
                OnTdFrontConnected();
                break;
            case MsgTypeTdFrontDisconnected:
            {
                MsgTdFrontDisconnected_t* pMsgTdFrontDisconnected = (MsgTdFrontDisconnected_t*)&pMessage->Message;
                OnTdFrontDisconnected(pMsgTdFrontDisconnected->nReason);
            }
                break;
            case MsgTypeRspTdUserLogin:
            {
                MsgRspTdUserLogin_t* pMsgRspTdUserLogin = (MsgRspTdUserLogin_t*)&pMessage->Message;
                OnRspTdUserLogin(pMsgRspTdUserLogin->RspUserLogin, pMsgRspTdUserLogin->RspInfo, pMsgRspTdUserLogin->nRequestID);
            }
                break;
            case MsgTypeRspQryShareholderAccount:
            {
                MsgRspQryShareholderAccount_t* pMsgRspQryShareholderAccount = (MsgRspQryShareholderAccount_t*)&pMessage->Message;
                OnRspQryShareholderAccount(pMsgRspQryShareholderAccount->ShareholderAccount, pMsgRspQryShareholderAccount->RspInfo, pMsgRspQryShareholderAccount->nRequestID, pMsgRspQryShareholderAccount->bIsLast);
            }
                break;
            case MsgTypeRspQrySecurity:
            {
                MsgRspQrySecurity_t* pMsgRspQrySecurity = (MsgRspQrySecurity_t*)&pMessage->Message;
                OnRspQrySecurity(pMsgRspQrySecurity->Security, pMsgRspQrySecurity->RspInfo, pMsgRspQrySecurity->nRequestID, pMsgRspQrySecurity->bIsLast);
            }
                break;
            case MsgTypeRspQryOrder:
            {
                MsgRspQryOrder_t* pMsgRspQryOrder = (MsgRspQryOrder_t*)&pMessage->Message;
                OnRspQryOrder(pMsgRspQryOrder->Order, pMsgRspQryOrder->RspInfo, pMsgRspQryOrder->nRequestID, pMsgRspQryOrder->bIsLast);
            }
                break;
            case MsgTypeRspQryTrade:
            {
                MsgRspQryTrade_t* pMsgRspQryTrade = (MsgRspQryTrade_t*)&pMessage->Message;
                OnRspQryTrade(pMsgRspQryTrade->Trade, pMsgRspQryTrade->RspInfo, pMsgRspQryTrade->nRequestID, pMsgRspQryTrade->bIsLast);
            }
                break;
            case MsgTypeRtnOrder:
            {
                MsgRtnOrder_t* pMsgRtnOrder = (MsgRtnOrder_t*)&pMessage->Message;
                OnRtnOrder(pMsgRtnOrder->Order);
            }
                break;
            case MsgTypeRtnTrade:
            {
                MsgRtnTrade_t* pMsgRtnTrade = (MsgRtnTrade_t*)&pMessage->Message;
                OnRtnTrade(pMsgRtnTrade->Trade);
            }
                break;
            case MsgTypeRspOrderInsert:
            {
                MsgRspOrderInsert_t* pMsgRspOrderInsert = (MsgRspOrderInsert_t*)&pMessage->Message;
                OnRspOrderInsert(pMsgRspOrderInsert->InputOrder, pMsgRspOrderInsert->RspInfo, pMsgRspOrderInsert->nRequestID);
            }
                break;
            case MsgTypeRspOrderAction:
            {
                MsgRspOrderAction_t* pMsgRspOrderAction = (MsgRspOrderAction_t*)&pMessage->Message;
                OnRspOrderAction(pMsgRspOrderAction->InputOrderAction, pMsgRspOrderAction->RspInfo, pMsgRspOrderAction->nRequestID);
            }
                break;
            case MsgTypeRtnMarketStatus:
            {
                MsgRtnMarketStatus_t* pMsgRtnMarketStatus = (MsgRtnMarketStatus_t*)&pMessage->Message;
                OnRtnMarketStatus(pMsgRtnMarketStatus->MarketStatus);
            }
                break;
            case MsgTypeRspQryPosition:
            {
                MsgRspQryPosition_t* pMsgRspQryPosition = (MsgRspQryPosition_t*)&pMessage->Message;
                OnRspQryPosition(pMsgRspQryPosition->Position, pMsgRspQryPosition->RspInfo, pMsgRspQryPosition->nRequestID, pMsgRspQryPosition->bIsLast);
            }
                break;
            case MsgTypeRspQryTradingAccount:
            {
                MsgRspQryTradingAccount_t* pMsgRspQryTradingAccount = (MsgRspQryTradingAccount_t*)&pMessage->Message;
                OnRspQryTradingAccount(pMsgRspQryTradingAccount->TradingAccount, pMsgRspQryTradingAccount->RspInfo, pMsgRspQryTradingAccount->nRequestID, pMsgRspQryTradingAccount->bIsLast);
            }
                break;
            case MsgTypeMdFrontConnected:
            {
                MDOnFrontConnected();
            }
                break;
            case MsgTypeMdFrontDisconnected:
            {
                auto pMsgMdFrontDisconnected  = (MsgMdFrontDisconnected_t*)&pMessage->Message;
                MDOnFrontDisconnected(pMsgMdFrontDisconnected->nReason);
            }
                break;
            case MsgTypeRspMdUserLogin:
            {
                auto pMsgRspMdUserLogin = (MsgRspMdUserLogin_t *)&pMessage->Message;
                MDOnRspUserLogin(pMsgRspMdUserLogin->RspUserLogin );
            }
                break;
			case MsgTypeClientConnected:
			{
				auto pMsgClientConnected = (MsgClientConnected_t*)&pMessage->Message;
				OnRemoteConnected(pMsgClientConnected->s);
			}
				break;
			case MsgTypeClientDisconnected:
			{
				auto pMsgClientDisconnected = (MsgClientDisconnected_t*)&pMessage->Message;
				OnRemoteDisconnected(pMsgClientDisconnected->s);
			}
				break;
			case MsgTypeClientRequest:
			{
				auto pMsgClientRequest = (MsgClientRequest_t*)&pMessage->Message;
				OnRemoteMessage(pMsgClientRequest->s,std::string(pMsgClientRequest->request));
			}
				break;
			default:
                break;
        };
    }
}

void CTickTradingFramework::Run()
{
	if (DBConnect() < 0) {
		return;
	}

	m_pTdImpl->Run();

	// Handle Messages
	HandleQueueMessage();
}

TORASTOCKAPI::CTORATstpSecurityField* CTickTradingFramework::GetInstrument(const std::string InstrumentID)
{
	auto iter = m_pInstruments+atol(InstrumentID.c_str());
	if (iter->SecurityID[0] == '\0')
		return nullptr;

	return iter;
}

TORASTOCKAPI::CTORATstpTradingAccountField& CTickTradingFramework::GetTradingAccount()
{
	return m_TradingAccount;
}

void CTickTradingFramework::AddStrategy(std::string StrategyID, std::string InstrumentIDs, CStrategy* pStrategy)
{
	m_mStrategies[StrategyID + "." + InstrumentIDs] = pStrategy;
}

void CTickTradingFramework::DelStrategy(std::string StrategyID, std::string InstrumentIDs)
{
	auto iter = m_mStrategies.find(StrategyID + "." + InstrumentIDs);
	if (iter != m_mStrategies.end()) {
		delete iter->second;
		m_mStrategies.erase(iter);
	}
}

// 订阅行情
void CTickTradingFramework::Subscribe(const std::string& InstrumentID, CStrategy* pStrategy)
{
	// 关联策略
	if(pStrategy)
		m_mRelatedStrategies[InstrumentID].push_back(pStrategy);

	// 订阅行情,如果已经订阅了就增加引用计数
	auto iter = m_mSubscribeRef.find(InstrumentID);
	if (iter == m_mSubscribeRef.end()) {
		m_mSubscribeRef[InstrumentID] = 1;
		
		//if (m_WorkingStatus == WORKING_STATUS_RUNNING) {
		//	char* pInstrumentIDs[1];
		//	pInstrumentIDs[0] = (char*)InstrumentID.c_str();
		//	m_pMdL2Impl->GetApi()->SubscribeOrderDetail(pInstrumentIDs, 1, GetInstrument(InstrumentID)->ExchangeID);
		//	m_pMdL2Impl->GetApi()->SubscribeTransaction(pInstrumentIDs, 1, GetInstrument(InstrumentID)->ExchangeID);
		//	m_pMdL2Impl->GetApi()->SubscribeNGTSTick(pInstrumentIDs, 1, GetInstrument(InstrumentID)->ExchangeID);
		//}
	}
	else
		iter->second++;
}

// 退订行情
void CTickTradingFramework::UnSubscribe(const std::string& InstrumentID, CStrategy* pStrategy)
{
	// 取关策略
	if (pStrategy) {
		auto iter = m_mRelatedStrategies.find(InstrumentID);
		if (iter != m_mRelatedStrategies.end()) {
			for (auto iterStrategy = iter->second.begin(); iterStrategy != iter->second.end(); iterStrategy++) {
				if (*iterStrategy == pStrategy) {
					iter->second.erase(iterStrategy);
					break;
				}
			}
		}
	}

	// 退订行情，只在引用计数为0的时候退订
	auto iter = m_mSubscribeRef.find(InstrumentID);
	if (iter == m_mSubscribeRef.end())
		return;
	if (0 == --iter->second) {
		m_mSubscribeRef.erase(InstrumentID);

		//if (m_WorkingStatus == WORKING_STATUS_RUNNING) {
		//	char* pInstrumentIDs[1];
		//	pInstrumentIDs[0] = (char*)InstrumentID.c_str();
		//	m_pMdL2Impl->GetApi()->UnSubscribeOrderDetail(pInstrumentIDs, 1, GetInstrument(InstrumentID)->ExchangeID);
		//	m_pMdL2Impl->GetApi()->UnSubscribeTransaction(pInstrumentIDs, 1, GetInstrument(InstrumentID)->ExchangeID);
		//	m_pMdL2Impl->GetApi()->UnSubscribeNGTSTick(pInstrumentIDs, 1, GetInstrument(InstrumentID)->ExchangeID);
		//}
	}
}


void CTickTradingFramework::OnTdFrontConnected()
{
	std::cout << "交易通道已连接." << std::endl;

	TORASTOCKAPI::CTORATstpReqUserLoginField LoginReq;

	memset(&LoginReq, 0x00, sizeof(LoginReq));
	LoginReq.LogInAccountType = TORASTOCKAPI::TORA_TSTP_LACT_UserID;
	//strncpy(LoginReq.DepartmentID, pReqUserLogin->BrokerID, sizeof(LoginReq.DepartmentID) - 1);
	strncpy(LoginReq.LogInAccount, m_pTdImpl->m_UserID.c_str(), sizeof(LoginReq.LogInAccount) - 1);
	strncpy(LoginReq.Password, m_pTdImpl->m_Password.c_str(), sizeof(LoginReq.Password) - 1);
	strncpy(LoginReq.UserProductInfo, m_pTdImpl->m_UserProductInfo.c_str(), sizeof(LoginReq.UserProductInfo) - 1);
	snprintf(LoginReq.TerminalInfo, sizeof(LoginReq.TerminalInfo), "PC;IIP=NA;IPORT=NA;LIP=%s;MAC=%s;HD=%s;@%s",
		m_LIP.c_str(), m_MAC.c_str(), m_HD.c_str(), m_UserProductInfo.c_str());

	m_pTdImpl->m_pTdApi->ReqUserLogin(&LoginReq, 0);
}
void CTickTradingFramework::OnTdFrontDisconnected(int nReason)
{
	std::cout << "交易通道已断开." << std::endl;
}

void CTickTradingFramework::OnRspTdUserLogin(TORASTOCKAPI::CTORATstpRspUserLoginField& RspUserLogin, TORASTOCKAPI::CTORATstpRspInfoField& RspInfo, int nRequestID)
{
	if (RspInfo.ErrorID != 0) {
		std::cout << "交易通道登录失败: " << RspInfo.ErrorMsg << std::endl;
		return;
	}
	std::cout << "交易通道登录成功." << std::endl;

	m_nFrontID = RspUserLogin.FrontID;
	m_nSessionID = RspUserLogin.SessionID;
	m_nMaxOrderRef = 1;

	// 如果不是新交易日且已经初始化完成就不再重复初始化
	if (strcmp(RspUserLogin.TradingDay,m_TradingDay)==0 && m_WorkingStatus == WORKING_STATUS_RUNNING)
		return;

	// Reset
	strncpy(m_TradingDay, RspUserLogin.TradingDay, sizeof(m_TradingDay) - 1);
	m_WorkingStatus = WORKING_STATUS_INITING;
	m_mInputingOrders.clear();
	m_mOrders.clear();
	m_mTrades.clear();
	m_mLongPositions.clear();
	memset(&m_TradingAccount, 0x00, sizeof(m_TradingAccount));

	// 查询股东账号
	TORASTOCKAPI::CTORATstpQryShareholderAccountField Req;
	memset(&Req, 0x00, sizeof(Req));
	m_pTdImpl->m_pTdApi->ReqQryShareholderAccount(&Req, 0);
}

void CTickTradingFramework::OnRspQryShareholderAccount(TORASTOCKAPI::CTORATstpShareholderAccountField& ShareholderAccount, TORASTOCKAPI::CTORATstpRspInfoField& RspInfo, int nRequestID, bool bIsLast)
{
	switch (ShareholderAccount.ExchangeID) {
	case TORASTOCKAPI::TORA_TSTP_EXD_SSE:
		strcpy(m_ShareholderID_SSE, ShareholderAccount.ShareholderID);
		break;
	case TORASTOCKAPI::TORA_TSTP_EXD_SZSE:
		strcpy(m_ShareholderID_SZSE, ShareholderAccount.ShareholderID);
		break;
	case TORASTOCKAPI::TORA_TSTP_EXD_BSE:
		break;
	case TORASTOCKAPI::TORA_TSTP_EXD_HK:
		break;
	default:
		break;
	}

	if (bIsLast) {
		// 查询合约
		printf("查询合约 ...\n");
		std::this_thread::sleep_for(std::chrono::seconds(1));
		TORASTOCKAPI::CTORATstpQrySecurityField Req = { 0 };
		m_pTdImpl->m_pTdApi->ReqQrySecurity(&Req, 0);
	}
}

void CTickTradingFramework::OnRspQrySecurity(TORASTOCKAPI::CTORATstpSecurityField& Security, TORASTOCKAPI::CTORATstpRspInfoField& RspInfo, int nRequestID, bool bIsLast)
{
	if (Security.SecurityID[0] != '\0' && Security.bPriceLimit &&
         (Security.SecurityType == TORASTOCKAPI::TORA_TSTP_STP_SHAShares ||
         Security.SecurityType == TORASTOCKAPI::TORA_TSTP_STP_SHKC ||
         Security.SecurityType == TORASTOCKAPI::TORA_TSTP_STP_SZCDR ||
         Security.SecurityType == TORASTOCKAPI::TORA_TSTP_STP_SZGEM ||
         Security.SecurityType == TORASTOCKAPI::TORA_TSTP_STP_SZMainAShares)) {
		memcpy(m_pInstruments + atol(Security.SecurityID), &Security, sizeof(Security));
	}

	if (!bIsLast)
		return;

	std::cout << "合约查询完成" << std::endl;

	// 流控
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	// 查询订单
	TORASTOCKAPI::CTORATstpQryOrderField Req;
	int r = 0;

	memset(&Req, 0x00, sizeof(Req));
	strncpy(Req.InvestorID, m_pTdImpl->m_UserID.c_str(), sizeof(Req.InvestorID) - 1);
	while ((r = m_pTdImpl->m_pTdApi->ReqQryOrder(&Req, 0)) == -2 || r == -3)
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

void CTickTradingFramework::OnRspQryOrder(TORASTOCKAPI::CTORATstpOrderField& Order, TORASTOCKAPI::CTORATstpRspInfoField& RspInfo, int nRequestID, bool bIsLast)
{
	if (RspInfo.ErrorID != 0) {
		std::cout << "查询订单失败:" << RspInfo.ErrorMsg << std::endl;
		return;
	}

	if (Order.SecurityID[0] != '\0') {
		if (Order.OrderSysID[0] != '\0') {
			m_mOrders[std::string(1, Order.ExchangeID) + "." + Order.OrderSysID] = Order;
		}
		else if (Order.OrderStatus == TORASTOCKAPI::TORA_TSTP_OST_Unknown) {
			// 只保存活跃订单（被拒绝的单子忽略）
			m_mInputingOrders[std::to_string(Order.FrontID) + "." + std::to_string(Order.SessionID) + "." + std::to_string(Order.OrderRef)] = Order;
		}
	}

	if (!bIsLast)
		return;
	std::cout << "查询订单成功:" << RspInfo.ErrorMsg << std::endl;

	// 流控
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	// 查询成交
	TORASTOCKAPI::CTORATstpQryTradeField Req;
	int r = 0;

	memset(&Req, 0x00, sizeof(Req));
	strncpy(Req.InvestorID, m_pTdImpl->m_UserID.c_str(), sizeof(Req.InvestorID) - 1);
	while ((r = m_pTdImpl->m_pTdApi->ReqQryTrade(&Req, 0)) == -2 || r == -3)
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

void CTickTradingFramework::OnRspQryTrade(TORASTOCKAPI::CTORATstpTradeField& Trade, TORASTOCKAPI::CTORATstpRspInfoField& RspInfo, int nRequestID, bool bIsLast)
{
	if (RspInfo.ErrorID != 0) {
		std::cout << "查询成交失败:" << RspInfo.ErrorMsg << std::endl;
		return;
	}

	if (Trade.SecurityID[0] != '\0') {
		std::string key = std::string(1, Trade.ExchangeID) + "." + Trade.TradeID + "." + Trade.OrderSysID;
		m_mTrades[key] = Trade;
	}

	if (!bIsLast)
		return;
	std::cout << "查询成交成功:" << RspInfo.ErrorMsg << std::endl;

	// 流控
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	TORASTOCKAPI::CTORATstpQryTradingAccountField Req;
	int r = 0;

	memset(&Req, 0x00, sizeof(Req));
	strncpy(Req.InvestorID, m_pTdImpl->m_UserID.c_str(), sizeof(Req.InvestorID) - 1);
	while ((r = m_pTdImpl->m_pTdApi->ReqQryTradingAccount(&Req, 0)) == -2 || r == -3)
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

}

void CTickTradingFramework::OnRspOrderInsert(TORASTOCKAPI::CTORATstpInputOrderField& InputOrder, TORASTOCKAPI::CTORATstpRspInfoField& RspInfo, int nRequestID)
{
	if (RspInfo.ErrorID != 0) {
		logger->error("OnRspOrderInsert:{} - {}", RspInfo.ErrorID, RspInfo.ErrorMsg);

		// 下单失败，释放资金与持仓冻结

		auto pInstrument = GetInstrument(InputOrder.SecurityID);
		if (pInstrument == nullptr)
			return;

		std::map<std::string, TORASTOCKAPI::CTORATstpPositionField>::iterator iterPosition;

		if (InputOrder.Direction == TORASTOCKAPI::TORA_TSTP_D_Buy) { // 买开
			if ((iterPosition = m_mLongPositions.find(InputOrder.SecurityID)) == m_mLongPositions.end()) {
				m_mLongPositions[InputOrder.SecurityID] = TORASTOCKAPI::CTORATstpPositionField({ 0 });
				iterPosition = m_mLongPositions.find(InputOrder.SecurityID);
			}
			//iterPosition->second.LongFrozen -= InputOrder.VolumeTotalOriginal;
		}			
		else { // 卖平
			if ((iterPosition = m_mLongPositions.find(InputOrder.SecurityID)) == m_mLongPositions.end()) {
				return;
			}
			//iterPosition->second.ShortFrozen -= InputOrder.VolumeTotalOriginal;
		}


		// 清除错单
		std::string key = std::to_string(m_nFrontID) + "." + std::to_string(m_nSessionID) + "." + std::to_string(InputOrder.OrderRef);
		auto iterInputingOrder = m_mInputingOrders.find(key);
		if (iterInputingOrder == m_mInputingOrders.end())
			return;
		m_mInputingOrders.erase(iterInputingOrder);

		// 找到订单所属的策略
		auto iter = m_mStrategies.find(std::string(InputOrder.SInfo) + "." + InputOrder.SecurityID);
		if (iter != m_mStrategies.end()) {
			iter->second->OnRspOrderInsert(InputOrder, RspInfo, nRequestID);
		}

		return;
	}
}

void CTickTradingFramework::OnRspOrderAction(TORASTOCKAPI::CTORATstpInputOrderActionField& InputOrderAction, TORASTOCKAPI::CTORATstpRspInfoField& RspInfo, int nRequestID)
{
	logger->error("OnRspOrderAction:{} - {}", RspInfo.ErrorID, RspInfo.ErrorMsg);
}

void CTickTradingFramework::OnRspQryPosition(TORASTOCKAPI::CTORATstpPositionField& InvestorPosition, TORASTOCKAPI::CTORATstpRspInfoField& RspInfo, int nRequestID, bool bIsLast)
{
	if (RspInfo.ErrorID != 0) {
		std::cout << "查询持仓失败:" << RspInfo.ErrorMsg << std::endl;
		return;
	}

	// 计算上日持仓
	if (InvestorPosition.SecurityID[0] != '\0') {
		TORASTOCKAPI::CTORATstpPositionField Position = { 0 };
		Position.CurrentPosition = InvestorPosition.PrePosition;
		m_mLongPositions[InvestorPosition.SecurityID] = Position;
	}
	
	if (!bIsLast)
		return;
	std::cout << "查询持仓成功:" << RspInfo.ErrorMsg << std::endl;

	// 计算资金、持仓
	// 
	// 通过成交明细更新持仓
	// 
	for (auto iterTrade = m_mTrades.begin(); iterTrade != m_mTrades.end(); iterTrade++) {
		auto pInstrument = GetInstrument(iterTrade->second.SecurityID);
        if (pInstrument == nullptr)
            continue;

		std::map<std::string, TORASTOCKAPI::CTORATstpPositionField>::iterator iterPosition;

		if (iterTrade->second.Direction == TORASTOCKAPI::TORA_TSTP_D_Buy) { // 买开
			if ((iterPosition = m_mLongPositions.find(iterTrade->second.SecurityID)) == m_mLongPositions.end()) {
				m_mLongPositions[iterTrade->second.SecurityID] = TORASTOCKAPI::CTORATstpPositionField({ 0 });
				iterPosition = m_mLongPositions.find(iterTrade->second.SecurityID);
			}
			iterPosition->second.CurrentPosition += iterTrade->second.Volume;
		}
		else { // 卖平
			if ((iterPosition = m_mLongPositions.find(iterTrade->second.SecurityID)) == m_mLongPositions.end()) {
				return;
			}
			iterPosition->second.CurrentPosition -= iterTrade->second.Volume;
		}
	}

	// 通过未成交委托明细冻结持仓（m_mOrders）
	for (auto iterOrder = m_mOrders.begin(); iterOrder != m_mOrders.end(); iterOrder++) {
		if (iterOrder->second.OrderStatus == TORASTOCKAPI::TORA_TSTP_OST_AllCanceled
			|| iterOrder->second.OrderStatus == TORASTOCKAPI::TORA_TSTP_OST_PartTradeCanceled
			|| iterOrder->second.OrderStatus == TORASTOCKAPI::TORA_TSTP_OST_Rejected
			|| iterOrder->second.OrderStatus == TORASTOCKAPI::TORA_TSTP_OST_AllTraded)
			continue;
		
		auto pInstrument = GetInstrument(iterOrder->second.SecurityID);
		if (pInstrument == nullptr)
			continue;

		// 资金与持仓冻结
		
		std::map<std::string, TORASTOCKAPI::CTORATstpPositionField>::iterator iterPosition;

		if (iterOrder->second.Direction == TORASTOCKAPI::TORA_TSTP_D_Buy) { // 买开
			if ((iterPosition = m_mLongPositions.find(iterOrder->second.SecurityID)) == m_mLongPositions.end()) {
				m_mLongPositions[iterOrder->second.SecurityID] = TORASTOCKAPI::CTORATstpPositionField({ 0 });
				iterPosition = m_mLongPositions.find(iterOrder->second.SecurityID);
			}
			//iterPosition->second.LongFrozen += iterOrder->second.VolumeTotalOriginal - iterOrder->second.VolumeTraded;
		}
				
		else { // 卖平
			if ((iterPosition = m_mLongPositions.find(iterOrder->second.SecurityID)) == m_mLongPositions.end()) {
				return;
			}
			//iterPosition->second.ShortFrozen += iterOrder->second.VolumeTotalOriginal - iterOrder->second.VolumeTraded;
		}
	}

	// 通过未成交委托明细冻结持仓（m_mInputingOrders）
	for (auto iterOrder = m_mInputingOrders.begin(); iterOrder != m_mInputingOrders.end(); iterOrder++) {
		if (iterOrder->second.OrderStatus == TORASTOCKAPI::TORA_TSTP_OST_AllCanceled
			|| iterOrder->second.OrderStatus == TORASTOCKAPI::TORA_TSTP_OST_PartTradeCanceled
			|| iterOrder->second.OrderStatus == TORASTOCKAPI::TORA_TSTP_OST_Rejected
			|| iterOrder->second.OrderStatus == TORASTOCKAPI::TORA_TSTP_OST_AllTraded)
			continue;

		auto pInstrument = GetInstrument(iterOrder->second.SecurityID);
		if (pInstrument == nullptr)
			continue;

		// 资金与持仓冻结
		std::map<std::string, TORASTOCKAPI::CTORATstpPositionField>::iterator iterPosition;

		if (iterOrder->second.Direction == TORASTOCKAPI::TORA_TSTP_D_Buy) { // 买开
			if ((iterPosition = m_mLongPositions.find(iterOrder->second.SecurityID)) == m_mLongPositions.end()) {
				m_mLongPositions[iterOrder->second.SecurityID] = TORASTOCKAPI::CTORATstpPositionField({ 0 });
				iterPosition = m_mLongPositions.find(iterOrder->second.SecurityID);
			}
			//iterPosition->second.LongFrozen += iterOrder->second.VolumeTotalOriginal - iterOrder->second.VolumeTraded;
		}

		else { // 卖平
			if ((iterPosition = m_mLongPositions.find(iterOrder->second.SecurityID)) == m_mLongPositions.end()) {
				return;
			}
			//iterPosition->second.ShortFrozen += iterOrder->second.VolumeTotalOriginal - iterOrder->second.VolumeTraded;
		}
	}

	// 查询保证金率、手续费率

	m_WorkingStatus = WORKING_STATUS_RUNNING;

	std::cout << "初始化完成。" << std::endl;

	// 启动监控服务
	m_pWebServer->run();

	// 启动行情
	m_pMdL2Impl->Start();
}


int CTickTradingFramework::DBConnect()
{
	try {
		// 打开数据库
		soci::session* db = new soci::session("sqlite3", "db=balibali.db");

		*db << "CREATE TABLE IF NOT EXISTS strategy (strategy CHAR(30), instrument CHAR(30) , parameters TEXT, status CHAR(15), message CHAR(100), PRIMARY KEY(strategy,instrument))";

		m_db = db;
	}
	//catch (const soci::mysql_soci_error& e)
	//{
	//	logsend(lp, LOG_ERROR, "System", "Table tblOrder insert failed![%s]", (const char*)e.what());
	//	return -1;
	//}
	catch (const soci::soci_error& e)
	{
		std::cout << e.what() << std::endl;
		logger->error("DB connect failed.{}", (const char*)e.what());
		return -1;
	}
	catch (...)
	{
		std::cout << "DB connect failed." << std::endl;
		logger->error("DB connect failed.");
		return -1;
	}

	// 所有策略置为停止状态
	*m_db << "UPDATE strategy set status='stopped'";

	return 0;
}


bool CTickTradingFramework::StrategyExists(const std::string StrategyID, const std::string InstrumentID)
{
	// 查询策略
	int nExists = 0;

	*m_db << "SELECT count(*) FROM strategy where strategy=:strategy and instrument=:instrument", soci::use(StrategyID), soci::use(InstrumentID), soci::into(nExists);

	return nExists != 0;
}

void CTickTradingFramework::OnRspQryTradingAccount(TORASTOCKAPI::CTORATstpTradingAccountField& TradingAccount, TORASTOCKAPI::CTORATstpRspInfoField& RspInfo, int nRequestID, bool bIsLast)
{
	if (RspInfo.ErrorID != 0) {
		std::cout << "查询资金失败:" << RspInfo.ErrorMsg << std::endl;
		return;
	}

	if (TradingAccount.AccountID[0] != '\0') {
		if (TradingAccount.CurrencyID == TORASTOCKAPI::TORA_TSTP_CID_CNY)
			memcpy(&m_TradingAccount, &TradingAccount, sizeof(m_TradingAccount));
	}

	if (!bIsLast)
		return;

	std::cout << "查询资金成功:" << RspInfo.ErrorMsg << std::endl;

	// 流控
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	TORASTOCKAPI::CTORATstpQryPositionField Req;
	int r = 0;

	memset(&Req, 0x00, sizeof(Req));
	strncpy(Req.InvestorID, m_pTdImpl->m_UserID.c_str(), sizeof(Req.InvestorID) - 1);
	while ((r = m_pTdImpl->m_pTdApi->ReqQryPosition(&Req, 0)) == -2 || r == -3)
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

//
// 委托处理：
// 开仓：冻结资金
// 平仓：冻结仓位
// 委托回报中不关注成交，只关注受理、撤单情况，委托的成交情况在成交回报中处理。
// 
void CTickTradingFramework::OnRtnOrder(TORASTOCKAPI::CTORATstpOrderField& Order)
{
	if (m_WorkingStatus != WORKING_STATUS_RUNNING) {
		// 初始化期间只保存一下成交单即可，待初始化完成后统一计算
		if (Order.OrderStatus == TORASTOCKAPI::TORA_TSTP_OST_Unknown) {
			if (Order.OrderSysID[0] == '\0') {
				std::string key = std::to_string(Order.FrontID) + "." + std::to_string(Order.SessionID) + "." + std::to_string(Order.OrderRef);
				m_mInputingOrders[key] = Order;
			}
			else {
				std::string key = std::string(1, Order.ExchangeID) + "." + Order.OrderSysID;
				m_mOrders[key] = Order;
			}
		}
		else if (Order.OrderSysID[0] != '\0') {
			std::string key = std::string(1, Order.ExchangeID) + "." + Order.OrderSysID;
			m_mOrders[key] = Order;
		}

		return;
	}

	if (Order.OrderStatus == TORASTOCKAPI::TORA_TSTP_OST_Unknown) {
		std::string key = std::to_string(Order.FrontID) + "." + std::to_string(Order.SessionID) + "." + std::to_string(Order.OrderRef);
		std::map<std::string, TORASTOCKAPI::CTORATstpOrderField>::iterator iterInputingOrder;

		if ((iterInputingOrder = m_mInputingOrders.find(key)) != m_mInputingOrders.end()) {
			if (Order.OrderSysID[0] != '\0') {
				m_mInputingOrders.erase(iterInputingOrder);
				m_mOrders[std::string(1, Order.ExchangeID) + "." + Order.OrderSysID] = Order;
			}
			return; // New Order Of Local Session
		}
		else {
			// New Order Of Other Sessions

			auto pInstrument = GetInstrument(Order.SecurityID);
			if (pInstrument == nullptr)
				return;

			// 新订单的资金与持仓冻结
			
			std::map<std::string, TORASTOCKAPI::CTORATstpPositionField>::iterator iterPosition;

			if (Order.Direction == TORASTOCKAPI::TORA_TSTP_D_Buy) { // 买开
				if ((iterPosition = m_mLongPositions.find(Order.SecurityID)) == m_mLongPositions.end()) {
					m_mLongPositions[Order.SecurityID] = TORASTOCKAPI::CTORATstpPositionField({ 0 });
					iterPosition = m_mLongPositions.find(Order.SecurityID);
				}
				//iterPosition->second.LongFrozen += Order.VolumeTotalOriginal;
			}
			else { // 卖平
				if ((iterPosition = m_mLongPositions.find(Order.SecurityID)) == m_mLongPositions.end()) {
					return;
				}
				//iterPosition->second.ShortFrozen += Order.VolumeTotalOriginal;
			}

			m_mInputingOrders[key] = Order;
		}
	}
	else if (Order.OrderStatus != TORASTOCKAPI::TORA_TSTP_OST_PartTradeCanceled
		&& Order.OrderStatus != TORASTOCKAPI::TORA_TSTP_OST_AllCanceled
		&& Order.OrderStatus != TORASTOCKAPI::TORA_TSTP_OST_Rejected) {
		// 保存OrderSysID
		std::string key = std::to_string(Order.FrontID) + "." + std::to_string(Order.SessionID) + "." + std::to_string(Order.OrderRef);
		std::map<std::string, TORASTOCKAPI::CTORATstpOrderField>::iterator iterInputingOrder;
		if ((iterInputingOrder = m_mInputingOrders.find(key)) != m_mInputingOrders.end()) {
			if (Order.OrderSysID[0] != '\0') {
				m_mInputingOrders.erase(iterInputingOrder);
				m_mOrders[std::string(1, Order.ExchangeID) + "." + Order.OrderSysID] = Order;
			}
		}
	}
	else if (Order.OrderStatus != TORASTOCKAPI::TORA_TSTP_OST_PartTradeCanceled
		&& Order.OrderStatus != TORASTOCKAPI::TORA_TSTP_OST_AllCanceled
		&& Order.OrderStatus != TORASTOCKAPI::TORA_TSTP_OST_Rejected) {
		// 订单被拒或者撤消，释放冻结资金和持仓

		auto pInstrument = GetInstrument(Order.SecurityID);
		if (pInstrument == nullptr)
			return;

		
		std::map<std::string, TORASTOCKAPI::CTORATstpPositionField>::iterator iterPosition;

		if (Order.Direction == TORASTOCKAPI::TORA_TSTP_D_Buy) { // 买开
			if ((iterPosition = m_mLongPositions.find(Order.SecurityID)) == m_mLongPositions.end()) {
				m_mLongPositions[Order.SecurityID] = TORASTOCKAPI::CTORATstpPositionField({ 0 });
				iterPosition = m_mLongPositions.find(Order.SecurityID);
			}
			//iterPosition->second.LongFrozen -= Order.VolumeTotalOriginal - Order.VolumeTraded;
		}
		else { // 卖平
			if ((iterPosition = m_mLongPositions.find(Order.SecurityID)) != m_mLongPositions.end()) {
				return;
			}
			//iterPosition->second.ShortFrozen -= Order.VolumeTotalOriginal - Order.VolumeTraded;
		}
	}
	
	// 找到订单所属的策略
	auto iter = m_mStrategies.find(std::string(Order.SInfo) + "." + Order.SecurityID);
	if (iter != m_mStrategies.end()) {
		iter->second->OnRtnOrder(Order);
	}
}

//
// 成交处理：
// 开仓：增加仓位，释放冻结资金，增加占用保证金。
// 平仓：减少仓位，释放冻结仓位，计算盈亏。
// 
void CTickTradingFramework::OnRtnTrade(TORASTOCKAPI::CTORATstpTradeField& Trade)
{
	if (m_WorkingStatus != WORKING_STATUS_RUNNING) {
		// 初始化期间只保存一下成交单即可，待初始化完成后统一计算
		std::string key = std::string(1, Trade.ExchangeID) + "." + Trade.TradeID + "." + Trade.OrderSysID;
		m_mTrades[key] = Trade;
		return;
	}

	auto pInstrument = GetInstrument(Trade.SecurityID);
	
	std::map<std::string, TORASTOCKAPI::CTORATstpPositionField>::iterator iterPosition;

	if (Trade.Direction == TORASTOCKAPI::TORA_TSTP_D_Buy) { // 买开
		if ((iterPosition = m_mLongPositions.find(Trade.SecurityID)) == m_mLongPositions.end()) {
			m_mLongPositions[Trade.SecurityID] = TORASTOCKAPI::CTORATstpPositionField({ 0 });
			iterPosition = m_mLongPositions.find(Trade.SecurityID);
		}
		iterPosition->second.CurrentPosition += Trade.Volume;
		//iterPosition->second.LongFrozen -= Trade.Volume;
	}
	else { // 卖平
		if ((iterPosition = m_mLongPositions.find(Trade.SecurityID)) == m_mLongPositions.end()) {
			return;
		}
		iterPosition->second.CurrentPosition -= Trade.Volume;
		//iterPosition->second.ShortFrozen -= Trade.Volume;
	}

	// 更新订单信息
	std::string key = std::string(1, Trade.ExchangeID) + "." + Trade.OrderSysID;
	auto Order = m_mOrders[key];
	Order.VolumeTraded += Trade.Volume;
	if (Order.VolumeTraded == Order.VolumeTotalOriginal)
		Order.OrderStatus = TORASTOCKAPI::TORA_TSTP_OST_AllTraded; // 完全成交
	else
		Order.OrderStatus = TORASTOCKAPI::TORA_TSTP_OST_PartTraded; // 部分成交

	// 找到订单所属的策略
	auto iter = m_mStrategies.find(std::string(Order.SInfo) + "." + Order.SecurityID);
	if (iter != m_mStrategies.end()) {
		iter->second->OnRtnTrade(Trade);
	}
}

void CTickTradingFramework::OnErrRtnOrderInsert(TORASTOCKAPI::CTORATstpInputOrderField& InputOrder, TORASTOCKAPI::CTORATstpRspInfoField& RspInfo, int nRequestID)
{
	logger->error("OnErrRtnOrderInsert:{} - {}", RspInfo.ErrorID, RspInfo.ErrorMsg);
}

void CTickTradingFramework::OnErrRtnOrderAction(TORASTOCKAPI::CTORATstpInputOrderActionField& InputOrderAction, TORASTOCKAPI::CTORATstpRspInfoField& RspInfo, int nRequestID)
{
	logger->error("OnErrRtnOrderAction:{} - {}", RspInfo.ErrorID, RspInfo.ErrorMsg);
}

void CTickTradingFramework::OnRtnMarketStatus(TORASTOCKAPI::CTORATstpMarketStatusField& MarketStatus)
{
	std::cout << "MarketStatus: ";
	switch (MarketStatus.MarketID)
	{
	case TORASTOCKAPI::TORA_TSTP_MKD_SHA:
		std::cout << "上海 - ";
		break;
	case TORASTOCKAPI::TORA_TSTP_MKD_SZA:
		std::cout << "深圳 - ";
		break;
	case TORASTOCKAPI::TORA_TSTP_MKD_BJMain:
		std::cout << "北京 - ";
		break;
	default:
		break;
	}

	switch (MarketStatus.MarketStatus)
	{
	case TORASTOCKAPI::TORA_TSTP_MST_BeforeTrading:
		std::cout << "开盘前";
		break;
	case TORASTOCKAPI::TORA_TSTP_MST_OpenCallAuction:
		std::cout << "集合竞价";
		break;
	case TORASTOCKAPI::TORA_TSTP_MST_Continous:
		std::cout << "连续交易";
		break;
	case TORASTOCKAPI::TORA_TSTP_MST_Closed:
		std::cout << "收盘";
		break;
	default:
		break;
	}
	std::cout << std::endl;
}


/************************************   MD   Start *********************************************/
void CTickTradingFramework::MDOnFrontConnected()
{
	printf("行情通道已连接!!!\n");
}

void CTickTradingFramework::MDOnFrontDisconnected(int nReason)
{
	printf("行情通道断开连接!!! 原因:%d\n", nReason);
}

bool CTickTradingFramework::IsTradingExchange(TTORATstpExchangeIDType ExchangeID)
{
	for (auto iter : m_vExchanges) {
		switch (ExchangeID)
		{
		case TORA_TSTP_EXD_SSE:
			if (iter == "SSE")
				return true;
			break;
		case TORA_TSTP_EXD_SZSE:
			if (iter == "SZSE")
				return true;
			break;
		default:
			break;
		}
	}
	return false;
}

void CTickTradingFramework::MDOnRspUserLogin(TORALEV2API::CTORATstpRspUserLoginField &RspUserLoginField)
{
	printf("行情通道登陆成功!!!TradingDay:%s FrontID:%d SessionID:%d\n", 
           RspUserLoginField.TradingDay, RspUserLoginField.FrontID, RspUserLoginField.SessionID);

	// 订阅行情（全市场订阅）
	if (m_WorkingStatus != WORKING_STATUS_RUNNING) return;
    auto subscribeCnt = 0;
    for (size_t i=0; i< TTF_INSTRUMENT_ARRAY_SIZE; i++) {
		TORASTOCKAPI::CTORATstpSecurityField* pInstrument = m_pInstruments + i;
		if (pInstrument->SecurityID[0] == '\0')
			continue;
		if (!IsTradingExchange(pInstrument->ExchangeID))
			continue;
        char* pInstrumentIDs[1];
        pInstrumentIDs[0] = (char*)pInstrument->SecurityID;
        if (pInstrument->ExchangeID == TORA_TSTP_EXD_SSE) {
            m_pMdL2Impl->GetApi()->SubscribeNGTSTick(pInstrumentIDs, 1, pInstrument->ExchangeID);
        } else {
            m_pMdL2Impl->GetApi()->SubscribeOrderDetail(pInstrumentIDs, 1, pInstrument->ExchangeID);
            m_pMdL2Impl->GetApi()->SubscribeTransaction(pInstrumentIDs, 1, pInstrument->ExchangeID);
        }
        subscribeCnt++;
    }
    printf("Subscribe Security Count:%d\n", subscribeCnt);
}

void CTickTradingFramework::OnRtnMarketData(TORALEV1API::CTORATstpMarketDataField& MarketData)
{
    // 找到与合约关联的策略，逐一回调
    auto iter = m_mRelatedStrategies.find(MarketData.SecurityID);
    if (iter != m_mRelatedStrategies.end()) {
        for (auto iterStrategy = iter->second.begin(); iterStrategy != iter->second.end(); iterStrategy++)
            (*iterStrategy)->OnRtnMarketData(MarketData);
    }

	// 找到订阅了该合约的客户端，逐一推送
    for (auto s : m_websessions) {
        if (!s) continue;
        auto iterClient = s->m_subInstrument.find(MarketData.SecurityID);
        if (iterClient != s->m_subInstrument.end()) {
			json msg;
			msg["MsgType"] = "OnRtnDepthMarketData";
			msg["DepthMarketData"]["InstrumentID"] = MarketData.SecurityID;
			msg["DepthMarketData"]["ExchangeID"] = MarketData.ExchangeID;
			msg["DepthMarketData"]["BidPrice1"] = MarketData.BidPrice1;
			msg["DepthMarketData"]["BidVolume1"] = MarketData.BidVolume1;
			msg["DepthMarketData"]["AskPrice1"] = MarketData.AskPrice1;
			msg["DepthMarketData"]["AskVolume1"] = MarketData.AskVolume1;
			msg["DepthMarketData"]["BidPrice2"] = MarketData.BidPrice2;
			msg["DepthMarketData"]["BidVolume2"] = MarketData.BidVolume2;
			msg["DepthMarketData"]["AskPrice2"] = MarketData.AskPrice2;
			msg["DepthMarketData"]["AskVolume2"] = MarketData.AskVolume2;
			msg["DepthMarketData"]["BidPrice3"] = MarketData.BidPrice3;
			msg["DepthMarketData"]["BidVolume3"] = MarketData.BidVolume3;
			msg["DepthMarketData"]["AskPrice3"] = MarketData.AskPrice3;
			msg["DepthMarketData"]["AskVolume3"] = MarketData.AskVolume3;
			msg["DepthMarketData"]["BidPrice4"] = MarketData.BidPrice4;
			msg["DepthMarketData"]["BidVolume4"] = MarketData.BidVolume4;
			msg["DepthMarketData"]["AskPrice4"] = MarketData.AskPrice4;
			msg["DepthMarketData"]["AskVolume4"] = MarketData.AskVolume4;
			msg["DepthMarketData"]["BidPrice5"] = MarketData.BidPrice5;
			msg["DepthMarketData"]["BidVolume5"] = MarketData.BidVolume5;
			msg["DepthMarketData"]["AskPrice5"] = MarketData.AskPrice5;
			msg["DepthMarketData"]["AskVolume5"] = MarketData.AskVolume5;
			std::string message = msg.dump();
            s->send_message(message);
        }
    }
}

/************************************   MD   End *********************************************/

void CTickTradingFramework::addsession(session *ses) {
    m_websessions.insert(ses);
}

void CTickTradingFramework::delsession(session *ses) {
    m_websessions.erase(ses);
}

void CTickTradingFramework::OnRemoteConnected(session* s)
{
	std::cout << "Client connected." << std::endl;
    addsession(s);
}

void CTickTradingFramework::OnRemoteDisconnected(session* s)
{
	std::cout << "Client disconnected." << std::endl;
    delsession(s);
}

bool CTickTradingFramework::OnRemoteMessage(session* s, const std::string& request)
{
	json req;
	std::string response;
    try
    {
        req = json::parse(request);
        auto it = req.find("MsgType");
        if (it == req.end()) return false;

        std::string MsgType;
        MsgType = req["MsgType"];

        if (MsgType == "ping") {
            OnRemotePing(req, response);
        }
        else if (MsgType == "login_req") {
            OnRemoteLogin(req, response);
        }
        else if (MsgType == "SubscribeMarketData") {
            OnRemoteSubscribeMarketData(s, req, response);
        }
        else if (MsgType == "UnSubscribeMarketData") {
            OnRemoteUnSubscribeMarketData(s, req, response);
        }
        else if (MsgType == "strategy_query_req") {
            OnRemoteQueryStratety(req, response);
        }
        else if (MsgType == "strategy_add_req") {
            OnRemoteAddStratety(req, response);
        }
        else if (MsgType == "strategy_delete_req") {
            OnRemoteDeleteStratety(req, response);
        }
        else if (MsgType == "strategy_modify_req") {
            OnRemoteModifyStratety(req, response);
        }
        else if (MsgType == "strategy_control_req") {
            OnRemoteStartStopStratety(req, response);
        }
        else if (MsgType == "ReqQryInstrument") {
            OnRemoteQryInstrument(req, response);
        }
        else if (MsgType == "ReqQryInvestorPosition") {
            OnRemoteQryInvestorPosition(req, response);
        }
        else if (MsgType == "ReqQryTradingAccount") {
            OnRemoteQryTradingAccount(req, response);
        }
    } catch (std::exception& e) {
        std::cout << " not a json string :" << e.what() << std::endl;
		return false;
    }

	s->SendResponseEvent(response);

	return true;
}

void CTickTradingFramework::OnRemotePing(const json& request, std::string& response)
{
	json rsp;
	rsp["MsgType"] = "ping";
	response = rsp.dump();
}

void CTickTradingFramework::OnRemoteLogin(const json& request, std::string& response)
{
	const json login_req = request["login_req"];
	std::string User = login_req["user"];
	std::string Password = login_req["password"];

	json rsp;
	rsp["MsgType"] = "login_rsp";
	if (m_AdminPassword != "") {
		if (User != m_Admin || Password != m_AdminPassword) {
			rsp["rsp_code"] = "1";
			rsp["rsp_message"] = toUTF8("用户名或密码错误");
		}
	}
	else {
		rsp["rsp_code"] = "0";
		rsp["rsp_message"] = "succeed";
	}
	response = rsp.dump();
}

void CTickTradingFramework::OnRemoteSubscribeMarketData(session* s, const json& request, std::string& response)
{
	std::string Instrument = request["Instruments"][0];

	// 检查重复订阅
	auto iterClient = s->m_subInstrument.find(Instrument);
	if (iterClient != s->m_subInstrument.end()) {
        // 重复订阅
        // 发送响应
        json rsp;
        rsp["MsgType"] = "OnRspSubscribeMarketData";
        rsp["RspInfo"]["ErrorID"] = 1;
        rsp["RspInfo"]["ErrorMsg"] = toUTF8("重复订阅");
        rsp["SpecificInstrument"]["InstrumentID"] = Instrument;

        response = rsp.dump();

        return;
	}
	else {
		s->m_subInstrument[Instrument] = true;
	}

	// 发送响应
	json rsp;
	rsp["MsgType"] = "OnRspSubscribeMarketData";
	rsp["RspInfo"]["ErrorID"] = 0;
	rsp["RspInfo"]["ErrorMsg"] = "succeed";
	rsp["SpecificInstrument"]["InstrumentID"] = Instrument;

	response = rsp.dump();
}

void CTickTradingFramework::OnRemoteUnSubscribeMarketData(session* s, const json& request, std::string& response)
{
	std::string Instrument = request["Instruments"][0];
    if (s->m_subInstrument.find(Instrument) != s->m_subInstrument.end()) {
        s->m_subInstrument.erase(Instrument);
    }

	// 发送响应
	json rsp;
	rsp["MsgType"] = "OnRspUnSubscribeMarketData";
	rsp["RspInfo"]["ErrorID"] = 0;
	rsp["RspInfo"]["ErrorMsg"] = "succeed";
	rsp["SpecificInstrument"]["InstrumentID"] = Instrument;

	response = rsp.dump();
}

void CTickTradingFramework::OnRemoteQueryStratety(const json& request, std::string& response)
{
	const json strategy_query_req = request["strategy_query_req"];
	std::string StrategyID = strategy_query_req["strategy"];

	// 查询数据
	std::string sql = "SELECT * FROM strategy";
	if (StrategyID.length() > 0) {
		sql += " where strategy='" + StrategyID + "'";
	}

	json rsp;
	rsp["MsgType"] = "strategy_query_rsp";
	rsp["rsp_code"] = "0";
	rsp["rsp_message"] = "succeed";
	soci::rowset<soci::row> records = (m_db->prepare << sql);
	for (auto iter = records.begin(); iter != records.end(); iter++) {
		json strategy;
		strategy["strategy"] = (*iter).get<std::string>("strategy");
		strategy["instrument"] = (*iter).get<std::string>("instrument");
		strategy["status"] = (*iter).get<std::string>("status");
		strategy["message"] = (*iter).get<std::string>("message");
		strategy["parameters"] = json::parse((*iter).get<std::string>("parameters"));

		rsp["strategy_query_rsp"]["strategy"].push_back(strategy);
	}

	response = rsp.dump();
}

void CTickTradingFramework::OnRemoteAddStratety(const json& request, std::string& response)
{
	const json strategy_add_req = request["strategy_add_req"];
	std::string StrategyID = strategy_add_req["strategy"];
	std::string InstrumentID = strategy_add_req["instrument"];
	std::string Parameters = strategy_add_req["parameters"].dump();

	// CheckExists
	if (StrategyExists(StrategyID, InstrumentID)) {
		// 策略已存在
		json rsp;
		rsp["MsgType"] = "strategy_add_rsp";
		rsp["rsp_code"] = "1";
		rsp["rsp_message"] = toUTF8("策略已存在");
		rsp["strategy_add_rsp"]["instrument"] = InstrumentID;
		rsp["strategy_add_rsp"]["strategy"] = StrategyID;
		response = rsp.dump();
		return;
	}

	// 添加策略
	*m_db << "INSERT INTO strategy (strategy, instrument, parameters,status,message) VALUES (:strategy, :instrument, :parameters, 'stopped', '')", soci::use(StrategyID), soci::use(InstrumentID), soci::use(Parameters);

	// 发送响应
	json rsp;
	rsp["MsgType"] = "strategy_add_rsp";
	rsp["rsp_code"] = "0";
	rsp["rsp_message"] = "succeed";
	rsp["strategy_add_rsp"]["instrument"] = InstrumentID;
	rsp["strategy_add_rsp"]["strategy"] = StrategyID;

	response = rsp.dump();
}

void CTickTradingFramework::OnRemoteDeleteStratety(const json& request, std::string& response)
{
	const json strategy_delete_req = request["strategy_delete_req"];
	std::string StrategyID = strategy_delete_req["strategy"];
	std::string InstrumentID = strategy_delete_req["instrument"];

	// CheckExists
	if (!StrategyExists(StrategyID, InstrumentID)) {
		// 策略不存在
		json rsp;
		rsp["MsgType"] = "strategy_delete_rsp";
		rsp["rsp_code"] = "1";
		rsp["rsp_message"] = toUTF8("策略不存在");
		rsp["strategy_delete_rsp"]["instrument"] = InstrumentID;
		rsp["strategy_delete_rsp"]["strategy"] = StrategyID;
		response = rsp.dump();
		return;
	}

	// 查询数据
	std::string Status;
	*m_db << "SELECT status FROM strategy where strategy=:strategy and instrument=:instrument", soci::use(StrategyID), soci::use(InstrumentID), soci::into(Status);
	if (Status == "running") {
		// 策略已启动，如需删除策略，请先停止策略
		json rsp;
		rsp["MsgType"] = "strategy_delete_rsp";
		rsp["rsp_code"] = "1";
		rsp["rsp_message"] = toUTF8("策略已启动，如需删除策略，请先停止策略");
		rsp["strategy_delete_rsp"]["instrument"] = InstrumentID;
		rsp["strategy_delete_rsp"]["strategy"] = StrategyID;
		response = rsp.dump();
		return;
	}

	*m_db << "DELETE FROM strategy where strategy=:strategy and instrument=:instrument", soci::use(StrategyID), soci::use(InstrumentID);

	json rsp;
	rsp["MsgType"] = "strategy_delete_rsp";
	rsp["rsp_code"] = "0";
	rsp["rsp_message"] = "succeed";
	rsp["strategy_delete_rsp"]["instrument"] = InstrumentID;
	rsp["strategy_delete_rsp"]["strategy"] = StrategyID;

	response = rsp.dump();
}

void CTickTradingFramework::OnRemoteModifyStratety(const json& request, std::string& response)
{
	const json strategy_modify_req = request["strategy_modify_req"];
	std::string StrategyID = strategy_modify_req["strategy"];
	std::string InstrumentID = strategy_modify_req["instrument"];
	std::string Parameters = strategy_modify_req["parameters"].dump();

	// CheckExists
	if (!StrategyExists(StrategyID, InstrumentID)) {
		// 策略不存在
		json rsp;
		rsp["MsgType"] = "strategy_modify_rsp";
		rsp["rsp_code"] = "1";
		rsp["rsp_message"] = toUTF8("策略不存在");
		rsp["strategy_modify_rsp"]["instrument"] = InstrumentID;
		rsp["strategy_modify_rsp"]["strategy"] = StrategyID;
		response = rsp.dump();
		return;
	}

	// 查询数据
	std::string Status;
	*m_db << "SELECT status FROM strategy where strategy=:strategy and instrument=:instrument", soci::use(StrategyID), soci::use(InstrumentID), soci::into(Status);
	if (Status == "running") {
		// 策略已启动，如需修改参数，请先停止策略
		json rsp;
		rsp["MsgType"] = "strategy_modify_rsp";
		rsp["rsp_code"] = "1";
		rsp["rsp_message"] = toUTF8("策略已启动，如需修改参数，请先停止策略");
		rsp["strategy_modify_rsp"]["instrument"] = InstrumentID;
		rsp["strategy_modify_rsp"]["strategy"] = StrategyID;
		response = rsp.dump();
		return;
	}

	// 修改参数
	*m_db << "UPDATE strategy set parameters=:parameters where strategy=:strategy and instrument=:instrument", soci::use(Parameters), soci::use(StrategyID), soci::use(InstrumentID);

	json rsp;
	rsp["MsgType"] = "strategy_modify_rsp";
	rsp["rsp_code"] = "0";
	rsp["rsp_message"] = "succeed";
	rsp["strategy_modify_rsp"]["instrument"] = InstrumentID;
	rsp["strategy_modify_rsp"]["strategy"] = StrategyID;

	response = rsp.dump();
}

void CTickTradingFramework::OnRemoteStartStopStratety(const json& request, std::string& response)
{
	const json strategy_control_req = request["strategy_control_req"];
	std::string StrategyID = strategy_control_req["strategy"];
	std::string InstrumentID = strategy_control_req["instrument"];
	std::string Operation = strategy_control_req["operation"];
	std::string Status;

	// CheckExists
	if (!StrategyExists(StrategyID, InstrumentID)) {
		// 策略不存在
		json rsp;
		rsp["MsgType"] = "strategy_control_rsp";
		rsp["rsp_code"] = "1";
		rsp["rsp_message"] = toUTF8("策略不存在");
		rsp["strategy_control_rsp"]["instrument"] = InstrumentID;
		rsp["strategy_control_rsp"]["strategy"] = StrategyID;
		response = rsp.dump();
		return;
	}

	// 查询数据
	std::string Parameters;
	*m_db << "SELECT parameters,status FROM strategy where strategy=:strategy and instrument=:instrument", soci::use(StrategyID), soci::use(InstrumentID), soci::into(Parameters), soci::into(Status);

	if ((Operation == "run" && Status == "running") || (Operation == "stop" && Status == "stopped")) {
		// 策略已启动或停止
		json rsp;
		rsp["MsgType"] = "strategy_control_rsp";
		rsp["rsp_code"] = "1";
		rsp["rsp_message"] = toUTF8("策略已启动或停止");
		rsp["strategy_control_rsp"]["instrument"] = InstrumentID;
		rsp["strategy_control_rsp"]["strategy"] = StrategyID;
		response = rsp.dump();
		return;
	}
	if (Operation == "run")
		Status = "running";
	else
		Status = "stopped";

	*m_db << "UPDATE strategy set status=:status where strategy=:strategy and instrument=:instrument", soci::use(Status), soci::use(StrategyID), soci::use(InstrumentID);

	if (Operation == "run") {
		if (StrategyID == "Entry") {
			// 打板策略
			AddStrategy(StrategyID, InstrumentID, new CEntryStrategy(this, InstrumentID, Parameters));
		}
		else if (StrategyID == "Exit") {
			// 卖出策略
			AddStrategy(StrategyID, InstrumentID, new CExitStrategy(this, InstrumentID, Parameters));
		}
	}
	else {
		// 停止策略
		DelStrategy(StrategyID, InstrumentID);
	}

	json rsp;
	rsp["MsgType"] = "strategy_control_rsp";
	rsp["rsp_code"] = "0";
	rsp["rsp_message"] = "succeed";
	rsp["strategy_control_rsp"]["instrument"] = InstrumentID;
	rsp["strategy_control_rsp"]["strategy"] = StrategyID;
	rsp["strategy_control_rsp"]["status"] = Status;

	response = rsp.dump();
}

void CTickTradingFramework::OnRemoteQryInstrument(const json& request, std::string& response)
{
    json rsp;
    rsp["MsgType"] = "OnRspQryInstrument";
    rsp["rsp_code"] = "0";
    rsp["rsp_message"] = "succeed";

    for (size_t i=0; i< TTF_INSTRUMENT_ARRAY_SIZE; i++) {
        if (m_pInstruments[i].SecurityID[0] == '\0') continue;
        json instrument;
        instrument["InstrumentID"] = m_pInstruments[i].SecurityID;
        instrument["InstrumentName"] = toUTF8(m_pInstruments[i].SecurityName);
        if (m_pInstruments[i].ExchangeID == TORA_TSTP_EXD_SSE)
            instrument["ExchangeID"] = EXCHANGEID_SH;
        else if (m_pInstruments[i].ExchangeID == TORA_TSTP_EXD_SZSE)
            instrument["ExchangeID"] = EXCHANGEID_SZ;
        else
            instrument["ExchangeID"] = EXCHANGEID_UN;
        instrument["VolumeMultiple"] = m_pInstruments[i].VolumeMultiple;
        rsp["Instruments"].push_back(instrument);
    }
    response = rsp.dump();
}

void CTickTradingFramework::OnRemoteQryInvestorPosition(const json& request, std::string& response) {
    json rsp;
    rsp["MsgType"] = "OnRspQryInvestorPosition";
    rsp["rsp_code"] = "0";
    rsp["rsp_message"] = "succeed";
    for (auto& it : m_mLongPositions) {
        json position;
        position["InstrumentID"] = it.second.SecurityID;
        position["Position"] = it.second.CurrentPosition;
        position["PositionCost"] = it.second.TotalPosCost;
        position["PositionProfit"] = 0;
        position["Commission"] = 0;
        rsp["InvestorPosition"].push_back(position);
    }
    response = rsp.dump();
}

void CTickTradingFramework::OnRemoteQryTradingAccount(const json& request, std::string& response) {
    json rsp;
    rsp["MsgType"] = "OnRspQryTradingAccount";
    rsp["rsp_code"] = "0";
    rsp["rsp_message"] = "succeed";
    rsp["TradingAccount"]["Available"] = m_TradingAccount.UsefulMoney;
    rsp["TradingAccount"]["WithdrawQuota"] = m_TradingAccount.FetchLimit;
    rsp["TradingAccount"]["Commission"] = m_TradingAccount.Commission;
    rsp["TradingAccount"]["CloseProfit"] = 0;
    rsp["TradingAccount"]["PositionProfit"] = 0;
    response = rsp.dump();
}