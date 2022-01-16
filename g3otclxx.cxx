/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+*******************************************************************

	Module Name:		g3otclxx.cxx (was tcl_xx.cxx)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Tcl Interface Implementation

	History:
		Who			When				Description
	----------	------------	----------------------------
	Alwyn Teh	94Q4 - 95Q2		Initial Creation
	Alwyn Teh	14 June 1995	Add VarEval()
	Alwyn Teh	22 June 1995	Remove tcl_free() - this was
								Barry's code. Core dumped when
								mixing Tcl 7.3 code with 7.4
								code, because TCL_DYNAMIC has
								been re-#defined from
								((Tcl_FreeProc *) free) to
								((Tcl_FreeProc *) 3)!

*******************************************************************-*/

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <tcl.h>
#include <g3otclxh.h>

Tcl_cl::Tcl_cl()
{
	interp = Tcl_CreateInterp();
}

Tcl_cl::~Tcl_cl()
{
	Tcl_DeleteInterp(interp);
}

char * Tcl_cl::Result()
{
	return interp->result;
}

Tcl_Interp * Tcl_cl::GetInterp()
{
	return interp;
}

void Tcl_cl::CreateCommand(	const char *cmdName,
							Tcl_CmdProc *proc,
							ClientData data,
							Tcl_CmdDeleteProc *deleteProc )
{
	Tcl_CreateCommand(interp, (char*) cmdName, proc, data, deleteProc);
}

void Tcl_cl::DeleteCommand (const char *cmdName)
{
	Tcl_DeleteCommand (interp, (char *)cmdName);
}

void Tcl_cl::SetResult( const char *msg )
{
	char *newmsg = strdup( msg );

	Tcl_SetResult( interp, newmsg, TCL_DYNAMIC );
}

void Tcl_cl::SetResult( char *msg, Tcl_FreeProc *freeProc )
{
	Tcl_SetResult( interp, msg, freeProc );
}

int Tcl_cl::Eval( const char *cmd )
{
	return Tcl_Eval( interp, (char *)cmd );
}

int Tcl_cl::VarEval( const char *args, ... )
{
	va_list ap;
	char *array[100];
	int argno = 0;

	va_start(ap, args);

	if ((array[argno++] = (char *)args) != 0)
	  while ((array[argno++] = va_arg( ap, char * )) != 0)
		   ;

	va_end(ap);

	char *command = Tcl_Concat ( argno-1, array );

	int rc = Eval( command );

	if ( command != 0 )
	  free( command );

	return rc;
}

char * Tcl_cl::GetResult(void)
{
	return interp->result;
}
