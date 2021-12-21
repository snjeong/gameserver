#include "stdafx.h"

#include "GeneratedFiles/RoomServerInteralMessage.h"
#include "TaskManager.h"

#include <XBase/XPlatform.h>

TaskManager::TaskManager( Dispatcher & dispatcher )
	: dispatcher_( dispatcher ), numDefaultBesWorkBench_( 64 )
{
}

TaskManager::~TaskManager(void)
{
}

bool TaskManager::Initialize( ServiceFactory * services, int numWork )
{
	services_ = services;

	int numProcessor = XPlatform::GetProcessorCount();
	if( numWork < numProcessor * 2 )
		numWorker_ = numProcessor * 2;
	else
		numWorker_ = numWork;

	for( int idx = 0; numWorker_ > idx; idx++ )
	{
		Worker * worker = new Worker;
		if( nullptr == worker )
		{
			LOG_ERROR( "Fail to create worker (" << idx << ")" );
			return false;
		}

		if( false == worker->Initialize( &TaskManager::DoBenchwork, this ) )
		{
			LOG_ERROR( "Fail to initialize worker (" << idx << ")" );
			return false;
		}

		workerPool_.push_back( worker );
		standByWorkerPool_.push_back( worker );
	}

	timeOutTaskChecker_ = new Worker;
	if( nullptr == timeOutTaskChecker_ )
	{
		LOG_ERROR( "Fail to create time out checker." );
		return false;
	}

	if( false == timeOutTaskChecker_->Initialize( &TaskManager::CheckTimeOut, this ) )
	{
		LOG_ERROR( "Fail to initialize time out checker." );
		return false;
	}

	return true;
}

void TaskManager::UnInitialize( void )
{
	{
		XSystem::Threading::ScopedLock < XSystem::Threading::CriticalSection > guard( csBesWorkBenchs_ );
		besWorkBenchs_.clear();
	}

	{
		XSystem::Threading::ScopedLock < XSystem::Threading::CriticalSection > guard( csStandByTasks_ );
		standByWorkerPool_.clear();
	}

	for( auto it = workerPool_.begin() ; workerPool_.end() != it; it++ )
	{
		Worker * worker = *it;
		worker->Uninitialize();
		delete worker;
	}

	workerPool_.clear();

	timeOutTaskChecker_->Uninitialize();
	delete timeOutTaskChecker_;
	timeOutTaskChecker_ = NULL;
}

bool TaskManager::Run( void )
{
	for( auto it = workerPool_.begin() ; workerPool_.end() != it; it++ )
	{
		if( false == (*it)->Run() )
		{
			LOG_ERROR( "Fail to run worker." );
			return false;
		}
	}

	return true;
}

bool TaskManager::MakeToWork( ContextType context )
{
	XSystem::Threading::ScopedLock < XSystem::Threading::CriticalSection > guard( csStandByTasks_ );

	if( 0 >= context->taskCount_.load() )
		return false;

	standByTasks_.push_back( context );

	if( 0 < standByWorkerPool_.size() )
	{
		auto pos = standByWorkerPool_.begin();
		(*pos)->WakeUp();
		standByWorkerPool_.erase( pos );
	}

	return true;
}

bool TaskManager::AddTask( ContextType context, Task task )
{
	XSystem::Threading::ScopedLock < XSystem::Threading::CriticalSection > guard( csStandByTasks_ );

	bool mustBeStandBy = true;
	if( 0 < context->taskCount_.load() )
		mustBeStandBy = false;

	context->AddTask( task );

	if( false == mustBeStandBy )
		return false;

	standByTasks_.push_back( context );

	if( 0 < standByWorkerPool_.size() )
	{
		auto pos = standByWorkerPool_.begin();
		(*pos)->WakeUp();
		standByWorkerPool_.erase( pos );
	}

	return true;
}

struct WorkParam
{
	WorkParam() : context_( nullptr ), tm_( nullptr ) {}
	~WorkParam() {}

	ContextType context_;
	TaskManager *	tm_;
};

typedef XSystem::XTL::SmartPtr < WorkParam > WorkParamType;
	
void TaskManager::DoTask( LPVOID arg )
{
	WorkParamType * workParam = reinterpret_cast < WorkParamType * > ( arg );
	if( nullptr == workParam || nullptr == *workParam || nullptr == (*workParam)->context_ || nullptr == (*workParam)->tm_ )
	{
		LOG_ERROR( "Invlaid arg" );
		return;
	}

	ContextType		context		= (*workParam)->context_;
	TaskManager *	tm			= (*workParam)->tm_;
	
	*workParam = NULL;
	delete workParam;

	while( Context::OnProcessing == context->processState_ )
	{
		Task task;
		if( false == context->GetTask( task ) )
		{
			context->processState_ = Context::OnTerminating;
			context->Terminate();
			context->Reyield();
			return;
		}

		task();
		context->Reyield();
	}

	context->processState_ = Context::OnTerminating;
}

