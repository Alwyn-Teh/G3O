/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+*******************************************************************

	Module Name:		g3ouicmd.cxx (was ui_cmd.cxx)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Built-In Command Class Implementations

	History:
		Who			When				Description
	----------	-------------	----------------------------------
	Alwyn Teh	94Q4 - 95Q2		Initial Creation
	Alwyn Teh	13th June 1995	QueryMessages() and
								QueryMessage() to calculate
								maxlen excluding trailing
								acronym to set field width.
	Alwyn Teh	16th June 1995, Commands must return string
				3rd July 1995	result in Tcl interp, instead
								of printing to cout or cerr.
	Alwyn Teh	6-7 July 1995	Implement receive_msg command.
	Alwyn Teh	13 July 1995	Write rcvmsg values into Tcl
								variables for automation use.
	Alwyn Teh	24 July 1995	Implement filter_cic command
								for CIC decode filtering.
	Alwyn Teh	28 July 1995	Implement preview_msg command.
	Alwyn Teh	1-8 August 1995	Implement set_auto_cic and
								disp_header commands.
	Alwyn Teh	19 August 1995	Implement per-link user variables.

********************************************************************-*/

#include <time.h>
#include <errno.h>

#include <rw/cstring.h>
#include <rw/regexp.h>

#include <g3ouih.h>
#include <g3ouicmh.h>
#include <g3otclvh.h>

#include <string.h>
#include <iostream>
#include <fstream.h>
#include <strstream.h>

#include <ctype.h>
#include <malloc.h>
#include <assert.h>

#include <tbf.h>
#include <gal.h>
#include <gcm.h>

#include <g3otclxh.h>
#include <g3oslpxh.h>
#include <g3oatpxh.h>
#include <g3opmdfh.h>
#include <g3ouvarh.h>
#include <g3obitsh.h>

using namespace std;

extern "C" const char * Tbf_Video( uint8 video_code, const char *string );

#if NATIVE_TIME_FORMAT
static char *time_format = "%a %b %e %T %Z %Y"; // e.g. Fri Jul 7 17:09:04 BST 1995
#else
static char *time_format = "%a %d-%b-%Y %H:%M:%S %Z"; // e.g. Wed 12-Jul-1995 21:25:23 BST
#endif

inline int min( int a, int b )
{
	return a < b ? a : b;
}

Atp_Result send_msg_cmd::do_cmd( int argc, char **argv, int _preview_only )
{
	preview_only = _preview_only;
	Atp_Result result = cmd( argc, argv );
	preview_only = 0;
	return result;
}

Atp_Result send_msg_cmd::cmd(int, char **)
{
	Atp_Result result	= ATP_OK;

	int link_no			= (int) Atp_Num("link");
	int index			= (int) Atp_Index("msg");
	char *msg_name		= ui->msg_types[ index ].keyword; // get original
	char *msg_desc		= ui->msg_types[ index ].KeywordDescription;

	bit_string msg_bits;

	user_variables format_vars;

	// Get the header parameters first.
	if ( ui->per_link_user_variables[ link_no ] )
	  format_vars = *(ui->per_link_user_variables[ link_no ]);

	format_vars += *ui->uv(Genie3_cli::default_uv_name);	// init from the default vars

	// If override message data supplied by user, send this instead.
	Atp_DataDescriptor parm_list = Atp_RptBlockDesc("parm_list");
	if (parm_list.count != 0 && parm_list.data != 0)
	{
	  for ( int arg = parm_list.count-1; arg >= 0; arg-- )
		 format_vars += *ui->uv( ((const char **)(parm_list.data))[arg] );
	}
	else
	{
	  format_vars += *ui->uv();	// then add in the current vars
	}

	// If Auto-CIC required, send message with appropriate CIC value,
	if ( ui->supports_auto_cic () )
	{
		int cic = 0;
		if ( ui->get_auto_cic_cmd()->get_auto_cic_flag( link_no ) && format_vars.exists( ui->get_cic_uv_name() ) )
		{
		  // Modify CIC bits but leave the rest alone.
		  cic = ui->get_auto_cic_cmd()->get_auto_cic_value( link_no );
		  if ( cic >= 0 )
		  {
			user_variables::uv_info * cic_uv_value = format_vars.value( ui->get_cic_uv_name() );
			cic_uv_value->bit_width = ui->get_cic_bitwidth();
			cic_uv_value->uv_bits->replace_lsb( 0, cic_uv_value->bit_width, cic );
		  }
		}
	}

	ostrstream output;

	try
	{
		ui->protocol->Encode(msg_name, &format_vars, &msg_bits);

		if ( preview_only )
		  output << "Preview (link " << link_no << ") ";
		else
		  output << ">>>> Sending on logical link " << link_no << endl;

		output << flush << Tbf_Video(BOLD, "OUTGOING") << " " << flush;

		ui->Format( ( preview_only ) ? ui->preview_msg_info : ui->outgoing_msg_info,
					&msg_bits, &format_vars, &output );

		output << endl;

		//
		// Now, actually send the message.
		//
		if ( !preview_only )
		{
		  Gcm_Result send_result = Gcm_Send( link_no, (char *)msg_bits.pointer(), msg_bits.length(8) );

		  if ( send_result.rc != GCM_OK )
		  {
			output << "GCM TRANSMISSION ERROR: " << send_result.string << endl;
			result = ATP_ERROR;
		  }
		  else
			output << "Sent (" << msg_bits.length(8) << " bytes)" << endl;
		}
	}
	catch(Protocol::FieldTooSmall e)
	{
		output << "[USER ERROR] Cannot " << (( preview_only ) ? "preview" : "encode") << " message " << msg_name << "." << endl;
		output << "\tUse def_" << (e.group.toLower(), e.group);
		output << " to correctly define " << e.field << "." << endl;
		result = ATP_ERROR;
	}
	catch(Protocol::FieldNotDefined e)
	{
		output << "[USER ERROR] Cannot " << (( preview_only ) ? "preview" : "encode") << " message " << msg_name << "." << endl;
		output << "\tUse def_" << (e.group.toLower(), e.group);
		output << " to define " << e.field << "." << endl;
		result = ATP_ERROR;
	}
	catch(Protocol::MessageShort)
	{
		output << "[USER ERROR] Cannot " << (( preview_only ) ? "preview" : "encode") << " message " << msg_name << "." << endl;
		output << "\tMessage is short." << endl;
		result = ATP_ERROR;
	}

	output << flush << ends;
	ui->Tcl.SetResult( output.str(), TCL_DYNAMIC );

	return result;
}

Atp_Result send_hex_cmd::cmd(int, char **)
{
	Atp_Result result = ATP_OK;
	Atp_DataDescriptor Data = Atp_DataBytesDesc("data");
	int link_no = Atp_Num("link");

	user_variables format_vars;

	// Get the header parameters first.
	if ( ui->per_link_user_variables[ link_no ] )
	  format_vars = *(ui->per_link_user_variables[ link_no ]);

	format_vars += *ui->uv(Genie3_cli::default_uv_name);	// init from the default vars
	format_vars += *ui->uv();								// then add in the current vars

	bit_string msg_bits;
	msg_bits.append( int(Data.count*8), (byte *)Data.data );

	ostrstream output;

	output << ">>>> Sending on logical link " << link_no << endl;
	output << endl;

	ui->Format( ui->outgoing_msg_info, &msg_bits, &format_vars, &output );

	Gcm_Result send_result;
	send_result = Gcm_Send( link_no, (char *)msg_bits.pointer(), msg_bits.length(8) );
	if ( send_result.rc != GCM_OK )
	{
	  output << "GCM TRANSMISSION ERROR: " << send_result.string << endl;
	  result = ATP_ERROR;
	}
	else
	  output << "Sent (" << msg_bits.length(8) << " bytes)" << endl;

	output << flush << ends;
	ui->Tcl.SetResult( output.str(), TCL_DYNAMIC );

	return result;
}

Tcl_CmdProc * receive_cmd::Get_TclCmdProc( const char *cmdname )
{
	// Hijack Tel built-in command callback...
	// instead of using Tcl_Eval to scan the hash table everytime

	Tcl_Interp *interp = ui->Tcl.GetInterp();

	Tcl_CmdInfo Cmdlnfo;

	if ( Tcl_GetCommandInfo(interp, (char *)cmdname, &CmdInfo) )
	  return Cmdlnfo.proc;
	else
	  return (Tcl_CmdProc *)0;
}

