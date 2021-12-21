#pragma once

#include "Service.h"

#include "Dispatcher.h"
#include "MsMRSConnector.h"
#include "GeneratedFiles/MonSummMsgForRoomServerAndMMS.h"

class MatchMakingService : public Service
{
public:
	MatchMakingService(void);
	~MatchMakingService(void);

	virtual bool Initialize( Config & config, ServiceFactory * services );
	virtual void Uninitialize( void );
	virtual void LogStat( void );
	virtual bool Run( void );

private:
	static bool GetRoomServerInfo	( ContextType context, XStream::Handle recvStream, void * userData );
	static bool CreateRoom			( ContextType context, XStream::Handle recvStream, void * userData );

private:
	ServiceFactory *					services_;

	MsMRSConnector						connector_;
	MRS::Address::Handle				mmsAddr_;

	RoomServerInfo						roomServerInfo_;
};