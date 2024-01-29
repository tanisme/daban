#include "TdImpl.h"
#include "TickTradingFramework.h"

CTdImpl::CTdImpl(CTickTradingFramework* pFramework, std::string Host, std::string User, std::string Password, std::string UserProductInfo, const std::string LIP, const std::string MAC, const std::string HD) :
    m_pFramework(pFramework),
    m_Server(Host),
    m_UserID(User),
    m_Password(Password)
{
    m_LIP = LIP;
    m_MAC = MAC;
    m_HD = HD;
    m_pTdApi = TORASTOCKAPI::CTORATstpTraderApi::CreateTstpTraderApi();
    m_pTdApi->RegisterSpi(this);
}

CTdImpl::~CTdImpl() {
    if (m_MessageArray) {
        free(m_MessageArray);
    }
}

void CTdImpl::Run()
{
    m_MessageArray = (TTF_Message_t*)calloc(TTF_MESSAGE_ARRAY_SIZE/10, sizeof(TTF_Message_t));

    m_pTdApi->RegisterFront((char*)m_Server.c_str());

	// 使用QUICK模式，先查订单、成交、持仓、资金等数据，后面根据推送实时计算持仓等
	m_pTdApi->SubscribePrivateTopic(TORASTOCKAPI::TORA_TERT_QUICK);
	m_pTdApi->SubscribePublicTopic(TORASTOCKAPI::TORA_TERT_QUICK);
	m_pTdApi->Init();
}

void CTdImpl::OnFrontConnected()
{
	TTF_Message_t* Message = MessageAllocate();
	Message->MsgType = MsgTypeTdFrontConnected;
	m_pFramework->m_queue.enqueue(Message);
}

void CTdImpl::OnFrontDisconnected(int nReason)
{
	TTF_Message_t* Message = MessageAllocate();
	Message->MsgType = MsgTypeTdFrontDisconnected;
	MsgTdFrontDisconnected_t* pMsgTdFrontDisconnected = (MsgTdFrontDisconnected_t*)&Message->Message;
	pMsgTdFrontDisconnected->nReason = nReason;
	m_pFramework->m_queue.enqueue(Message);
}

///用户登录应答
void CTdImpl::OnRspUserLogin(TORASTOCKAPI::CTORATstpRspUserLoginField* pRspUserLoginField, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID)
{
	TTF_Message_t* Message = MessageAllocate();
	Message->MsgType = MsgTypeRspTdUserLogin;
	MsgRspTdUserLogin_t* pMsgRspTdUserLogin = (MsgRspTdUserLogin_t*)&Message->Message;
	memcpy(&pMsgRspTdUserLogin->RspUserLogin, pRspUserLoginField, sizeof(pMsgRspTdUserLogin->RspUserLogin));
	memcpy(&pMsgRspTdUserLogin->RspInfo, pRspInfo, sizeof(pMsgRspTdUserLogin->RspInfo));
	pMsgRspTdUserLogin->nRequestID = nRequestID;
	m_pFramework->m_queue.enqueue(Message);
}

//查询股东账户
void CTdImpl::OnRspQryShareholderAccount(TORASTOCKAPI::CTORATstpShareholderAccountField* pShareholderAccount, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	TTF_Message_t* Message = MessageAllocate();
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
}

///合约查询应答
void CTdImpl::OnRspQrySecurity(TORASTOCKAPI::CTORATstpSecurityField* pSecurity, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	TTF_Message_t* Message = MessageAllocate();
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
}

//订单查询
void CTdImpl::OnRspQryOrder(TORASTOCKAPI::CTORATstpOrderField* pOrder, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	TTF_Message_t* Message = MessageAllocate();
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
}

//成交查询
void CTdImpl::OnRspQryTrade(TORASTOCKAPI::CTORATstpTradeField* pTrade, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	TTF_Message_t* Message = MessageAllocate();
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
}

//持仓查询
void CTdImpl::OnRspQryPosition(TORASTOCKAPI::CTORATstpPositionField* pPosition, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	TTF_Message_t* Message = MessageAllocate();
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
}

//资金查询
void CTdImpl::OnRspQryTradingAccount(TORASTOCKAPI::CTORATstpTradingAccountField* pTradingAccount, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	TTF_Message_t* Message = MessageAllocate();
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
	TTF_Message_t* Message = MessageAllocate();
	Message->MsgType = MsgTypeRtnOrder;
	MsgRtnOrder_t* pMessage = (MsgRtnOrder_t*)&Message->Message;
	memcpy(&pMessage->Order, pOrder, sizeof(pMessage->Order));
	m_pFramework->m_queue.enqueue(Message);
}

//成交回报
void CTdImpl::OnRtnTrade(TORASTOCKAPI::CTORATstpTradeField* pTrade)
{
	TTF_Message_t* Message = MessageAllocate();
	Message->MsgType = MsgTypeRtnTrade;
	MsgRtnTrade_t* pMessage = (MsgRtnTrade_t*)&Message->Message;
	memcpy(&pMessage->Trade, pTrade, sizeof(pMessage->Trade));
	m_pFramework->m_queue.enqueue(Message);
}

///报单录入响应
void CTdImpl::OnRspOrderInsert(TORASTOCKAPI::CTORATstpInputOrderField* pInputOrderField, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfoField, int nRequestID)
{
}

///报单错误回报
void CTdImpl::OnErrRtnOrderInsert(TORASTOCKAPI::CTORATstpInputOrderField* pInputOrderField, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfoField, int nRequestID)
{
}

///撤单响应
void CTdImpl::OnRspOrderAction(TORASTOCKAPI::CTORATstpInputOrderActionField* pInputOrderActionField, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfoField, int nRequestID)
{
}

///撤单错误回报
void CTdImpl::OnErrRtnOrderAction(TORASTOCKAPI::CTORATstpInputOrderActionField* pInputOrderActionField, TORASTOCKAPI::CTORATstpRspInfoField* pRspInfoField, int nRequestID)
{
}

TTF_Message_t* CTdImpl::MessageAllocate()
{
    TTF_Message_t* Message = (TTF_Message_t*)(m_MessageArray + m_MessageCursor);
    if (m_MessageCursor == TTF_MESSAGE_ARRAY_SIZE - 1)
        m_MessageCursor = 0;
    else
        m_MessageCursor++;

    return Message;
}
