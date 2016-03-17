/*********************************************************************
 *
 * This is based on code created by Peter Harvey,
 * (pharvey@codebydesign.com).
 *
 * Modified and extended by Nick Gorham
 * (nick@easysoft.com).
 *
 * Any bugs or problems should be considered the fault of Nick and not
 * Peter.
 *
 * copyright (c) 1999 Nick Gorham
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 **********************************************************************
 *
 * $Id: SQLFetch.c,v 1.3 2003/10/30 18:20:45 lurcher Exp $
 *
 * $Log: SQLFetch.c,v $
 * Revision 1.3  2003/10/30 18:20:45  lurcher
 *
 * Fix broken thread protection
 * Remove SQLNumResultCols after execute, lease S4/S% to driver
 * Fix string overrun in SQLDriverConnect
 * Add initial support for Interix
 *
 * Revision 1.2  2002/12/05 17:44:30  lurcher
 *
 * Display unknown return values in return logging
 *
 * Revision 1.1.1.1  2001/10/17 16:40:05  lurcher
 *
 * First upload to SourceForge
 *
 * Revision 1.4  2001/04/12 17:43:36  nick
 *
 * Change logging and added autotest to odbctest
 *
 * Revision 1.3  2001/01/14 10:09:39  nick
 *
 * Add sqi/test to build
 *
 * Revision 1.2  2001/01/09 22:33:13  nick
 *
 * Stop passing NULL into SQLExtendedFetch
 * Further fixes to unicode to ansi conversions
 *
 * Revision 1.1.1.1  2000/09/04 16:42:52  nick
 * Imported Sources
 *
 * Revision 1.9  2000/02/11 00:41:46  ngorham
 *
 * Added a couple of fixes for drivers without SQLExtendedFetch
 *
 * Revision 1.8  2000/02/02 07:55:21  ngorham
 *
 * Add flag to disable SQLFetch -> SQLExtendedFetch mapping
 *
 * Revision 1.7  1999/11/13 23:40:59  ngorham
 *
 * Alter the way DM logging works
 * Upgrade the Postgres driver to 6.4.6
 *
 * Revision 1.6  1999/10/24 23:54:18  ngorham
 *
 * First part of the changes to the error reporting
 *
 * Revision 1.5  1999/09/21 22:34:24  ngorham
 *
 * Improve performance by removing unneeded logging calls when logging is
 * disabled
 *
 * Revision 1.4  1999/07/10 21:10:16  ngorham
 *
 * Adjust error sqlstate from driver manager, depending on requested
 * version (ODBC2/3)
 *
 * Revision 1.3  1999/07/04 21:05:07  ngorham
 *
 * Add LGPL Headers to code
 *
 * Revision 1.2  1999/06/30 23:56:54  ngorham
 *
 * Add initial thread safety code
 *
 * Revision 1.1.1.1  1999/05/29 13:41:06  sShandyb
 * first go at it
 *
 * Revision 1.2  1999/06/02 23:48:45  ngorham
 *
 * Added more 3-2 mapping
 *
 * Revision 1.1.1.1  1999/05/27 18:23:17  pharvey
 * Imported sources
 *
 * Revision 1.4  1999/05/03 19:50:43  nick
 * Another check point
 *
 * Revision 1.3  1999/04/30 16:22:47  nick
 * Another checkpoint
 *
 * Revision 1.2  1999/04/29 20:47:37  nick
 * Another checkpoint
 *
 * Revision 1.1  1999/04/25 23:06:11  nick
 * Initial revision
 *
 *
 **********************************************************************/

#include "drivermanager.h"

static char const rcsid[]= "$RCSfile: SQLFetch.c,v $ $Revision: 1.3 $";