int receive_cmd::is_script_mode( void )
{
	// If inside a script, e.g. don't output to screen/cout.

	Tcl_Interp *interp = ui->Tcl.GetInterp();

	static char *info_argv[3] = { "info", "script", 0 };

	if ( scriptFile != 0 )
	{
	  free(scriptFile);
	  scriptFile = 0;
	}

	Tcl_ResetResult(interp);

	static Tcl_CmdProc *infoCmdProc = 0;

	if ( infoCmdProc == 0 )
	  infoCmdProc = Get_TclCmdProc(info_argv[0]);

	if ( infoCmdProc )
	{
	  int rc = (*infoCmdProc)(0, interp, 2, info_argv);

	  if ( rc == TCL_OK && interp->result != 0 && *interp->result != '\0' )
		scriptFile = strdup(interp->result);

	  script_mode = ( scriptFile == 0 || *scriptFile == '\0' ) ? 0 : 1;

	  Tcl_ResetResult(interp);

	  return script_mode;
	}

	return 0;
}

int receive_cmd::SourceProcWrapper( ClientData clientData, Tcl_Interp *interp, int argc, char *argv[] )
{
	receive_cmd *myself = (receive_cmd *)clientData;

	enum receive_mode curr_mode = myself->mode;

	Tcl_ResetResult( interp );

	myself->set_mode( manual, 0 );

	int result = (*OldSourceProc)( clientData, interp, argc, argv );

	char *save_result = (interp->result == 0 || *interp->result == '\0' ) ? 0 : strdup(interp->result);

	myself->set_mode( curr_mode, 0 );

	if ( save_result != 0 )
	  Tcl_SetResult( interp, save_result, TCL_DYNAMIC );
	else
	  Tcl_SetResult( interp, "", TCL_STATIC );

	return result;
}

// receive_cmd::cmd() is the callback for the "receive_msg" command.
Atp_Result receive_cmd::cmd(int, char **)
{
	ostrstream output;

	Atp_Result result = ATP_OK;

	int   timeout		 = (int)Atp_Num(_timeout);
	int   link_option	 = (int)Atp_Num(_link_option);
	uint8 linkno		 = 0;

	char *decoded_ic_msg = 0;
	int	carry_on		 = 1;

	is_script_mode(); // set script_mode

	if ( link_option == RCV_LINK_NO )
	  linkno = (uint8) Atp_Num(_linkno);

	ostream *out = &cout; // need warmfeeling if in interactive mode
	if ( script_mode )
	  out = &output;

	*out << endl << "<<<< Receiving message";

	if ( link_option == RCV_LINK_NO )
	  *out << " on link " << ((int)linkno);
	else
	if ( link_option == RCV_ALL_LINKS )
	  *out << " on all links";

	*out << " with timeout " << timeout << " second(s)" << endl << flush;

	// Helper Tcl variable $msg_received
	char *msg_received = "msg_received";
	Tcl_SetVar( ui->Tcl.GetInterp(), msg_received, "0", TCL_GLOBAL_ONLY|TCL_LEAVE_ERR_MSG );

	// Check if there're any queued up messages,
	if ( queue.next != fcqueue )
	{
	  Message *m = queue.next;
	  while ( m != &queue )
	  {
		  assert( m != 0 );
		  if ( (link_option == RCV_LINK_NO && m->linkno == (int)linkno) || link_option == RCV_ALL_LINKS )
		  {
			  m->Remove();

			  output << "<<<< Retrieved message from queue on link " << m->linkno << " (" << m->length << " bytes)." << endl;

			  ui->IncomingMessage( m->linkno, m->data, m->length, m->timestamp, &output, script_mode, 0 );

			  delete m;

			  Tcl_SetVar( ui->Tcl.GetInterp(), msg_received, "1", TCL_GLOBAL_ONLY|TCL_LEAVE_ERR_MSG );

			  carry_on = 0;

			  break;
		  }
		  m = m->next;
	  }
	}

	if ( carry_on )
	{
	  Gcm_Result rcv_result;

	  uint8	rcv_linkno	= 0;
	  int	rcv_timeout	= timeout;
	  int	rcv_length	= 0;
	  char	*rcv_msg	= 0;

	  struct timeval tv;
	  time_t rcv_time;
	  double initial_sec, initial_usec;
	  int elapsed;
	  int time_rc = 0;
	  char	*time_errmsg = "gettimeofday () failed - ";

	  memset(&tv, 0, sizeof(struct timeval));
	  errno = 0;
	  time_rc = gettimeofday(&tv, 0);
	  if ( time_rc != 0 )
		output << time_errmsg << strerror(errno) << endl;

	  initial_sec	= tv.tv_sec;
	  initial_usec	= tv.tv_usec;

	  for (;;)
	  {
		 time_rc = gettimeofday(&tv, 0);
		 if ( time_rc != 0 )
		 {
		   output << time_errmsg << strerror(errno) << endl;
		   break;
		 }

		 elapsed = (int) ((tv.tv_sec - initial_sec) + ((tv.tv_usec-initial_usec)/1000000));

		 rcv_result = Gcm_Receive( &rcv_linkno, &rcv_msg, &rcv_length, rcv_timeout-elapsed );

		 if (rcv_result.rc == GCM_OK)
		 {
		   rcv_time = time(0); // register time of received message

		   if ( linkno == rcv_linkno || link_option == RCV_ALL_LINKS )
		   {
			 ui->IncomingMessage( (int) rcv_linkno, (unsigned char *) rcv_msg,
					 	 	 	  rcv_length, rcv_time, &output, script_mode, 0 );
			 Tcl_SetVar( ui->Tcl.GetInterp(), msg_received, "1", TCL_GLOBAL_ONLY|TCL_LEAVE_ERR_MSG );
			 break;
		   }
		   else
		   {
			 // queue up this message
			 Message *m = new Message( (int) rcv_linkno, (unsigned char *) rcv_msg, rcv_length, rcv_time );

			 m->Insert( queue.prev );

			 if ( rcv_msg != 0 )
			 {
			   free(rcv_msg);
			   rcv_msg = 0;
			 }

			 // continues looping until timeout or received appropriate message
		   }
		 }
		 else
		 if (rcv_result.rc == GCM_RETURN) // i.e. timed out because received nothing
		 {
		   // GCM will return something like "Timed-out waiting for a message"
		   output << rcv_result.string;
		   result = ATP_RETURN;
		   break;
		 }
		 else
		 if (rcv_result.rc == GCM_ERROR)
		 {
		   output << endl << "GCM RECEIVE ERROR: " << rcv_result.string;
		   result = ATP_ERROR;
		   break;
		 }
	  }

	  if ( rcv_msg != 0 )
	  {
		free(rcv_msg);
		rcv_msg = 0;
	  }
	}

	output << endl << flush << ends;

	ui->Tcl.SetResult( output.str(), TCL_DYNAMIC );

	script_mode = 0; // reset to zero; if nested script files, next call will detect this

	return result;
}

