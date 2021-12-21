#pragma once

#include < functional >
#include < atomic >
#include < thread >

#include <XBase/XSystem.h>

#define DEFAULT_WAIT_MILLISECOND		( 3600 * 1000 )

class Worker
{
public:
	Worker(void);
	~Worker(void);

	bool Initialize( std::function < bool ( void * arg, Worker * worker ) > fnWork, void* arg );
	void Uninitialize( void );
	bool Run( void );

	void SetAlarm( void );
	void GoSleep( int duration = DEFAULT_WAIT_MILLISECOND );
	void WakeUp( void );

private:
	std::thread								thread_;
	std::atomic < bool >					mustBeWorking_;

	XSystem::Threading::ManualResetEvent	event_;
	void *									manager_;

	std::function < bool ( void * arg, Worker * worker ) >	fnWork_;
};
