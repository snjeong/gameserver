#pragma once

#include <time.h>

#include "Service.h"

class Authenticator : public Service
{
public:
	Authenticator(void);
	~Authenticator(void);

	virtual bool Initialize( Config & config, ServiceFactory * services );
	virtual void Uninitialize( void );
	virtual void LogStat( void );
	virtual bool Run( void );

	std::string		MakeTicket( __int64 userNo, __int64 roomNo, __int64 matchNo, __time64_t timeStamp );
	bool			CheckTicket( const std::string & ticket, __int64 userNo, __int64 roomNo, __int64 matchNo, __time64_t timeStamp );
};

