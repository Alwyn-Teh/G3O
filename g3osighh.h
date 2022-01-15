/* EDITION AA02 (REL001), ITD ACST.175 {95/05/28 21:07:52) -- OPEN */

/*+*****************»*************************************************

	Module Name:		g3osighh.h

	Copyright:			BNR Europe Limited, 1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Signal Handler Header

	History:
		Who			When				Description
	----------	--------------	-----------------------------
	Alwyn Teh	28 May 1995		Initial Creation
	Alwyn Teh	27 June 1995	Add set_cleanup_proc() so
								that client can call
								Gcm_Close() when quiting.

*******************************************************************-*/

#ifndef _G3O_SIGNAL_HANDLER_H
#define _G3O_SIGNAL_HANDLER_H

#include <sys/ioctl.h>
#include <termio.h>
#include <unistd.h>
#include <signal.h>

class Genie3_SignalHandler
{
public:
		Genie3_SignalHandler();
		~Genie3_SignalHandler();

		class QuitSignal
		{
		public:
				QuitSignal(int i) { code = i; }
				~QuitSignal(void) {}
				int code;
		};

		void set_cleanup_proc( int (*proc)( void ) ) { cleanup_proc = proc; }

private:
		static struct termio orig_tty, curr_tty;
		static void Signal_Handler(int sigval);
		static int (*cleanup_proc)( void );
};

#endif // _G3O_SIGNAL_HANDLER_H