SQLRETURN SQLFetch( SQLHSTMT statement_handle )
{
    DMHSTMT statement = (DMHSTMT) statement_handle;
    SQLRETURN ret;
    SQLCHAR s1[ 100 + LOG_MESSAGE_LEN ];

    /*
     * check statement
     */

    if ( !__validate_stmt( statement ))
    {
        dm_log_write( __FILE__, 
                    __LINE__, 
                    LOG_INFO, 
                    LOG_INFO, 
                    "Error: SQL_INVALID_HANDLE" );

        return SQL_INVALID_HANDLE;
    }

    function_entry( statement );

    if ( log_info.log_flag )
    {
        sprintf( statement -> msg, "\n\t\tEntry:\
            \n\t\t\tStatement = %p",
                statement );

        dm_log_write( __FILE__, 
                __LINE__, 
                LOG_INFO, 
                LOG_INFO, 
                statement -> msg );
    }

    /*
     * check states
     */

    thread_protect( SQL_HANDLE_STMT, statement );

    if ( statement -> state == STATE_S1 ||
            statement -> state == STATE_S2 ||
            statement -> state == STATE_S3 )
    {
        dm_log_write( __FILE__, 
                __LINE__, 
                LOG_INFO, 
                LOG_INFO, 
                "Error: HY010" );

        __post_internal_error( &statement -> error,
                ERROR_HY010, NULL,
                statement -> connection -> environment -> requested_version );

        return function_return( SQL_HANDLE_STMT, statement, SQL_ERROR );
    }

    if ( statement -> state == STATE_S4 )
    {
        dm_log_write( __FILE__, 
                __LINE__, 
                LOG_INFO, 
                LOG_INFO, 
                "Error: 24000" );

        __post_internal_error( &statement -> error,
                ERROR_24000, NULL,
                statement -> connection -> environment -> requested_version );

        return function_return( SQL_HANDLE_STMT, statement, SQL_ERROR );
    }

    if ( statement -> state == STATE_S8 ||
            statement -> state == STATE_S9 ||
            statement -> state == STATE_S10 )
    {
        dm_log_write( __FILE__, 
                __LINE__, 
                LOG_INFO, 
                LOG_INFO, 
                "Error: HY010" );

        __post_internal_error( &statement -> error,
                ERROR_HY010, NULL,
                statement -> connection -> environment -> requested_version );

        return function_return( SQL_HANDLE_STMT, statement, SQL_ERROR );
    }

    if ( statement -> state == STATE_S11 ||
            statement -> state == STATE_S12 )
    {
        if ( statement -> interupted_func != SQL_API_SQLFETCH )
        {
            dm_log_write( __FILE__, 
                    __LINE__, 
                    LOG_INFO, 
                    LOG_INFO, 
                    "Error: HY010" );

            __post_internal_error( &statement -> error,
                    ERROR_HY010, NULL,
                    statement -> connection -> environment -> requested_version );

            return function_return( SQL_HANDLE_STMT, statement, SQL_ERROR );
        }
    }

    if ( CHECK_SQLFETCH( statement -> connection ))
    {
        /*
         * this is odd, but its in the book
         */

        if ( statement -> connection -> driver_act_ver == SQL_OV_ODBC2 &&
            CHECK_SQLEXTENDEDFETCH( statement -> connection ) &&
            statement -> connection -> ex_fetch_mapping )
        {
            /*
             * the row_st_arr can't be null
             */

            if ( statement -> row_st_arr )
            {
                ret = SQLEXTENDEDFETCH( statement -> connection,
                            statement -> driver_stmt,
                            SQL_FETCH_NEXT,
                            NULL,
                            statement -> row_ct_ptr,
                            statement -> row_st_arr );
            }
            else
            {
                SQLUSMALLINT    *row_st_arr;
                int             row_count;
                SQLUSMALLINT    row_status;

                if ( statement -> row_array_size <= 1 )
                {
                    row_count = 1;
                    row_st_arr = &row_status; 
                }
                else
                {
                    row_count = statement -> row_array_size;
                    row_st_arr = malloc( sizeof( SQLUSMALLINT ) * row_count );
                }

                ret = SQLEXTENDEDFETCH( statement -> connection,
                            statement -> driver_stmt,
                            SQL_FETCH_NEXT,
                            NULL,
                            statement -> row_ct_ptr,
                            row_st_arr );

                if ( row_count > 1 )
                {
                    free( row_st_arr );
                }
            }
        }
        else
        {
            ret = SQLFETCH( statement -> connection,
                    statement -> driver_stmt );

	    if( statement -> connection -> driver_act_ver == SQL_OV_ODBC2 &&
		statement -> row_ct_ptr ) {
		if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO ) {
		    *statement -> row_ct_ptr = 1;
		} else {
		    *statement -> row_ct_ptr = 0;
		}
	    }
        }
    }
    else
    {
        dm_log_write( __FILE__, 
                __LINE__, 
                LOG_INFO, 
                LOG_INFO, 
                "Error: IM001" );

        __post_internal_error( &statement -> error,
                ERROR_IM001, NULL,
                statement -> connection -> environment -> requested_version );

        return function_return( SQL_HANDLE_STMT, statement, SQL_ERROR );
    }

    if ( ret == SQL_STILL_EXECUTING )
    {
        statement -> interupted_func = SQL_API_SQLFETCH;
        if ( statement -> state != STATE_S11 &&
                statement -> state != STATE_S12 )
            statement -> state = STATE_S11;
    }
    else if ( SQL_SUCCEEDED( ret ))
    {
        statement -> state = STATE_S6;
    }

    if ( log_info.log_flag )
    {
        sprintf( statement -> msg, 
                "\n\t\tExit:[%s]",
                    __get_return_status( ret, s1 ));

        dm_log_write( __FILE__, 
                __LINE__, 
                LOG_INFO, 
                LOG_INFO, 
                statement -> msg );
    }

    return function_return( SQL_HANDLE_STMT, statement, ret );
}
