/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+*******************************************************************

	Module Name:		g3omain.cxx (was g3o_main.cxx)

	Copyright:			BNR Europe Limited, 1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Object-Oriented GENIE-III Main Program

	History:
		Who				When				Description
	-----------		--------------	--------------------------------
	Barry Scott		24 March 1995	Initial Creation
	Alwyn Teh		26 May 1995		Use of banner box.
									Include version information.
									Include copyright information.
									Handle interrupt/quit signal.
	Alwyn Teh		14 June 1995	Add -f<filename> option for
									script to source from.
	Alwyn Teh		20 June 1995	Add RogueWave copyright and
									version information.
	Alwyn Teh		27 June 1995	Add cleanup proc to signal
									handler to close GCM properly.
	Alwyn Teh		3 July 1995		Extern BNR_Copyright and
									CCITT_CCS7. Put banner contents
									in respective protocol objects.
	Alwyn Teh		12 Dec 1995		Beta release has been soaking in
									the field for almost 6 months,
									submit source to PLS ITD.

********************************************************************-*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include <g3obtuph.h>
#include <g3octuph.h>
#include <g3ouih.h>
#include <g3osighh.h>

#include <rw/tooldefs.h>
#include <rw/cstring.h>
#include <rw/regexp.h>

#include <gcm.h>

#include <slp.h>
#include <atph.h>
#include <atp2tclh.h>

using namespace std;

int __verbose = 0;
int __debug = 0;

// Global variables
char * G3O_tool_name	 = "GENIE III";
char * G3O_BNR_Copyxight = "(c)Copyright BNR Europe Ltd 1995";
char * G3O_CCITT_CCS7	 = "CCITT Common Channel Signalling System No.7";

// Local variables and functions
static int cleanup_proc( void );
static Genie3_SignalHandler signal_handler;

static char *script_filename = 0;

static char * rw_version_str( unsigned int version );
static int rwCopyrightVersionIndex = 8;
static char * RogueWave_Copyright[] =
{
"",
"This program uses the Rogue Wave C++ Class Library, Tools.h++:",
"",
"**************************************************************************",
"*                                                                        *",
"*                           R O G U E  W A V E                           *",
"*                 T O O L S . H + +  C L A S S  L I B R A R Y            *",
"*                                                                        *",
"*                      Rogue Wave Tools.h++: Version %s                  *",
"*                                                                        *",
"*                                                              July 1994 *",
"*                                                                        *",
"* Rogue Wave Software, Inc.                                              *",
"* P.0. Box 2328                                                          *",
"* Corvallis, OR 97339                                                    *",
"* U.S.A.                                                                 *",
"* TEL: 1-503-754-3010 (Sales)                                            *",
"*      1-503-754-2311 (Tech support)                                     *",
"* FAX: 1-503-757-6650                                                    *",
"* Email: support@roguewave.com                                           *",
"*                                                                        *",
"* (c) Copyright 1989, 1990, 1991, 1992, 1993, 1994                       *",
"*     Rogue Wave Software, Inc.                                          *",
"*     ALL RIGHTS RESERVED                                                *",
"*                                                                        *",
"* The software and information contained herein are proprietary to, and  *",
"* comprise valuable trade secrets of, Rogue Wave Software, Inc., which   *",
"* intends to preserve as trade secrets such software and information.    *",
"* This software is furnished pursuant to a written license agreement and *",
"* may be used, copied, transmitted, and stored only in accordance with   *",
"* the terms of such license and with the inclusion of the above          *",
"* copyright notice. This software and information or any other copies    *",
"* thereof may not be provided or otherwise made available to any other   *",
"* person.                                                                *",
"*                                                                        *",
"* Notwithstanding any other lease or license that may pertain to, or     *",
"* accompany the delivery of, this computer software and information, the *",
"* rights of the Government regarding its use, reproduction and           *",
"* disclosure are as set forth in Section 52.227-19 of the FARS Computer  *",
"* Software-Restricted Rights clause.                                     *",
"*                                                                        *",
"* Use, duplication, or disclosure by the Government is subject to        *",
"* restrictions as set forth in subparagraph (c)(1)(ii) of the Rights in  *",
"* Technical Data and Computer Software clause at DFARS 52.227-7013.      *",
"*                                                                        *",
"* This computer software and information is distributed with \"restricted *",
"* rights.\" Use, duplication or disclosure is subject to restrictions as *",
"* set forth in NASA FAR SUP 18-52.227-79 (April 1985) \"Commercial       *",
"* Computer Software-Restricted Rights (April 1985).\" If the Clause at   *",
"* 18-52.227-74 \"Rights in Data General\" is specified in the contract,  *",
"* then the \"Alternate III\" clause applies.                             *",
"*                                                                        *",
"**************************************************************************",
(char *)0
};

