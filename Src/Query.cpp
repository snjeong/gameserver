#include "stdafx.h"

#include "Query.h"

Query::Query( void )
{
}

Query::~Query( void )
{
}

#define QUERY_TIME_OUT 5 * 1000
bool Query::Execute( nbase_tx *tx, std::string & ipAddr, unsigned short tcpPort, int64 userNo, nbase_callback_t *fn, void *arg )
{
	std::string keySpace = "";
	std::string ckey = "";
	if( Query::COMMON_KEY == userNo )
	{
		keySpace = "monsum_comm";
		ckey = "common";
	}
	else
	{
		keySpace = "monsum_acc";
		ckey = std::to_string( userNo );
	}

	int retvalue = 0;
	if( NULL == tx )
	{
		retvalue = nbase_query_callback_opts
		(
			ipAddr.c_str(), tcpPort,
			keySpace.c_str(), ckey.c_str(),
			queryString_.c_str(),
			QUERY_TIME_OUT, NBASE_RESULT_FORMAT_JSON, fn, arg
		);
	}
	else
	{
		nbase_query_callback_opts_with_tx
		(
			tx,
			keySpace.c_str(), ckey.c_str(), 
			queryString_.c_str(),
			QUERY_TIME_OUT, NBASE_RESULT_FORMAT_JSON, fn, arg 
		);
	}

	if( 0 > retvalue )
		return false;
	
	return true;
}

bool Query::PostExecute( nquery_res result )
{
	resultString_ = nbase_get_result_str( result );
	return true;
}

bool Query::GetJRows( Json::Value & j_rows )
{
	Json::Value root; 
	Json::Reader reader; 

	bool bIsParsed = reader.parse( resultString_, root);

	if(!bIsParsed)
		return false; 

	const Json::Value retdata = root["retdata"];
	int retcode = root["retcode"].asInt(); 

	if(retcode < 0)
		return false; 

	j_rows = retdata["rows"]; 

	return true; 
}

bool Query::GetRID( int64 & rid )
{
	Json::Value root;
	Json::Reader reader;
	bool bIsParsed = reader.parse( resultString_, root );

	if(!bIsParsed)
		return false; 

	const Json::Value retdata = root["retdata"];
	int retcode = root["retcode"].asInt(); 

	if( retcode <= 0 )
		return false; 

	if( false == root["rid"].isConvertibleTo( Json::uintValue ) )
		return false; 

	rid = root["rid"].asInt64();

	return true; 
}

bool Query::GetValue( Json::Value j_row, int index, int64 & retValue )
{
	if( false == j_row[index].isConvertibleTo( Json::uintValue ) )
	{
		LOG_ERROR( "Fail to Convert Index(" << index << ")(" << queryString_ << ")(" << resultString_ << ")" );
		return false;
	}

	retValue = j_row[index].asInt64();

	return true;
}

bool Query::GetValue( Json::Value j_row, int index, int & retValue )
{
	if( false == j_row[index].isConvertibleTo( Json::intValue ) )
	{
		LOG_ERROR( "Fail to Convert Index(" << index << ")(" << queryString_ << ")(" << resultString_ << ")" );
		return false;
	}

	retValue = j_row[index].asInt();

	return true;
}

bool Query::GetValue( Json::Value j_row, int index, std::string & retValue )
{
	if( false == j_row[index].isConvertibleTo( Json::stringValue ) )
	{
		LOG_ERROR( "Fail to Convert Index(" << index << ")(" << queryString_ << ")(" << resultString_ << ")" );
		return false;
	}

	retValue = j_row[index].asString();

	return true;
}