// set_mode is used by the "set_receive" command
void receive_cmd::set_mode( enum receive_mode new_mode, int set_tcl_result_flag )
{
	ostrstream output;

	if ( is_script_mode() && new_mode == automatic )
	{
	  cerr << '\007' << flush; // sound the bell
	  cerr << "WARNING: \"set_receive automatic\" ignored inside script file \n" << scriptFile << "\"." << endl;
	  cerr << "         \"manual\" receive mode always enforced during script execution." << endl;

	  output << "Receive message mode is MANUAL." << endl;
	  output << flush << ends;

	  if ( set_tcl_result_flag )
		ui->Tcl.SetResult( output.str(), TCL_DYNAMIC );

	  return;
	}

	if ( mode != new_mode )
	{
	  mode = new_mode;

	  switch( mode )
	  {
	  	  case automatic:
	  	  	  	  {
	  	  	  		  // empty any queued messages
	  	  	  		  int queued_msgs = 0;

	  	  	  		  output << endl << "Receive message mode is AUTOMATIC." << endl;
	  	  	  		  if ( queue.next != &queue )
	  	  	  			output << endl << "Emptying message queue..." << endl;

	  	  	  		  while( queue.next != &queue )
	  	  	  		  {
	  	  	  			  Message *m = queue.next;

	  	  	  			  m->Remove();

	  	  	  			  ui->IncomingMessage( m->linkno, m->data, m->length, m->timestamp, &output, script_mode, 0 );

	  	  	  			  delete m;

	  	  	  			  queued_msgs++;
	  	  	  		  }

	  	  	  		  if ( queued_msgs )
	  	  	  			output << queued_msgs << " message(s) removed from queue." << endl;

	  	  	  		  output << "The receive message queue is empty." << endl;

	  	  	  		  break;
	  	  	  	  }

	  	  case manual:
	  		  	  output << "Receive message mode is MANUAL." << endl;
	  		  	  output << "Use the \"" << receive_cmd_name << "\" command to process messages." << endl;
	  		  	  // start queuing messages
	  		  	  break;
	  }
	}
	else
	{
		output << "Receive message mode is ";
		switch( new_mode )
		{
			case automatic: output << "AUTOMATIC." << endl; break;
			case manual:	output << "MANUAL." << endl; break;
		}
	}

	if ( set_tcl_result_flag )
	{
	  output << flush << ends;
	  ui->Tcl.SetResult( output.str(), TCL_DYNAMIC );
	}
}

void receive_cmd::IncomingMessage( int link_no, unsigned char *msg, int length, time_t _timestamp, ostream *output )
{
	switch( mode )
	{
		case automatic:
				// just process
				ui->IncomingMessage( link_no, msg, length, _timestamp, output, script_mode );
				break;

		case manual:
				{
					// queue up this message
					Message *m = new Message( link_no, msg, length, _timestamp );

					m->Insert( queue.prev );
				}
				break;
	}
}

int receive_cmd::reset_queue( void )
{
	int msgs_deleted = 0;

	while ( queue.next != &queue )
	{
		Message *m = queue.next;

		m->Remove();

		delete m;

		msgs_deleted++;
	}

	return msgs_deleted;
}

Atp_Result reset_msg_queue_cmd::cmd(int, char **)
{
	int msgs_deleted = receive_cmd_ptr->reset_queue();

	ostrstream output;

	if ( msgs_deleted )
	  output << "reset msg queue: Deleted " << msgs_deleted << " unretrieved message (s)." << flush << ends;
	else
	  output << "reset_msg_queue: Message queue is empty." << flush << ends;

	ui->Tcl.SetResult( output.str(), TCL_DYNAMIC );

	return ATP_OK;
}

Atp_Result set_receive_cmd::cmd(int, char **)
{
	enum receive_cmd::receive_mode mode = (enum receive_cmd::receive_mode)Atp_Num("mode");

	receive_cmd_ptr->set_mode( mode );

	return ATP_OK;
}

Atp_Result def_parm_hex_cmd::cmd(int, char **)
{
	int index = (int)Atp_Index("parm");
	char *parmname = ui->parameters[ index ].keyword;
	char *parmdesc = ui->parameters[ index ].KeywordDescription;

	Atp_DataDescriptor Data = Atp_DataBytesDesc("data");
	bit_string bits;
	int bitwidth = (int)Data.count * 8;
	bits.append(bitwidth, (byte *)Data.data);

	// Define override value in current user variables set
	user_variables *curr_uv = ui->uv();
	user_variables::uv_info *curr_uv_value = curr_uv->value( parmname );
	curr_uv->define( Protocol::OverrideName( parmname ), bits, bitwidth,
					 curr_uv_value->field_type, curr_uv_value->field_format,
					 curr_uv_value->long_description );

	ostrstream output;
	output << "Parameter " << parmname;
	output << " (" << parmdesc << ") defined in hex." << endl;

	user_variables uv( ui->uv() );

	ui->Format( &bits, &uv, parmname, &output );
	output << flush << ends;

	ui->Tcl.SetResult( output.str(), TCL_DVUAMIC );

	return ATP_OK;
}

Atp_Result disp_parm_cmd::cmd( int, char ** )
{
	Atp_Result result = ATP_OK;
	int index = (int)Atp_Index("parm");
	char *parmname = ui->parameters[ index ].keyword;
	char *parmdesc = ui->parameters[ index ] . KeywordDescription;

	bit_string bits;
	ostrstream output;

	try {
		ui->protocol->Encode(ui->uv(), &bits, parmname);

		output << "Parameter " << parmname;
		output << " (" << parmdesc << "):" << endl;

		ui->Format(&bits, ui->uv(), parmname, &output);

		output << flush << ends;
	}
	catch(Protocol::FieldTooSmall e)
	{
		output << "[USER ERROR] Use def_" << (e.group.toLower(), e.group);
		output << " to correctly define " << e.field << "." << flush << ends;
		result = ATP_ERROR;
	}
	catch( Protocol::FieldNotDefined e )
	{
		output << "[USER ERROR] Parameter " << e.group << " does not have field \"" << e.field
			   << "\" set to any value" << "." << endl;
		output << "\tUse def_" << (e.group.toLower(), e.group);
		output << " to define " << e.field << "." << flush << ends;
		result = ATP_ERROR;
	}
	catch (...)
	{
		output << "[USER ERROR] Parameter " << parmname;
		output << " (" << parmdesc << ") undefined. [Unknown exception thrown]" << flush << ends;
		result = ATP_ERROR;
	}

	ui->Tcl.SetResult( output.str(), TCL_DYNAMIC );

	return result;
}

Atp_Result def_parm_cmd::cmd(int argc, char ** argv)
{
	user_variables *uv = 0;

	// Check if this is a "per-link" group, if so, values are stored separately from current uv set.
	int link_no = 0;
	if ( properties & Protocol::per_link )
	{
	  link_no = Atp_Num( "link" );
	  if ( ui->per_link_user_variables[ link_no ] == 0 )
		ui->per_link_user_variables[ link_no ] = new user_variables;
	  uv = ui->per_link_user_variables[ link_no ];
	}
	else
	{
	  uv = ui->uv();
	}

	uv->undefine( Protocol::OverrideName( group_name ).data() );

	Protocol::iterate_fields_of_group next( ui->protocol, group_name );

	while ( next() )
		switch( ui->type_mapper(next.type(), next.format()) )
		{
			case ATP_NUM:
				{
					Atp_NumType num = Atp_Num( (char *)next.variable_field().data() );
					uv->define( next.variable_name(), sizeof(Atp_NumType)*8, num, next.type(), next.format(), next.long_description() );

#if _DEBUG_UI_PROT_UI
					if ( __debug )
					{
					  cout << "(" << next.variable_name() << ") = ";
					  cout << (Atp_NumType)uv->value( next.variable_name(), sizeof (Atp_NumType) *8 );
					  cout << endl;
					}
#endif
					break;
				}
			case ATP_UNS_NUM:
				{
					Atp_UnsNumType unum = Atp_UnsignedNum( (char *)next.variable_field().data() );
					uv->define( next.variable_name(), sizeof(Atp_UnsNumType)*8, unum, next.type(), next.format(), next.long_description() );

#if _DEBUG_UI_PROT_UI
					if ( __debug )
					{
					  cout << "(" << next.variable_name() << ") = ";
					  cout << (Atp_UnsNumType)uv->value( next.variable_name(), sizeof(Atp_UnsNumType)*8 );
					  cout << endl;
					}
#endif
					break;
				}
			case ATP_DATA:
				{
					Atp_DataDescriptor data_desc = Atp_DataBytesDesc( (char *)next.variable_field().data() );
					bit_string data_bits;
					int bitwidth = (int)data_desc.count * 8;
					data_bits.append(bitwidth, (byte *)data_desc.data);
					uv->define(next.variable_name(), data_bits, bitwidth, next.type(), next.format(), next.long_description() );

#if _DEBUG_UI_PROT_UI
					if ( __debug )
					{
					  cout << "(" << next.variable_name() << ") = "
						   << *uv->value(next.variable_name ()) << endl;
					}
#endif
					break;
				}
			case ATP_BCD:
				{
					Atp_DataDescriptor bcd_desc = Atp_BcdDigitsDesc( (char *)next.variable_field().data() );
					bit_string bcd_bits;
					int bitwidth = (int)bcd_desc.count * 4;
					bcd_bits.append(bitwidth, (byte *)bcd_desc.data);
					uv->define(next.variable_name (), bcd_bits, bitwidth, next.type () , next.format () , next.long_description() );

#if _DEBUG_UI_PROT_UI
					if ( __debug )
					{
					  cout << "(" << next.variable_name() << ") = "
						   << *uv->value(next.variable_name()) << endl;
					}
#endif
					break;
				}
			default:
				{
					throw Genie3_InternalError(	"Unknown field type in def_parm_cmd::cmd (command = %s, field = %s)",
												(argc > 0) ? argv[0] : (char *)next.variable_field().data());
				}
		}

	return ATP_OK;
}