static char * G3O_Version[3];

int g3o_main(int argc, char **argv);

int main(int argc, char **argv)
{
	int return_code = 0;

	for ( int arg = 1; arg < argc; arg++ )
	{
	   if ( strcmp( "—verbose", argv[arg] ) == 0 )
		 __verbose++;
	   else
	   if ( strcmp( "--debug", argv[arg] ) == 0 )
		 __debug++;
	   else
	   if ( strncmp ( "-f", argv[arg], 2 ) = 0 )
	   {
		 if ( argv[arg][2] != 0 )
		   script_filename = &argv[arg][2];
		 else
		 if ( arg < argc )
		   script_filename = argv[++arg];

		 if ( script_filename == 0 )
		 {
		   cout << argv[0] << ": <filename> not found following -f option." << endl;
		   exit(EINVAL); // Invalid (null) argument
		 }
		 else
		 {
		   int rc = 0;
		   errno = 0;

		   // Check file exists and is readable,
		   rc = access(script_filename, F_OK);
		   if ( rc != 0 )
		   {
			 cout << argv[0] << Cannot use \"" << script_filename << "\" - " << strerror(errno) << "." << endl;
			 exit(errno);
		   }
		   else
		   {
			 errno = rc = 0;
			 rc = access(script_filename, R_OK);
			 if ( rc != 0 )
			 {
			   cout << argv[0] << ": Cannot read file \"" << script_filename << "\" - " << strerror(errno) << "." << endl;
			   exit(errno);
			 }
		   }

		   // Make sure file is not a directory!
		   struct stat stat_buf;
		   errno = rc = 0;
		   rc = stat(script_filename, &stat_buf);
		   if (rc == 0 && ((stat_buf.st_mode & S_IFMT) == S_IFDIR))
		   {
			 cout << argv[0] << ": Cannot use directory \"" << script_filename << "\" as command file!" << endl;
			 exit(EISDIR);
		   }

		   // if execution reaches here, script_filename is OK to use!
		 }

		 errno = 0;
	   }
	}

	try {
		return_code = g3o_main( argc, argv );
		return return_code;
	}
	catch( Genie3_InternalError ie )
	{
		cerr << endl;
		cerr << "Genie III Internal Error -- " << ie.message << endl;
		cerr << "Genie III client aborting." << endl;
	}
	catch( Genie3_SignalHandler::QuitSignal sig )
	{
		cerr << endl;
		cerr << "GENIE III program aborting." << endl;
		return_code = sig.code;
		return return_code;
	}
	catch (...)
	{
		cerr << endl;
		cerr << "Unknown exception caught by main." << endl;
		cerr << "Genie III client aborting." << endl;
	}

	return 0;
}

