/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+********************************************************************

	Module Name:		g3ouiini.cxx (was ui_init.cxx)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Genie3_cli Implementation

	History:
		Who			When				Description
	-----------	------------	------------------------------------
	Alwyn Teh	94Q4			Initial Creation
	Barry Scott	95Q1			Exception handling
								Interface to GAL
	Alwyn Teh	28 May 1995		Signal handling
	Alwyn Teh	14 June 1995	Add batch processing to run().
	Alwyn Teh	21 June 1995	Add FieldValueMeanings
								when creating parmdefs
								where appropriate.
	Alwyn Teh	3rd July 1995	Implement "query protocol"
								and update "query info".
	Alwyn Teh	3rd July 1995	Intercept "query info" command
								because it's hard-wired to call
								Gal_ToolDescription().
	Alwyn Teh	4th July 1995	Implement "receive_msg"	command
								properly.
	Alwyn Teh	10th July 1995	Add "reset_msg_queue" command.
	Alwyn Teh	11th July 1995	Call OlCustomlnit() on protocol
								to set up protocol-specific
								things that are weird.
	Alwyn Teh	28 July 1995	Implement preview_msg command.
	Alwyn Teh	1 August 1995	Implement set_auto_cic and
								disp_header commands.
	Alwyn Teh	16 August 1995	Add status command.
	Alwyn Teh	23 August 1995	Package Slp.StdinHandler()
								in G3o_StdinHandler() with
								Gal_LogTestFile().

*******************************************************************-*/

#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream.h>
#include <strstream.h>
#include <ctype.h>
#include <unistd.h>
#include <assert.h>

#include <atph.h>

#include <g3ouih.h>
#include <g3ouicmh.h>
#include <g3ouvarh.h>
#include <g3obitsh.h>
#include <g3opmdfh.h>
#include <g3odaryh.h>

#include <tbf.h>
#include <gal.h>
#include <ipc.h>

using namespace std;

#if _DEBUG_LOG_FILE
static int parmtype_count [13] [8] = {0};
static const char *logfilename = 0;
#endif

#if _DEBUG_LOG_FILE || _DEBUG_UI_PROT_UI
static char * show_field_type(enum Protocol::field_type FieldType);
static char * show_field_format(enum Protocol::field_format FieldFormat);
#endif

static Atp_KeywordType * ConvertToAtpKeywordTable(const Protocol::Table * fvm, const char *toolname = 0, int table_size = 256);

//
//
// ui exceptions
//
//

Genie3_Exception::Genie3_Exception(void)
{
	code = 0;
}

Genie3_InternalError::Genie3_InternalError( const char *m ) : message( m )
{ }

Genie3_InternalError::Genie3_InternalError( const char *m, int a )
{
	char buf[200];
	sprintf ( buf, m, a );
	message = buf;
}

Genie3_InternalError::Genie3_InternalError( const char *m, const char *a, const char *b, const char *c )
{
	char buf [200] ;
	sprintf( buf, m, a, b, c );
	message = buf;
}

Genie3_SyntaxError::Genie3_SyntaxError( const char *m ) : message( m )
{ }

Genie3_SyntaxError::Genie3_SyntaxError( const char *m, int a )
{
	char buf [200] ;
	sprintf( buf, m, a );
	message = buf;
}

Genie3_SyntaxError::Genie3_SyntaxError( const char *m, const char *a )
{
	char buf [200];
	sprintf( buf, m, a );
	message = buf;
}

//
//
// Command - implementation
//
//
Command::Command( Genie3_cli *cli )
{
	ui = cli;
	pd = 0;
	Command_ID = G3O_COMMAND_ID;
}

void Command::Create(const char *cmdName, const char *cmdDesc, int help_id)
{
	ui->Atp.CreateCommand((char *) cmdName, (char *) cmdDesc, help_id, Atp2Tcl_cb,
			(pd) ? pd->arrayPtr() : 0, (ClientData) this, delete_command);
}

void Command::delete_command( ClientData cd )
{
	Command *it = (Command *)cd;

	if ( it->Command_ID == G3O_COMMAND_ID )
	  delete it;
}

Command::~Command(void)
{
	if ( pd != 0 )
	  delete pd;
}

Atp_Result Command::Atp2Tcl_cb(ClientData cd, Tcl_Interp *, int argc, char **argv)
{
	Command *cmd = (Command *)cd;

	return cmd->do_cmd( argc, argv );
}

Atp_Result Command::do_cmd(int argc, char **argv)
{
	try {
		return cmd( argc, argv );
	}
	catch( Genie3_InternalError ie )
	{
		cerr << "User Interface Internal Error -- ";
		cerr << ie.message << endl;
	}
	catch( Protocol::InternalError ie )
	{
		cerr << "Protocol Internal Error — ";
		cerr << ie.message << endl;
	}
	catch( user_variables::InternalError ie )
	{
		cerr << "User Variables Internal Error -- ";
		cerr << ie.message << endl;
	}
	catch( bit_string::FetchTooMuch )
	{
		cerr << "bit_string FetchTooMuch" << endl;
	}
	catch(...)
	{
		cerr << "Unhandled Exception raised executing command" << endl;
	}
	return ATP_ERROR;
}

