#pragma once

#include <XBase/XInNetwork.h>

#include "Config.h"
#include "MsAcceptor.h"
#include "ServiceFactory.h"
#include "Authenticator.h"
#include "DBMS.h"
#include "RoomService.h"
#include "MatchMakingService.h"

class ServiceManager : public ServiceFactory
{
public:
	ServiceManager(void);
	~ServiceManager(void);

	bool Initialize( void );
	void Uninitialize( void );
	bool Run( void );

	virtual Authenticator		* GetAuthenticator		( void ) { return authenticator_;	}
	virtual DBMS				* GetDBMS				( void ) { return dbms_;			}
	virtual RoomService			* GetRoomService		( void ) { return roomService_;		}
	virtual MatchMakingService	* GetMatchMakingService	( void ) { return mms_;				}

private:
	Config					config_;
	Authenticator		*	authenticator_;
	DBMS				*	dbms_;
	RoomService			*	roomService_;
	MatchMakingService	*	mms_;

	std::vector< Service * >	services_;	
};

