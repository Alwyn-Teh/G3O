/* EDITION AA02 (REL001), ITDACST.175 (95/07/13 13:00:00) -- OPEN */

/*+*******************************************************************

	Module Name:		g3otclvh.h

	Copyright:			BNR Europe	Limited,	1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Tcl User Variables
						For mapping G3O user variables of a received
						message into Tcl string variables. Used for
						integrated automation purposes.

	History:
		Who			When					Description
	----------	----------------	---------------------------
	Alwyn Teh	13-24 July 1995		Initial Creation

*******************************************************************-*/

#ifndef __TCL_USERVAR_HEADER_INCLUDED__
#define __TCL_USERVAR_HEADER_INCLUDED__

#include <rw/cstring.h>
#include <rw/regexp.h>
#include <rw/hashdict.h>
#include <rw/collect.h>
#include <rw/collstr.h>

#include <g3obitsh.h>
#include <g3ouvarh.h>

#include <tcl.h>

#if __TCL_VARIABLES_API__
extern "C" char *	Tcl_SetVar		( interp, varName, newValue, flags );
extern "C" char *	Tcl_SetVar2		( interp, name1, name2, newValue, flags );
extern "C" char *	Tcl_GetVar		( interp, varName, flags );
extern "C" char *	Tcl_GetVar2		( interp, name1, name2, flags);
extern "C" int		Tcl_UnsetVar	( interp, varName, flags);
extern "C" int		Tcl_UnsetVar2	( interp, name1, name2,	flags);
#endif

class user_tcl_variables : public RWCollectable
{
public:
		user_tcl_variables( Tcl_Interp *, const char *uv_name, user_variables & );
		~user_tcl_variables( void );

		char *	set_var( const char *varName, const char *newValue );
		char *	get_var( const char *varName );
		int		unset_var( const char *varName );
		int		var_exists( const char *varName );

		char *	inside_script( void );

		void clearAndDestroy( void );

private:
		Tcl_Interp *	interp;
		char *			tcl_rtn_str;
		RWCString		utv_name;
		void			OutputError( void );
};

#endif // __TCL_USERVAR_HEADER_INCLUDED__