//
//
// genie_cli implementation
//
//
Genie3_cli::Genie3_cli(Protocol *p, const char *toolname, const char *capability, const char *prompt)
{
	typedef void (*PFV)(void);
	typedef void (*PFV_C)(char *);
	typedef int (*PFI)(void);

	char *_toolname = ( toolname == 0 || *toolname == '\0' ) ? "G3O" : (char *) toolname;
	Genie3_cli::toolname = new char[strlen(_toolname)+1];
	strcpy(Genie3_cli::toolname, _toolname);

	curr_capability = ( capability == 0 || *capability == '\0' ) ? strdup("CCITT_#7") : strdup(capability);

	cli = this;

	rcvmsg_vars = 0;
	use_cic_filtering = 0;
	cic_bitwidth = 0;
	cic_uv_name = 0;
	use_auto_cic = 0;

	for (int links = 0; links < GAL_MAX_LINKS + 1; links++)
	   per_link_user_variables[ links ] = 0;

#if _DEBUG_LOG_FILE
	logfilename = "Genie3_cli";
#endif

	prompt_suffix = prompt;
	prompt_suffix += "> ";

	Init();
	init_protocol( p );

	Tcl_Interp *iptr = Slp.GetSlpInterp();

	// Intercept the "query" command because GAL didn't use
	// Gal_GetToolDescProc() for "query info" !!!!!
	// This has been fixed in GAL.AA23 but we'll keep this just in case.
	Tcl_CmdInfo queryInfo;
	char *queryCmd = "query";
	if (Tcl_GetCommandInfo(iptr, queryCmd, &queryInfo))
	{
	  OldQueryProc = queryInfo.proc;
	  queryInfo.proc = QueryProcWrapper;
	  Tcl_SetCommandInfo(iptr, queryCmd, &queryInfo);
	}

	// Wrap the "source" command in order to set receive mode to manual when inside a script.
	Tcl_CmdInfo SourceInfo;
	char *sourceCmd = "source";
	if (Tcl_GetCommandInfo(iptr, sourceCmd, &SourceInfo))
	{
	  receive_cmd_ptr->OldSourceProc = SourceInfo.proc;
	  SourceInfo.proc = receive_cmd_ptr->SourceProcWrapper;
	  SourceInfo.clientData = receive_cmd_ptr;
	  Tcl_SetCommandInfo (iptr, sourceCmd, &SourceInfo);
	}

	Gal_SetTextHandIer( (PFV) G3o_StdinHandler );
	Gal_SetMsgHandler( IncomingMessageHandler );
	Gal_SetQueryMsgProc( (PFV_C) QueryMessageHandler );
	Gal_SetProtocolDescProc( (PFI) ProtocolDescProc );
	Gal_SetToolDescProc( (PFI) ToolDescProc );
	Gal_SetMsgDecodeProc( (PFV) DecodeMessageHandler );
	Gal_SetMsgAction( (PFV) MessageActionHandler );
	Gal_SetMsgMnemonicToType( (PFV) MessageMnemonic );
	Gal_SetParmMnemonicToType( (PFV) ParameterMnemonic );

	Gal_SetToolName ( (char *) toolname );
	Gal_SetCapability( curr_capability );

	int num_entries = protocol->message_type_entries();

	parm_format_msg_info = new MessageInfo;
	incoming_msg_info = new MessageInfo[num_entries];
	outgoing_msg_info = new MessageInfo [num_entries] ;
	preview_msg_info = new MessageInfo [num_entries] ;

	int i;

	parm_format_msg_info[0].init( protocol, 1, GAL_DECODE_FIELDS );

	for ( i = 0; i < num_entries; i++ )
	   incoming_msg_info[i].init( protocol, 0, GAL_DECODE_NONE );

	for ( i = 0; i < num_entries; i++ )
	   outgoing_msg_info[i].init( protocol, 1, GAL_DECODE_NAME );

	for ( i = 0; i < num_entries; i++ )
	   preview_msg_info[i].init( protocol, 1, GAL_DECODE_FIELDS );
}

const RWCString Genie3_cli::default_uv_name("default") ;
const RWCString Genie3_cli::rcvmsg_uv_name("rcvmsg");

Genie3_cli::~Genie3_cli()
{
	if (send_msg_Cmd_ptr)		delete send_msg_Cmd_ptr ;
	if (send_hex_Cmd_ptr)		delete send_hex_Cmd_ptr;
	if (def_parm_hex_Cmd_ptr)	delete def_parm_hex_Cmd_ptr;
	if (disp_parm_Cmd_ptr)		delete disp_parm_Cmd_ptr;
	if (filter_cic_cmd_ptr)		delete filter_cic_cmd_ptr;
	if (set_auto_cic_cmd_ptr)	delete set_auto_cic_cmd_ptr;
	if (disp_header_cmd_ptr)	delete disp_header_cmd_ptr;
	if (status_cmd_ptr)			delete status_cmd_ptr;

	variables.clearAndDestroy();

	if ( rcvmsg_vars != 0 )
	  rcvmsg_vars->clearAndDestroy();
	rcvmsg_vars = 0;

	for (int links = 0; links < GAL_MAX_LINKS + 1; links++)
	{
	   if ( per_link_user_variables[ links ] != 0 )
		 delete per_link_user_variables[ links ];
	   per_link_user_variables[ links ] = 0;
	}

	delete incoming_msg_info;
	delete outgoing_msg_info;
	delete preview_msg_info;

	Genie3_cli::toolname[0] = '\0';
	delete[] Genie3_cli::toolname;
	Genie3_cli::toolname = 0;

	if ( curr_capability != 0 )
	  free( curr_capability );
	curr_capability = 0;
}

int Genie3_cli::Init()
{
	Tcl_Interp *iptr = Tcl.GetInterp();

	/*
	 *	- Redefine "source” command for one that does in-line comments
	 *	- Redefine "exit" command for one that cleans up stdio before exiting
	 *	- Add "echo" command.
	 *	- Call Slp_SetTclInterp(interp);
	 */
	Slp.InitSlpInterp(iptr);
	Slp.SetSlpInterp(iptr);

	Atp.InitAtp(iptr);

	Slp.SetPrintfProc(Atp2Tcl_GetPager(iptr));

	uv_set( default_uv_name );

	// init the commands provided by GAL
	Gal_BindCmnCmds();

	return 1;
}

int Genie3_cli::SetPrompt(const char *prompt)
{
	cur_prompt = prompt;
	cur_prompt += prompt_suffix;

	return Slp.SetPrompt(cur_prompt);
}

