#pragma once

#include <atomic>
#include <memory>
#include <XBase/XInNetwork.h>

#include "Utils.h"
#include "MsLink.h"
#include "Dispatcher.h"
#include "UserContext.h"
#include "ServiceFactory.h"
#include "Worker.h"

#define ASYNC_TASK_TIME_OUT 10 * 1000

class TaskManager
{
public:
	class Synchro;

	TaskManager( Dispatcher & dispatcher );
	~TaskManager(void);

	bool Initialize( ServiceFactory * services, int numWork );
	void UnInitialize( void );
	bool Run( void );

	bool					AddTask( ContextType context, Task task );
	static void __stdcall	DoTask( LPVOID arg );

	//////////////////////////////////////////////////////////////////
	// 비동기 작업 처리
	Synchro					Async( ContextType context, std::function < bool ( void ) > fn );
	void					Async( ContextType context, Synchro & sync, std::function < bool ( void ) > fn );
	bool					Complete( Synchro sync, std::function < bool ( void ) > fn );

	//////////////////////////////////////////////////////////////////
	// 사용자 메시지 처리
	static void	 __stdcall	OnUserLinkDestroy	( XInNetwork::Link::Handle link, void * userData );
	static void	 __stdcall	OnUserAccepted		( XInNetwork::Link::Handle link, void * userData );
	static void	 __stdcall	OnUserReceived		( XInNetwork::Link::Handle link, AutoDestroyStreamPtr msgStream, void * userData );
	static void	 __stdcall	OnUserClosed		( XInNetwork::Link::Handle link, void * userData );

	//////////////////////////////////////////////////////////////////
	// 백엔드 서비스 메시지 처리
	bool					AddBesTask( MRS::Socket::Handle socket, MRS::Address::Handle source, MRS::Address::Handle dest, int messageId, AutoDestroyStreamPtr msgStream );

	static void __stdcall	DoPostProcessForBesMsg( ContextType context, void * userData );
	static void __stdcall	OnBesReceive( MRS::Socket::Handle socket, MRS::Address::Handle source, MRS::Address::Handle dest, int messageId, AutoDestroyStreamPtr msgStream, void * userData );
	static void __stdcall	OnBesError( MRS::Socket::Handle socket, int errorCode, void* userData );
	static void __stdcall	OnBesDeliverFail( MRS::Socket::Handle socket, int errorCode, MRS::Address::Handle dest, void* packet, size_t packetSize, void* userData );

private:
	bool MakeToWork( ContextType context );
	bool MakeToSuspend( Synchro sync );

	static bool __stdcall	DoBenchwork( void * arg, Worker * worker );
	static bool __stdcall	CheckTimeOut( void * arg, Worker * worker );

private:
	ServiceFactory		*				services_;
	Dispatcher			&				dispatcher_;

	int									numWorker_;
	Worker				*				timeOutTaskChecker_;

	XSystem::Threading::CriticalSection	csStandByTasks_;
	std::vector < ContextType >			standByTasks_;
	std::vector < Worker * >			workerPool_;
	std::vector < Worker * >			standByWorkerPool_;

	XSystem::Threading::CriticalSection	csSuspenedTasks_;
	std::vector < Synchro >				suspenedTasks_;

	int									numDefaultBesWorkBench_;
	XSystem::Threading::CriticalSection	csBesWorkBenchs_;
	std::vector < ContextType >			besWorkBenchs_;
};

class TaskManager::Synchro
{
	friend class TaskManager;

	struct Entity
	{
		Entity( ContextType context ) : state_( SUSPENDED ), context_( context ), timeOutDate_( 0 ), isWaitting_( true ) {}
		~Entity(){}

		XSystem::Threading::CriticalSection	cs_;
		__time64_t							timeOutDate_;
		int									state_;							// -1 : time out, 0: suspended, 1: successfully complete, 2: failure complete
		ContextType							context_;
		bool								isWaitting_;
	};

public:
	enum { INVALID = -2, TIME_OUT = -1, SUSPENDED = 0, COMPLETE_SUCCESS, COMPLETE_FAILURE, COMPLETE_MEANINGLESS };

	Synchro( void ) : entity_( NULL ), isWaiter_( false )
	{
	}

	Synchro( ContextType context ) : entity_( NULL ), isWaiter_( true )
	{
		entity_ = std::make_shared < Entity >( context );
	}

	Synchro( const Synchro & other ) : entity_( other.entity_), isWaiter_( false )
	{
	}

	Synchro( Synchro && other ) : entity_( other.entity_), isWaiter_( other.isWaiter_ )
	{
		other.entity_ = NULL;
		other.isWaiter_ = false;
	}

	~Synchro( void )
	{
		if( nullptr == entity_ || false == isWaiter_ )
			return;

		XSystem::Threading::ScopedLock < XSystem::Threading::CriticalSection > guard( entity_->cs_ );
		entity_->isWaitting_ = false;
	}

	Synchro & operator=( const Synchro & other )
	{
		if( this == &other )
			return *this;

		this->entity_ = other.entity_;
		this->isWaiter_ = false;

		return *this;
	}

	int GetState( void )
	{
		if( nullptr == entity_ )
		{
			return INVALID;
		}

		entity_->cs_.Enter();
		while( SUSPENDED == entity_->state_ )
		{
			entity_->cs_.Leave();
			entity_->context_->Reyield();
			entity_->cs_.Enter();
		}

		entity_->cs_.Leave();

		return entity_->state_;
	}

private:
	std::shared_ptr	< Entity >				entity_;
	bool									isWaiter_;
};