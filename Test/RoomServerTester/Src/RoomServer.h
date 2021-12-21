#pragma once

#include "MRSConnector.h"
#include "TcpConnector.h"

class RoomServer
{
public:
	RoomServer(void);
	~RoomServer(void);

	bool Initialize( void );
	void Uninitialize( void );

	bool Run( void );

	bool GetInfoReq( void );
	bool CreateRoomReq( void );
	bool JoinRoomReq( void );
	bool LeaveRoomReq( void );
	
	static bool __stdcall SubscribeSig	( XStream::Handle messageStream, void * additionalData, void * context );
	static bool __stdcall GetInfoAns	( XStream::Handle messageStream, void * additionalData, void * context );
	static bool __stdcall CreateRoomAns	( XStream::Handle messageStream, void * additionalData, void * context );
	static bool __stdcall JoinRoomAns	( XStream::Handle messageStream, void * additionalData, void * context );
	static bool __stdcall LeaveRoomAns	( XStream::Handle messageStream, void * additionalData, void * context );

private:
	MRS::Address::Handle				roomServiceAddr_;
	MRS::Address::Handle				roomServiceUnitAddr_;

	XSystem::Threading::AutoResetEvent	sync_;

	MRSConnector						mrsConnector_;
	TcpConnector						tcpConnector_;
};

