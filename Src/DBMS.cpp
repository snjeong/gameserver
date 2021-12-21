#include "stdafx.h"

#include "DBMS.h"
#include "Utils.h"
#include "RoomService.h"

DBMS::Transaction::Transaction( void )
	: state_ ( NONE )
{
}

DBMS::Transaction::~Transaction( void )
{
	if( OnBegin == state_ )
		End();
}

void DBMS::Transaction::Begin( int64 key )
{
	int ret = 0;
	nbase_ipstr cs_addr;
	unsigned short cs_port;
	char ex_ip_list[1024];
	memset(ex_ip_list, 0x00, sizeof(ex_ip_list));

	std::string ckey = "";
	if( Query::COMMON_KEY != key )
		ckey = std::to_string( key );

	// nBase server와의 연동 작업이 아니길.....
	// pick up an address of live container server.
	ret = nbase_get_rand_cs(ckey.c_str(), cs_addr, &cs_port, ex_ip_list);
	if (ret < 0) {
		LOG_ERROR( "Fail to get cs address, ret:" << ret );
		state_ = NONE;
	}

	ret = nbase_begin_tx(cs_addr, cs_port, ckey.c_str(), 10000, & tx_);
	if (ret < 0)
	{
		LOG_ERROR( "fail to nbase_begin_tx, ret:" << ret );
		state_ = NONE;
	}
}

DBMS::Transaction::Result DBMS::Transaction::End( void )
{
	for( auto it = syncs_.begin(); syncs_.end() != it; it++ )
	{
		if( TaskManager::Synchro::COMPLETE_SUCCESS != it->GetState() )
		{
			nbase_end_tx( &tx_, ROLLBACK );
			syncs_.clear();
			state_ = OnEnd;
			return Rollbacked;
		}
	}

	syncs_.clear();

	nbase_end_tx( &tx_, COMMIT );
	state_ = OnEnd;
	return Commited;
}

DBMS::DBMS(void)
	: ipAddr_( "10.98.133.158" ), tcpPort_( 0 ), services_( NULL )
{
}

DBMS::~DBMS(void)
{
}

bool DBMS::Initialize( Config & config, ServiceFactory * services )
{
	services_ = services;

	nbase_param_t param;
	nbase_get_param( &param );
	param.syslog_ident = "nbase";
	param.log_path = "./Log/nBase";
	param.log_level = 8;
	
	int ret = nbase_init( &param );
	if( 0 > ret )
	{
		LOG_ERROR( "Fail to initialize nBase : " << ret);
		return false;
	}

	ret = nbase_register_cs_list( ipAddr_.c_str(), 6219 );
	if( ret < 0)
	{
		LOG_ERROR( "register cs list fail:" << ret);
		return false;
	}

	tcpPort_	= 6220;

	return true;
}

void DBMS::Uninitialize( void )
{
	nbase_unregister_cs_list();
	nbase_finalize();
}

void DBMS::LogStat( void )
{
}

bool DBMS::Run( void )
{
	return true;
}

void DBMS::QueryCompletionCallBack ( nquery_res result, void *arg )
{
	CompletionCallbackPram * param = reinterpret_cast < CompletionCallbackPram * > ( arg );
	ScopedInvoker scopedAction ( [ param ](){ if( NULL != param )	delete param; } );
	TaskManager & tm = param->dbms_->services_->GetRoomService()->tm_;

	tm.Complete( param->sync_, [ param, result ]()
	{
		return param->postAction_( result );
	});
}
