#pragma once

#include <XDR/XDR.h>
#include <XBase/XInNetwork.h>

class MsLink
{
public:
	MsLink( void );
	~MsLink( void );

	MsLink( const MsLink & link );
	MsLink( XInNetwork::Link::Handle xlink );

	void		Set		( XInNetwork::Link::Handle xlink );
	void		Destory	( void );

	int			Send	( XDR::IMessage & msg );
	bool		Close	( void );

	MsLink &	operator=( const MsLink & rhs );

private:
	XInNetwork::Link::Handle xLink_;
};