void Genie3_cli::IncomingMessageHandler( int link_no, unsigned char *msg, int length )
{
	time_t time_received = time(0);
	cli->receive_cmd_ptr->IncomingMessage( link_no, msg, length, time_received, &cout );
}

void Genie3_cli::IncomingMessage( int link_no, unsigned char *msg, int length, time_t timestamp,
								  ostream *output, int script_mode, int output_to_cout )
{
	ostrstream decode_buffer;

	// place the message into a bit_string
	bit_string m;
	m.append( length*8, (byte *)msg );

	// put out a banner
	char time_buffer[100];
	time_buffer[0] = '\0';
	struct tm *time_rec = localtime( &timestamp );
	strftime(time_buffer, 100, time_format, time_rec);

	decode_buffer << endl << "<<<< Message received on link " << (int)link_no;
	decode_buffer << " (" << time_buffer << ")" << endl;

	//
	// Make a copy of the default user variables set which is expected to contain
	// things like SPCSIZE information needed for decoding.
	// Then, obtain those user variable(s) only required for decoding.
	//
	user_variables default_uv_clone( uv( default_uv_name ) );
	user_variables temp_uv_set;
	(void) default_uv_clone.decode_uv_set( temp_uv_set, protocol->decode_uv_names );

	// Decode and format incoming message into the temporary user variables set and decode buffer.
	decode_buffer << Tbf_Video(BOLD, "INCOMING") << " " << flush;
	Format( cli->incoming_msg_info, &m, &temp_uv_set, &decode_buffer );

	//If required, filter out unwanted message by examining its CIC field,
	int cic = 0;
	int keep_message = 1; // default action
	if ( use_cic_filtering )
	{
	  if ( temp_uv_set.exists( cic_uv_name ) )
	  {
		cic = temp_uv_set.value( cic_uv_name, cic_bitwidth );
		keep_message = filter_cic_cmd_ptr->get_cic_filter_setting( cic );
	  }
	}

	// If Auto-CIC required, record CIC value for link,
	if ( use_auto_cic )
	{
	  if ( set_auto_cic_cmd_ptr->get_auto_cic_flag( link_no ) && temp_uv_set .exists ( cic_uv_name ) )
	  {
		cic = (cic == 0) ? temp_uv_set.value( cic_uv_name, cic_bitwidth ) : cic ;
		set_auto_cic_cmd_ptr->set_auto_cic_value( link_no, cic );
	  }
	}

	user_variables *rcvmsg = Q;

	if ( keep_message )
	{
	  //
	  // Prepare the rcvmsg user variables set
	  //
	  uv_del( rcvmsg_uv_name ) ; // get rid of previous one if any
	  rcvmsg = uv( rcvmsg_uv_name );
	  *rcvmsg = temp_uv_set;

	  // Get the message decoded earlier and transfer it to the given output stream,
	  char *decoded_msg = 0;
	  decode_buffer << flush << ends;
	  decoded_msg = decode_buffer.str();

	  *output << decoded_msg << flush;

	  if ( decoded_msg )
		free( decoded_msg );
	}
	else
	{
	  return; // ignore unwanted message with matching CIC value
	}

	// Write rcvmsg values into Tel variables (ACST.175)
	if ( rcvmsg_vars != 0 )
	  delete rcvmsg_vars;
	rcvmsg_vars = new user_tcl_variables( Tcl.GetInterp(), rcvmsg_uv_name.data(), *rcvmsg );

	// Write helper Tcl variables for scripting -
	// $rcvmsg_timestamp and $rcvmsg_link (to indicate link number if receiving on all links).
	RWCString helpvar( rcvmsg_uv_name );
	helpvar += "_";
	int posn = helpvar.length();
	char LinkNum[10];
	sprintf( LinkNum, "%d", link_no );
	rcvmsg_vars->set_var( helpvar.insert(posn, "link"), LinkNum );
	rcvmsg_vars->set_var( helpvar.replace(posn, 10, "timestamp"), time_buffer );

	const char *msg_name = rcvmsg->value( Protocol::MessageTypeName() )->uv_bits->fetch_str();

	if ( strcasecmp( msg_name, "UNKNOWN" ) != 0 && rcvmsg->exists( Protocol::MessageTypeName() ) )
	{
	  MessageInfo *in_mi = &incoming_msg_info[ protocol->message_type_index( msg_name ) ];

	  if ( !in_mi->action.isNull() )
	  {
		*output << "++++ Message action started ++++" << endl;

		int result = Tcl.Eval( in_mi->action );
		const char *result_text = Tcl.GetResult();
		if ( result_text == NULL )
		  result_text = "";

		switch( result )
		{
			case TCL_OK:
				if (result_text[0] != '\0' )
				  *output << result_text << endl;
				break;
			case TCL_ERROR:
				*output << "Error: " << result_text << endl;
				break;
			default:
				*output << "Error " << dec << result <<	": " << result_text << endl;
				break;
		}

		*output << "------ Message action complete -----" << endl;
	  }
	}

	*output << endl << flush;

	// Output to screen if in interactive mode and if not already going to cout.
	// Otherwise, caller will manage contents in output stream anyway.
	// Don't output to cout if output_to_cout isn't set - e.g. "set_receive" command wants output returned.
	if ( !script_mode && output != &cout && output_to_cout )
	{
	  char *result = 0;

	  result = ((ostrstream *)output)->str();

	  cout << result << flush;

	  if ( result )
		free( result );
	}

	//
	//	finally reprompt
	//
	if ( output_to_cout )
	  Slp.OutputPrompt();
}

Atp_Result Genie3_cli::QueryMessages(void)
{
	static char *indent="    ";
	int maxlen = 0;
	int len = 0;
	RWCRegexp re(" ([A-Z]*)$");

	Protocol::iterate_message_types next( protocol );

	// Find maximum length of description string (excluding acronym)
	while ( next() )
	{
		RWCString desc( next.description() );
		desc(re) = "";
		len = desc.length();
		maxlen = (len > maxlen) ? len : maxlen;
	}

	next(-1); // reset to beginning of list

	ostrstream output;

	output << "Available Messages:" << endl;
	output << endl;
	output << indent << left << setw(maxlen+1)
		   << "Message name" << setw(12) << "Mnemonic" << "Level" << endl;
	output << endl ;

	while( next() )
	{
		MessageInfo *in_mi = &incoming_msg_info[ protocol->message_type_index( next.name() ) ];
		MessageInfo *out_mi = &outgoing_msg_info[ protocol->message_type_index( next.name() ) ];

		RWCString desc( next.description() );
		desc(re) = "";
		output << indent << left << setw(maxlen+1) << desc
			   << setw(12) << next.name()
			   << "[" << in_mi->decode_level_code()
			   << "/" << out_mi->decode_level_code() << "] "
			   << endl;
	}

	output << right << endl;
	output << left << setw(maxlen+strlen(indent)+1) << "Decode level:" << setw(12) << "[ <incoming> / <outgoing> ]" << endl;
	output << left << indent << setw(maxlen+1) << "<incoming> or <outgoing>:";
	output << left << setw(12-strlen(indent)) << "(S)uppress, (N)ame, (O)ctet," << endl;
	output << left << setw(maxlen+strlen(indent)+1) << setfill(' ') << "" << "(F)ield,    (H)ex,  (-)Disabled." << "" << endl;
	output << endl;
	output << "For further help on message, type \"query msg <msg_mnemonic>\"" << endl;
	output << endl << flush;

	output << ends;
	cli->Tcl.SetResult( output.str(), TCL_DYNAMIC );

	return ATP_OK;
}

