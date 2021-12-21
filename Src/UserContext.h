#pragma once

#include < atomic >
#include <functional>
#include <XBase/XSystem.h>
#include <XBase/XStream.h>

#include "Context.h"
#include "Task.h"
#include "MsLink.h"
#include "User.h"

class UserContext : public Context
{
public:
	enum { INVALID, UNSETTLE, ON_SUSPENDED, ON_PROCESSING, ON_DISCONNETED };

	UserContext( XInNetwork::Link::Handle link, Context::WorkType work, LPVOID arg );
	virtual ~UserContext(void);

	void	Reset		( UserContext & rhs );
	int		Send		( XDR::IMessage & msg );
	bool	Close		( void );

	static inline UserContext * ConvertToPtr( ContextType context )
	{
		return static_cast < UserContext * > ( ContextType::PointerType( context ) );
	}

protected:
	virtual void Terminate( void );

public:
	int									state_;
	UserType							user_;
	MsLink								link_;
};

