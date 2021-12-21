#include "stdafx.h"

#include "GeneratedFiles/MonSummMsgForRoomServer.h"
#include "GeneratedFiles/RoomServerInteralMessage.h"
#include "RoomService.h"
#include "Model.h"
#include "Authenticator.h"
#include "DBMS.h"

RoomService::RoomService( void )
	: tm_( dispatcher_ ), services_( nullptr ), serverId_( 0 )
{
}


RoomService::~RoomService( void )
{
	Uninitialize();
}

bool RoomService::Initialize( Config & config, ServiceFactory * services )
{
	services_ = services;

	if( false == tm_.Initialize( services_, 10 ) )
	{
		LOG_ERROR( "Fail to initialize task manager." );
		return false;
	}

	if( false == acceptor_.Initialize( "127.0.0.1", 10004, &tm_, &TaskManager::OnUserAccepted, &TaskManager::OnUserReceived, &TaskManager::OnUserClosed, &TaskManager::OnUserLinkDestroy ) )
	{
		LOG_ERROR( "Fail to initialize acceptor." );
		return false;
	}

	dispatcher_.RegMsgHandler( __messageid( MonSummJoinRoomReq ), &EnterRoom, this );
	dispatcher_.RegMsgHandler( __messageid( MonSummLeaveRoomReq ), &LeaveRoom, this );
	dispatcher_.RegMsgHandler( __messageid( InternalMessage_UserOnClose ), &OnUserClose, this );

	serverId_ = 1;

	return true;
}

void RoomService::Uninitialize( void )
{
	acceptor_.Stop();
	tm_.UnInitialize();
}

void RoomService::LogStat( void )
{
}

bool RoomService::Run( void )
{
	if( false == tm_.Run() )
	{
		LOG_ERROR( "Fail to run task manager." );
		return false;
	}

	if( false == acceptor_.Start() )
	{
		LOG_ERROR( "Fail to start acceptor." );
		return false;
	}

	return true;
}

bool RoomService::Login( UserContext * userContext )
{
	//Model::item_equipped::Row row;
	//auto syncSelectEquippedItem = dbms->Execute( userContext, Model::item_equipped::Select(1), row );
	//int retValue = syncSelectEquippedItem.GetState();

	return true;
}

bool RoomService::CreateRoom( ContextType context, std::vector < int64 > & userIds, int64 & roomNo, int64 & matchNo, std::map< int64, std::string > & tickets )
{
	auto auth = services_->GetAuthenticator();
	auto dbms = services_->GetDBMS();

	RoomType room = new Room( serverId_ );

	for( auto it = userIds.begin(); userIds.end() != it; it++ )
	{
		auto pos = userContexts_.find( *it );
		if( userContexts_.end() != pos )
		{
			LOG_ERROR( "Duplicated user no in user contexts. (" << std::to_string( *it ) << ")" );
			return false;
		}
	}

	roomNo = room->no_;
	matchNo = room->matchNo_;


	std::vector < TaskManager::Synchro > syncs;
	for( auto it = userIds.begin(); userIds.end() != it; it++ )
	{
		if( tickets.end() != tickets.find( *it ) )
		{
			LOG_ERROR( "Duplicated user no in request. (" << std::to_string( *it ) << ")" );
			return false;
		}

		std::string ticket = auth->MakeTicket( *it, room->no_, room->matchNo_, room->createdTime_ );
		tickets.insert( std::make_pair ( *it, ticket ) );

		Model::match_log::Row matchLog;
		matchLog.match_no	= room->matchNo_;
		matchLog.type		= 1;
		matchLog.result		= 0;
		auto syncSelectEquippedItem = dbms->Execute( context, *it, Model::match_log::Insert( matchLog ), matchNo );
		syncs.push_back( std::move( syncSelectEquippedItem ) );
	}

	for( auto it = syncs.begin(); syncs.end() != it; it++ )
	{
		if( TaskManager::Synchro::COMPLETE_SUCCESS != it->GetState() )
		{
			LOG_ERROR( "Fail to write match log record. " );
			return false;
		}
	}

	{
		XSystem::Threading::ScopedLock < XSystem::Threading::CriticalSection >  guard( csRooms_ );
		rooms_.insert( std::make_pair ( roomNo, room ) );
	}

	//auto syncSelectEquippedItem = dbms->Execute( userContext, Model::item_equipped::Select(1), row );
	//int retValue = syncSelectEquippedItem.GetState();
	return true;
}

