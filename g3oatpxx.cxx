/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+*******************************************************************

	Module Name:		g3oatpxx.cxx (was atp_xx.cxx)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		ATP Interface Implementation

	History:
		Who			When				Description
	----------	-------------	----------------------------
	Alwyn Teh	94Q4 - 95Q2		Initial Creation

*******************************************************************-*/

#include <g3oatpxh.h>
#include <string.h>

int Atp2Tcl_cl::BeginParms(const char *name)
{
	curr_parm_name = (char*) name;
	char *cmdname = new char[strlen(name) + 5];
	sprintf(cmdname, "def_%s", name);

	pd = new Atp_ParmDefEntry[10];
	pdi = 0;
	no_of_parm_entries = 10;

	pd[pdi++].parmcode = ATP_BPM;

	return pdi;
}

int Atp2Tcl_cl::EndParms()
{
	pd[pdi++].parmcode = ATP_EPM;

	return pdi;
}

int Atp2Tcl_cl::NumParmDef(	const char *name, const char *desc,
							Atp_NumType min, Atp_NumType max,
							Atp_VprocType vproc )
{
	pd[pdi].parmcode = ATP_NUM;
	pd[pdi].parser = Atp_ProcessNumParm;
	pd[pdi].Name = (char *)name;
	pd[pdi].Desc = (char *)desc;
	pd[pdi].Min = min;
	pd[pdi].Max = max;
	pd[pdi++].vproc = vproc;

	return pdi;
}
char * Atp2Tcl_cl::CmdName()
{
	return cmdname;
}

char * Atp2Tcl_cl::ParmName()
{
	return curr_parm_name;
}

Atp_ParmDefEntry * Atp2Tcl_cl::ParmDef()
{
	return pd;
}
