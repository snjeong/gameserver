#pragma once

#include <time.h>
#include <vector>
#include <atomic>

#include "UserContext.h"
#include "Match.h"

class Room
{
	friend class RoomService;

public:
	enum Stat{ OnCreate, OnCompeteEnterence, OnCompleteReady, OnStartMatch, OnEndMatch, Invalid };

	Room( unsigned short serverId );
	~Room(void);

	bool		Enter( ContextType context );
	bool		Leave( ContextType context );

	bool		SendToAll( XDR::IMessage & message );
	bool		SendToOther( ContextType context, XDR::IMessage & message );

private:
	int64								no_;
	int64								matchNo_;

	__time64_t							createdTime_;

	std::vector	< int64 >				reservationList_;
	std::vector < ContextType >			participants_;

	static std::atomic_uint16_t			sequnce_;
};

typedef XSystem::XTL::SmartPtr < Room > RoomType;