#pragma once

#include <functional>

#include <XDR\XDR.h>
#include <MRS\MRSAPI.h>

#include "MRSSvcContext.h"
#include "Utils.h"

class MsMRSConnector
{
	typedef void ( __stdcall *PFN_RECV_CALLBACK)	( MRS::Socket::Handle socket, MRS::Address::Handle source, MRS::Address::Handle dest, int messageId, AutoDestroyStreamPtr recvStream, void * userData );
	typedef void ( __stdcall *PFN_ERROR_CALLBACK)	( MRS::Socket::Handle socket, int errorCode, void* userData );
	typedef void ( __stdcall *PFN_DELIVERFAIL_CALLBACK)	( MRS::Socket::Handle socket, int errorCode, MRS::Address::Handle dest, void* packet, size_t packetSize, void* userData );

public:
	MsMRSConnector(void);
	~MsMRSConnector(void);

	bool Initialize
	(					
			std::string					serviceAddr,
			PFN_RECV_CALLBACK			fnOnReceived, 
			PFN_ERROR_CALLBACK			fnOnError, 
			PFN_DELIVERFAIL_CALLBACK	fnOnDeliverFail,
			void *						userData
	);

	bool	Start( void );
	void	Stop( void );

	bool	SendTo( MRS::Address::Handle dstAddr, XDR::IMessage & message );
	bool	SendToPeer( MRSSvcContext & context, XDR::IMessage & message );

private:
	static void __stdcall	OnReceive( MRS::Socket::Handle socket, void* data, size_t size, MRS::Address::Handle source, MRS::Address::Handle dest, void* userData );
	static void __stdcall	OnError( MRS::Socket::Handle socket, int errorCode, void* userData );
	static void __stdcall	OnDeliverFail( MRS::Socket::Handle socket, int errorCode, MRS::Address::Handle dest, void* packet, size_t packetSize, void* userData );

private:
	bool											isReady_;

	MRS::Environment::Handle						env_;
	MRS::Socket::Handle								socket_;

	void *											userData_;

	std::string										serviceAddr_;
	PFN_RECV_CALLBACK								fnOnReceived_;
	PFN_ERROR_CALLBACK								fnOnError_;
	PFN_DELIVERFAIL_CALLBACK						fnOnDeliverFail_;
};