bool TaskManager::DoBenchwork( void * arg, Worker * worker )
{
	TaskManager * tm = reinterpret_cast < TaskManager * >( arg );

	ContextType context;
	{
		tm->csStandByTasks_.Enter();
		auto pos = tm->standByTasks_.begin();
		if( tm->standByTasks_.end() == pos )
		{
			tm->standByWorkerPool_.push_back( worker );
			worker->SetAlarm();
			tm->csStandByTasks_.Leave();
			worker->GoSleep();
			return false;
		}

		context = *pos;
		tm->standByTasks_.erase( pos );
		tm->csStandByTasks_.Leave();
	}

	context->Resume();
	--context->taskCount_;

	if( nullptr != context->postProcess_ )
		context->postProcess_( context, tm );

	tm->MakeToWork( context );

	return true;
}

#define DEFAULT_SLEEP_TIME 60 * 60 * 1000
bool TaskManager::CheckTimeOut( void * arg, Worker * worker )
{
	TaskManager * tm = reinterpret_cast < TaskManager * >( arg );

	tm->csSuspenedTasks_.Enter();
	auto pos = tm->suspenedTasks_.begin();
	if( tm->suspenedTasks_.end() == pos )
	{
		tm->timeOutTaskChecker_->SetAlarm();
		tm->csSuspenedTasks_.Leave();
		tm->timeOutTaskChecker_->GoSleep( DEFAULT_SLEEP_TIME );
		return false;
	}

	Synchro sync = *pos;
	tm->suspenedTasks_.erase( pos );
	tm->csSuspenedTasks_.Leave();

	if( Synchro::SUSPENDED != sync.entity_->state_ )
		return true;

	sync.entity_->cs_.Enter();
	{
		__time64_t duration = _time64( NULL ) - sync.entity_->timeOutDate_;
		while( 0 < duration )
		{
			tm->timeOutTaskChecker_->SetAlarm();
			sync.entity_->cs_.Leave();
			tm->timeOutTaskChecker_->GoSleep( ( int ) duration );

			sync.entity_->cs_.Enter();
			if( Synchro::SUSPENDED != sync.entity_->state_ )
				return true;

			duration = _time64( NULL ) - sync.entity_->timeOutDate_;
		}

		if( Synchro::SUSPENDED == sync.entity_->state_ )
			sync.entity_->state_ = Synchro::TIME_OUT;
	}

	sync.entity_->cs_.Leave();
	sync.entity_->context_->Resume();

	return true;
}

bool TaskManager::MakeToSuspend( Synchro sync )
{
	XSystem::Threading::ScopedLock < XSystem::Threading::CriticalSection > guard( sync.entity_->cs_ );
	sync.entity_->timeOutDate_ = _time64( NULL ) + ASYNC_TASK_TIME_OUT;
	suspenedTasks_.push_back( sync );
	if( 1 == suspenedTasks_.size() )
		timeOutTaskChecker_->WakeUp();

	return true;
}

TaskManager::Synchro TaskManager::Async( ContextType context, std::function < bool ( void ) > fn )
{
	bool retValue = fn();

	Synchro sync( context );
	
	if( false == retValue )
	{
		XSystem::Threading::ScopedLock < XSystem::Threading::CriticalSection > guard( sync.entity_->cs_ );
		sync.entity_->state_ = Synchro::COMPLETE_FAILURE;
		return std::move( sync );
	}

	MakeToSuspend( sync );

	return std::move( sync );
}

void TaskManager::Async( ContextType context, Synchro & sync, std::function < bool ( void ) > fn )
{
	bool retValue = fn();

	if( false == retValue )
	{
		XSystem::Threading::ScopedLock < XSystem::Threading::CriticalSection > guard( sync.entity_->cs_ );
		sync.entity_->state_ = Synchro::COMPLETE_FAILURE;
		sync.entity_->context_ = context;
		return;
	}

	MakeToSuspend( sync );
}

bool TaskManager::Complete( Synchro sync, std::function < bool ( void ) > fn )
{
	{
		XSystem::Threading::ScopedLock < XSystem::Threading::CriticalSection > guard( sync.entity_->cs_ );

		if( false == sync.entity_->isWaitting_ )
		{
			sync.entity_->state_ = Synchro::COMPLETE_MEANINGLESS;
			return true;
		}

		bool ret = fn();
		if( true == ret )
		{
			sync.entity_->state_ = TaskManager::Synchro::COMPLETE_SUCCESS;
		}
		else
		{
			LOG_ERROR( "Fail to process query output." );
			sync.entity_->state_ = TaskManager::Synchro::COMPLETE_FAILURE;
		}
	}

	MakeToWork( sync.entity_->context_ );
	return true;
}

//////////////////////////////////////////////////////////////////
// 사용자 메시지 처리

void __stdcall TaskManager::OnUserLinkDestroy( XInNetwork::Link::Handle link, void * userData )
{
	ContextType * context = ( ContextType * ) XInNetwork::Link::GetData( link );
	XInNetwork::Link::SetData( link, NULL );
	delete context;
}

void __stdcall TaskManager::OnUserAccepted( XInNetwork::Link::Handle link, void * userData )
{
	TaskManager * tm = reinterpret_cast < TaskManager * > ( userData );

	WorkParamType * warkParam = new WorkParamType;
	*warkParam = new WorkParam;

	ContextType * context = new ContextType;
	*context = new UserContext( link, &TaskManager::DoTask, warkParam );

	(*warkParam)->context_ = *context;
	(*warkParam)->tm_ = tm;
}

