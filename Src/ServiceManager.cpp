#include "stdafx.h"

#include "ServiceManager.h"

ServiceManager::ServiceManager(void)
	: config_( "Config\\RoomServerConfig.xml" )
{
	// ������ ����Ͽ� ���� ���� ��������.
	authenticator_		= new Authenticator;
	dbms_				= new DBMS;
	roomService_		= new RoomService;
	mms_				= new MatchMakingService;

	// ������ �������� �Է�����.
	services_.push_back( authenticator_ );
	services_.push_back( dbms_ );
	services_.push_back( roomService_ );
	services_.push_back( mms_ );
}


ServiceManager::~ServiceManager(void)
{
}

bool ServiceManager::Initialize( void )
{
	if( false == config_.LoadFile() )
	{
		LOG_ERROR( "Fail to load config." );
		return false;
	}

	int serviceIdx = 0;
	for( auto it = services_.begin(); services_.end() != it; it++ )
	{
		if( false == (*it)->Initialize( config_, this ) )
		{
			LOG_ERROR( "Fail to initialize service. (" << serviceIdx << ")" );
			return false;
		}

		serviceIdx++;
	}

	return true;
}

void ServiceManager::Uninitialize( void )
{
	for( auto it = services_.rbegin(); services_.rend() != it; it++ )
	{
		(*it)->Uninitialize();
	}

	services_.clear();
}

bool ServiceManager::Run( void )
{
	int serviceIdx = 0;
	for( auto it = services_.begin(); services_.end() != it; it++ )
	{
		if( false == (*it)->Run() )
		{
			LOG_ERROR( "Fail to run service.(" << serviceIdx << ")" );
			return false;
		}

		serviceIdx++;
	}

	return true;
}