bool RoomService::EnterRoom( ContextType context, XStream::Handle recvStream, void * userData )
{
	UserContext		* userContext	= UserContext::ConvertToPtr( context );
	RoomService		* roomService	= reinterpret_cast < RoomService * > ( userData );
	DBMS			* dbms			= roomService->services_->GetDBMS();
	Authenticator	* Authenticator	= roomService->services_->GetAuthenticator();

	MonSummJoinRoomReq request;
	MonSummJoinRoomAns answer;
	ScopedInvoker scopedAction( [ userContext, &answer ]()
	{
		userContext->Send( answer );
	});

	if( false == request.Load( recvStream ) )
	{
		LOG_ERROR( "Fail to load XDR message. (" << __messageid( MonSummJoinRoomReq ) << ")" );
		answer.returnCode = -1;
		return false;
	}

	// 입장 대상 룸 검색
	RoomType room = nullptr;
	{
		XSystem::Threading::ScopedLock < XSystem::Threading::CriticalSection > guard( roomService->csRooms_ );
		auto pos = roomService->rooms_.find( request.roomNo );
		if( roomService->rooms_.end() == pos )
		{
			LOG_ERROR( "Not exist room. (" << request.userNo << ")(" << request.roomNo << ")(" << request.matchNo << ")(" << request.ticket << ")" );
			answer.returnCode = -1;
			return false;
		}

		room = pos->second;
	}

	// 경기 번호 확인
	if( room->matchNo_ != request.matchNo )
	{
		LOG_ERROR( "Invalid match no. (" << request.userNo << ")(" << request.roomNo << ")(" << room->matchNo_ << ":" << request.matchNo << ")(" << request.ticket << ")" );
		answer.returnCode = -1;
		return false;
	}

	// 티켓 확인
	if( false == Authenticator->CheckTicket( request.ticket, request.userNo, request.roomNo, request.matchNo, room->createdTime_ ) )
	{
		LOG_ERROR( "Invalid ticket. (" << request.userNo << ")(" << request.roomNo << ")(" << request.matchNo << ")(" << request.ticket << ")(" << room->createdTime_ << ")" );
		answer.returnCode = -1;
		return false;
	}

	// 네트워크 연결에 사용할 user context 세팅
	bool mustBeLogin = false;
	{
		XSystem::Threading::ScopedLock < XSystem::Threading::CriticalSection > guard( roomService->csUserContexts_ );
		auto pos = roomService->userContexts_.find( request.userNo );
		if( roomService->userContexts_.end() == pos )
		{	// 신규 연결
			userContext->user_ = new User;
			userContext->user_->no_ = request.userNo;
			roomService->userContexts_.insert( std::make_pair( request.userNo, userContext ) );
			mustBeLogin = true;
		}
		else 
		{	// 재 접속
			UserContext * queuedUserContext = UserContext::ConvertToPtr( pos->second );
			userContext->Reset( *( queuedUserContext ) );
			pos->second = userContext;
		}
	}

	if( true == mustBeLogin )
	{
		if( false == roomService->Login( userContext ) )
		{
			LOG_ERROR( "Fail to login. (" << request.userNo << ")(" << request.roomNo << ")(" << request.matchNo << ")(" << request.ticket << ")(" << room->createdTime_ << ")" );
			answer.returnCode = -1;
			return false;
		}
	}

	if( false == room->Enter( context ) )
	{
		LOG_ERROR( "Fail to enter room. (" << request.userNo << ")(" << request.roomNo << ")(" << request.matchNo << ")(" << request.ticket << ")(" << room->createdTime_ << ")" );
		answer.returnCode = -1;
		return false;
	}

	return true;
}

bool RoomService::LeaveRoom( ContextType context, XStream::Handle recvStream, void * userData )
{
	RoomService * roomService = reinterpret_cast < RoomService * > ( userData );
	UserContext * userContext = UserContext::ConvertToPtr( context );

	MonSummLeaveRoomAns answer;
	ScopedInvoker scopedAction( [ userContext, &answer ]()
	{
		userContext->Send( answer );
	});

	MonSummLeaveRoomReq request;
	if( false == request.Load( recvStream ) )
	{
		LOG_ERROR( "Fail to load XDR message. (" << __messageid( MonSummLeaveRoomReq ) << ")" );
		answer.returnCode = -1;
		return false;
	}

	// if( 게임 진행 중이 아니면 )
	userContext->Close();

	// 게임 진행 중이면?

	return true;
}

bool RoomService::OnUserClose( ContextType context, XStream::Handle recvStream, void * userData )
{
	// Leave Room을 한 상태에서 연결이 끊겼을 경우
	UserContext * userContext = UserContext::ConvertToPtr( context );
	RoomService * roomService = reinterpret_cast < RoomService * > ( userData );
	{
		XSystem::Threading::ScopedLock < XSystem::Threading::CriticalSection >  guard( roomService->csUserContexts_ );
		auto pos = roomService->userContexts_.find( userContext->user_->no_ );
		if( roomService->userContexts_.end() != pos )
		{
			roomService->userContexts_.erase( pos );
			// userContext->Close();
		}
	}

	// Leave Room을 하지 않는 상태에서 연결이 끊겼을 경우

	return true;
}