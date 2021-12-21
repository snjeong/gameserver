#pragma once

#include <string>

#include <Json/json.h>
#include <XDR/XDR.h>
#include <nBase/nbase.h>

class Query
{
	friend class DBMS;

public:
	enum { COMMON_KEY };

protected:
	Query( void );
	virtual ~Query( void );

	bool	Execute( nbase_tx *tx, std::string & ipAddr, unsigned short tcpPort, int64 userNo, nbase_callback_t *fn, void *arg );

protected:
	bool	PostExecute( nquery_res result );
	bool	GetJRows( Json::Value & j_rows );
	bool	GetRID( int64 & rid );
	bool	GetValue( Json::Value j_row, int index, int64		& retValue );
	bool	GetValue( Json::Value j_row, int index, int			& retValue );
	bool	GetValue( Json::Value j_row, int index, std::string	& retValue );

protected:
			std::string		queryString_;
			std::string		resultString_;
};