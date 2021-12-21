#include "stdafx.h"

#include "Utils.h"
#include "MsLink.h"

MsLink::MsLink(void)
{
}

MsLink::~MsLink(void)
{
	Destory();
}

MsLink::MsLink( XInNetwork::Link::Handle xLink )
{
	if( 0 > XInNetwork::Link::AddRef( xLink ) )
		xLink_ = nullptr;
	else
		xLink_ = xLink;
}

MsLink::MsLink( const MsLink & xLink )
{
	if( 0 > XInNetwork::Link::AddRef( xLink_ ) )
		xLink_ = nullptr;
	else
		xLink_ = xLink.xLink_;
}

void MsLink::Destory( void )
{
	if( nullptr != xLink_ )
	{
		XInNetwork::Link::Release( xLink_ );
		xLink_ = nullptr;
	}
}

void MsLink::Set( XInNetwork::Link::Handle xLink )
{
	Destory();

	if( 0 > XInNetwork::Link::AddRef( xLink ) )
		xLink_ = nullptr;
	else
		xLink_ = xLink;
}

MsLink & MsLink::operator=( const MsLink & rhs )
{
	Destory();
	this->xLink_ = rhs.xLink_;
	return *this;
}

int	MsLink::Send( XDR::IMessage & message )
{
	if( nullptr == xLink_ )
	{
		LOG_ERROR( "Invlid X link." << message.GetID() );
		return -1;
	}

	int length = message.GetLength();
	length = HostToNetwork( length );

	AutoDestroyStream sendStream = XStream::CreateHandle();
	if( nullptr == sendStream.Get() )
	{
		LOG_ERROR( "Fail to send stream." << message.GetID() );
		return -1;
	}

	XSystem::MemoryPool::XMemory memory = XStream::GetXMemory ( sendStream.Get() );

	XStream::Write( sendStream.Get(), &length, sizeof( length ) );
	if( false == message.Save( sendStream.Get() ) )
	{
		LOG_ERROR( "Fail to save XDR message to X stream." << message.GetID() );
		return -1;
	}

	return XInNetwork::Link::Send ( xLink_, memory );
}

bool MsLink::Close( void )
{
	if( nullptr == xLink_ )
	{
		LOG_ERROR( "The link is not set." );
		return false;
	}

	XInNetwork::Link::Close( xLink_ );
	Destory();

	return true;
}