int Genie3_cli::run( run_mode mode, const char *scriptfile )
{
	int rc = 0;

	/* Initialize the Interprocess Communication (ipc) */
	if (ipc_init(GAL_IPC_PROCNAME) < 0)
	{
	  fprintf(stderr, "%s: ipc_init() failed\n", GAL_IPC_PROCNAME);
	  exit(-1);
	}

	/* Initialize the terminal capabilities, e.g. highlight capabilities */
	Tbf_InitTerminal();

	/* Initialise GCM Hosts to the users host machine */
	size_t	size_of_hostname = GAL_MAX_STRING_LENGTH;
	char	Default_host[GAL_MAX_STRING_LENGTH];

	rc = gethostname(Default_host, size_of_hostname);

	if ( rc == -1 )
	{
	  fprintf(stderr, "\"gethostname\" error \n");
	  exit(1);
	}

	Gal_SetLiveHostName(Default_host);
	Gal_SetLoopHostName(Default_host);

	/* Run profiles */
	rc = Tcl.Eval
			(
				"if [ file exists /bnr/tools/genie/etc/sys.profile ] {source /bnr/tools/genie/etc/sys.profile}"
			);
	if (rc != TCL_OK)
	  cerr << Slp.GetSlpInterp()->result << endl;

	/* Interactive or batch ? */
	if ( mode == batch )
	{
	  // DO NOT USE Tcl_EvalFile(interp, filename) directly.
	  // Use the "source" command instead.
	  // This is because SLP has enhanced the "source" command to provide
	  // additional features such as \c command line continuation with comments.
	  // The "source" command will call the following unexported function:
	  // extern "C" int Slp_TclEvalFile(Tcl_Interp *interp, char *fileName)

	  rc = Tcl.VarEval( "source", scriptfile, (char *) 0 );

	  char *result_string = Tcl.GetResult();

	  if ( rc == TCL_OK )
	  {
		if ( result_string != 0 )
		  cout << result_string << endl;
	  }
	  else
	  if ( rc == TCL_ERROR )
	  {
		if ( result_string != 0 )
		  cerr << "Error: " << result_string << endl;
		else
		  cerr << "Error encountered while executing file \"" << scriptfile << "\"." << endl;
	  }
	  else
		cout << result_string << endl;

	  return ( rc == TCL_OK ) ? 0 : rc;
	}
	else
	if ( mode == interactive )
	{
	  /* Display prompt */
	  Slp.OutputPrompt();

	  /*
	   * Define routine to be called for a event representing activity on
	   * stdin. (file descriptor = 0.)
	   */
	  ipc_addinput( 0, IPC_READSELECT, (ipc_inputcallback) Gal_GetTextHandler(), (char *)NULL );

	  ipc_mainloop();
	}

	return 1;
}

void Genie3_cli::G3o_StdinHandler( char * client_ptr, int * file_desc, int * input_id )
{
	if ( cli == 0 )
	  return;

	Tcl_Interp *interp = 0;

	interp = cli->Slp.GetSlpInterp();

	cli->Slp.StdinHandler( client_ptr, file_desc, input_id );

	Gal_LogTestFile( interp );
}

Atp_Result Genie3_cli::QueryMessageHandler( const char *msg_name )
{
	if ( strcmp( msg_name, "*" ) == 0 )
	  return cli->QueryMessages();
	else
	  return cli->QueryMessage( msg_name );
}

Atp_Result Genie3_cli::ProtocolDescProc( void )
{
	if ( protocol != 0 )
	{
	  // Bloody Gal_Return() uses TBF to get output string,
	  // assumes Tbf_AdvPrintf() is used every time, even in callbacks!

	  Tbf_AdvPrintf("%s", protocol->ProtocolDesc);

	  return ATP_OK;
	}
	return ATP_ERROR;
}

Atp_Result Genie3_cli::ToolDescProc( void )
{
	Tbf_AdvPrintf("%s\n%s", G3O_tool_name, G3O_BNR_Copyright);

	return ATP_OK;
}

// def_parm_hex command bits and pieces
Atp_KeywordTab check_status =
{
	{"CHECK",	GAL_CHECK_LENGTH,	"Length checking enabled"},
	{"NOCHECK",	GAL_NOCHECK_LENGTH,	"Length checking disabled"},
	{0}
};

// disp_parm command bits and pieces
Atp_KeywordType style_keys[] =
{
	{"field",	GAL_DECODE_FIELDS},
	{"hex",		GAL_DECODE_HEX},
	{"string",	GAL_DECODE_STRING},
	{0}
};

// send_msg command
send_msg_cmd::send_msg_cmd( Genie3_cli *cli ) : Command(cli)
{
	pd = new parmdef;

	pd->BeginParms();
	  pd->NumDef("link", "Logical link number", 1, GAL_MAX_LINKS, 0);
	  pd->KeywordDef("msg", "Mnemonic of message to send", &ui->msg_types[0], 0);
	  pd->BeginRepeat("parm_list", "Additional user variables set(s) from which to encode the message", 0, 0, GAL_MAX_LINKS-1, 0) ;
	    pd->StrDef( "name", "user variables set name", 1, 32, NULL );
	  pd->EndRepeat(OPTL);
	pd->EndParms();

	Create("send_msg", "Send a message", Gal_GetHelpAreaMsgId() );

	preview_only = 0;
}

//send_msg_cmd::~send_msg_cmd()
// { }

// send_hex command
send_hex_cmd::send_hex_cmd( Genie3_cli *cli ) : Command(cli)
{
	pd = new parmdef;

	pd->BeginParms();
	  pd->NumDef("link", "Logical link number", 1, GAL_MAX_LINKS, 0);
	  pd->DatabytesDef("data", "Hex data bytes", 1, 0, 0);
	pd->EndParms();

	Create("send_hex", "Send raw data specified in hexadecimal format", Gal_GetHelpAreaMsgId() );
}

//send_hex_cmd::~send_hex_cmd()
// { }

// set_receive command
set_receive_cmd::set_receive_cmd( Genie3_cli *cli, receive_cmd *rx_cmd )
					: Command(cli), receive_cmd_ptr( rx_cmd )
{
	static Atp_KeywordType keyword[3] =
	{
		{"automatic", receive_cmd::automatic, "automatic receive mode"},
		{"manual", receive_cmd::manual, "manual receive mode - use \"receive_msg\" command"},
		{NULL}
	};

	pd = new parmdef;
	pd->BeginParms();
	  pd->KeywordDef("mode", "Receive message mode", keyword, 0) ;
	pd->EndParms();

	Create("set_receive", "Set message receive mode", Gal_GetHelpAreaMsgId() );
}