int g3o_main(int /* argc */, char **argv)
{
	enum tool { default_tool=1, btup, ctup, etsi_isup };

	enum tool tool = default_tool;

	G3O_Version[0] = NULL;
	G3O_Version[1] = NULL;
	G3O_Version[2] = NULL;

	{
		RWCString toolfilename( argv[0] );
		toolfilename.toLower();

		RWCRegexp re( "[^a-z]btup$" );
		if ( toolfilename.index( re ) != RW_NPOS )
		  tool = btup;

		re = "[^a-z]ctup$";
		if ( toolfilename.index( re ) != RW_NPOS )
		  tool = ctup;

		//	re = "[^a-z]etsi[^a-z]isup$" ;
		//	if ( toolfilename.index( re ) != RW_NPOS )
		//	  tool = etsi_isup;
	}

	Genie3_cli *ui =0;
	Protocol *protocol = 0;

	switch( tool )
	{
		default:
			cout << " Genie III generic client has not been installed correctly." << endl;
			cout << endl;
			cout << " Defaulting protocol..." << endl;
			cout << endl;
		case btup:
			ui = new Genie3_cli( protocol = new BTUP_Protocol(), "btup", "CCITT_#7", "BTUP" );
			break;
		case ctup:
			ui = new Genie3_cli( protocol = new CTUP_Protocol(), "ctup", "CCHT_#7", "CTUP" );
			break;
//		case etsi_isup:
//			ui = new Genie3_cli( protocol = new ETSI_ISUP_Protocol(), "etsi_isup", "CCITT_#7", "ETSI ISUP" );
//			break;
	}

	protocol->banner.display();

	cout << endl << flush;

	/* Add version information to help system. */
	G3O_Version[0] = getenv("G3O_VERSION");
	G3O_Version[1] = "12 DECEMBER 1995";

	if (G3O_Version[0] != NULL)
	  ui->Atp.AddHelpInfo(	ATP_HELP_AREA_SUMMARY,
			  	  	  	  	ATP_HELPCMD_OPTION_VERSION,
							G3O_Version );
#ifdef G3O_VERSION
	else {
	  G3O_Version[0] = G3O_VERSION;
	  ui->Atp.AddHelpInfo(	ATP_HELP_AREA_SUMMARY,
							ATP_HELPCMD_OPTION_VERSION,
							G3O_Version );
	}
#endif

	cout << endl << "VERSION: " << G3O_Version[0] << " (" << G3O_Version[1] << ")" << endl << endl;

	// Insert RogueWave version number in its copyright banner,
	char *rwVersionString = rw_version_str(rwToolsVersion());
	char *rwVersionInsStr = strstr(RogueWave_Copyright[rwCopyrightVersionIndex], "%s");
	if ( rwVersionInsStr != NULL )
	  strncpy( rwVersionInsStr, rwVersionString, strlen(rwVersionString) );
	  // Add version info for RogueWave
	  const char * RogueWave_VersionInfo[2];
	  RWCString rwV("Rogue Wave C++ Class Library Tools.h++ Version ");
	  rwV += rwVersionString;
	  RogueWave_VersionInfo[0] = rwV.data();
	  RogueWave_VersionInfo[1] = NULL;
	  ui->Atp.AddHelpInfo(ATP_HELP_AREA_SUMMARY, ATP_HELPCMD_OPTION_VERSION,
			  	  	  	  RogueWave_VersionInfo);

	  // Add version info for the SLP command line editor
	  ui->Atp.AddHelpInfo(ATP_HELP_AREA_SUMMARY, ATP_HELPCMD_OPTION_VERSION,
			  	  	  	  Slp_VersionInfo);

	  /* Add copyright information to help system. */
	  static char *copyright = "copyright";
	  Atp_CreateHelpArea(copyright, "Copyright information");

	  protocol->banner.append("");
	  protocol->banner.append("GENIE III Object-Oriented Application Framework");
	  protocol->banner.append("Authors: Alwyn Teh & Barry A. Scott (1995)");

	  //
	  // IF USE MODIFY THIS SOFTWARE, PLEASE SAY SO IN HERE; AND SUMMARIZE WHAT YOU'VE DONE.
	  //
	  // protocol->banner.append("Modified by: <first name> <last name> (<date>)");
	  //

	  /* Add copyright information to help system. */
	  const char ** Copyrights[5];
	  Copyrights[0] = protocol->banner.strs();
	  Copyrights[1] = RogueWave_Copyright;
	  Copyrights[2] = Atp_Copyright;
	  Copyrights[3] = Tcl_Copyright;
	  Copyrights[4] = Slp_Copyright;

	  for (int x = 0; x < 5; x++)
		 ui->Atp.AddHelpInfo(ATP_HELP_AREA_SUMMARY,
							 copyright,
							 Copyrights[x]);

	  ui->Atp.AddHelpInfo(ATP_HELP_AREA_SUMMARY,
						  ATP_HELPCMD_OPTION_MISC,
						  Slp_KeystrokesHelpText);

	  if ( script_filename != 0 )
	  {
		char *toolname = ( tool == btup ) ? "BTUP V3" : ( tool == ctup ) ? "CTUP" : ( tool == etsi_isup ) ? "ETSI ISUP" : "BTUP V3";

		cout << argv[0] << ": Executing GENIE III (" << toolname << ") command file \"" << script_filename << "\"..." << endl << endl;

		signal_handler.set_cleanup_proc( cleanup_proc );

		return ui->run( Genie3_cli::batch, script_filename );
	  }
	  else
	  {
		signal_handler.set_cleanup_proc( cleanup_proc );

		ui->run( Genie3_cli::interactive ); // never returns
	  }

	  return 0;
}

// Code to extract version number RWTOOLS returned by rwToolsVersion()
// and converted from hexadecimal to version string.

static char * rw_version_str( unsigned int version )
{
	// Usage: rw_version_str(rwToolsVersionO); can also use RWTOOLS macro to get version number

	static char vstr[80];
	char tmp[80];
	int x, y;

	sprintf(tmp, "%x", version);
	for (x = y = 0; tmp[x] != '\0'; x++, y++)
	{
	   vstr[y++] = tmp[x];
	   vstr[y]   = '.';
	}
	vstr[y-1] = '\0';
	return (char *)vstr;
}

static int cleanup_proc( void )
{
	Gcm_Result result;

	cout << "GCM client terminating..." << flush;

	result = Gcm_Close();

	cout << " ( " << result.string << " )" << endl;

	return result.rc;
}
