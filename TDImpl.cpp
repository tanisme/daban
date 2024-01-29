#include "TdImpl.h"
#include "TickTradingFramework.h"
#include <iostream>

void CTdImpl::Run()
{
	m_pTdApi->RegisterFront((char*)m_Server.c_str());

	// 使用QUICK模式，先查订单、成交、持仓、资金等数据，后面根据推送实时计算持仓等
	m_pTdApi->SubscribePrivateTopic(TORASTOCKAPI::TORA_TERT_QUICK);
	m_pTdApi->SubscribePublicTopic(TORASTOCKAPI::TORA_TERT_QUICK);
	m_pTdApi->Init();
}

void CTdImpl::OnFrontConnected()
{
	TTF_Message_t* Message = m_pFramework->MessageAllocate();
	Message->MsgType = MsgTypeTdFrontConnected;
	m_pFramework->m_queue.enqueue(Message);
	//m_pFramework->m_io_service.post(boost::bind(&CTickTradingFramework::OnTdFrontConnected, m_pFramework));
}

void CTdImpl::OnFrontDisconnected(int nReason)
{
	TTF_Message_t* Message = m_pFramework->MessageAllocate();
	Message->MsgType = MsgTypeTdFrontDisconnected;
	MsgTdFrontDisconnected_t* pMsgTdFrontDisconnected = (MsgTdFrontDisconnected_t*)&Message->Message;
	pMsgTdFrontDisconnected->nReason = nReason;
	m_pFramework->m_queue.enqueue(Message);
	//m_pFramework->m_io_service.post(boost::bind(&CTickTradingFramework::OnTdFrontDisconnected, m_pFramework, nReason));
}

///用户登录应答
void CTdImpl::OnRspUserLogin(TORASTOCKAPI::CTORATstpRspUserLoginField* pRspUserLoginField, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID)
{
	TTF_Message_t* Message = m_pFramework->MessageAllocate();
	Message->MsgType = MsgTypeRspTdUserLogin;
	MsgRspTdUserLogin_t* pMsgRspTdUserLogin = (MsgRspTdUserLogin_t*)&Message->Message;
	memcpy(&pMsgRspTdUserLogin->RspUserLogin, pRspUserLoginField, sizeof(pMsgRspTdUserLogin->RspUserLogin));
	memcpy(&pMsgRspTdUserLogin->RspInfo, pRspInfo, sizeof(pMsgRspTdUserLogin->RspInfo));
	pMsgRspTdUserLogin->nRequestID = nRequestID;
	m_pFramework->m_queue.enqueue(Message);
	//m_pFramework->m_io_service.post(boost::bind(&CTickTradingFramework::OnRspTdUserLogin, m_pFramework, *pRspUserLoginField, *pRspInfo, nRequestID));
}

//查询股东账户
void CTdImpl::OnRspQryShareholderAccount(TORASTOCKAPI::CTORATstpShareholderAccountField* pShareholderAccount, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	TTF_Message_t* Message = m_pFramework->MessageAllocate();
	Message->MsgType = MsgTypeRspQryShareholderAccount;
	MsgRspQryShareholderAccount_t* pMsgRspQryShareholderAccount = (MsgRspQryShareholderAccount_t*)&Message->Message;
	if (pShareholderAccount)
		memcpy(&pMsgRspQryShareholderAccount->ShareholderAccount, pShareholderAccount, sizeof(pMsgRspQryShareholderAccount->ShareholderAccount));
	else
		memset(&pMsgRspQryShareholderAccount->ShareholderAccount, 0x00, sizeof(pMsgRspQryShareholderAccount->ShareholderAccount));

	if (pRspInfo)
		memcpy(&pMsgRspQryShareholderAccount->RspInfo, pRspInfo, sizeof(pMsgRspQryShareholderAccount->RspInfo));
	else
		memset(&pMsgRspQryShareholderAccount->RspInfo, 0x00, sizeof(pMsgRspQryShareholderAccount->RspInfo));

	pMsgRspQryShareholderAccount->nRequestID = nRequestID;
	pMsgRspQryShareholderAccount->bIsLast = bIsLast;
	m_pFramework->m_queue.enqueue(Message);

	//TORASTOCKAPI::CTORATstpShareholderAccountField ShareholderAccount;
	//TORASTOCKAPI::CTORATstpRspInfoField RspInfo;

	//memset(&ShareholderAccount, 0x00, sizeof(ShareholderAccount));
	//memset(&RspInfo, 0x00, sizeof(RspInfo));
	//if (pShareholderAccount)
	//	memcpy(&ShareholderAccount, pShareholderAccount, sizeof(ShareholderAccount));
	//if (pRspInfo)
	//	memcpy(&RspInfo, pRspInfo, sizeof(RspInfo));
	//m_pFramework->m_io_service.post(boost::bind(&CTickTradingFramework::OnRspQryShareholderAccount, m_pFramework, ShareholderAccount, RspInfo, nRequestID, bIsLast));
}

