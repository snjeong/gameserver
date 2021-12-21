#pragma once

#include "Config.h"
#include "ServiceFactory.h"

class Service
{
public:
	virtual ~Service(void) {};

	virtual bool Initialize( Config & config, ServiceFactory * services ) = 0;
	virtual void Uninitialize( void ) = 0;
	virtual bool Run( void ) = 0;

	virtual void LogStat() = 0;
};

