#include "stdafx.h"

#include "TcpConnector.h"


TcpConnector::TcpConnector(void)
{
}

TcpConnector::~TcpConnector(void)
{
}

bool TcpConnector::Initialize( std::string ipAddr, unsigned short port )
{
	ipAddr_		= ipAddr;
	tcpPort_	= port;

	workers_ = XSystem::ThreadPool::CreateHandle( 2 );
	if( NULL == workers_ )
		return false;

	connector_ = XInNetwork::Connector::CreateHandle( workers_ );
	if(NULL == connector_)
		return false;

	XInNetwork::Connector::SetOnConnected(connector_, OnConnected, this);
	XInNetwork::Connector::SetOnDisconnected(connector_, OnDisconnected, this);
	XInNetwork::Connector::SetOnReceived(connector_, OnReceived, this);

	return true;
}

void TcpConnector::Uninitialize( void )
{
	if( NULL != connector_ )
	{
		XInNetwork::Connector::DestroyHandle( connector_ );
		connector_ = NULL;
	}

	if( NULL != workers_ )
	{
		XSystem::ThreadPool::DestroyHandle( workers_ );
		workers_ = NULL;
	}
}

bool TcpConnector::RegMsgHandler( int messageId, XDR::Dispatcher< bool >::TCallback handler, void* arg )
{
	return dispatcher_.AddHandler( messageId, handler, arg );
}

bool TcpConnector::Run( void )
{
	XInNetwork::Connector::Connect( connector_, ipAddr_.c_str(), tcpPort_ );

	return true;
}

#define RECV_PENDING_BUFFER_SIZE	1024

void __stdcall TcpConnector::OnConnected( XInNetwork::Connector::Handle xConnector, XInNetwork::Link::Handle link, void* additionalData )
{
	TcpConnector * connector = static_cast< TcpConnector * >( additionalData );
	connector->link_ = link;

	XInNetwork::Link::Recv(link, RECV_PENDING_BUFFER_SIZE);
}

void __stdcall TcpConnector::OnDisconnected( XInNetwork::Connector::Handle xConnector, XInNetwork::Link::Handle link, void* additionalData )
{
	TcpConnector * connector = static_cast<TcpConnector*>( additionalData );

	if(NULL == connector->link_)
		return;

	XInNetwork::Link::Close(connector->link_);
}

void __stdcall TcpConnector::OnReceived( XInNetwork::Connector::Handle xConnector, XInNetwork::Link::Handle link, void* additionalData )
{
	TcpConnector * connector = static_cast<TcpConnector*>( additionalData );
	XStream::Handle recvStream = XInNetwork::Link::LockReadStream( link );

	// 메시지 길이
	unsigned long length = 0;
	XStream::Peek( recvStream, & length, sizeof( length ) );
	length = NetworkToHost( length );
	XStream::RemoveLeft( recvStream, sizeof( length ) );

	// 메시지 스트림
	XStream::Handle messageStream = XStream::CreateHandle();
	XStream::Write( messageStream, XStream::GetXMemory(recvStream ), length );
	XStream::RemoveLeft( recvStream, length );

	XInNetwork::Link::UnlockReadStream( link );

	XInNetwork::Link::Recv(link, RECV_PENDING_BUFFER_SIZE);

	connector->dispatcher_.DoDispatch( messageStream, connector );
}