Atp_Result Genie3_cli::QueryMessage( const char *rosg_name )
{
	static char *indent = "    ";
	int maxlen = 0;
	int len = 0;
	int msg_index = 0;

	if ( !protocol->message_type_exists( msg_name ) )
	{
	  RWCString errmsg( "Unknown message type " );
	  errmsg += msg_name;

	  Tcl.SetResult( errmsg.data() );

	  return ATP_ERROR;
	}

	Protocol::iterate_message_types next_msg( protocol );

	// position the iterator to this msg
	msg_index = protocol->message_type_index( msg_name );
	next_msg( msg_index );

	Protocol::iterate_groups_of_message_type next_group( protocol, msg_name );
	Protocol::iterate_groups_of_message_type next_grp( protocol, msg_name );

	// Find maximum length of description string
	while ( nextgrp() )
	{
		len = next_grp.description().length();
		maxlen = (len > maxlen) ? len : maxlen;
	}

	MessageInfo *in_mi = &incoming_msg_info[ msg_index ];
	MessageInfo *out_mi = &outgoing_msg_info[ msg_index ];

	ostrstream output;

	output << left << setw(maxlen+strlen(indent)+1) << "Whole message" << setw(19) << "Mnemonic" << "Level" << endl;
	output << indent << setw(maxlen+1) << next_msg.description() << setw(19) << msg_name
		   << "[" << in_mi->decode_level_code()
		   << "/" << out_mi->decode_level_code() << "]" << endl;
	output << endl;

	int last_was_optional = -1;
	int parm_number = 0;

	while( next_group () )
	{
		int parm_optional = protocol->optional_group_exists( nextgroup.name() );

		int index = protocol->group_index( next_group.name() );

		if ( parm_number == 0 )
		{
		  output << left << setw(maxlen+strlen(indent)+1) << "Message Header" << setw(13) << "Mnemonic" << "Type Level";

		  // force the opt/man switching code to trigger
		  last_was_optional = -1;
		}

		if ( parm_number == protocol->header_entries() )
		{
		  output << endl << left << setw(maxlen+strlen(indent)+1)  << "Parameter" <<
				  	  	  	  	  	  	  	  	  	  	  setw(13) << "Mnemonic" << "Type  Level  Included";

		  // force the opt/man switching code to trigger
		  last_was_optional = -1;
		}

		parm_number++;

		// add a blank line at the change from mand to optl
		if ( last_was_optional != parm_optional )
		  output << endl;

		if ( ! ( next_group.get_properties() & Protocol::invisible ) )
		{
		  output << indent << left << setw(maxlen+1) << next_group.description();
		  output << setw(13) << next_group.name();

		  if ( parm_optional )
			output << "Optl  ";
		  else
			output << "Mand  ";

		  output << "[" << in_mi->decode_level_code( index )
				 << "/" << out_mi->decode_level_code( index ) << "] ";

		  if ( parm_optional )
			output << "Maybe";
		  else
		  if ( (parm_number-1) >= protocol->header_entries() )
			output << "Yes";

		  output << endl;
		}

		last_was_optional = parm_optional;
	}

	output << right << endl;

	if ( !in_mi->action.isNull() )
	{
	  output << "Message action" << endl;
	  output << "    " << in_mi->action << endl;

	  // A rather more roundabout way but which tests GAL's interface for message action...
	  //
	  // char action[GAL_MAX_LENGTH_ACTION];
	  // memset( action, 0, GAL_MAX_LENGTH_ACTION );
	  // typedef void (*msg_action_proc_type)(int, char *, int);
	  // msg_action_proc_type msg_action_proc = (msg_action_proc_type) Gal_GetMsgAction();
	  // (*msg_action_proc) ( msg_index, action, GAL_GET_ACTION );
	  // output << "	" << action << endl;

	  output << endl;
	}

	output << left << setw(maxlen+strlen(indent)+1) << "Decode level:" << setw(13) << "[ <incoming> / <outgoing> ] " << endl;
	output << left <<= indent << setw(maxlen+1) << "<incoming> or <outgoing>:";
	output << left << setw(13-strlen(indent)) << "(S)uppress, (N)ame, (O)ctet," << endl;
	output << left << setw(raaxlen+strlen(indent)+1) << setfill(' ') << "" << "(F)ield,    (H)ex," << endl;
	output << left << setw(maxlen+strlen(indent)+1) << setfill(' ') << "" << "(-)Disabled/Overriden." << endl;
	output << endl;
	output << indent << "For further help on parameter, type \"man def_<parm_mnemonic>\"" << endl;
	output << endl << flush;

	output << ends;
	cli->Tcl.SetResult( output.str(), TCL_DYNAMIC );

	return ATP_OK;
}

int Genie3_cli::MessageMnemonic( const char *name, int *rc )
{
	int index = cli->protocol->message_type_index( name );

	if ( index < 0 )
	  *rc = EXIT_FAILURE;
	else
	  *rc = EXIT_SUCCESS;

	return index;
}

int Genie3_cli::ParameterMnemonic( const char *name, int *rc )
{
	int index = -1;

	try
	{
		index = cli->protocol->group_index( name );

		if ( index < 0 )
		  *rc = EXIT_FAILURE;
		else
		  *rc = EXIT_SUCCESS;
	}
	catch( Protocol::InternalError )
	{
		if ( rc != 0 )
		  *rc = EXIT_FAILURE;
	}

	return index;
}

int Genie3_cli::DecodeMessageHandler( int dir, int targets, int level, int msg, int parm )
{
#if _DEBUG_UI_GAL
	cerr << "DecodeMessageHandler("
			  << " dir: " << dir << ", targets: " << targets << ", level: " << level << ", msg: " << msg << ", parm: " << parm
			  << ")" << endl;
#endif

	return cli->DecodeMessage( dir, targets, level, msg, parm );
}

int Genie3_cli::MessageActionHandler( int msg, char *action, int op )
{
#if _DEBUG_UI_GAL
	cerr << "MessageActionHandler("
			  << " msg: " << msg << ", action: " << action << ", op: " << op
			  << ")" << endl;
#endif

	return cli->MessageAction( msg, action, op );
}

int Genie3_cli::DecodeMessage( int direction, int targets, int level, int msg, int parm )
{
	int rc = EXIT_SUCCESS;

	if ( direction & GAL_INCOMING_DECODE )
	  rc = DecodeMessage( incoming_msg_info, targets, level, msg, parm );

	if ( rc != EXIT_SUCCESS )
	  return rc;

	if ( direction & GAL_OUTGOING_DECODE )
	  rc = DecodeMessage( outgoing_msg_info, targets, level, msg, parm );

	return rc;
}

