#pragma once

#include <XBase/XSystem.h>

class User
{
	friend class UserContext;

public:
	User(void);
	~User(void);

public:
	long long		no_;

private:
};

typedef XSystem::XTL::SmartPtr < User > UserType;