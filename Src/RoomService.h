#pragma once

#include "Service.h"
#include "MsAcceptor.h"
#include "TaskManager.h"
#include "Dispatcher.h"
#include "Room.h"

class RoomService : public Service
{
public:
	RoomService( void );
	~RoomService( void );

	virtual bool Initialize( Config & config, ServiceFactory * services );
	virtual void Uninitialize( void );
	virtual void LogStat( void );
	virtual bool Run( void );

	bool CreateRoom	( ContextType context, std::vector < int64 > & userIds, int64 & roomNo, int64 & matchNo, std::map< int64, std::string > & tickets );

	static bool EnterRoom	( ContextType context, XStream::Handle recvStream, void * userData );
	static bool LeaveRoom	( ContextType context, XStream::Handle recvStream, void * userData );
	static bool OnUserClose	( ContextType context, XStream::Handle recvStream, void * userData );

private:
	bool Login( UserContext * userContext );

public:
	unsigned short						serverId_;
	ServiceFactory *					services_;

	MsAcceptor							acceptor_;
	TaskManager							tm_;
	Dispatcher		 					dispatcher_;

	XSystem::Threading::CriticalSection	csUserContexts_;
	std::map < int64, ContextType >	userContexts_;

	XSystem::Threading::CriticalSection	csRooms_;
	std::map < int64, RoomType >		rooms_;
};