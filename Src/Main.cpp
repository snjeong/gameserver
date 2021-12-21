// RoomServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


#include "XBase/XSystem.h"
#include "ServiceManager.h"

XSystem::Threading::AutoResetEvent theQuitEvent;

BOOL __stdcall CtrlHandler(DWORD dwType)
{
    switch (dwType)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    //case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        theQuitEvent.Set();
        return TRUE;
    }

    return FALSE;
}

int _tmain(int argc, _TCHAR* argv[])
{
	if( false == Log4MonSumm::Initialize() )
		return -100;

	LOG_ERROR( "\n\n" << "<< Start Server...... !! >>"  << "\n\n");

	if(FALSE == ::SetConsoleCtrlHandler(&CtrlHandler, TRUE))
	{
		LOG_ERROR( "Fail to SetConsoleCtrlHandler" );
		return -200;
	}

	ServiceManager manager;
	if(false == manager.Initialize())
	{
		LOG_ERROR("Fail to initialize manager.");
		return -300;
	}

	if(false == manager.Run())
	{
		LOG_ERROR("Fail to run manager.");
		return -400;
	}

	theQuitEvent.Wait();
	
	manager.Uninitialize();

	if( false == Log4MonSumm::Uninitialize() )
		return -500;

	return 0;
}

