#pragma once

class Authenticator;
class DBMS;
class RoomService;
class MatchMakingService;

class ServiceFactory
{
public:
	virtual Authenticator		* GetAuthenticator( void )		= 0;
	virtual DBMS				* GetDBMS( void )				= 0;
	virtual RoomService			* GetRoomService( void )		= 0;
	virtual MatchMakingService	* GetMatchMakingService( void )	= 0;
};
