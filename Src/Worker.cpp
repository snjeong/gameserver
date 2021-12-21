#include "stdafx.h"

#include "Worker.h"

Worker::Worker(void)
	: mustBeWorking_( true ), manager_( NULL ) 
{
}

Worker::~Worker(void)
{
}

bool Worker::Initialize( std::function < bool ( void * arg, Worker * worker ) > fnWork, void* arg )
{
	fnWork_ = fnWork;
	manager_ = arg;

	return true;
}

void Worker::Uninitialize( void )
{
	mustBeWorking_.store( false );
	{
		event_.Set();
	}

	if( thread_.joinable() == true )
		thread_.join();

	manager_ = NULL;
}

void Worker::SetAlarm( void )
{
	event_.Reset();
}

void Worker::GoSleep( int duration )
{
	event_.Wait( duration );
}

void Worker::WakeUp( void )
{
	event_.Set();
}

bool Worker::Run( void )
{
	thread_ = std::thread( [ this ]()
	{
		ConvertThreadToFiber( (LPVOID)GetCurrentThreadId() );

		do
		{
			fnWork_( manager_, this );
		} while ( mustBeWorking_.load() == true );
	});

	return true;
}
