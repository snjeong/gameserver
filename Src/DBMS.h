#pragma once

#include "Query.h"

#include "Service.h"
#include "UserContext.h"
#include "TaskManager.h"

class DBMS : public Service
{
	struct CompletionCallbackPram
	{
		CompletionCallbackPram( void ) : dbms_( nullptr ), postAction_( nullptr ){}
		TaskManager::Synchro			sync_;
		DBMS *							dbms_;
		std::function < bool ( const nquery_res & result ) > postAction_;
	};

public :
	class Transaction;

	DBMS(void);
	~DBMS(void);

	virtual bool Initialize( Config & config, ServiceFactory * services );
	virtual void Uninitialize( void );
	virtual void LogStat( void );
	virtual bool Run( void );

	//template < typename QueryType, typename RetType >
	//TaskManager::Synchro Execute( UserContextType userContext, QueryType query, RetType & value );

	template < typename QueryType, typename RetType >
	TaskManager::Synchro Execute( ContextType context, int64 key, QueryType query, RetType & value );

	template < typename QueryType, typename RetType >
	bool Execute( Transaction & tx, ContextType context, int64 key, QueryType query, RetType & value );

private :
	std::string		ipAddr_;
	unsigned short	tcpPort_;

	ServiceFactory * services_;
	static void QueryCompletionCallBack( nquery_res result, void *arg );
};

class DBMS::Transaction
{
	enum { ROLLBACK = 0, COMMIT };
	enum { NONE, OnBegin, OnEnd };
	friend class DBMS;

public:
	enum Result{ Commited, Rollbacked };

	Transaction( void );
	virtual ~Transaction( void );

	void	Begin	( int64 key );
	Result	End		( void );

private:
	nbase_tx								tx_;
	int										state_;
	std::vector < TaskManager::Synchro >	syncs_;
};

template < typename QueryType, typename RetType >
TaskManager::Synchro DBMS::Execute( ContextType context, int64 key, QueryType query, RetType & value )
{
	RoomService	* roomService = services_->GetRoomService();

	TaskManager::Synchro sync( context );
	CompletionCallbackPram * callbackParam = new CompletionCallbackPram();
	callbackParam->dbms_ = this;
	callbackParam->sync_ = sync;
	callbackParam->postAction_ = [ &query, &value ] ( nquery_res result )
	{
		query.PostExecute( result );
		return query.Output( value );
	};

	roomService->tm_.Async( context, sync, [ this, key, &query, callbackParam ]()
	{
		return query.Execute( NULL, ipAddr_, tcpPort_, key, &QueryCompletionCallBack, callbackParam );
	});

	return std::move( sync );
}

template < typename QueryType, typename RetType >
bool DBMS::Execute( Transaction & tx, ContextType context, int64 key, QueryType query, RetType & value )
{
	if( OnBegin != tx.state_ )
	{
		LOG_ERROR( "Invlaid transaction state (" << tx.state_ << ")" );
		return false;
	}

	RoomService	* roomService = services_->GetRoomService();

	CompletionCallbackPram * callbackParam = new CompletionCallbackPram( context );
	callbackParam->dbms_ = this;
	callbackParam->postAction_ = [ &query, &value ] ( nquery_res result )
	{
		query.PostExecute( result );
		return query.Output( value );
	};

	roomService->tm_.Async( context, callbackParam->sync_, [ this, &tx, key, &query, callbackParam ]()
	{
		return query.Execute( &tx.tx_, ipAddr_, tcpPort_, key, &QueryCompletionCallBack, callbackParam );
	});

	tx.syncs_.push_back( std::move( callbackParam->sync_ ) );

	return true;
}