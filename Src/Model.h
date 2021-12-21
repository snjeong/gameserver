#pragma once

#include <vector>

#include "Json/json.h"
#include "Query.h"

namespace Model
{
	//////////////////////////////////////////////////////////////////////////////////////////
	// TABLE: item_equipped
	namespace item_equipped
	{
		struct table
		{
			table(): equipped_item_id(0), summonee_id(0), item_info_id(0), inven_item_id(0) {}
			~table() {}

			int64 equipped_item_id;
			int64 summonee_id;
			int64 item_info_id;
			int64 inven_item_id;
		};

		typedef table Row;
		typedef std::vector < Row > Rows;
		
		//////////////////////////////////////////////////////////////////////////////////////////

		struct Select : public Query
		{
			Select( __int64 item_id )
			{
				queryString_ = "SELECT equipped_item_id, summonee_id, item_info_id, inven_item_id, reg_date, mod_date FROM item_equipped where equipped_item_id =" + std::to_string( item_id );
			}
			bool Output( Row & row )
			{
				Json::Value j_rows; 

				if( !GetJRows( j_rows ) )
					return false; 

				if( 0 == j_rows.size() )
					return true;

				const Json::Value j_row = j_rows[0];

				if( false == GetValue( j_row, 0, row.equipped_item_id ) )	// equipped_item_id
					return false;

				if( false == GetValue( j_row, 1, row.summonee_id ) )		// summonee_id
					return false;

				if( false == GetValue( j_row, 2, row.item_info_id ) )		// item_info_id
					return false;

				if( false == GetValue( j_row, 3, row.inven_item_id ) )		// inven_item_id
					return false;

				return true; 
			}
		};
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// TABLE: match_log
	namespace match_log
	{
		struct table
		{
			table(): match_no(0), type(0), result(0), start_date( "" ), end_date( "" ) {}
			~table() {}

			int64			match_no;
			int				type;
			int				result;
			std::string		start_date;
			std::string		end_date;
		};

		typedef table Row;
		typedef std::vector < Row > Rows;
		
		//////////////////////////////////////////////////////////////////////////////////////////

		struct SelectAll : public Query
		{
			SelectAll()
			{
				queryString_ = "SELECT match_no, type, state, reg_date, mod_date, FROM match_log";
			}
			bool Output( Rows & rows )
			{
				Json::Value j_rows;
				if( false == GetJRows( j_rows ) )
				{
					LOG_ERROR("Fail to get rows. (" << resultString_ << ")" );
					return false;
				}

				for( unsigned int i = 0; i < j_rows.size(); i++ )
				{
					const Json::Value j_row = j_rows[i]; 

					Row row;

					if( false == GetValue( j_row, 0, row.match_no ) )	// match_no
						return false;

					if( false == GetValue( j_row, 1, row.type ) )		// type
						return false;

					if( false == GetValue( j_row, 2, row.result ) )		// state
						return false;

					if( false == GetValue( j_row, 3, row.start_date ) )	// reg_date
						return false;

					if( false == GetValue( j_row, 4, row.end_date ) )	// mod_date
						return false;

					rows.push_back( row );
				}

				return true; 
			}
		};

		//////////////////////////////////////////////////////////////////////////////////////////

		struct Insert : public Query
		{
			Insert( Row & row )
			{
				queryString_ = "Insert into match_log( match_no, type, result, start_date ) values ( ";
				queryString_+= std::to_string( row.match_no) + "," + std::to_string( row.type ) + "," + std::to_string( row.result ) + ", sysdatetime )";
			}
			bool Output( int64 & match_no )
			{
				//if( false == GetRID( match_no ) )
				//{
				//	LOG_ERROR("Fail to parse json. (" << resultString_ << ")" );
				//	return false; 
				//}

				return true; 
			}
		};

		//////////////////////////////////////////////////////////////////////////////////////////

		struct UpdateResult : public Query
		{
			UpdateResult( int64 match_no, int result )
			{
				queryString_ = "update match_log set result = " + std::to_string( result ) + ", mod_date = sysdatetime where match_no = " + std::to_string( match_no );
			}
			bool Output( void )
			{
				return true; 
			}
		};
	}
}