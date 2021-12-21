#include "stdafx.h"
#include "MRSSvcContext.h"


MRSSvcContext::MRSSvcContext( Context::WorkType work, LPVOID arg )
	: Context( work, arg ), peerAddr_( NULL )
{
}


MRSSvcContext::~MRSSvcContext(void)
{
	if( NULL != peerAddr_ )
	{
		MRS::Address::DestroyHandle( peerAddr_ );
		peerAddr_ = NULL;
	}
}

void MRSSvcContext::SetPeerAddr( MRS::Address::Handle peerAddr )
{
	if( NULL != peerAddr )
		peerAddr_ = MRS::Address::Clone( peerAddr );
}

MRS::Address::Handle MRSSvcContext::GetPeerAddr( void )
{
	return peerAddr_;
}

void MRSSvcContext::Terminate( void )
{
}