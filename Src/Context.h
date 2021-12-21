#pragma once

#include <windows.h>
#include <atomic>
#include <XBase/XSystem.h>

#include "Task.h"

// CAUTION: lock�� ���� ���¿��� Reyield�� ȣ����� �ʵ��� �Ѵ�.
//			�ٸ� fiber�� ������ �����尡 critical section�� ������ ���, lock�� ��ȿȭ �ȴ�.

class Context
{
	friend class TaskManager;
	enum YSTATE{ RESUMED, REYIELDED };

public:
	enum { OnInvalid = -1, OnTerminating = 0, OnProcessing = 1, ON_SUSPENDED = 2 };
	typedef void ( __stdcall * WorkType )( LPVOID arg );
	typedef void ( __stdcall * PostProcessType )( XSystem::XTL::SmartPtr < Context > context, void * userData );

	Context( WorkType work, LPVOID arg );
	virtual ~Context( void );

	void Resume( void );
	void Reyield( void );

	bool			AddTask	( Task task );
	bool			GetTask	( Task & task );

protected:
	virtual void Terminate( void ) = 0;

protected:
	int									processState_;
	PostProcessType						postProcess_;

	XSystem::Threading::CriticalSection	csTask_;
	std::atomic < int >					taskCount_;
	std::vector< Task >					tasks_;

private:
	HANDLE								yieldee_;
	LPVOID								yielder_;

	XSystem::Threading::CriticalSection csYieldState_;
	int									yieldState_;
};

typedef XSystem::XTL::SmartPtr < Context > ContextType;