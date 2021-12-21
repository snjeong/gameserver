#pragma once

#include <MRS/MRSAPI.h>
#include <XDR/XDR.h>

class MRSConnector
{
public:
	MRSConnector(void);
	~MRSConnector(void);

	bool Initialize( std::string mmsAddr );
	void Uninitialize( void );

	bool Run( void );
	bool RegMsgHandler( int messageId, XDR::Dispatcher< bool >::TCallback handler, void* additionalData );

	bool SendTo( MRS::Address::Handle dstAddr, XDR::IMessage & message );

private:
	static void __stdcall	OnReceive( MRS::Socket::Handle socket, void* data, size_t size, MRS::Address::Handle source, MRS::Address::Handle dest, void* additionalData );
	static void __stdcall	OnError( MRS::Socket::Handle socket, int errorCode, void* additionalData );
	static void __stdcall	OnDeliverFail( MRS::Socket::Handle socket, int errorCode, MRS::Address::Handle dest, void* packet, size_t packetSize, void* additionalData );

private:
	bool											isReady_;
	XDR::Dispatcher < bool >						dispatcher_;

	MRS::Environment::Handle						env_;
	MRS::Socket::Handle								socket_;
	MRS::Address::Handle							mmsAddr_;
};