int Genie3_cli::DecodeMessage( MessageInfo *info, int targets, int level, int msg, int parm )
{
	int m, first_msg, last_msg = 0;
	int p, first_parm, last parm = 0;

	// Determine message types to be decoded. User entered "*" or <mnemonic>.
	if ( targets & GAL_ALL_MSG )
	{
	  first_msg = 0;
	  last_msg = protocol->message_type_entries() - 1;
	}
	else
	{
	  // msg is internal index for message type
	  // actual numerical H0/H1 message type value is not calculated, too complicated
	  first_msg = last_msg = msg;
	}

	// Set level of decoding.
	if ( targets & GAL_MSG_UNIT ) // unit means decode everything
	{
	  for ( m = first_msg; m <= last_msg; m++ )
	  {
		 info[m].message_overrides_parameter = 1;
		 info[m].decode = level;
		 // cout << "DecodeMessage(): [" << m << "] " << protocol->message_type_name( m ) << endl;
	  }
	}
	else
	if ( targets & GAL_MSG_HDR )
	{
	  // Can't use header_entries() to index into parm_info because it's just a convenience sorted list.
	  // Only accurate way is to iterate into protocol.

	  Protocol::iterate_message_types next_msg( protocol );

	  for ( m = first_msg; m <= last_msg; m++ )
	  {
		 info[m].message_overrides_parameter = 0;
		 info[m].decode = GAL_DECODE_NONE;

		 // position the iterator to this msg
		 const char *msg_name = protocol->message_type_name( m ) ;
		 next_msg( protocol->message_type_index( msg_name ) );

		 // cout << "DecodeMessage(): GAL_MSG_HDR - [" << m << "] " << msg_name << endl;

		 Protocol::iterate_groups_of_message_type next_group( protocol, msg_name );

		 while ( next group() )
		 {
			 // see if field group contains field(s) which is/are part of the message header
			 int is_part_of_header = 0;
			 Protocol::iterate_fields_of_group next_field( protocol, next_group.name() );
			 while ( next_field() )
			 {
				 if ( next_field.get_properties() & Protocol::is_header )
				   is_part_of_header = 1;
			 }

			 if ( is_part_of_header )
			 {
			   const char *name = next_group.name().data();
			   int index = protocol->group_index( name );

			   // cout << "    DecodeMessage(): GAL_MSG_HDR - " << name;
			   // cout << " [" << index << "] " << protocol->get_group_name( index );
			   // cout << " decode level = " << level << endl;

			   info[m].parm_info[ index ].decode = level;
			 }
		 }
	  }
	}
	else
	{
		// Determine parameter(s) to be decoded.
		if ( targets & GAL_ALL_PARM ) // user entered "parm *"
		{
		  first_parm = 0;
		  last_parm = protocol->group_entries() - 1;
		}
		else
		{
		  first_parm = last_parm = parm;

		  // Check that parm exists for this particular message,
		  if ( first_msg == last_msg )
		  {
			const char *msg_name = protocol->message_type_name( msg );
			const char *parm_name = protocol->get_group_name( parm );
			if ( ! protocol->is_parm_of_msg( msg_name, parm_name ) )
			  return EXIT_FAILURE;
		  }
		}

		for ( m = first_msg; m <= last_msg; m++ )
		{
		   info[m].message_overrides_parameter = 0;
		   info[m].decode = GAL_DECODE_NONE;

		   // cout << "DecodeMessage(): [" << m << "] " << protocol->message_type_name( m ) << endl;

		   for ( p = first_parm; p <= last_parm; p++ )
		   {
			  info[m].parm_info[p].decode = level;
			  // cout << "    DecodeMessage(): [" << p << "] " << protocol->get_group_name( p ) << endl;
		   }
		}
	}

	return EXIT_SUCCESS;
}

int Genie3_cli::MessageAction( int msg, char *action, int op )
{
	MessageInfo *mi = &incoming_msg_info[msg];

	// mi->action is an RWCString, operator=() copies string

	switch( op )
	{
		case GAL_SET_ACTION:
		{
			if ( action == 0 )
			  mi->action = "";
			else
			  mi->action = action;

			break;
		}
		case GAL_GET_ACTION:
		{
			if ( action != 0 )
			{
			  strcpy( action, mi->action.data() );
			}

			break;
		}
		default:
			throw Genie3_InternalError( "Unknown Message Action op code %d", op );
	}

	return EXIT_SUCCESS;
}

Genie3_cli::MessageInfo::MessageInfo()
{
	parm_info = 0;
}

void Genie3_cli::MessageInfo::init( Protocol *p, int override, int level )
{
	if ( parm_info )
	  throw Genie3_InternalError("MessageInfo::init has already been called");

	message_overrides_parameter = override;
	decode = level;

	int grp_entries = p->group_entries();
	parm_info = new ParameterInfo[grp_entries];
}

Genie3_cli::ParameterInfo::ParameterInfo(void)
{
	decode = GAL_DECODE_FIELDS;
}

char Genie3_cli::MessageInfo::decode_level_code( int parm_index )
{
	// see if the message setting overrides the parameter settings
	if ( message_overrides_parameter )
	  // overridden
	  return '-';
	else
	  // parameter setting will be used
	  return parm_info[ parm_index ].decode_level_code();
}

char Genie3_cli::MessageInfo::decode_level_code()
{
	if ( message_overrides_parameter )
	  switch( decode )
	  {
	  	  case GAL_DECODE_NONE:		return 'S';
	  	  case GAL_DECODE_NAME:		return 'N';
	  	  case GAL_DECQDE_OCTETS:	return 'O';
	  	  case GAL_DECODE_FIELDS:	return 'F';
	  	  case GAL_DECODE_HEX:		return 'H';
	  	  default:
	  		  return '?';
	  }
	else
	  return '-';
}

int Genie3_cli::MessageInfo::decode_level( int parm_index )
{
	if ( message_overrides_parameter )
	  return decode;
	else
	  return parm_info[parm_index].decode;
}

char Genie3_cli::ParameterInfo::decode_level_code()
{
	switch( decode )
	{
		case GAL_DECODE_NONE:	return	'S';
		case GAL_DECODE_NAME:	return	'N';
		case GAL_DECODE_OCTETS:	return	'O';
		case GAL_DECODE_FIELDS:	return	'F' ;
		case GAL_DECODE_HEX:	return	'H';
		default:
			return '?';
	}
}

void Genie3_cli::Format( MessageInfo *info, bit_string *msg, user_variables *uv, ostream *output )
{
	RWCString msg_name;
	RWCString msg_type_uvname;

	try {
		protocol->Decode( msg, uv );
	}
	catch (...)
	{ }

	msg_type_uvname = Protocol::MessageTypeName();

	if ( uv->exists( msg_type_uvname ) )
	  msg_name = uv->value( msg_type_uvname )->uv_bits->fetch_str();

	if ( msg_name.isNull() )
	{
	  bit_string unknown_name( "UNKNOWN" );
	  uv->define( msg_type_uvname, unknown_name, unknown_name.length() * 8,
			  	  Protocol::simple. Protocol::ff_ascii, "Unknown Message Type" );

	  *output << "Message type: " << Tbf_Video(STANDOUT, "UNKNOWN") << endl;
	  *output << *msg << endl;
	}
	else
	{
	  MessageInfo *fmt_msg_info = &info[ protocol->message_type_index( msg_name ) ];
	  if ( fmt_msg_info->message_overrides_parameter && fmt_msg_info->decode != GAL_DECODE_FIELDS )
		switch( fmt_msg_info->decode )
		{
			case GAL_DECODE_NONE:	// suppress
					break;
			case GAL_DECODE_NAME:	// name only
					*output << "Message Type: " << Tbf_Video(STANDOUT, msg_name) << endl;
					break;
			case GAL_DECODE_OCTETS: // octet format is not supported - assume field
			// case GAL_DECODE_FIELDS:	// format the fields
					// handled in the else below
			case GAL_DECODE_HEX:
					*output << "Message Type: " << Tbf_Video(STANDOUT, msg_name) << endl;
					*output << *msg << endl;
		}
	  else
	  {
		*output << "Message Type: " << Tbf_Video(STANDOUT, msg_name) << endl;

		Genie3_Format pf( protocol, msg, fmt_msg_info, output );
		pf.decode_not_required();
		protocol->Format( msg, uv, &pf );

		pf.all_done();
	  }
	}
}

void Genie3_cli::Format( bit_string *msg, user_variables *uv, const char *ui_group_name, ostream *output )
{
	Genie3_Format pf( protocol, msg, parm_format_msg_info, output );
	protocol->Format( msg, uv, &pf, ui_group_name );
}

static int label_depth = 0;
static int label_depth_output = 0;
static const char *labels[32];

Genie3_Format::Genie3_Format( Protocol *p, bit_string *msg, Genie3_cli::MessageInfo *info, ostream *os )
							: ProtocolFormat( os ),
							  fmt last group (""),
							  fmt_group("")
{
	protocol = p;
	fmt_hex_start = 0;
	fmt_msg = msg;
	fmt_msg_info = info;
}

Genie3_Format::~Genie3_Format()
{ }

