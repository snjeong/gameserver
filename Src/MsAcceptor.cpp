#include "stdafx.h"

#include "Utils.h"
#include "MsAcceptor.h"

MsAcceptor::MsAcceptor(void)
{
}


MsAcceptor::~MsAcceptor(void)
{
}

bool MsAcceptor::Initialize(const char * address, unsigned short port, void *userData, PFN_CALLBACK fnOnAccepted, PFN_RECV_CALLBACK	fnOnReceived, PFN_CALLBACK fnOnClosed, PFN_CALLBACK fnOnLinkDestroyed)
{
	userData_				= userData;

	fnOnAccepted_			= fnOnAccepted;
	fnOnReceived_			= fnOnReceived;
	fnOnClosed_				= fnOnClosed;
	fnOnLinkDestroyed_		= fnOnLinkDestroyed;

	threadPool_ = XSystem::ThreadPool::CreateHandle( XPlatform::GetProcessorCount() * 2 );
	if( nullptr == threadPool_ )
	{
		LOG_ERROR( "nullptr == threadPool_ (" << XPlatform::GetProcessorCount() * 2 << ")" );
		return false;
	}

	acceptor_ = XInNetwork::Acceptor::CreateHandle( threadPool_ );
	if( nullptr == threadPool_ )
	{
		LOG_ERROR( "nullptr == threadPool_" << XPlatform::GetProcessorCount() * 2 << ")" );
		return false;
	}

	XInNetwork::Acceptor::SetOnAccepted	(acceptor_, &OnAccepted,	this);
	XInNetwork::Acceptor::SetOnReceived	(acceptor_, &OnReceived,	this);
	XInNetwork::Acceptor::SetOnClosed	(acceptor_, &OnClosed,		this);

	XInNetwork::Acceptor::AddBinding ( acceptor_, address, port );

	LOG_TRACE( "Binding szIP " << address << " iPort " << port );

	return true;
}

bool MsAcceptor::Start(void)
{
	return XInNetwork::Acceptor::Start( acceptor_ );
}

void MsAcceptor::Stop(void)
{
	if( nullptr != acceptor_ )
	{
		XInNetwork::Acceptor::Stop( acceptor_ );
		XInNetwork::Acceptor::DestroyHandle( acceptor_ );
		acceptor_ = nullptr;
	}

	if( nullptr != threadPool_ )
	{
		XSystem::ThreadPool::DestroyHandle( threadPool_ );
		threadPool_ = nullptr;
	}
}

void __stdcall	MsAcceptor::OnLinkDestroy( XInNetwork::Link::Handle link, void *userData )
{
	MsAcceptor * pThis = reinterpret_cast < MsAcceptor * > ( userData );

	if( nullptr != pThis->fnOnLinkDestroyed_ )
	{
		pThis->fnOnLinkDestroyed_( link, pThis->userData_ );
	}
}

void __stdcall	MsAcceptor::OnAccepted( XInNetwork::Acceptor::Handle acceptor, XInNetwork::Link::Handle link, void * userData )
{
	MsAcceptor * pThis = reinterpret_cast < MsAcceptor * > ( userData );

	if( nullptr != pThis->fnOnAccepted_ )
		pThis->fnOnAccepted_( link, pThis->userData_ );

	XInNetwork::Link::SetData( link, pThis, &OnLinkDestroy, userData );
}

void __stdcall	MsAcceptor::OnReceived( XInNetwork::Acceptor::Handle acceptor, XInNetwork::Link::Handle link, void * userData )
{
	ScopedInvoker scopedAction_Close( [ link ] ()
	{
		XInNetwork::Link::Close( link );
	});

	MsAcceptor * pThis = reinterpret_cast < MsAcceptor * > ( userData );
	if( nullptr == pThis->fnOnReceived_ )
	{
		LOG_ERROR( "The user's OnReceived is null." );
		return;
	}

	AutoDestroyStream recvStream = XStream::CreateHandle();
	if( NULL == recvStream.Get() )
	{
		LOG_ERROR( "Fail to create recieve stream." );
		return;
	}
	else
	{
		XSystem::MemoryPool::XMemory  datagram = nullptr;
		XStream::Handle tempStream = XInNetwork::Link::LockReadStream( link );
		datagram = XStream::Detach( tempStream );
		XInNetwork::Link::UnlockReadStream( link );
		XStream::Attach( recvStream.Get(), datagram );
		if( 0 == XStream::GetLength( recvStream.Get() ) )
		{
			LOG_ERROR( "Fail to attach datagram to receive stream." );
			return;
		}
	}

	unsigned long datagramSize = 0;
	while( 0 < ( datagramSize = XStream::GetLength( recvStream.Get() ) ) )
	{
		unsigned long length = 0;
		if( false == XStream::Peek( recvStream.Get(), & length, sizeof( length ) ) )
		{
			LOG_ERROR( "Fail to peek length from received stream." );
			return;
		}

		length = NetworkToHost( length );
		if( datagramSize < length + sizeof( length ) )
		{
			LOG_ERROR( "Invlaid length of received stream." );
			return;
		}

		XStream::RemoveLeft( recvStream.Get(), sizeof( length ) );

		AutoDestroyStreamPtr msgStreamPtr = new AutoDestroyStream( XStream::CreateHandle() );
		if( nullptr == msgStreamPtr->Get() )
		{
			LOG_ERROR( "Fail to create xstream for message." );
			return;
		}
		XStream::Write( msgStreamPtr->Get(), XStream::GetXMemory( recvStream.Get() ), length );
		XStream::RemoveLeft( recvStream.Get(), length );

		pThis->fnOnReceived_( link, msgStreamPtr, pThis->userData_ );
	}

	XInNetwork::Link::Recv( link, 2048 );

	scopedAction_Close.Cancel();
}

void __stdcall	MsAcceptor::OnClosed( XInNetwork::Acceptor::Handle acceptor, XInNetwork::Link::Handle link, void * userData )
{
	MsAcceptor * pThis = reinterpret_cast < MsAcceptor * > ( userData );
	
	if( nullptr == pThis->fnOnClosed_ )
	{
		LOG_ERROR( "The user's OnClose is null." );
		return;
	}

	pThis->fnOnClosed_( link, pThis->userData_ );
}