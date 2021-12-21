// RoomServerTester.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "RoomServer.h"

int _tmain(int argc, _TCHAR* argv[])
{
	RoomServer rs;

	if( false == rs.Initialize() )
		return -1;

	rs.Run();

	return 0;
}