void Genie3_Format::format_field
(
	enum Protocol::field_type field_type, // the storage type of this field
	enum Protocol::field_format field_format,	// how to format this field
	bit_string *value,				// the value of this field
	int start_pos,					// the starting bit position of this field
	int value_size,					//	the bit size of this field
	const char	*ui_group,			//	the name of the grouping
	const char	*variable_name,		//	variable that holds this	value
	const char	*long_description,	//	full description of this	field
	const char	*value_description,	//	name of this value
	Protocol::CustomUIOps *custom_ops
)
{
	// default this and last decode levels
	int this_decode = GAL_DECODE_FIELDS;
	int last_decode = GAL_DECODE_FIELDS;

	if ( ui_group != NULL && ui_group[0] != '\0' )
	  // fetch this fields decode level
	  this_decode = fmt_msg_info->decode_level( protocol->group_index( ui_group ) );

	if ( !fmt_last_group.isNull() )
	  last_decode = fmt_msg_info->decode_level( protocol->group_index( fmt_last_group ) );

	if ( fmt_last_group != ui_group )
	{
	  if ( last_decode == GAL_DECODE_HEX )
	  {
		bit_string hex_msg( *fmt_msg );
		*output << "Hex for " << fmt_last_group << " from bit " << fmt_hex_start << " to " << start_pos << endl;

		hex_msg.set_position( fmt_hex_start );
		for ( ; fmt_hex_start < start_pos; fmt_hex_start += 8 )
		   *output << " " << hex << setw(2) << hex_msg.fetch_lsb( min( 8, start_pos-fmt_hex_start ) );
		*output << dec << endl;
	  }

	  switch( this_decode )
	  {
		case GAL_DECODE_NAME:
			while ( label_depth_output < label_depth )
				ProtocolFormat::format_labelling_prologue( 0,	labels[label_depth_output++] );

			*output << "    " << setw(35) << long_description << "--------" << ui_group << endl;
			break;
		case GAL_DECODE_HEX:
			fmt_hex_start = start_pos;
	  }
	}

	if ( this_decode == GAL_DECODE_FIELDS )
	{
	  while ( label_depth_output < label_depth )
		  ProtocolFormat::format_labelling_prologue( 0,	labels[label_depth_output++] );

	  // pass on to user_interface
	  ProtocolFormat::format_field
	  (
		field_type,
		field_format,
		value,
		start_pos,
		value_size,
		ui_group,
		variable_name,
		long_description,
		value_description,
		custom_ops
	  );
	}

	if ( ui_group != NULL && ui_group [0] != '\0' )
	  fmt_last_group = ui_group;
}

void Genie3_Format::format_labelling_prologue( int, const char *, const char *long_description )
{
	labels[label_depth++] = long_description;
}

void Genie3_Format::format_labelling_epilogue( int, const char *, const char * )
{
	label_depth--;
	while ( label_depth_output > label_depth )
		ProtocolFormat::format_labelling_epilogue( 0, "", labels[label_depth_output--] );
}

void Genie3_Format::all_done(void)
{
	if ( !fmt_last_group.isNull() )
	{
	  int last_decode = fmt_msg_info->decode_level( protocol->group_index( fmt_last_group ) );
	  if ( last_decode == GAL_DECODE_HEX )
	  {
		int start_pos = fmt_msg->position(1);
		*output << "Hex for " << fmt_last_group << " from bit " << fmt_hex_start << " to " << start_pos << endl;

		fmt_msg->set_position( fmt_hex_start );
		for ( ; fmt_hex_start < start_pos; fmt_hex_start += 8 )
		   *output << " " << hex << setw(2) << fmt_msg->fetch_lsb ( min( 8, start_pos-fmt_hex_start ) );
		*output << dec << endl;
	  }
	}
}

receive_cmd::Message::Message(void)
{
	next = this;
	prev = this;

	data = NULL;
	linkno = 0;
	length = 0;
	memset(&timestamp, 0, sizeof(time_t));
}

receive_cmd::Message::Message( int a_linkno, unsigned char *a_data, int a_length, time_t _timestamp )
{
	next = NULL; prev = NULL;

	linkno = a_linkno;
	length = a_length;
	data = (unsigned char *)malloc( length );
	memcpy( data, a_data, length );
	timestamp = _timestamp;
};

receive_cmd::Message::~Message()
{
	assert( next == NULL );
	assert( prev == NULL );

	if ( data )
	  free( (void *)data );
}

void receive_cmd::Message::Insert( Message *position )
{
	next = position->next;
	position->next = this;
	prev = position;
	next->prev = this;
}

void receive_cmd::Message::Remove( void )
{
	// unlink from queue
	next->prev = prev;
	prev->next = next;

	// zap pointers
	next = prev = NULL;
}

int Genie3_cli::QueryProcWrapper( ClientData clientData, Tcl_Interp *interp, int argc, char *argv[] )
{
	int result = (*OldQueryProc)(clientData, interp, argc, argv);

	if ( argc == 2 && (strcasecmp(argv[1], "info") == 0) )
	{
	  if ( interp->result != 0 && strstr(interp->result, G3O_tool_name) == 0 )
	  {
		Tcl_ResetResult(interp);
		Tcl_AppendResult(interp, G3O_tool_name, "\n", G3O_BNR_Copyright, (char *)0);
	  }
	}

	return result;
}

Atp_Result filter_cic_cmd::cmd( int, char ** )
{
	enum cic_filter action = (enum cic_filter) Atp_Num( "action" );
	enum cic_filter option = (enum cic_filter) Atp_Num( "option" );
	ostrstream output;
	Atp_DataDescriptor cic_series;
	Atp_NumType lower_cic, upper_cic = 0;

	if ( action == list )
	{
	  switch( option )
	  {
	  	  case range:
	  		  list_cic_filter_settings( Atp_Num("lower_cic"), Atp_Num("upper_cic"), output );
	  		  break;
	  	  case series:
	  		  cic_series = Atp_RptBlockDesc("cics");
	  		  list_cic_filter_settings( series_count, (int *)cic_series.data, output );
	  		  break;
	  	  case all:
	  	  default:
	  		  list_cic_filter_settings( 0, cic_max_value, output );
	  		  break;
	  }
	}
	else
	  switch( option ) // include or exclude
	  {
	  	  case range:
	  	  {
	  		  lower_cic = Atp_Num("lower_cic");
	  		  upper_cic = Atp_Num ("upper_cic");
	  		  set_range_of_cics( lower_cic, upper_cic, action );
	  		  list_cic_filter_settings( lower_cic, upper_cic, output );
	  		  break;
	  	  }
	  	  case series:
	  	  {
	  		  cic_series = Atp_RptBlockDesc("cics");
	  		  // Use series_count instead of cic_series.count as vproc may have removed duplicates
	  		  for ( int i = 0; i < series_count; i++ )
	  			 set_individual_cic( ((int *)(cic_series.data))[i], action );
	  		  list_cic_filter_settings( series_count, (int *)cic_series.data, output );
	  		  break;
	  	  }
	  	  case all:
	  	  {
	  		  set_range_of_cics( 0, cic_max_value, action );
	  		  list_cic_filter_settings( 0, cic_max_value, output );
	  		  break;
	  	  }
	  	  default:
	  		  break;
	  }

	output << flush << ends;

	ui->Tcl.SetResult( output.str(), TCL_DYNAMIC );

	return ATP_OK;
}

void filter_cic_cmd::set_range_of_cics( int lower, int upper, enum cic_filter action )
{
	for ( register int index = lower; index <= upper; index++ )
	   (*cic_settings)[ index ] = ( action == exclude ) ? 0 : 1;
}

void filter_cic_cmd::set_individual_cic( int cic, enum cic_filter action )
{
	(*cic_settings)[ cic ] = ( action == exclude ) ? 0 : 1;
}

int filter_cic_cmd::get_cic_filter_setting( int cic )
{
	int rc = 0;
	rc = (*cic_settings)[ cic ];
	return rc;
}

void filter_cic_cmd::list_cic_filter_settings( int lower, int upper, ostream& output )
{
	register int i, j = 0;
	register int curr_setting, next_setting = 0;
	register char *cic_state = 0;
	int column = 1;

	if ( upper == lower )
	  output << endl << "CIC filter setting for " << dec << lower << ":" << endl;
	else
	  output << endl << "CIC filter settings between " << dec << lower << " and " << upper << ":" << endl;

	for ( i = j = lower; i <= upper; i++ )
	{
	   curr_setting = (*cic_settings)[ i ];
	   for ( j = i + 1; j <= upper; j++ )
	   {
		  next_setting = (*cic_settings)[ j ];
		  if ( curr_setting == next_setting )
			continue;
		  else
			break;
	   }

	   if ( column >=70 ) { output << endl; column = 1; }

	   output << " " << dec << i;

	   column += ( 1 + calc_dec_width( i ) );

	   if ( ( j - 1 - i ) >= 2 )
	   {
		 output << " - ";
		 i = j - 1;
		 output << dec << i;
		 column += ( 3 + calc_dec_width( i ) );
	   }

	   cic_state = ((*cic_settings)[ i ] == TRUE) ? "(INCL)" : "(EXCL)";

	   output << " " << cic_state;
	   column += 7;

	   if ( i+1 >= upper )
		 output << ".";
	   else
		 output << ",";
	   column += 1;
	}

	output << endl << flush;
}

