#include "stdafx.h"
#include "Room.h"

std::atomic_uint16_t Room::sequnce_ = 0;

Room::Room( unsigned short serverId )
	: no_( 0 ), matchNo_( 0 ), createdTime_( 0 )
{
	int64 gerneratedNumber = serverId;
	gerneratedNumber <<= 16;

	unsigned short currentSequnce = sequnce_++;
	gerneratedNumber += currentSequnce;
	gerneratedNumber <<= 32;

	createdTime_ = _time64( NULL );

	int64 lowTime = 0x00000000ffffffff & createdTime_;

	gerneratedNumber = gerneratedNumber | lowTime;
	
	no_			= gerneratedNumber;
	matchNo_	= gerneratedNumber;
}


Room::~Room(void)
{
}


bool Room::Enter( ContextType context )
{
	return true;
}

bool Room::Leave( ContextType context )
{
	return true;
}