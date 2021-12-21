#pragma once

#include <Windows.h>

#include <map>
#include <functional>

#include "ServiceFactory.h"
#include "UserContext.h"
#include "MRSSvcContext.h"

//template < typename ContextType >
class Dispatcher
{
public:
	typedef std::function < bool ( ContextType, XStream::Handle, void * ) > HandlerType ;

	Dispatcher	( void )	{}
	~Dispatcher	( void )	{}

	bool RegMsgHandler( int messageId, HandlerType handler, void* additionalData )
	{
		auto pos = handlers_.find( messageId );
		if( handlers_.end() != pos)
		{
			LOG_ERROR( "Already existed message handler (" << messageId << ")" );
			return false;
		}

		handlers_.insert( std::make_pair( messageId, std::make_pair( handler, additionalData ) ) );

		return true;
	}

	bool DoProcess( ContextType context, int messageId, XStream::Handle recvStream )
	{
		auto pos = handlers_.find( messageId );
		if( handlers_.end() == pos)
		{
			LOG_ERROR( "Not existed message handler (" << messageId << ")" );
			return false;
		}

		return pos->second.first( context, recvStream, pos->second.second );
	}

private:
	std::map < int, std::pair < HandlerType, void* > >	handlers_;
};

//typedef Dispatcher < UserContextType	>		UserDispatcherType;		// dispatching user message
//typedef Dispatcher < MRSSvcContextType	>		MRSSvcDispatcherType;	// dispatching MMS message