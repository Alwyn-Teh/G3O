/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+********************************************************************

	Module Name:		g3osighx.cxx

	Copyright:			BNR Europe Limited, 1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Signal Handler

	History:
		Who			When				Description
	----------	------------	-----------------------------------
	Alwyn Teh	28 May 1995		Initial	Creation
								Handle signals SIGTERM, SIGHUP
								and SIGINT.
	Alwyn Teh	27 June 1995	Initialize and call cleanup_proc

*********************************************************************-*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#include <g3osighh.h>

Genie3_SignalHandler::Genie3_SignalHandler()
{
	signal(SIGTERM,	&Genie3_SignalHandler::Signal_Handler);
	signal(SIGHUP,	&Genie3_SignalHandler::Signal_Handler);
	signal(SIGINT,	&Genie3_SignalHandler::Signal_Handler);

	ioctl(0, TCGETA, &orig_tty);
	curr_tty = orig_tty;

	cleanup_proc = 0;
}

Genie3_SignalHandler::~Genie3_SignalHandler()
{
	signal(SIGTERM,	SIG_DFL);
	signal(SIGHUP,	SIG_DFL);
	signal(SIGINT,	SIG_DFL);

	ioctl(0, TCSETAF, &orig_tty);
	curr_tty = orig_tty;

	cleanup_proc = 0;
}

void Genie3_SignalHandler::Signal_Handler(int sigval)
{
	int c = 0;
	char ch = 0;
	char *notice = 0;
	char *question = 0;

	ioctl(0, TCGETA,  &curr_tty);
	ioctl(0, TCSETAW, &orig_tty);

	switch(sigval)
	{
		case SIGTERM:
			notice = "Software termination - ";
			break;
		case SIGHUP:
			notice = "Terminal hung"; // e.g. window is gone, can't input
			break;
		case SIGINT:
			notice = "Interrupt - ";
			break;
		default:
			break;
	}

	printf("\n") ;
	fflush(stdout);

	signal(sigval, SIG_IGN);

	question = "Quit program (q) or Continue (c) : ";

	if ( sigval == SIGHUP )
	{
	  printf("%s\n", notice);
	}
	else
	do {
		c = 0 ;
		ch = 0;
		errno = 0;

		printf("%s%s", notice, question);
		fflush(stdout);

		c = read(0, &ch, 1) ;
		ch = tolower(ch);

		if (c == 1 && ch != 'q' && ch != 'c')
		{
		  printf ("\r") ;
		  fflush(stdout);
		}
	} while ((c == 1  && ch != 'q' && ch != 'c') ||
			 (c == -1 && errno == EINTR));

	printf("\n");
	fflush(stdout);

	errno = 0; // reset
	signal(sigval, SIG_DFL);

	if ( ch == 'q' || ch == '\0' || ch != 'c' )
	{
	  if ( cleanup_proc != 0 )
	  {
		int rc = 0;
		printf("\ncleaning up...\n") ;
		rc = cleanup_proc();
		printf("cleanup procedure return code = %d\n", rc);
	  }

	  printf("\nProgram aborting - exit code %d\n", sigval);

#ifdef DEBUG
	  // throw() in debug mode doesn't work properly here somehow
	  // an abort() is called and core dumps
	  exit(sigval);
#else
	  throw QuitSignal(sigval);
#endif
	}
	else
	  signal(sigval, Signal_Handler);

	ioctl(0, TCSETAF, &curr_tty);

	printf("\rPress RETURN                                                               \r");
	fflush(stdout);

	raise(SIGWINCH); // kludge to get SLP to redraw input line if any
}
