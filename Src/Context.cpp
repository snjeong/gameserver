#include "stdafx.h"
#include "Context.h"

Context::Context( WorkType work, LPVOID arg )
{
	processState_ = OnProcessing;
	yieldee_ = CreateFiber( 0, work, arg );
	yieldState_ = REYIELDED;

	postProcess_ = nullptr;
}

Context::~Context( void )
{
	DeleteFiber( yieldee_ );
}

void Context::Resume( void )
{
	if( OnTerminating == processState_ )
		return;

	{
		XSystem::Threading::ScopedLock < XSystem::Threading::CriticalSection > guard( csYieldState_ );
		if( RESUMED == yieldState_ )
			return;
		else
			yieldState_ = RESUMED;
	}

	yielder_ = GetCurrentFiber();
	SwitchToFiber( yieldee_ );
}

void Context::Reyield( void )
{
	if( OnTerminating == processState_ )
		return;

	{
		XSystem::Threading::ScopedLock < XSystem::Threading::CriticalSection > guard( csYieldState_ );
		if( REYIELDED == yieldState_ )
			return;
		else
			yieldState_ = REYIELDED;
	}

	SwitchToFiber( yielder_ );
}

bool Context::AddTask( Task task )
{
	XSystem::Threading::ScopedLock < XSystem::Threading::CriticalSection > guard( csTask_ );
	taskCount_++;

	tasks_.push_back( task );

	return true;
}

bool Context::GetTask( Task & task )
{
	XSystem::Threading::ScopedLock < XSystem::Threading::CriticalSection > guard( csTask_ );

	if( 0 == tasks_.size() )
		return false;

	auto pos = tasks_.begin();
	task = *pos;
	tasks_.erase( pos );

	return true;
}