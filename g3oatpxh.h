/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+*******************************************************************

	Module Name:		g3oatpxh.h (was atpxx.h)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		ATP Interface Class

	History:
		Who			When				Description
	----------	--------------	-----------------------------
	Alwyn Teh	94Q4 - 95Q2		Initial Creation
	Alwyn Teh	1 August 1995	Initialize interp for later
								use within the class.

********************************************************************-*/

#ifndef _ATP_PLUSPLUS_INCLUDED_
#define _ATP_PLUSPLUS_INCLUDED_

#include <atp2tclh.h>
#include <limits.h>

// Unset the optional flag in parmcode if set.
#define Atp_PARMCODE(parmcode) ((parmcode) & ~ATP_OPTPARM_MASK)

class Atp2Tcl_cl
{
public:
		Atp2Tcl_cl() { interp = 0; }
		~Atp2Tcl_cl() {}

		Atp_Result	InitAtp(Tcl_Interp *_interp)
								{ interp = _interp; return Atp2Tcl_Init(_interp); }

		Atp_Result	CreateCommand(	const char	*name,
									const char	*desc,
									int	help_id,
									Atp_Result	(*callback)(ClientData, Tcl_Interp *, int, char **),
									Atp_ParmDef	parmdef,
									ClientData	clientdata,
									Tcl_CmdDeleteProc *delProc )
		{
			return Atp2Tcl_CreateCommand(	interp,
											(char *)name, (char *)desc,
											help_id,
											(Tcl_CmdProc *)callback, parmdef,
											clientdata, delProc );
		}

		Atp_Result AddHelpInfo(	int	text_type,
								const char	*HelpName,
								const char	**DescText	)
		{
			return Atp2Tcl_AddHelpInfo(	interp, text_type,
										(char *)HelpName,
										(char **)DescText );
		}

		int BeginParms(const char *name);

		int EndParms();

		int NumParmDef(	const char *name, const char *desc = 0,
						Atp_NumType min = INT_MIN, Atp_NumType max = INT_MAX,
						Atp_VprocType vproc = 0 );

		char *CmdName();
		char * ParmName();
		Atp_ParmDefEntry * ParmDef();

private:
		Tcl_Interp *interp;
		char *curr_parm_name;
		char *cmdname;
		Atp_ParmDefEntry *pd;
		int pdi;
		int no_of_parm_entries;
};

#endif /* _ATP_PLUSPLUS_INCLUDED_ */

