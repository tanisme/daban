#pragma once

#include "TORA/TORATstpTraderApi.h"
#include "TORA/TORATstpXMdApi.h"
#include "TORA/TORATstpLev2MdApi.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/async.h"


class session; // Defined in websocket

enum CWorkingStatus { WORKING_STATUS_INITING, WORKING_STATUS_RUNNING };

enum TTF_MsgType {
	MsgTypeTdFrontConnected,
	MsgTypeTdFrontDisconnected,
	MsgTypeRspTdUserLogin,
	MsgTypeRspQryShareholderAccount,
	MsgTypeRspQrySecurity,
	MsgTypeRspQryOrder,
	MsgTypeRspQryTrade,
	MsgTypeRtnOrder,
	MsgTypeRtnTrade,
	MsgTypeRspOrderInsert,
	MsgTypeRspOrderAction,
	MsgTypeRspQryTradingAccount,
	MsgTypeRspQryPosition,
	MsgTypeRtnMarketStatus,
	MsgTypeMdFrontConnected,
	MsgTypeMdFrontDisconnected,
	MsgTypeRspMdUserLogin,
	MsgTypeRtnOrderDetail,
	MsgTypeRtnTransaction,
	MsgTypeRtnNGTSTick,
	MsgTypeClientConnected,
	MsgTypeClientDisconnected,
	MsgTypeClientRequest
};

typedef struct {
	int nReason;
} MsgTdFrontDisconnected_t;

typedef struct {
	TORASTOCKAPI::CTORATstpRspUserLoginField RspUserLogin;
	TORASTOCKAPI::CTORATstpRspInfoField RspInfo;
	int nRequestID;
} MsgRspTdUserLogin_t;

typedef struct {
	TORASTOCKAPI::CTORATstpShareholderAccountField ShareholderAccount;
	TORASTOCKAPI::CTORATstpRspInfoField RspInfo;
	int nRequestID;
	bool bIsLast;
} MsgRspQryShareholderAccount_t;

typedef struct {
	TORASTOCKAPI::CTORATstpSecurityField Security;
	TORASTOCKAPI::CTORATstpRspInfoField RspInfo;
	int nRequestID;
	bool bIsLast;
} MsgRspQrySecurity_t;

typedef struct {
	TORASTOCKAPI::CTORATstpOrderField Order;
	TORASTOCKAPI::CTORATstpRspInfoField RspInfo;
	int nRequestID;
	bool bIsLast;
} MsgRspQryOrder_t;

typedef struct {
	TORASTOCKAPI::CTORATstpTradeField Trade;
	TORASTOCKAPI::CTORATstpRspInfoField RspInfo;
	int nRequestID;
	bool bIsLast;
} MsgRspQryTrade_t;

typedef struct {
	TORASTOCKAPI::CTORATstpOrderField Order;
} MsgRtnOrder_t;

typedef struct {
	TORASTOCKAPI::CTORATstpTradeField Trade;
} MsgRtnTrade_t;

typedef struct {
	TORASTOCKAPI::CTORATstpInputOrderField InputOrder;
	TORASTOCKAPI::CTORATstpRspInfoField RspInfo;
	int nRequestID;
} MsgRspOrderInsert_t;

typedef struct {
	TORASTOCKAPI::CTORATstpTradingAccountField TradingAccount;
	TORASTOCKAPI::CTORATstpRspInfoField RspInfo;
	int nRequestID;
	bool bIsLast;
} MsgRspQryTradingAccount_t;

typedef struct {
	TORASTOCKAPI::CTORATstpPositionField Position;
	TORASTOCKAPI::CTORATstpRspInfoField RspInfo;
	int nRequestID;
	bool bIsLast;
} MsgRspQryPosition_t;

typedef struct {
	TORASTOCKAPI::CTORATstpInputOrderActionField InputOrderAction;
	TORASTOCKAPI::CTORATstpRspInfoField RspInfo;
	int nRequestID;
} MsgRspOrderAction_t;

typedef struct {
	TORASTOCKAPI::CTORATstpMarketStatusField MarketStatus;
} MsgRtnMarketStatus_t;

/*Md*/
typedef struct {
	int nReason;
} MsgMdFrontDisconnected_t;

typedef struct {
	TORALEV2API::CTORATstpRspUserLoginField RspUserLogin;
	TORALEV2API::CTORATstpRspInfoField RspInfo;
	int nRequestID;
} MsgRspMdUserLogin_t;

typedef struct {
	TORALEV2API::CTORATstpLev2OrderDetailField OrderDetail;
} MsgRtnOrderDetail_t;

typedef struct {
	TORALEV2API::CTORATstpLev2TransactionField Transaction;
} MsgRtnTransaction_t;

typedef struct {
	TORALEV2API::CTORATstpLev2NGTSTickField NGTSTick;
} MsgRtnNGTSTick_t;

typedef struct {
	session* s;
} MsgClientConnected_t;

typedef struct {
	session* s;
} MsgClientDisconnected_t;

typedef struct {
	session* s;
	char request[1000];
} MsgClientRequest_t;


typedef struct {
	TTF_MsgType MsgType;
	union {
		MsgTdFrontDisconnected_t MsgTdFrontDisconnected;
		MsgRspTdUserLogin_t MsgRspTdUserLogin;
		MsgRspQryShareholderAccount_t MsgRspQryShareholderAccount;
		MsgRspQrySecurity_t MsgRspQrySecurity;
		MsgRspQryOrder_t MsgRspQryOrder;
		MsgRspQryTrade_t MsgRspQryTrade;
		MsgRtnOrder_t MsgRtnOrder;
		MsgRtnTrade_t MsgRtnTrade;
		MsgRspOrderInsert_t MsgRspOrderInsert;
		MsgRspQryTradingAccount_t MsgRspQryTradingAccount;
		MsgRspQryPosition_t MsgRspQryPosition;
		MsgRspOrderAction_t MsgRspOrderAction;
		MsgRtnMarketStatus_t MsgRtnMarketStatus;
		MsgMdFrontDisconnected_t MsgMdFrontDisconnected;
		MsgRspMdUserLogin_t MsgRspMdUserLogin;
		MsgRtnOrderDetail_t MsgRtnOrderDetail;
		MsgRtnTransaction_t MsgRtnTransaction;
		MsgRtnNGTSTick_t MsgRtnNGTSTick;
		MsgClientConnected_t MsgClientConnected;
		MsgClientDisconnected_t MsgClientDisconnected;
		MsgClientRequest_t MsgClientRequest;
	} Message;
} TTF_Message_t;

extern std::shared_ptr<spdlog::logger> logger;