// receive_msg command
receive_cmd::receive_cmd( Genie3_cli *cli ) : Command(cli), mode( automatic ), queued()
{
	script_mode = 0;
	scriptFile = 0;

	pd = new parmdef;

	static Atp_ChoiceDescriptor def_link_option = {0};
	def_link_option.CaseValue = RCV_ALL_LINKS;

	// Set names, for consistency, also used in g3ouicmd.cxx
	_timeout		= "timeout";
	_link_option	= "link_option";
	_linkno			= "linkno";
	_rcv_all_links	= "*";

	pd->BeginParms();
	  pd->NumDef(_timeout, "timeout in seconds", 0, 0, 3600, 0);
	  pd->BeginChoice(_link_option, "set receive link(s)", &def_link_option, 0);
	    pd->BeginCase("link" , "receive on this link", RCV_LINK_NO);
	      pd->NumDef(_linkno, "link number", 1, GAL_MAX_LINKS, 0);
	    pd->EndCase() ;
	    pd->Case(_rcv_all_links, "receive on all links", RCV_ALL_LINKS);
	  pd->EndChoice(OPTL);
	pd->EndParms();

	receive_cmd_name = "receive_msg";

	Create(receive_cmd_name, "Receive message within given timeout period", Gal_GetHelpAreaMsgId() );
}

reset_msg_queue_cmd::reset_msg_queue_cmd( Genie3_cli *cli, receive_cmd *rx_cmd ) : Command( cli ), receive_cmd_ptr( rx_cmd )
{
	Create("reset_msg_queue", "Reset the message queue for the receive_msg command", Gal_GetHelpAreaMsgId() );
}

// def_parm_hex command
def_parm_hex_cmd::def_parm_hex_cmd( Genie3_cli *cli ) : Command( cli )
{
	pd = new parmdef;

	pd->BeginParms();
	  pd->KeywordDef("parm", "Parameter mnemonic", &ui->parameters[0], 0);
	  pd->DatabytesDef("data", "Hex data bytes", 1, 0, 0);
	  pd->KeywordDef("check", "Check Length", GAL_CHECK_LENGTH, check_status, 0);
	pd->EndParms();

	Create("def_parm_hex", "Define a parameter in hex", Gal_GetHelpAreaParmId());
}

//def_parm_hex_cmd::~def_parm_hex_cmd()
// { }

// disp_parm command
disp_parm_cmd::disp_parm_cmd( Genie3_cli *cli ) : Command( cli )
{
	pd = new parmdef;

	pd->BeginParms();
	  pd->KeywordDef("parm", "Parameter mnemonic", &ui->parameters[0], 0);
	  pd->KeywordDef("style", "Display style", GAL_DECODE_FIELDS, style_keys, 0);
	pd->EndParms();

	const char *command_name = "disp_parm";
	Create( command_name, "Display the value of a parameter", Gal_GetHelpAreaParmId() );

	static char *disp_parm_help_info[] =
	{
		"Display the value of a parameter.",
		"For parameters in the message header or label, use the \"disp_header\" command instead.",
		(char *)0
	};

	cli->Atp.AddHelpInfo( ATP_MANPAGE_HEADER, command_name, disp_parm_help_info );
}

//disp_parm_cmd::~disp_parm_cmd()
// { }

def_parm_cmd::def_parm_cmd
	(
		Genie3_cli *cli,
		const char *grp_name, const char *desc,
		parmdef *p,
		int props
	)
	: Command( cli ), cmd_name( "def_" ), properties( props )
{
	group_name = grp_name;
	cmd_name += grp_name;
	pd = p;
	Create(cmd_name, strdup(desc), Gal_GetHelpAreaParmId());
}

//def_parm_cmd::~def_parm_cmd()
// { }

static char * chk_cic_vproc( void *cicPtr, Atp_BoolType )
{
	Atp_NumType upper_cic = *(Atp_NumType *)cicPtr;

	if ( upper_cic < Atp_Num("lower_cic") )
	  return "Upper CIC bound must be greater than Lower CIC bound.";
	else
	  return (char *)0;
}

static int compare_values( const void *a, const void *b )
{
	Atp_NumType *p = (Atp_NumType *)a;
	Atp_NumType *q = (Atp_NumType *)b;

	return ( *p - *q );
}

char * filter_cic_cmd::tidyup_series_vproc ( void *seriesPtr, Atp_BoolType )
{
	Atp_DataDescriptor *series = (Atp_DataDescriptor*) seriesPtr;
	Atp_NumType *numPtr = (Atp_NumType *) series->data;
	Atp_NumType count = series->count;
	register int i, j = 0;

	series_count = count; // before

	qsort(numPtr, count, sizeof(Atp_NumType), compare_values);

	// remove duplicate entries
	for (i = j = 0; j < count; i++, j++) {
	   numPtr[i] = numPtr[j];
	   while ((j < (count - 1)) && numPtr[i] == numPtr[j + 1])
			j++;
	}

	// calculate new count
	series->count = count - (j - i);

	series_count = series->count; // after

	// zero out remaining numbers
	while (i < count)
		 numPtr[i++] = 0;

	return 0;
}

int calc_dec_width( int value )
{
	char tmpbuf[80];
	sprintf( tmpbuf, "%d", value );
	return strlen( tmpbuf );
}

