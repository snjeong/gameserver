#include "stdafx.h"
#include "Authenticator.h"

Authenticator::Authenticator(void)
{
}

Authenticator::~Authenticator(void)
{
}

bool Authenticator::Initialize( Config & config, ServiceFactory * services )
{
	return true;
}

void Authenticator::Uninitialize( void )
{
}

void Authenticator::LogStat( void )
{
}

bool Authenticator::Run( void )
{
	return true;
}

std::string Authenticator::MakeTicket( __int64 userNo, __int64 roomNo, __int64 matchNo, __int64 timestamp )
{
	return "";
}

bool Authenticator::CheckTicket( const std::string & ticket, __int64 userNo, __int64 roomNo, __int64 matchNo, __time64_t timeStamp )
{
	return true;
}