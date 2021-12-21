#include "stdafx.h"

#include <Utils.h>

#include "MRSConnector.h"

MRSConnector::MRSConnector(void)
	: isReady_( false ), env_( NULL ), socket_( NULL ), mmsAddr_( NULL )
{
}


MRSConnector::~MRSConnector(void)
{
}

bool MRSConnector::Initialize( std::string mmsAddr )
{
	env_ = MRS::Environment::CreateHandle( 2 );
	if( NULL == env_ )
		return false;

	socket_ = MRS::Socket::CreateHandle( env_ );
	if( NULL == socket_ )
		return false;

	mmsAddr_ = MRS::Address::CreateHandle( mmsAddr.c_str() );
	isReady_ = true;

	return true;
}

void MRSConnector::Uninitialize( void )
{
	if( NULL != socket_ )
	{
		MRS::Socket::DestroyHandle( socket_ );
		socket_ = NULL;
	}

	if( NULL != env_ )
	{
		MRS::Environment::DestroyHandle( env_ );
		env_ = NULL;
	}

	if( NULL != mmsAddr_ )
	{
		MRS::Address::DestroyHandle( mmsAddr_ );
		mmsAddr_ = NULL;
	}
}

bool MRSConnector::Run( void )
{
	if(false == MRS::Socket::SetOnReceived( socket_, &OnReceive, this ) )
		return false;

	if(false == MRS::Socket::SetOnError(socket_, &OnError, this ))
		return false;

	if(false == MRS::Socket::SetOnDeliverFail(socket_, &OnDeliverFail, this ))
		return false;

	if(false == MRS::Socket::Join(socket_, mmsAddr_ ))
		return false;

	return true;
}

bool MRSConnector::RegMsgHandler( int messageId, XDR::Dispatcher< bool >::TCallback handler, void* additionalData )
{
	return dispatcher_.AddHandler( messageId, handler, additionalData );
}

bool MRSConnector::SendTo( MRS::Address::Handle dstAddr, XDR::IMessage & message )
{
	XStream::Handle sendStream = XStream::CreateHandle();
	if( NULL == sendStream )
		return false;

	ScopedInvoker invoker( [ &sendStream ]()
	{
		if( NULL != sendStream )
		{
			XStream::DestroyHandle( sendStream );
			sendStream = NULL;
		}
	});

	int length = message.GetLength();
	length = HostToNetwork( length );
	XStream::Write( sendStream, &length, sizeof( length ) );
	message.Save( sendStream );

	if( false == MRS::Socket::SendTo( socket_, dstAddr, XStream::GetXMemory( sendStream ), XStream::GetLength( sendStream ) ) )
		return false;

	return true;
}

void __stdcall MRSConnector::OnReceive( MRS::Socket::Handle socket, void* data, size_t size, MRS::Address::Handle source, MRS::Address::Handle dest, void* additionalData )
{
	MRSConnector * connector = reinterpret_cast < MRSConnector * > ( additionalData );

	unsigned int messageSize = NetworkToHost( *( reinterpret_cast< unsigned int* >( data ) ) );
	if( messageSize > size - sizeof( messageSize ) )
		return;

	XStream::Handle msgStream = XStream::CreateHandle();
	int retValue = XStream::Write( msgStream, ( unsigned char* ) data + sizeof( messageSize ), messageSize );
	if( 0 >= retValue )
		return;

	MRS::Address::Handle peerAddr = MRS::Address::Clone( source );
	connector->dispatcher_.DoDispatch( msgStream, peerAddr );
}

void __stdcall MRSConnector::OnError( MRS::Socket::Handle socket, int errorCode, void* additionalData )
{
	LOG_ERROR( "Error code: " << errorCode );
}

void __stdcall MRSConnector::OnDeliverFail( MRS::Socket::Handle socket, int errorCode, MRS::Address::Handle dest, void* packet, size_t packetSize, void* additionalData )
{
	LOG_ERROR( "Error code: " << errorCode );
}