void __stdcall TaskManager::OnUserReceived( XInNetwork::Link::Handle link, AutoDestroyStreamPtr msgStream, void * userData )
{
	ScopedInvoker scopedAction_Close( [ link ] ()
	{
		XInNetwork::Link::Close( link );
	});

	TaskManager * tm = reinterpret_cast < TaskManager * > ( userData );

	int messageId = 0;
	if( false == XStream::Peek( msgStream->Get(), & messageId, sizeof( messageId ) ) )
	{
		LOG_ERROR( "Fail to peek message ID." );
		return;
	}
	messageId = NetworkToHost( messageId );

	ContextType	*	contextPtr	= ( ContextType * ) XInNetwork::Link::GetData( link );
	ContextType		context		= *contextPtr;

	tm->AddTask( context, [ tm, context, messageId, msgStream ]()
	{
		tm->dispatcher_.DoProcess( context, messageId, msgStream->Get() );
	});
	XInNetwork::Link::Recv( link, 2048 );

	scopedAction_Close.Cancel();
}

void __stdcall TaskManager::OnUserClosed( XInNetwork::Link::Handle link, void * userData )
{
	TaskManager * tm = reinterpret_cast < TaskManager * > ( userData );

	ContextType * contextPtr = ( ContextType * )XInNetwork::Link::GetData( link );
	ContextType context = ( nullptr != contextPtr ) ? *contextPtr : nullptr;
	if( nullptr == context )
	{
		LOG_ERROR( "The User Context is null." );
		return;
	}

	int messageId = __messageid( InternalMessage_UserOnClose );
	tm->AddTask( context, [ tm, context, messageId ]()
	{
		tm->dispatcher_.DoProcess( context, messageId, nullptr );
	});
}

//////////////////////////////////////////////////////////////////
// 백엔드 서비스 메시지 처리

#define MAX_QUEUED_BES_TASK 5

void __stdcall TaskManager::DoPostProcessForBesMsg( ContextType context, void * userData )
{
	TaskManager * tm = reinterpret_cast < TaskManager * >( userData );

	if( MAX_QUEUED_BES_TASK >= context->taskCount_.load() )
	{
		XSystem::Threading::ScopedLock < XSystem::Threading::CriticalSection > guard( tm->csBesWorkBenchs_ );
		tm->besWorkBenchs_.push_back( context );
	}
}

bool TaskManager::AddBesTask( MRS::Socket::Handle socket, MRS::Address::Handle source, MRS::Address::Handle dest, int messageId, AutoDestroyStreamPtr msgStream )
{
	ContextType context = nullptr;
	{
		XSystem::Threading::ScopedLock < XSystem::Threading::CriticalSection > guard( csBesWorkBenchs_ );
		if( 0 == besWorkBenchs_.size() )
		{
			WorkParamType * warkParam = new WorkParamType;
			*warkParam = new WorkParam;

			MRSSvcContext * mrsSvcContext = new MRSSvcContext( &TaskManager::DoTask, warkParam );
			mrsSvcContext->SetPeerAddr( source );
			context = mrsSvcContext;

			(*warkParam)->context_ = context;
			(*warkParam)->tm_ = this;
		}
		else
		{
			auto pos = besWorkBenchs_.begin();
			context = * pos;
			besWorkBenchs_.erase( pos );
		}
	}

	context->postProcess_ = &TaskManager::DoPostProcessForBesMsg;

	AddTask( context, [ this, context, messageId, msgStream ]()
	{
		dispatcher_.DoProcess( context, messageId, msgStream->Get() );
	});

	if( MAX_QUEUED_BES_TASK >= context->taskCount_.load() )
	{
		XSystem::Threading::ScopedLock < XSystem::Threading::CriticalSection > guard( csBesWorkBenchs_ );
		besWorkBenchs_.push_back( context );
	}

	return true;
}

void __stdcall TaskManager::OnBesReceive( MRS::Socket::Handle socket, MRS::Address::Handle source, MRS::Address::Handle dest, int messageId, AutoDestroyStreamPtr msgStream, void * userData )
{
	if( NULL == socket || NULL == source || NULL == dest || NULL == msgStream->Get() || NULL == userData )
	{
		LOG_ERROR( "Invalid argument. (" << socket << ")(" << source << ")(" << dest << ")(" << msgStream->Get() << ")(" << userData << ")" );
		return;
	}

	TaskManager * tm = reinterpret_cast < TaskManager * > ( userData );

	tm->AddBesTask( socket, source, dest, messageId, msgStream );
}

void __stdcall TaskManager::OnBesError(MRS::Socket::Handle socket, int errorCode, void* userData )
{
	LOG_ERROR( "Error code: " << errorCode );
}

void __stdcall TaskManager::OnBesDeliverFail(MRS::Socket::Handle socket, int errorCode, MRS::Address::Handle dest, void* packet, size_t packetSize, void* userData )
{
	LOG_ERROR( "Error code: " << errorCode );
}