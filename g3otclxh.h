/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+*******************************************************************

	Module Name:		g3otclxh.h (was tclxx.h)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Tcl Interface Class

	History:
		Who			When				Description
	----------	-------------	-----------------------------
	Alwyn Teh	94Q4 - 95Q2		Initial Creation
	Alwyn Teh	14 June 1995	Add VarEval()
	Alwyn Teh	3 July 1995		Use original SetResult(char	*,
								Tcl_FreeProc * ) instead.
								TCL_DYNAMIC changes type to int
								in Tcl v7.4 before casting to
								Tcl_FreeProc * - causes memory
								fault and core dumps.

*******************************************************************-*/

#ifndef _TCL_PLUSPLUS_INCLUDED_
#define _TCL_PLUSPLUS_INCLUDED_

#include <tcl.h>

class Tcl_cl
{
public:
		Tcl_cl() ;
		~Tcl_cl () ;

		char * Result();

		Tcl_Interp * GetInterp();

		void CreateCommand(	const char *cmdName,
							Tcl_CmdProc *proc,
							ClientData data,
							Tcl_CmdDeleteProc *deleteProc );

		void DeleteCommand( const char *cmdName );

		void SetResult( const char *msg );
		void SetResult( char *msg, Tcl_FreeProc * proc );
		// where proc = TCL_VOLATILE, TCL_STATIC or TCL_DYNAMIC

		int Eval( const char *cmd );
		int VarEval( const char *args, ... );

		char *GetResult();

protected:

private:
		Tcl_Interp *interp;

};

#endif /* _TCL_PLUSPLUS_INCLUDED_ */
