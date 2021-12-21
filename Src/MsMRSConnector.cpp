#include "stdafx.h"
#include "MsMRSConnector.h"
#include "Dispatcher.h"

MsMRSConnector::MsMRSConnector(void)
	: env_( NULL ), socket_( NULL ), isReady_( false ), userData_( NULL ), serviceAddr_( "" ), fnOnReceived_( NULL ), fnOnError_( NULL ), fnOnDeliverFail_( NULL )
{
}

MsMRSConnector::~MsMRSConnector(void)
{
	Stop();
}

bool MsMRSConnector::Initialize( std::string serviceAddr, PFN_RECV_CALLBACK fnOnReceived, PFN_ERROR_CALLBACK fnOnError, PFN_DELIVERFAIL_CALLBACK fnOnDeliverFail, void * userData )
{
	if( NULL == fnOnReceived )
	{
		LOG_ERROR( "The received callback function of connector is null." );
		return false;
	}

	int numWorker = XPlatform::GetProcessorCount() * 2;
	if( 0 >= numWorker )
	{
		LOG_ERROR( "Fail to get processor count." );
		return false;
	}
		
	env_ = MRS::Environment::CreateHandle( numWorker );
	if( NULL == env_ )
	{
		LOG_ERROR( "Fail to create MRS environment handle. ("  << numWorker << ")" );
		return false;
	}

	socket_ = MRS::Socket::CreateHandle( env_ );
	if( NULL == socket_ )
	{
		LOG_ERROR( "Fail to create MRS socket. ("  << numWorker << ")(" << (void*) env_ << ")");
		return false;
	}

	LOG_ERROR( "[ INIT ][ SUCCESS ] MRSDispatcher is initialized. ("  << numWorker << ")" );

	userData_			= userData;
	serviceAddr_		= serviceAddr;
	fnOnReceived_		= fnOnReceived;
	fnOnError_			= fnOnError;
	fnOnDeliverFail_	= fnOnDeliverFail;
	isReady_			= true;

	return true;
}

void MsMRSConnector::Stop( void )
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

	LOG_ERROR( "[ TERMINATE ][ SUCCESS ] MRSDispatcher is uninitialized." );
}

bool MsMRSConnector::Start( void )
{
	if(false == MRS::Socket::SetOnReceived( socket_, &OnReceive, this ) )
	{
		LOG_ERROR( "Fail to set OnReceive callback." );
		return false;
	}

	if(false == MRS::Socket::SetOnError(socket_, &OnError, this ))
	{
		LOG_ERROR( "Fail to set OnError callback." );
		return false;
	}

	if(false == MRS::Socket::SetOnDeliverFail(socket_, &OnDeliverFail, this ))
	{
        LOG_ERROR( "Fail to set OnDeliverFail callback." );
		return false;
	}

	MRS::Address::Handle serviceMRSAddr = MRS::Address::CreateHandle( serviceAddr_.c_str() );
	if( NULL == serviceMRSAddr )
    {
		LOG_ERROR( "Fail to set MRS service address handle. (" << serviceAddr_ << ")" );
		return false;
    }

	if(false == MRS::Socket::Join(socket_, serviceMRSAddr))
	{
		LOG_ERROR( "Fail to join to service channel." );
		return false;
	}

	MRS::Address::DestroyHandle( serviceMRSAddr );

	LOG_ERROR( "[ RUN ][ SUCCESS ] Completed................. !!" );

	return true;
}

bool MsMRSConnector::SendTo( MRS::Address::Handle dstAddr, XDR::IMessage & message )
{
	XStream::Handle sendStream = XStream::CreateHandle();
	if( NULL == sendStream )
	{
		char mrsAddr[128];
		memset( mrsAddr, 0, sizeof( mrsAddr ) );
		MRS::Address::ToString( dstAddr, mrsAddr, sizeof( mrsAddr ) - 1 );
		LOG_ERROR( "Invlaid sendStream. (" << mrsAddr << ")(" << message.GetID() << ")" );
		return false;
	}

	ScopedInvoker scopedAction( [ &sendStream ]()
	{
		XStream::DestroyHandle( sendStream );
		sendStream = NULL;
	});


	int length = message.GetLength();
	length = HostToNetwork( length );
	XStream::Write( sendStream, &length, sizeof( length ) );
	message.Save( sendStream );

	if( false == MRS::Socket::SendTo( socket_, dstAddr, XStream::GetXMemory( sendStream ), XStream::GetLength( sendStream ) ) )
	{
		char mrsAddr[128];
		memset( mrsAddr, 0, sizeof( mrsAddr ) );
		MRS::Address::ToString( dstAddr, mrsAddr, sizeof( mrsAddr ) - 1 );
		LOG_ERROR( "Fail to send MRS message. (" << mrsAddr << ")(" << message.GetID() << ")" );
		return false;
	}

	return true;
}

bool MsMRSConnector::SendToPeer( MRSSvcContext & context, XDR::IMessage & message )
{
	MRS::Address::Handle peerAddr = context.GetPeerAddr();
	if( NULL == peerAddr )
	{
		LOG_ERROR( "Invalid peer address. (" << message.GetID() << ")" );
		return false;
	}

	return SendTo( peerAddr, message );
}

void __stdcall MsMRSConnector::OnReceive( MRS::Socket::Handle socket, void* data, size_t size, MRS::Address::Handle source, MRS::Address::Handle dest, void* userData )
{
	if( NULL == socket || NULL == source || NULL == dest || NULL == data || 0 >= size || NULL == userData )
	{
		LOG_ERROR( "Invalid argument. (" << socket << ")(" << source << ")(" << dest << ")(" << data << ")(" << size << ")(" << userData << ")" );
		return;
	}

	MsMRSConnector * connector = reinterpret_cast < MsMRSConnector * > ( userData );

	if( 4 > size )
	{
		LOG_ERROR( "The datagram is too short. (" << size << ")");
		return;
	}

	unsigned int messageSize = NetworkToHost( *( reinterpret_cast< unsigned int* >( data ) ) );
	if( messageSize > size - sizeof( messageSize ) )
	{
		LOG_ERROR( "The message size is bigger than datagram (" << messageSize << ")(" << size << ")");
		return;
	}

	AutoDestroyStreamPtr msgStreamPtr = new AutoDestroyStream( XStream::CreateHandle() );
	int retValue = XStream::Write( msgStreamPtr->Get(), ( unsigned char* ) data + sizeof( messageSize ), messageSize );
	if( 0 >= retValue )
	{
		LOG_ERROR( "Fail to write received datagram to X stream. (" << messageSize << ")(" << std::to_string( size ) << ")(" << std::to_string( retValue ) << ")" );
		return;
	}

	unsigned int messageId = 0;
	XStream::Peek( msgStreamPtr->Get(), &messageId, sizeof( messageId ) );
	messageId = NetworkToHost( messageId );

	if( NULL != connector->fnOnReceived_ )
		connector->fnOnReceived_( socket, source, dest, messageId, msgStreamPtr, connector->userData_ );
}

void __stdcall MsMRSConnector::OnError( MRS::Socket::Handle socket, int errorCode, void* userData )
{
	LOG_ERROR( "Error code: " << errorCode );
}

void __stdcall MsMRSConnector::OnDeliverFail( MRS::Socket::Handle socket, int errorCode, MRS::Address::Handle dest, void* packet, size_t packetSize, void* userData )
{
	LOG_ERROR( "Error code: " << errorCode );
}