///合约查询应答
void CTdImpl::OnRspQrySecurity(TORASTOCKAPI::CTORATstpSecurityField* pSecurity, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	TTF_Message_t* Message = m_pFramework->MessageAllocate();
	Message->MsgType = MsgTypeRspQrySecurity;
	MsgRspQrySecurity_t* pMsgRspQrySecurity = (MsgRspQrySecurity_t*)&Message->Message;
	if (pSecurity)
		memcpy(&pMsgRspQrySecurity->Security, pSecurity, sizeof(pMsgRspQrySecurity->Security));
	else
		memset(&pMsgRspQrySecurity->Security, 0x00, sizeof(pMsgRspQrySecurity->Security));

	if (pRspInfo)
		memcpy(&pMsgRspQrySecurity->RspInfo, pRspInfo, sizeof(pMsgRspQrySecurity->RspInfo));
	else
		memset(&pMsgRspQrySecurity->RspInfo, 0x00, sizeof(pMsgRspQrySecurity->RspInfo));

	pMsgRspQrySecurity->nRequestID = nRequestID;
	pMsgRspQrySecurity->bIsLast = bIsLast;
	m_pFramework->m_queue.enqueue(Message);

	//TORASTOCKAPI::CTORATstpSecurityField Security;
	//TORASTOCKAPI::CTORATstpRspInfoField RspInfo;

	//memset(&Security, 0x00, sizeof(Security));
	//memset(&RspInfo, 0x00, sizeof(RspInfo));
	//if (pSecurity)
	//	memcpy(&Security, pSecurity, sizeof(Security));
	//if (pRspInfo)
	//	memcpy(&RspInfo, pRspInfo, sizeof(RspInfo));
	//m_pFramework->m_io_service.post(boost::bind(&CTickTradingFramework::OnRspQrySecurity, m_pFramework, Security, RspInfo, nRequestID,bIsLast));
}

//订单查询
void CTdImpl::OnRspQryOrder(TORASTOCKAPI::CTORATstpOrderField* pOrder, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	TTF_Message_t* Message = m_pFramework->MessageAllocate();
	Message->MsgType = MsgTypeRspQryOrder;
	MsgRspQryOrder_t* pMessage = (MsgRspQryOrder_t*)&Message->Message;
	if (pOrder)
		memcpy(&pMessage->Order, pOrder, sizeof(pMessage->Order));
	else
		memset(&pMessage->Order, 0x00, sizeof(pMessage->Order));

	if (pRspInfo)
		memcpy(&pMessage->RspInfo, pRspInfo, sizeof(pMessage->RspInfo));
	else
		memset(&pMessage->RspInfo, 0x00, sizeof(pMessage->RspInfo));

	pMessage->nRequestID = nRequestID;
	pMessage->bIsLast = bIsLast;
	m_pFramework->m_queue.enqueue(Message);

	//TORASTOCKAPI::CTORATstpOrderField Order;
	//TORASTOCKAPI::CTORATstpRspInfoField RspInfo;

	//memset(&Order, 0x00, sizeof(Order));
	//memset(&RspInfo, 0x00, sizeof(RspInfo));
	//if (pOrder)
	//	memcpy(&Order, pOrder, sizeof(Order));
	//if (pRspInfo)
	//	memcpy(&RspInfo, pRspInfo, sizeof(RspInfo));

	//m_pFramework->m_io_service.post(boost::bind(&CTickTradingFramework::OnRspQryOrder, m_pFramework, Order, RspInfo, nRequestID, bIsLast));
}

