#include "stdafx.h"
#include "UserContext.h"


UserContext::UserContext( XInNetwork::Link::Handle link, Context::WorkType work, LPVOID arg )
	: Context( work, arg )
{
	link_.Set( link );
}


UserContext::~UserContext(void)
{
}

int	UserContext::Send( XDR::IMessage & message )
{
	return link_.Send( message );
}

bool UserContext::Close( void )
{
	return link_.Close();
}

void UserContext::Terminate( void )
{
	Close();
}

void UserContext::Reset( UserContext & rhs )
{
	user_ = rhs.user_;

	return;
}