void filter_cic_cmd::list_cic_filter_settings( int count, int *series, ostream& output )
{
	int j = 0;
	char *cic_state = 0;
	int column = 1;

	output << endl << "CIC filter settings for specified series:" << endl;

	for ( int i = 0; i < count; i++ )
	{
	   j = series[ i ];
	   cic_state = ((*cic_settings)[ j ] == TRUE) ? "(INCL)" : "(EXCL)";

	   if ( column >= 80 )
	   {
		 output << endl;
		 column = 1;
	   }

	   output << " " << dec << j << " " << cic_state;
	   column += ( 7 + calc_dec_width( j ) );

	   if ( i+1 >= count )
		 output << ".";
	   else
		 output << ",";
	   column += 1;
	}

	output << endl << flush;
}

Atp_Result preview_msg_cmd::cmd( int argc, char **argv )
{
	return ui->get_send_msg_cmd()->do_cmd( argc, argv, 1 );
}

Atp_Result set_auto_cic_cmd::cmd( int, char ** )
{
	enum auto_cic_indicators mode = (enum auto_cic_indicators)Atp_Num("mode");
	enum auto_cic_indicators option = (enum auto_cic_indicators)Atp_Num("options");

	int i, min, max = 0;

	if ( option == auto_cic_all_links )
	{
	  min = 1;
	  max = num_of_links;
	}
	else
	if ( option == auto_cic_one_link )
	{
	  min = max = Atp_Num("linkno");
	}
	else
	{
	  ui->Tcl.SetResult( "Invalid Auto-CIC option", TCL_STATIC );
	  return ATP_ERROR;
	}

	switch ( mode )
	{
		case auto_cic_on:
			for ( i = min; i <= max; i++ )
			   set_auto_cic_flag( i );
			break;
		case auto_cic_off:
			for ( i = min; i <= max; i++ )
			   unset_auto_cic_flag( i ) ;
			break;
		case auto_cic_reset:
			for ( i = min; i <= max; i++ )
			   reset_auto_cic_value( i );
			break;
		case auto_cic_query:
		default:
		{
			ostrstream output;
			for ( i = min; i <= max; i++ )
			   display_cic_value( i, output );
			output << flush << ends;
			ui->Tcl.SetResult( output.str(), TCL_DYNAMIC );
			break;
		}
	}

	return ATP_OK;
}

Atp_Result disp_header_cmd::cmd( int, char ** )
{
	ostrstream output;
	user_variables uv;
	int link_no = Atp_Num( "link" );

	// Get the header parameters first,
	if ( ui->per_link_user_variables[ link_no ] )
	  uv = *(ui->per_link_user_variables[ link_no ]);
	else
	{
	  ostrstream output;
	  output << "Header parameters for link " << link_no << " are not set." ;
	  output << flush << ends;
	  ui->Tcl.SetResult( output.str(), TCL_DYNAMIC );
	  return ATP_ERROR;
	}

	try
	{
		int fw = 60; // field width

		output << endl << "Message Header for logical link " << link_no << ":" << endl << endl;

		output << setw(fw) << left << "Header Component";
		output << setw(12) << left << "Mnemonic";
		output << setw( 8) << left << "Value";
		output << endl << endl;

		// Protocol::iterate_field_groups next_group( ui->protocol );
		// This is NOT IN TRANSMISSION ORDER 1
		// So, we cheat by using a valid message type name.
		// Then, iterate instead of encode which will complain of any undefined parameters.
		// i.e. same approach as QueryMessage()

		Atp_KeywordType *msgtype_table = &ui->msg_types[0];
		const char *first_msg_type = msgtype_table->keyword;
		Protocol::iterate_groups_of_message_type nextgroup( ui->protocol, first_msg_type );

		while ( next_group() )
		{
			if ( ! ( next_group.get_properties() & Protocol::is_header ) )
			  continue;

			RWCString name = next_group.name();
			RWCString desc = next_group.description();

			output << "    " << setw(fw-4) << left << desc;
			output << setw(12) << left << name << endl;

			Protocol::iterate_fields_of_group next_field( ui->protocol, name.data() );
			while ( next_field() )
			{
				output << "	" << setw(fw-8+12) << left << next_field.long_description();
				const char *var_name = next_field.variable_name();
				output.width(5);
				output.flags(ios::right);

				if ( uv.exists( var_name ) )
				  uv.print_value( output, uv.value( var_name ) );

				output.width(0); // reset
				output.flags(ios::left);

				output << endl;
				// output << setw(12) << left << next_field.variable_field()) << endl;
			}

			output << endl;
		}

		if ( ui->supports_auto_cic() )
		{
		  int cic = 0;
		  int auto_cic_flag = ui->get_auto_cic_cmd()->get_auto_cic_flag( link_no );
		  if ( uv.exists( ui->get_cic_uv_name() ) )
		  {
			cic = ui->get_auto_cic_cmd()->get_auto_cic_value( link_no );
			output << endl << " Auto-CIC for link " << link_no << " is ";
			output << ((auto_cic_flag) ? "ON" : "OFF") << "." << endl;
		  }
		}

		output << flush << ends;
		ui->Tcl.SetResult( output.str(), TCL_DYNAMIC );
	}
	catch (...)
	{
		cout << "disp_header caught exception" << endl;
		return ATP_ERROR;
	}

	return ATP_OK;
}

Atp_Result status_cmd::cmd( int, char ** )
{
	ostrstream output;
	Gcm_Result gcm_result;
	const char *live_hostname = Gal_GetLiveHostName();
	const char *loop_hostname = Gal_GetLoopHostName();
	int live_in_use = 6;
	int loopback_in_use = 0;

	gcm_result = Gcm_Identify( 0 );

	// Detect if connected to LIVE or LOOPBACK GCM
	if ( gcm_result.rc == GCM_OK )
	{
	  int rc = ui->Tcl.Eval("query system");
	  if ( rc == TCL_OK )
	  {
		const char *result = ui->Tcl.GetResult();
		if ( strstr( result, live_hostname ) != 0 && strstr( result, "LIVE" ) != 0 )
		  live_in_use = 1;
		else
		if ( strstr( result, loop_hostname ) != 0 && strstr( result, "LOOPBACK" ) != 0 )
		  loopback_in_use = 1;
	  }
	}

	output << endl;
	output << "User Status:" << endl;
	output << "===========" << endl << endl;

	output << left << setw(50) << "    Special video effects (fx) are turned: " << setw(1O) <<
								  Tbf_Video( STANDOUT, Tbf_GetSpecialVideo() ? "ON" : "OFF" ) << endl << endl;

	// Check out GCM client status
	output << left << setw(50) << "    Live GCM host: " <<
					  setw(10) << Tbf_Video( STANDOUT, live_hostname ) <<
					  ( (live_in_use) ? " [ IN USE ]" : "" ) << endl << endl;
	output << left << setw(50) << "    Looped-back GCM host: " <<
					  setw(10) << Tbf_Video( STANDOUT, loop_hostname ) <<
					  ( (loopback_in_use) ? " [ IN USE ]" : "" ) << endl << endl;

	if ( gcm_result.rc != GCM_OK )
	  output << "    " << gcm_result.string << endl;
	else
	{
	  if ( strlen(gcm_result.string) == 0)
		output << left << setw(50) << "    You are not yet identified." << endl;
	  else
		output << left << setw(50) << "    You are identified as: " << setw(10) << Tbf_Video( STANDOUT, gcm_result.string ) << endl;
	}

	output << endl << flush << ends;
	ui->Tcl.SetResult( output.str(), TCL_DYNAMIC );

	return ATP_OK;
}