//成交查询
void CTdImpl::OnRspQryTrade(TORASTOCKAPI::CTORATstpTradeField* pTrade, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	TTF_Message_t* Message = m_pFramework->MessageAllocate();
	Message->MsgType = MsgTypeRspQryTrade;
	MsgRspQryTrade_t* pMessage = (MsgRspQryTrade_t*)&Message->Message;
	if (pTrade)
		memcpy(&pMessage->Trade, pTrade, sizeof(pMessage->Trade));
	else
		memset(&pMessage->Trade, 0x00, sizeof(pMessage->Trade));

	if (pRspInfo)
		memcpy(&pMessage->RspInfo, pRspInfo, sizeof(pMessage->RspInfo));
	else
		memset(&pMessage->RspInfo, 0x00, sizeof(pMessage->RspInfo));

	pMessage->nRequestID = nRequestID;
	pMessage->bIsLast = bIsLast;
	m_pFramework->m_queue.enqueue(Message);

	//TORASTOCKAPI::CTORATstpTradeField Trade;
	//TORASTOCKAPI::CTORATstpRspInfoField RspInfo;

	//memset(&Trade, 0x00, sizeof(Trade));
	//memset(&RspInfo, 0x00, sizeof(RspInfo));
	//if (pTrade)
	//	memcpy(&Trade, pTrade, sizeof(Trade));
	//if (pRspInfo)
	//	memcpy(&RspInfo, pRspInfo, sizeof(RspInfo));

	//m_pFramework->m_io_service.post(boost::bind(&CTickTradingFramework::OnRspQryTrade, m_pFramework, Trade, RspInfo, nRequestID, bIsLast));
}

//持仓查询
void CTdImpl::OnRspQryPosition(TORASTOCKAPI::CTORATstpPositionField* pPosition, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	TTF_Message_t* Message = m_pFramework->MessageAllocate();
	Message->MsgType = MsgTypeRspQryPosition;
	MsgRspQryPosition_t* pMessage = (MsgRspQryPosition_t*)&Message->Message;
	if (pPosition)
		memcpy(&pMessage->Position, pPosition, sizeof(pMessage->Position));
	else
		memset(&pMessage->Position, 0x00, sizeof(pMessage->Position));

	if (pRspInfo)
		memcpy(&pMessage->RspInfo, pRspInfo, sizeof(pMessage->RspInfo));
	else
		memset(&pMessage->RspInfo, 0x00, sizeof(pMessage->RspInfo));

	pMessage->nRequestID = nRequestID;
	pMessage->bIsLast = bIsLast;
	m_pFramework->m_queue.enqueue(Message);

	//TORASTOCKAPI::CTORATstpPositionField Position;
	//TORASTOCKAPI::CTORATstpRspInfoField RspInfo;

	//memset(&Position, 0x00, sizeof(Position));
	//memset(&RspInfo, 0x00, sizeof(RspInfo));
	//if (pPosition)
	//	memcpy(&Position, pPosition, sizeof(Position));
	//if (pRspInfo)
	//	memcpy(&RspInfo, pRspInfo, sizeof(RspInfo));

	//m_pFramework->m_io_service.post(boost::bind(&CTickTradingFramework::OnRspQryPosition, m_pFramework, Position, RspInfo, nRequestID, bIsLast));
}

//资金查询
void CTdImpl::OnRspQryTradingAccount(TORASTOCKAPI::CTORATstpTradingAccountField* pTradingAccount, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	TTF_Message_t* Message = m_pFramework->MessageAllocate();
	Message->MsgType = MsgTypeRspQryTradingAccount;
	MsgRspQryTradingAccount_t* pMessage = (MsgRspQryTradingAccount_t*)&Message->Message;
	if (pTradingAccount)
		memcpy(&pMessage->TradingAccount, pTradingAccount, sizeof(pMessage->TradingAccount));
	else
		memset(&pMessage->TradingAccount, 0x00, sizeof(pMessage->TradingAccount));

	if (pRspInfo)
		memcpy(&pMessage->RspInfo, pRspInfo, sizeof(pMessage->RspInfo));
	else
		memset(&pMessage->RspInfo, 0x00, sizeof(pMessage->RspInfo));

	pMessage->nRequestID = nRequestID;
	pMessage->bIsLast = bIsLast;
	m_pFramework->m_queue.enqueue(Message);

	//TORASTOCKAPI::CTORATstpTradingAccountField TradingAccount;
	//TORASTOCKAPI::CTORATstpRspInfoField RspInfo;

	//memset(&TradingAccount, 0x00, sizeof(TradingAccount));
	//memset(&RspInfo, 0x00, sizeof(RspInfo));
	//if (pTradingAccount)
	//	memcpy(&TradingAccount, pTradingAccount, sizeof(TradingAccount));
	//if (pRspInfo)
	//	memcpy(&RspInfo, pRspInfo, sizeof(RspInfo));

	//m_pFramework->m_io_service.post(boost::bind(&CTickTradingFramework::OnRspQryTradingAccount, m_pFramework, TradingAccount, RspInfo, nRequestID, bIsLast));
}

