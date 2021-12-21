#include "stdafx.h"

#include "RoomServer.h"
#include "GeneratedFiles/MonSummMsgForRoomServer.h"
#include "GeneratedFiles/MonSummMsgForRoomServerAndMMS.h"

RoomServer::RoomServer(void)
	: roomServiceAddr_( NULL ), roomServiceUnitAddr_( NULL )
{
}

RoomServer::~RoomServer(void)
{
	if( NULL != roomServiceAddr_ )
	{
		MRS::Address::DestroyHandle( roomServiceAddr_ );
		roomServiceAddr_ = NULL;
	}
}

bool RoomServer::Initialize( void )
{
	if( false == mrsConnector_.Initialize( "M:0:68001:0:0:0" ) )
		return false;

	if( false == tcpConnector_.Initialize( "127.0.0.1", 10004 ) )
		return false;

	mrsConnector_.RegMsgHandler( __messageid( MonSummSubscribeRoomServerInfoSig )	,	& RoomServer::SubscribeSig,			this );
	mrsConnector_.RegMsgHandler( __messageid( MonSummGetRoomServerInfoAns )			,	& RoomServer::GetInfoAns,			this );
	mrsConnector_.RegMsgHandler( __messageid( MonSummCreateRoomAns )				,	& RoomServer::CreateRoomAns,		this );

	tcpConnector_.RegMsgHandler( __messageid( MonSummJoinRoomAns )					,	& RoomServer::JoinRoomAns,			this );
	tcpConnector_.RegMsgHandler( __messageid( MonSummLeaveRoomAns )					,	& RoomServer::LeaveRoomAns,			this );

	roomServiceAddr_ = MRS::Address::CreateHandle( "M:0:68002:0:0:0" );
	if( NULL == roomServiceAddr_ )
		return false;

	return true;
}

void RoomServer::Uninitialize( void )
{
	tcpConnector_.Uninitialize();
	mrsConnector_.Uninitialize();
}

#define DEFAULT_WAIT_TIME 10 * 1000

bool RoomServer::GetInfoReq( void )
{
	MonSummGetRoomServerInfoReq request;

	mrsConnector_.SendTo( roomServiceAddr_, request );

	if( false == sync_.Wait( DEFAULT_WAIT_TIME ) )
		return false;

	return true;
}

bool RoomServer::CreateRoomReq( void )
{
	MonSummCreateRoomReq request;

	request.users.push_back( 1 );
	request.users.push_back( 2 );

	mrsConnector_.SendTo( roomServiceUnitAddr_, request );

	if( false == sync_.Wait( DEFAULT_WAIT_TIME ) )
		return false;

	return true;
}

bool RoomServer::JoinRoomReq( void )
{
	MonSummJoinRoomReq request;

	if( false == sync_.Wait( DEFAULT_WAIT_TIME ) )
		return false;

	return true;
}

bool RoomServer::LeaveRoomReq( void )
{
	MonSummLeaveRoomReq request;

	if( false == sync_.Wait( DEFAULT_WAIT_TIME ) )
		return false;

	return true;
}

bool __stdcall RoomServer::SubscribeSig( XStream::Handle messageStream, void * additionalData, void * context )
{
	MonSummSubscribeRoomServerInfoSig signal;

	RoomServer * roomServer = static_cast < RoomServer * > ( additionalData );

	signal.Load( messageStream );

	return true;
}

bool __stdcall RoomServer::GetInfoAns( XStream::Handle messageStream, void * additionalData, void * context )
{
	RoomServer * roomServer = static_cast < RoomServer * > ( additionalData );
	MRS::Address::Handle peerAddr = static_cast < MRS::Address::Handle > ( context );
	
	MonSummGetRoomServerInfoAns answer;

	answer.Load( messageStream );

	XStream::DestroyHandle( messageStream );

	if( NULL != roomServer->roomServiceUnitAddr_ )
	{
		MRS::Address::DestroyHandle( roomServer->roomServiceUnitAddr_ );
		roomServer->roomServiceUnitAddr_ = NULL;
	}
	roomServer->roomServiceUnitAddr_ = peerAddr;

	roomServer->sync_.Set();

	return true;
}

bool __stdcall RoomServer::CreateRoomAns( XStream::Handle messageStream, void * additionalData, void * context )
{
	RoomServer * roomServer = static_cast < RoomServer * > ( additionalData );
	
	MonSummCreateRoomAns answer;

	answer.Load( messageStream );

	XStream::DestroyHandle( messageStream );

	roomServer->sync_.Set();

	return true;
}

bool __stdcall RoomServer::JoinRoomAns( XStream::Handle messageStream, void * additionalData, void * context )
{
	RoomServer * roomServer = static_cast < RoomServer * > ( additionalData );
	
	roomServer->sync_.Set();

	return true;
}

bool __stdcall RoomServer::LeaveRoomAns( XStream::Handle messageStream, void * additionalData, void * context )
{
	RoomServer * roomServer = static_cast < RoomServer * > ( additionalData );
	
	roomServer->sync_.Set();

	return true;
}

void PrintCommand(void )
{
	printf("\n\n\n");
	printf("//=================================================================\n");
	printf("    MonSumm room server tester. \n");
	printf("=================================================================//\n");
	printf("     1 - GetRoomServerInfo.                                        \n");
	printf("     2 - Create room.		                                       \n");
	printf("     3 - Join room.                                                \n");
	printf("     4 - Leave room.                                               \n");
	printf("     0 - Exit.                                                     \n");
	printf("//=================================================================\n");
	printf("                                             INPUT COMMAND:  ");
}


bool RoomServer::Run( void )
{
	if( false == mrsConnector_.Run() )
		return false;

	if( false == tcpConnector_.Run() )
		return false;

	int command = 0;
	while(1)
	{
		PrintCommand();
		scanf_s("%d", &command);
		printf("\n");

		switch(command)
		{
		case 0:
			return true;
		case 1:
			GetInfoReq();
			break;
		case 2:
			CreateRoomReq();
			break;
		case 3:
			JoinRoomReq();
			break;
		case 4:
			LeaveRoomReq();
			break;
		default:
			return false;
		}
	}
	return true;
}
