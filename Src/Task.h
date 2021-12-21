#pragma once

#include <functional>
//class Task
//{
//	typedef std::function< void ( void ) > WorkFn;
//
//public:
//	Task(void);
//	~Task(void);
//
//	int		eventId_;
//	WorkFn	work_;
//
//	Task& operator=( const Task & rhs )
//	{
//		if ( this == &rhs )
//			return *this;
//
//		this->eventId_ = rhs.eventId_;
//		this->work_ = rhs.work_;
//
//		return *this;
//	}
//};

typedef std::function< void ( void ) > Task;