#include "stdafx.h"

#include <time.h>

#include "MatchMakingService.h"
#include "RoomService.h"
#include "Utils.h"
#include "TaskManager.h"

MatchMakingService::MatchMakingService(void)
{
}

MatchMakingService::~MatchMakingService(void)
{
}

bool MatchMakingService::Initialize( Config & config, ServiceFactory * services )
{
	services_ = services;
	
	RoomService * roomService = services_->GetRoomService();
	if( false == connector_.Initialize( "M:0:68002:0:0:0", &TaskManager::OnBesReceive, &TaskManager::OnBesError, &TaskManager::OnBesDeliverFail, &( roomService->tm_ ) ) )
	{
		LOG_ERROR( "Fail to initialize MRS connector." );
		return false;
	}

	roomServerInfo_.serverId			= config.GetInt					( "/server/id/text()", 0 );
	roomServerInfo_.instanceId			= config.GetServerInstanceId	();
	roomServerInfo_.ipAddr				= config.GetString				( "/server/ip/text()", "" );
	roomServerInfo_.tcpPort				= config.GetInt					( "/server/port/text()", 10004 );
	roomServerInfo_.maxNumberOfRoom		= config.GetInt					( "/server/max_room/text()", 3000 );
	roomServerInfo_.maxNumberOfUser		= config.GetInt					( "/server/max_user/text()", 10000 );

	Dispatcher & Dispatcher = services_->GetRoomService()->dispatcher_;
	Dispatcher.RegMsgHandler( __messageid( MonSummGetRoomServerInfoReq ),	GetRoomServerInfo,	this );
	Dispatcher.RegMsgHandler( __messageid( MonSummCreateRoomReq ),			CreateRoom,			this );

	return true;
}

void MatchMakingService::Uninitialize( void )
{
	connector_.Stop();
}

void MatchMakingService::LogStat( void )
{
}

bool MatchMakingService::Run( void )
{
	if( false == connector_.Start() )
	{
		LOG_ERROR( "Fail to Run MRS connector." );
		return false;
	}

	MonSummSubscribeRoomServerInfoSig signal;
	signal.roomServerInfo = roomServerInfo_;
	
	mmsAddr_ = MRS::Address::CreateHandle( "M:0:68001:0:0:0" );
	connector_.SendTo( mmsAddr_, signal );

	return true;
}

bool MatchMakingService::GetRoomServerInfo( ContextType context, XStream::Handle recvStream, void * userData )
{
	if( NULL == userData )
	{
		LOG_ERROR( "Invalid user data." );
		return false;
	}

	MatchMakingService * mms = reinterpret_cast < MatchMakingService * > ( userData );
	MRSSvcContext * besContext = MRSSvcContext::ConvertToPtr( context );

	MonSummGetRoomServerInfoAns answer;

	answer.roomServerInfo = mms->roomServerInfo_;

	mms->connector_.SendTo( besContext->GetPeerAddr(), answer );

	return true;
}

bool MatchMakingService::CreateRoom( ContextType context, XStream::Handle recvStream, void * userData )
{
	if( NULL == userData )
	{
		LOG_ERROR( "Invalid user data." );
		return false;
	}

	MatchMakingService * mms = reinterpret_cast < MatchMakingService * > ( userData );
	MRSSvcContext * besContext = MRSSvcContext::ConvertToPtr( context );
	MonSummCreateRoomAns answer;
	ScopedInvoker scopedAction( [ mms, besContext, &answer ]()
	{
		mms->connector_.SendTo( besContext->GetPeerAddr(), answer );
	});

	MonSummCreateRoomReq request;
	if( false == request.Load( recvStream ) )
	{
		LOG_ERROR( "Fail to load XDR message. (" << std::to_string( __messageid( MonSummCreateRoomReq ) ) << ")(" << XStream::GetLength( recvStream ) << ")" );
		answer.retCode = 1;
		return false;
	}

	RoomService * roomService = mms->services_->GetRoomService();
	if( false == roomService->CreateRoom( context, request.users, answer.roomNo, answer.matchNo, answer.tickets ) )
	{
		answer.retCode = 1;
		return false;
	}

	answer.serviceAddr = mms->roomServerInfo_.ipAddr;
	answer.servicePort = mms->roomServerInfo_.tcpPort;

	return true;
}