filter_cic_cmd::filter_cic_cmd( Genie3_cli *cli, int cic_bitwidth ) : Command( cli )
{
	cic_bit_width = cic_bitwidth;

	// Calculate maximum value from bitwidth, without using pow() and linking the math library libm.a
	RWCString max_cic_bits( '1', cic_bit_width );
	cic_max_value = (int)strtol( max_cic_bits.data(), 0, 2 ); // base 2 for binary
	cic_max_dec_width = calc_dec_width( cic_max_value );

	static Atp_KeywordType cic_filter_flags[4] =
	{
		{"include",	include,	"Include incoming messages containing the following CIC(s)"},
		{"exclude",	exclude,	"Exclude incoming messages containing the following CIC(s)"},
		{"list",	list,		"List CIC filter settings"},
		{NULL}
	};

	static Atp_ChoiceDescriptor default_cic_filter_option = { all };

	pd = new parmdef;

	pd->BeginParms();
	  pd->KeywordDef("action", "CIC filter actions", list, cic_filter_flags, 0);
	  pd->BeginChoice("option", "CIC filter options", &default_cic_filter_option, 0);
	    pd->BeginCase("range", "Decode incoming messages with CICs in specific range", range);
	      pd->NumDef("lower_cic", "Lower (inclusive) bound", 0, cic_max_value, 0);
	      pd->NumDef("upper_cic", "Upper (inclusive) bound", 0, cic_max_value, chk_cic_vproc);
	    pd->EndCase();
	    pd->BeginCase("series", "Set a discrete series of individual CIC(s)", series);
	      pd->BeginRepeat("cics", "Series of one or more CIC(s)", 1, cic_max_value+1, tidyup_series_vproc);
	        pd->NumDef("cic", "CIC code", 0, cic_max_value, 0);
	      pd->EndRepeat();
	    pd->EndCase();
	    pd->Case("all", "Filter or list all CIC(s)", all);
	  pd->EndChoice(OPTL);
	pd->EndParms();

	Create("filter_cic", "Filter incoming messages according to Circuit Identification Code (CIC) values", Gal_GetHelpAreaSessionId())

	cic_settings << new RWBitVec( cic_max_value+1, TRUE );

	// series_count is a kludge to overcome ATP not re-reading and re-storing count field changed by vproc
	series_count = 0;
}

filter_cic_cmd::~filter_cic_cmd()
{
	if ( cic_settings != 0 )
	  delete cic_settings;
}

preview_msg_cmd::preview_msg_cmd( Genie3_cli *cli ) : Command ( cli )
{
	pd = new parmdef;

	pd->BeginParms();
	  pd->NumDef("link", "Logical link number", 1, GAL_MAX_LINKS, 0);
	  pd->KeywordDef("msg", "Mnemonic of message to preview", &ui->msg_types[0], 0);
	  pd->BeginRepeat("parm_list", "Additional user variables set(s) from which to preview the message", 0, 0, GAL_MAX_LINKS-1, 0);
	    pd->StrDef( "name", "user variables set name", 1, 32, NULL );
	  pd->EndRepeat(OPTL);
	pd->EndParms();

	Create("preview_msg", "Preview a message", Gal_GetHelpAreaMsgId () );
}

set_auto_cic_cmd::set_auto_cic_cmd( Genie3_cli *cli, int no_of_links, int cic_bitwidth ) : Command( cli )
{
	assert( cic_bitwidth > 0 && cic_bitwidth <= sizeof(int)*8 );
	assert( no_of_links > 0 );

	auto_cic_settings = 0;

	if ( no_of_links <= 0 )
	  no_of_links = GAL_MAX_LINKS;

	num_of_links = no_of_links;
	cic_bit_width = cic_bitwidth;
	RWCString max_cic_bits( '1', cic_bit_width );
	max_cic_value = (int)strtol( max_cic_bits.data(), 0, 2 ); // base 2 for binary
	max_cic_dec_width = calc_dec_width (max_cic_value) ;
	max_linkno_dec_width = calc_dec_width(no_of_links);

	static Atp_KeywordType auto_cic_modes [5] =
	{
		{"on",		auto_cic_on,	"Switch ON Auto-CIC feature"},
		{"off",		auto_cic_off,	"Switch OFF Auto-CIC feature"},
		{"reset",	auto_cic_reset,	"Reset to empty CIC value"},
		{"query",	auto_cic_query,	"Query Auto-CIC setting(s)"},
		{0}
	};

	static Atp_ChoiceDescriptor auto_cic_default_options = { auto_cic_all_links };

	pd = new parmdef;

	pd->BeginParms();
	  pd->KeywordDef("mode", "Auto-CIC operation mode", auto_cic_query, auto_cic_modes, 0);
	  pd->BeginChoice("options", "Operate on one or all links", &auto_cic_default_options, 0);
	    pd->BeginCase("link", "Operate on only one link", auto_cic_one_link);
	      pd->NumDef("linkno", "Logical link number", 1, no_of_links, 0);
	    pd->EndCase() ;
	    pd->Case("all", "Filter or list all CIC(s)", auto_cic_all_links);
	  pd->EndChoice(OPTL);
	pd->EndParms();

	const char *command_name = "auto_cic";
	Create(command_name, "Access \"Automatic CIC\" settings", Gal_GetHelpAreaSessionId() );

	static char *auto_cic_help_info[] =
	{
		"If Auto-CIC is enabled, the CIC (Circuit Identification Code) value of the previous incoming message, for a given link, is used in the next outgoing message, for that same link.",
		"The CIC value(s) stored in the user variables set(s) remain unaltered by this operation.",
		"If no previous CIC value is available for use, the current user variables set's CIC value is used, as if Auto-CIC is turned OFF.",
		(char *)0
	};

	cli->Atp.AddHelpInfo( ATP_MANPAGE_HEADER, command_name, auto_cic_help_info );

	auto_cic_settings = new struct auto_cic_setting [no_of_links+1];

	for (int i = 0; i < no_of_links+1; i++)
	{
	   auto_cic_settings[i].on_off_flag =0;	// off
	   auto_cic_settings[i].cic_value = -1;	// initial invalid value
	}
}

set_auto_cic_cmd::~set_auto_cic_cmd ( void )
{
	if ( auto_cic_settings )
	  delete[] auto_cic_settings;
}

int set_auto_cic_cmd::set_auto_cic_flag( int link_no )
{
	if ( link_no <= 0 || link_no > num_of_links )
	  return -1;

	auto_cic_settings[link_no].on_off_flag = 1;

	return 1;
}

int set_auto_cic_cmd::unset_auto_cic_flag( int link_no )
{
	if ( link_no <= 0 || link_no > num_of_links )
	  return -1;

	auto_cic_settings [link_no].on_off_flag = 0;

	return 1;
}