//订单回报
void CTdImpl::OnRtnOrder(TORASTOCKAPI::CTORATstpOrderField* pOrder)
{
#ifndef _WIN32
	if (pOrder->RequestID && pOrder->OrderSysID[0] != 0) {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		if (tv.tv_usec > pOrder->RequestID)
			std::cout << "time used of RtnOrder: " << tv.tv_usec - pOrder->RequestID << std::endl;
		else
			std::cout << "time used of RtnOrder: " << 1000000 + tv.tv_usec - pOrder->RequestID << std::endl;
	}
#endif
	TTF_Message_t* Message = m_pFramework->MessageAllocate();
	Message->MsgType = MsgTypeRtnOrder;
	MsgRtnOrder_t* pMessage = (MsgRtnOrder_t*)&Message->Message;
	memcpy(&pMessage->Order, pOrder, sizeof(pMessage->Order));
	m_pFramework->m_queue.enqueue(Message);
	//m_pFramework->m_io_service.post(boost::bind(&CTickTradingFramework::OnRtnOrder, m_pFramework, *pOrder));
}

//成交回报
void CTdImpl::OnRtnTrade(TORASTOCKAPI::CTORATstpTradeField* pTrade)
{
	TTF_Message_t* Message = m_pFramework->MessageAllocate();
	Message->MsgType = MsgTypeRtnTrade;
	MsgRtnTrade_t* pMessage = (MsgRtnTrade_t*)&Message->Message;
	memcpy(&pMessage->Trade, pTrade, sizeof(pMessage->Trade));
	m_pFramework->m_queue.enqueue(Message);

	//m_pFramework->m_io_service.post(boost::bind(&CTickTradingFramework::OnRtnTrade, m_pFramework, *pTrade));
}

///报单录入响应
void CTdImpl::OnRspOrderInsert(TORASTOCKAPI::CTORATstpInputOrderField* pInputOrderField, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfoField, int nRequestID)
{
	//m_pFramework->m_io_service.post(boost::bind(&CTickTradingFramework::OnRspOrderInsert, m_pFramework, *pInputOrderField, *pRspInfoField, nRequestID));
}

///报单错误回报
void CTdImpl::OnErrRtnOrderInsert(TORASTOCKAPI::CTORATstpInputOrderField* pInputOrderField, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfoField, int nRequestID)
{
	//m_pFramework->m_io_service.post(boost::bind(&CTickTradingFramework::OnErrRtnOrderInsert, m_pFramework, *pInputOrderField, *pRspInfoField, nRequestID));
}

///撤单响应
void CTdImpl::OnRspOrderAction(TORASTOCKAPI::CTORATstpInputOrderActionField* pInputOrderActionField, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfoField, int nRequestID)
{
	//m_pFramework->m_io_service.post(boost::bind(&CTickTradingFramework::OnRspOrderAction, m_pFramework, *pInputOrderActionField, *pRspInfoField, nRequestID));
}

///撤单错误回报
void CTdImpl::OnErrRtnOrderAction(TORASTOCKAPI::CTORATstpInputOrderActionField* pInputOrderActionField, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfoField, int nRequestID)
{
	//m_pFramework->m_io_service.post(boost::bind(&CTickTradingFramework::OnErrRtnOrderAction, m_pFramework, *pInputOrderActionField, *pRspInfoField, nRequestID));
}
