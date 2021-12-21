#pragma once

#include <XDR/XDR.h>
#include <XBase/XSystem.h>
#include <XBase/XStream.h>
#include <XBase/XInNetwork.h>

class TcpConnector
{
public:
	TcpConnector(void);
	~TcpConnector(void);

	bool Initialize( std::string ipAddr, unsigned short port );
	void Uninitialize( void );

	bool Run( void );

	bool RegMsgHandler( int messageId, XDR::Dispatcher< bool >::TCallback handler, void* arg );

private:
	static void __stdcall OnConnected				( XInNetwork::Connector::Handle connector, XInNetwork::Link::Handle link, void* userArg );
	static void __stdcall OnDisconnected			( XInNetwork::Connector::Handle connector, XInNetwork::Link::Handle link, void* userArg );
	static void __stdcall OnReceived				( XInNetwork::Connector::Handle connector, XInNetwork::Link::Handle link, void* userArg );

private:
	std::string										ipAddr_;
	unsigned short									tcpPort_;

	XDR::Dispatcher < bool >						dispatcher_;

	XInNetwork::Connector::Handle					connector_;
	XSystem::ThreadPool::THandle					workers_;
	XInNetwork::Link::Handle						link_;
};