int set_auto_cic_cmd::reset_auto_cic_value( int link_no )
{
	if ( link_no <= 0 || link_no > num_of_links )
	  return -1;

	auto_cic_settings[link_no].cic_value = -1;

	return 1;
}

int set_auto_cic_cmd::set_auto_cic_value( int link_no, int cic_value )
{
	if ( link_no <= 0 || link_no > num_of_links )
	  return -1;

	if ( cic_value < 0 || cic_value > max_cic_value )
	  return -1;

	auto_cic_settings[link_no].cic_value = cic_value;

	return 1;
}

int set_auto_cic_cmd::get_auto_cic_flag( int link_no )
{
	if ( link_no <= 0 || link_no > num_of_links )
	  return -1;

	return auto_cic_settings[link_no].on_off_flag;
}

int set_auto_cic_cmd::get_auto_cic_value( int link_no )
{
	if ( link_no <= 0 || link_no > num_of_links )
	  return -1;

	return auto_cic_settings[link_no].cic_value;
}

int set_auto_cic_cmd::display_cic_value( int link_no, ostream& output )
{
	if ( link_no <= 0 || link_no > num_of_links )
	  return -1;

	output << "Link " << setw(max_linkno_dec_width) << link_no << ",  ";
	output << "Auto-CIC = " << ((auto_cic_settings[link_no].on_off_flag) ? " ON" : "OFF") << ",	";
	output << "CIC = ";
	int cic = auto_cic_settings[link_no].cic_value;
	if ( cic >= 0 )
	  output << setw(max_cic_dec_width) << cic;
	else
	  output << setw(max_cic_dec_width) << "N/A";
	output << endl << flush;
	return cic;
}

disp_header_cmd::disp_header_cmd( Genie3_cli *cli ) : Command( cli )
{
	pd = new parmdef;

	pd->BeginParms();
	  pd->NumDef("link", "Logical link number", 1, GAL_MAX_LINKS, 0);
	pd->EndParms();

	const char *command_name = "disp_header";
	Create(command_name, "Display message header settings", Gal_GetHelpAreaSessionId() );

	static char *disp_header_help_info[] =
	{
		"Display the parameters of the message \"header\" (SIO and Label) for a given link.",
		(char *)0
	};

	cli->Atp.AddHelpInfo( ATP_MANPAGE_HEADER, command_name, disp_header_help_info );
}

status_cmd::status_cmd( Genie3_cli *cli ) : Command( cli )
{
	Create("status", "Displays status of miscellaneous user settings", Gal_GetHelpAreaSessionId() );
}
//--------------------------------------------------------------------------------------------------

int Genie3_cli::init_protocol(Protocol *p)
{
	protocol = p;

	// Get information on message types
	{
		Protocol::iterate_message_types next( protocol );
		while( next() )
		if ( !message_type( p, next.name(), next.description() ) )
		break;
	}

	// Get information on parameters
	{
		Protocol::iterate_field_groups next( protocol );
		while ( next() )
			 if ( !field_group( protocol, next.name(), next.description() , next.get_properties() ) )
			   break;
	}

	// Create built-in commands
	send_msg_Cmd_ptr		= new send_msg_cmd( this ) ;
	send_hex_Cmd_ptr		= new send_hex_cmd( this ) ;
	def_parm_hex_Cmd_ptr	= new def_parm_hex_cmd( this );
	disp_parm_Cmd_ptr		= new disp_parm_cmd( this );
	var_Cmd_ptr				= new var_cmd( this ) ;
	receive_cmd_ptr			= new receive_cmd( this ) ;
	set_receive_cmd_ptr		= new set_receive_cmd( this, receive_cmd_ptr ) ;
	reset_msg_queue_cmd_ptr	= new reset_msg_queue_cmd( this, receive_cmd_ptr );
	if ( use_cic_filtering )
	  filter_cic_cmd_ptr	= new filter_cic_cmd( this,	cic_bitwidth );
	preview_msg_cmd_ptr		= new preview_msg_cmd( this	);
	if ( use_auto_cic )
	  set_auto_cic_cmd_ptr	= new set_auto_cic_cmd( this, GAL_MAX_LINKS );
	disp_header_cmd_ptr		= new disp_header_cmd( this	);
	status_cmd_ptr			= new status_cmd( this );

	// Initialize any protocol-specific stuff such as creating custom built-in commands,
	protocol->UICustomInit( this );

#if _DEBUG_LOG_FILE // diagnostics only, to be deleted...
	// Show data on parameters
	ostrstream filename;

	filename << logfilename << ".log" << ends;
	fstream parmfile(filename.str(), ios::out);
	parmfile << filename.str() << ":" << endl;

	delete[] filename.str();

	for (int t = 0; t < 13 ; t++)
	{
	   for (int f = 0; f < 8; f++)
	   {
		  parmfile << "Parm count[" << show_field_type((Protocol::field_type)t) << "][";
		  parmfile << show_field_format((Protocol::field_format)f) << "] = ";
		  parmfile << parmtype_count[t][f] << endl;
	   }
	}
#endif

	return 1;
}

void Genie3_cli::set_mtp_capability ( const char *cap )
{
	if ( curr_capability != 0 )
	  free( curr_capability );

	if ( cap == 0 || *cap == '\0' )
	  curr_capability = 0;
	else
	  curr_capability = strdup( cap );
}

int Genie3_cli::message_type(Protocol *,
							 const char *msg_type_name,
							 const char *msg_type_desc)
{
	static Atp_KeywordType new_msg_type = {0};
	static int msg_types_count = 0;

	new_msg_type.keyword = (char *)msg_type_name;
	new_msg_type.KeyValue = msg_types_count++;
	new_msg_type.KeywordDescription = (char *)msg_type_desc;

	msg_types.append(new_msg_type);

#if _DEBUG_UI_PROT_UI
	if ( __debug )
	{
	  cout << "  " << new_msg_type.KeyValue << ": " << new_msg_type.keyword;
	  cout << " (" << new_msg_type.KeywordDescription << ")" << endl;
	}
#endif

	return 1;
}

