/* EDITION AA02 (REL001), ITD ACST.I75 (95/05/28 21:07:52) -- OPEN */

/*+*******************************************************************

	Module Name:		g3oslpxh.h (was slpxx.h)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		SLP Interface Class

	History:
		Who			When				Description
	----------	-------------	--------------------------
	Alwyn Teh	94Q4 - 95Q2		Initial Creation

********************************************************************-*/

#ifndef _SLP_PLUSPLUS_INCLUDED_
#define _SLP_PLUSPLUS_INCLUDED_

// #include <ipc.h>
#include <slp.h>

class Slp_cl
{
public:
		Slp_cl()	{ InitStdio(); }
		~Slp_cl()	{ gl_cleanup(); }

		// I/O stuff
		int	InitStdio()	{ return Slp_InitStdio(); }

#if 0
		typedef void (*ipc_inputcallback)
		(
			char *,	// client ptr
			int	*,	// file desc
			int	*	// input id
		) ;
#endif

		// StdinHandler is of type ipc_inputcallback.
		static void StdinHandler( char *, int * , int * ) { Slp_StdinHandler(); }

		void SetPrintfProc(int (*proc)(char *fmt, ...)) { Slp_Printf = proc; }

		// Associations with a Tcl interpreter
		char *	InitSlpInterp(Tcl_Interp *interp) // wrong, int
								{ return Slp_InitTclInterp(interp); }
		int	SetSlpInterp(Tcl_Interp *interp)
							{ return Slp_SetTclInterp(interp); }
		Tcl_Interp * GetSlpInterp()	{ return Slp_GetTclInterp(); }

		// Things to do with the prompt
		int	SetPrompt(const char *prompt)
						{ return Slp_SetPrompt((char*)prompt); }
		char * GetPrompt() { return Slp_GetPrompt() ; }
		void OutputPrompt() { Slp_OutputPrompt(); }

		// Cleanup routine (declaration in slp.h is wrong)
		void SetCleanupProc(int (*proc)()) { Slp_SetCleanupProc(proc); }

protected:
		// Getline interface
		void gl_init(int scrn_wdth)	{ Slp_gl_init(scrn_wdth); }
		void gl_cleanup()			{ Slp_gl_cleanup(); }
		void gl_redraw()			{ Slp_gl_redraw(); }
		void gl_replace()			{ Slp_gl_replace(); }
private:

};

#endif /* _SLP_PLUSPLUS_INCLUDED_ */
