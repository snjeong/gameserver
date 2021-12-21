#pragma once

// #include <functional>
#include <XBase/XInNetwork.h>

#include "Utils.h"
#include "MsLink.h"

class MsAcceptor
{
public:
	typedef void ( __stdcall *PFN_CALLBACK)			( XInNetwork::Link::Handle link, void * );
	typedef void ( __stdcall *PFN_RECV_CALLBACK)	( XInNetwork::Link::Handle link, AutoDestroyStreamPtr, void * );

	MsAcceptor( void );
	~MsAcceptor( void );

	bool Initialize
		(					const char *		address,
							unsigned short		port,
							void *				userData, 
							PFN_CALLBACK		fnOnAccepted, 
							PFN_RECV_CALLBACK	fnOnReceived, 
							PFN_CALLBACK		fnOnClosed, 
							PFN_CALLBACK		fnOnLinkDestroyed
		);

	bool Start(void);
	void Stop(void);

private:
	static void __stdcall	OnLinkDestroy	( XInNetwork::Link::Handle link, void *userData );
	static void __stdcall	OnAccepted		( XInNetwork::Acceptor::Handle acceptor, XInNetwork::Link::Handle link, void * userData );
	static void __stdcall	OnReceived		( XInNetwork::Acceptor::Handle acceptor, XInNetwork::Link::Handle link, void * userData );
	static void __stdcall	OnClosed		( XInNetwork::Acceptor::Handle acceptor, XInNetwork::Link::Handle link, void * userData );

private:
	XSystem::ThreadPool::Handle		threadPool_;
	XInNetwork::Acceptor::Handle	acceptor_;

	void *							userData_;

	PFN_CALLBACK					fnOnAccepted_;
	PFN_RECV_CALLBACK				fnOnReceived_;
	PFN_CALLBACK					fnOnClosed_;
	PFN_CALLBACK					fnOnLinkDestroyed_;
};