int Genie3_cli::field_group(Protocol *p,
							const char *group_name,
							const char *group_desc,
							int properties)
{
	// Record group name in parameters array
	static Atp_KeywordType new_parm_group = {0};
	static int parm_group_count = 0;

#if _DEBUG_DI_PROT_UI
	if ( __debug )
	{
	  cout << endl << "  " << parm_group_count << ": " << group_name;
	  cout << "(" << group_desc << ")" << endl;
	}
#endif

	// Do not create command for field group with property "invisible" e.g. MSGTYPE
	if ( properties & Protocol::invisible )
	  return 1;

	//
	// Define the command for this group
	//

	// Allocate a new parmdef for this group
	tmp_parmdef = new parmdef;

	tmp_parmdef->BeginParms();

	// Create an extra link parameter for per-link message parameters.
	if ( properties & Protocol::per_link )
	  tmp_parmdef->NumDef("link", "Logical link number", 1, GAL_MAX_LINKS, 0);

	int num_fields = 0;

	{
		Protocol::iterate_fields_of_group next( p, group_name );

		while( next() )
		{
			num_fields++;

			if ( ! field_of_group_init(	protocol,
										next.variable_name(),
										next.variable_group(),
										next.variable_field(),
										next.type(),
										next.format(),
										next.long_description(),
										next.controlled_by(),
										next.min_valid(),
										next.max_valid(),
										next.get_bitwidth(),
										next.get_table(),
										next.get_custom_ops(),
										next.get_properties()) )
				break;
		}
	}

	tmp_parmdef->EndParms();

	//
	// Ignore parm commands with no fields
	//
	if ( num_fields == 0 )
	{
	  delete tmp_parmdef;
	  return 1;
	}

	// per_link parameters belong to the header and can only be viewed by the disp_header command.
	if ( ! ( properties & Protocol::per_link ) )
	{
	  new_parm_group.keyword = (char *)group_name;
	  new_parm_group.KeyValue = parm_group_count++;
	  new_parm_group.KeywordDescription = (char *)group_desc;

	  parameters.append (new_parm_group) ;
	}

	Command *cmd = new def_parm_cmd( this, group_name, group_desc, tmp_parmdef, properties );
	tmp_parmdef = 0;

	return 1;
}

int Genie3_cli::field_of_group_init(Protocol *,
									const char *variable_name,
									const char * /* group_name */,
									const char *field_name,
									enum Protocol::field_type FieldType,
									enum Protocol::field_format FieldFormat,
									const char *long_description,
									const char * /* controlled_by */,
									int min_value,
									int max_value,
									int bitwidth,
									const Protocol::Table * FieldValueMeanings,
									Protocol::CustomUIOps *custom_ops,
									int properties
									)
{
#if _DEBUG_LOG_FILE
	parmtype_count[FieldType][FieldFormat]++;
#endif

#if _DEBUG_UI_PROT_UI
	if ( __debug )
	{
	  static int count = 0;
	  static RWCString prev_group_name("");

	  // Detect new group_name (just for printing... to be deleted)
	  if ( prev_group_name != group_name )
	  {
		prev_group_name = group_name;
		count = 0;
	  }

	  cout << "    " << count++ << ": " << group_name << ":" << field_name;
	  cout << " (" << long_description << ")";
	  cout << " (controlled by: " << controlled_by << ")";
	  cout << "   [" << show_field_type(FieldType);
	  cout << ", " << show_field_format(FieldFormat) << "]" << endl;
	}
#endif

	Atp_VprocType vproc = 0;
	if ( custom_ops != 0 )
	{
	  vproc = (Atp_VprocType) custom_ops->get_vproc();
	  if ( custom_ops->get_cic_filter_flag() )
	  {
		cic_uv_name = custom_ops->get_cic_uv_name();
		cic_bitwidth = bitwidth;
		use_cic_filtering = 1;
	  }

	  if ( custom_ops->get_auto_cic_flag() )
	  {
		use_auto_cic = 1;
	  }
	}

	int table_size = max_value+1;

	static Atp_DataDescriptor default_desc = {0};

	switch (type_mapper(FieldType, FieldFormat))
	{
		case ATP_NUM: {
			if (properties & Protocol::is_optional)
				tmp_parmdef->NumDef(field_name, long_description, 0, min_value,
						max_value, vproc,
						ConvertToAtpKeywordTable(FieldValueMeanings, get_toolname(),
								table_size));
			else
				tmp_parmdef->NumDef(field_name, long_description, min_value,
						max_value, vproc,
						ConvertToAtpKeywordTable(FieldValueMeanings, get_toolname(),
								table_size));
			break;
		}
		case ATP_UNS_NUM: {
			if (properties & Protocol::is_optional)
				tmp_parmdef->UnsignedNumDef(field_name, long_description, 0,
						min_value, max_value, vproc,
						ConvertToAtpKeywordTable(FieldValueMeanings, get_toolname(),
								table_size));
			else
				tmp_parmdef->UnsignedNumDef(field_name, long_description, min_value,
						max_value, vproc,
						ConvertToAtpKeywordTable(FieldValueMeanings, get_toolname(),
								table_size));
			break;
		}
		case ATP_DATA: {
			if (properties & Protocol::is_optional)
				tmp_parmdef->DatabytesDef(field_name, long_description,
						&default_desc, min_value, max_value, vproc);
			else
				tmp_parmdef->DatabytesDef(field_name, long_description, min_value,
						max_value, vproc);
			break;
		}
		case ATP_BCD: {
			if (properties & Protocol::is_optional)
				tmp_parmdef->BcdDigitsDef(field_name, long_description,
						&default_desc, min_value, max_value, vproc);
			else
				tmp_parmdef->BcdDigitsDef(field_name, long_description, min_value,
						max_value, vproc);
			break;
		}
		case ATP_STR: {
			if (properties & Protocol::is_optional)
				tmp_parmdef->StrDef(field_name, long_description, 0, min_value,
						max_value, vproc);
			else
				tmp_parmdef->StrDef(field_name, long_description, min_value,
						max_value, vproc);
			break;
		}
		case ATP_BOOL: {
			if (properties & Protocol::is_optional)
				tmp_parmdef->BoolDef(field_name, long_description, 0, vproc);
			else
				tmp_parmdef->BoolDef(field_name, long_description, vproc);
			break;
		}
		default:
			throw Genie3_InternalError(
					"field of group init() cannot handle field %s", variable_name);
	}

	return 1;
}

