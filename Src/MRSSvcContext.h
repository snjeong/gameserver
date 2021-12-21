#pragma once

#include <XBase/XSystem.h>
#include <MRS/MRSAPI.h>

#include "Context.h"

class MRSSvcContext : public Context
{
public:
	MRSSvcContext( Context::WorkType work, LPVOID arg );
	virtual ~MRSSvcContext(void);

	void					SetPeerAddr( MRS::Address::Handle peerAddr );
	MRS::Address::Handle	GetPeerAddr( void );

	static inline MRSSvcContext * ConvertToPtr( ContextType context )
	{
		return static_cast < MRSSvcContext * > ( ContextType::PointerType( context ) );
	}

protected:
	virtual void Terminate( void );

protected:
	MRS::Address::Handle	peerAddr_;
};