#if _DEBUG_LOG_FILE || _DEBUG_UI_PROT_UI
static char* show_field_type(enum Protocol::field_type FieldType)
{
	switch (FieldType)
	{
		case Protocol::unknown:
			return "unknown";
		case Protocol::simple:
			return "simple";
		case Protocol::bcd:
			return "bcd";
		case Protocol::bcd_pad8:
			return "bcd_pad8";
		case Protocol::octet:
			return "octet";
		case Protocol::bit8:
			return "bit8";
		case Protocol::length0:
			return "length0";
		case Protocol::length1:
			return "length1";
		case Protocol::pos_indicator:
			return "pos_indicator";
		case Protocol::neg_indicator:
			return "neg_indicator";
		case Protocol::group:
			return "group";
		default:
			return "don't know";
	}
}

static char* show_field_format(enum Protocol::field_format FieldFormat)
{
	switch (FieldFormat)
	{
		case Protocol::ff_spare:
			return "ff_spare";
		case Protocol::ff_group:
			return "ff_group";
		case Protocol::ff_int:
			return "ff_int";
		case Protocol::ff_uint:
			return "ff_uint";
		case Protocol::ff_bcd:
			return "ff_bcd";
		case Protocol::ff_ia5:
			return "ff_ia5";
		case Protocol::ff_hex:
			return "ff_hex";
		case Protocol::ff_bit:
			return "ff_bit";
		default:
			return "don't know";
	}
}
#endif

Atp_ParmCode Genie3_cli::type_mapper(Protocol::field_type FieldType, Protocol::field_format FieldFormat)
{
	/*
		simple,		// fixed width set of bits
		bed,		// BCD string (no padding)
		octet,		// octet string
		bit8,		// bit string padded to 8 bit boundary
	 */

	/*
		ff_int,		// this field a signed interger
		ff_uint,	// this field is an unsigned integer
		ff_bcd,		// this field is BCD
		ff_ia5,		// this field is IA5
		ff_hex,		// this field is octets formated as HEX
		ff_bit		// this field is a vector of bits
	 */
	/*
		ATP_BPM		BEGIN_PARMS
		ATP_EPM		END_PARMS
		ATP_BLS		BEGIN_LIST
		ATP_ELS		END_LIST
		ATP_BRP		BEGIN_REPEAT
		ATP_ERP		END_REPEAT
		ATP_BCH		BEGIN_CHOICE
		ATP_ECH		END_CHOICE
		ATP_BCS		BEGIN_CASE
		ATP_ECS		END_CASE
		ATP_NULL	NULL parameter
		ATP_NUM		Signed long number
		ATP_UNS_NUM	Unsigned long number
		ATP_DATA	Octet databyte string
		ATP_STR		Octet ASCII string
		ATP_KEYS	Keyword
		ATP_BOOL	Boolean
		ATP_REAL	Real number
		ATP_COM		Common parmdef
		ATP_EOP		End Of Parameter Store Marker
	 */

	Atp_ParmCode	atp_type = ATP_EOP;

	switch (FieldType)
	{
		case Protocol::simple:
			switch (FieldFormat)
			{
				case Protocol::ff_int:
					atp_type = ATP_NUM;
					break;
				case Protocol::ff_uint:
					atp_type = ATP_UNS_NUM;
					break;
				case Protocol::ff_hex:
					atp_type = ATP_DATA;
					break;
				default:
					throw Genie3_InternalError("type_mapper cannot handle simple %d",
												FieldFormat);
			}
			break;
		case Protocol::bcd:
			switch (FieldFormat)
			{
				case Protocol::ff_bcd:
					atp_type = ATP_BCD;
					break;
				default:
					throw Genie3_InternalError("type_mapper cannot handle bcd %d",
												FieldFormat);
			}
			break;
		case Protocol::octet:
			switch (FieldFormat)
			{
				case Protocol::ff_ia5:
				case Protocol::ff_hex:
					atp_type = ATP_DATA;
					break;
				default:
					throw Genie3_InternalError("type_mapper cannot handle octet %d",
												FieldFormat);
			}
			break;
		case Protocol::bit8:
			switch (FieldFormat) {
			case Protocol::ff_bit:
				atp_type = ATP_DATA;
				break;
			default:
				throw Genie3_InternalError("type_mapper cannot handle bit8 %d",
											FieldFormat);
			}
			break;
		default:
			throw Genie3_InternalError("type_mapper cannot handle %d", FieldType);
	}

	return atp_type;
}

// ACST.175
static Atp_KeywordType* ConvertToAtpKeywordTable(const Protocol::Table *fvm,
												 const char *toolname, int table_size)
{
	if (fvm == 0)
	  return 0;

	Protocol::Table *table = (Protocol::Table*) fvm;

	register int x;
	int bit_width = table->get_bit_width();

	Protocol::Table::num_str_pair value_pair;

	value_pair = table->next(0); // reset table to beginning

	/* Don't make an empty Atp_KeywordType table. */
	if (value_pair.string == 0)
	  return NULL;

	Atp_KeywordType entry;
	DArray<Atp_KeywordType> field_value_meanings(table_size);

	for (x = 0; value_pair.string != 0; value_pair = table->next())
	{
		entry.keyword = (char*) toolname;
		entry.KeyValue = value_pair.value;
		entry.KeywordDescription = (char*) value_pair.string;
		entry.internal_use = bit_width;

		field_value_meanings.append(entry);
	}

	Atp_Used_By_G3O = 1; // make sure it's always set

	Atp_KeywordType *field_value_meanings_table = field_value_meanings.dup_arrayPtr(); // caller must free() it later

	return field_value_meanings_table;
}
