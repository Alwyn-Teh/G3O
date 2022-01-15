/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+*******************************************************************

	Module Name:		g3ouih.h (was ui.h)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Command Line Interface Framework Class

	History:
		Who			When				Description
	----------	-------------	----------------------------------
	Alwyn Teh	94Q4 - 95Q2		Initial Creation
	Alwyn Teh	14 June 1995	Introduce batch or interactive
								mode for run().
	Alwyn Teh	21 June 1995	Add FieldValueMeanings argument
								to end of field_of_group_init.
	Alwyn Teh	3rd July 1995	Make protocol static because
								static GAL callback needs it.
								If multiple protocols needed,
								then whole world needs to be
								changed (incl. GAL and ATP).
								If so, then should extend
								GAL interface with clientdata
								for callbacks.
	Alwyn Teh	11-12 July 1995	Add GCM MTP capability.
	Alwyn Teh	17-24 July 1995	Add user Tcl variables.
	Alwyn Teh	24-28 July 1995	Do CIC filtering.
	Alwyn Teh	28 July 1995	Implement preview_msg command.
	Alwyn Teh	31 July 1995	Implement disp_header and
								set_auto_cic commands.
	Alwyn Teh	16 August 1995	Add status command.
	Alwyn Teh	19 August 1995	Add per-link user variables.
	Alwyn Teh	23 August 1995	Add G3O_StdinHandler() .

********************************************************************-*/

#ifndef __UI_HEADER_INCLUDED_
#define __UI_HEADER_INCLUDED_

#include <time.h>
#include <limits.h>
#include <iostream.h>
#include <iomanip.h>
#include <strstream.h>
#include <rw/cstring.h>

#include <gal.h>

#include <g3ouidbh.h>

#include <g3oproth.h>
#include <g3opforh.h>
#include <g3odaryh.h>

#include <g3oatpxh.h>
#include <g3otclxh.h>
#include <g3oslpxh.h>

#include <g3otclvh.h>

class parmdef;

extern char *G3O_tool_name;
extern char *G3O_BNR_Copyright;
extern char *G3O_CCITT_CCS7;

extern int  __verbose;
extern int  __debug;

//
//	Exception classes
//
class Genie3_Exception
{
public:
		Genie3_Exception(void);
		int code;
};

class Genie3_InternalError : public Genie3_Exception
{
public:
		RWCString message;
		Genie3_InternalError( const char *ra );
		Genie3_InternalError( const char *m, int a );
		Genie3_InternalError( const char *m, const char *a, const char *b = 0, const char *c = 0 );
};

class Genie3_SyntaxError : public Genie3_Exception
{
public:
		RWCString message;
		Genie3_SyntaxError( const char *m );
		Genie3_SyntaxError( const char *m, int a );
		Genie3_SyntaxError( const char *m, const char *a );
};

class bit_string;

// Built-in commands
class var_cmd;
class send_msg_cmd;
class send_hex_cmd;
class def_parm_hex_cmd;
class disp_parm_cmd;
class set_receive_cmd;
class receive_cmd;
class reset_msg_queue_cmd;
class filter_cic_cmd;
class preview_msg_cmd;
class disp_header_cmd;
class set_auto_cic_cmd;
class status_cmd;

// Miscellaneous function(s)
int calc_dec_width( int value );

class Genie3_cli
{
public:
		Genie3_cli (Protocol *p, const char *toolname, const char *capability, const char *prompt);
		~Genie3_cli();

		// forward class definitions
		class MessageInfo;

		void Format( MessageInfo *info, bit_string *msg, user_variables *uv, ostream *out );
		void Format( bit_string *msg, user_variables *uv, const char *ui_group_name, ostream *out );

		int SetPrompt(const char *prompt);

		enum run_mode { interactive, batch };

		int run( run_mode = interactive, const char *filename = 0 );

		int init_protocol(Protocol *p);

		const char * get_toolname(void) { return (const char *)toolname; }

		int message_type
		(
			Protocol *p,
			const char *msg_type_name,
			const char *msg_type_desc
		);

		int field_group
		(
			Protocol *p,
			const char *group_name,
			const char *group_desc,
			int properties = 0
		);

		int field_of_group
		(
			Protocol *p,
			const char *variable_name,
			const char *group_name,
			const char *field_name,
			enum Protocol::field_type FieldType,
			enum Protocol::field_format FieldFormat,
			const char *long_description,
			const char *controlled_by,
			int min_value,
			int max_value
		);

		user_variables * uv( const char * name = 0 );

		void uv_set( const char *name );
		void uv_del( const char *name );
		void uv_print( const char *name );
		void uv_copy( const char *from, const char *to );
		int uv_exists( const char *name );

		// per-link user_variables e.g. header settings for each link
		user_variables * per_link_user_variables[GAL_MAX_LINKS+1];

		DArray<Atp_KeywordType> msg_types;
		DArray<Atp_KeywordType > parameters;

		parmdef *tmp_parmdef;

		Tcl_cl	Tcl;

		class ParameterInfo
		{
			public:
					ParameterInfo(void);
					char decode_level_code();
					int decode;
					int properties;
		};

		class MessageInfo
		{
			public:
					MessageInfo();
					void init( Protocol *p, int o, int d );
					RWCString action;

					char decode_level_code();
					char decode_level_code( int parm_index );

					int decode_level( int parm_index );

					int message_overrides_parameter;
					int decode;

					ParameterInfo *parm_info;
		};

		MessageInfo *parm_format_msg_info;
		MessageInfo *incoming_msg_info;
		MessageInfo *outgoing_msg_info;
		MessageInfo *preview_msg_info;

protected:
		int Init();

		int field_of_group_init
		(
			Protocol *p,
			const char *variable_name,
			const char *group_name,
			const char *field_name,
			enum Protocol::field_type FieldType,
			enum Protocol::field_format FieldFormat,
			const char *long_description,
			const char *controlled_by,
			int min_value,
			int max_value,
			int bit_width,
			const Protocol::Table * FieldValueMeanings = 0,	// ACST.175
			Protocol::CustomUIOps *custom_ops = 0,			// ACST.175
			int properties = 0
		) ;

public:
		static Protocol *protocol;	//be careful - made static for sake of static callbacks for GAL
		static const RWCString default_uv_name;
		static const RWCString rcvmsg_uv_name;

		Atp2Tcl_cl	Atp;
		Slp_cl		Slp;

		Atp_ParmCode type_mapper( Protocol::field_type FieldType, Protocol::field_format FieldFormat );

		void IncomingMessage( int link_no, unsigned char *msg, int length, time_t timestamp = 0,
							  ostream *output = &cout, int script_mode = 0, int output_to_cout = 1 );

		void set_mtp_capability( const char *cap );

		const char * get_mtp_capability( void ) { return curr_capability; }

		RWHashDictionary& get_variables( void ) { return variables; }

		const char *get_cic_uv_name( void ) { return cic_uv_name; }
		int get_cic_bitwidth( void ) { return cic_bitwidth; }
		int supports_cic_filtering( void ) { return use_cic_filtering; }
		int supports_auto_cic( void ) { return use_auto_cic; }
		set_auto_cic_cmd * get_auto_cic_cmd( void ) { return set_auto_cic_cmd_ptr; }
		send_msg_cmd * get_send_msg_cmd( void ) { return sendmsg_Cmd_ptr; }

private:
		static void IncomingMessageHandler( int link_no, unsigned char *msg, int length );
		static Atp_Result QueryMessageHandler( const char *msg_name );
		static Atp_Result ProtocolDescProc( void );
		static Atp_Result ToolDescProc( void );

		// For intercepting stupid fixed "query info" implementation in galcmds.c
		// calling Gal_Tool_Description() instead of using API functions Gal_SetToolDescProc()
		// and Gal_GetToolDescProc() !!!! Should fix GAL when opportunity arises.
		static Tcl_CmdProc *OldQueryProc;
		// static Tcl_CmdProc QueryProcWrapper;
		static int QueryProcWrapper(ClientData, Tcl_Interp *, int, char **);

		static int DecodeMessageHandler( int direction, int targets, int level, int msg, int parm );
		static int MessageActionHandler( int msg, char *action, int op );

		static void G3O_StdinHandler( char *, int *, int * ); // of type ipc_inputcallback

		static int MessageMnemonic( const char *name, int *rc );
		static int ParameterMnemonic( const char *name, int *rc );

		Atp_Result QueryMessage( const char *msg_name );
		Atp_Result QueryMessages( void );

		int DecodeMessage( int direction, int targets, int level, int msg, int parm );
		int DecodeMessage( MessageInfo *info, int targets, int level, int msg, int parm );
		int MessageAction( int msg, char *action, int op );

		send_msg_cmd		*send_msg_Cmd_ptr;
		send_hex_cmd		*send_hex_Cmd_ptr;
		def_parm_hex_cmd	*def_parm_hex_Cmd_ptr;
		disp_parm_cmd		*disp_parm_Cmd_ptr;
		var_cmd				*var_Cmd_ptr;
		set_receive_cmd		*set_receive_cmd_ptr;
		receive_cmd			*receive_cmd_ptr;
		reset_msg_queue_cmd	*reset_msg_queue_cmd_ptr;
		filter_cic_cmd		*filter_cic_cmd_ptr;
		preview_msg_cmd		*preview_msg_cmd_ptr;
		disp_header_cmd		*disp_header_cmd_ptr;
		set_auto_cic_cmd	*set_auto_cic_cmd_ptr;
		status_cmd			*status_cmd_ptr;

		RWCString			current_uv_name;
		user_variables		*current_uv;
		RWHashDictionary	variables;
		user_tcl_variables	*rcvmsg_vars;
		int					use_cic_filtering;
		int					cic_bitwidth;
		const char *		cic_uv_name;
		int					use_auto_cic;

		RWCString			cur_prompt;
		RWCString			prompt_suffix;

		char *				toolname;
		char *				curr_capability;

		//
		// This nasty hack is forced on the good C++ developers by
		// the evil Gal library - don't blame us!
		//
		static Genie3_cli	*cli;
};

class Genie3_Format : public ProtocolFormat
{
public:
		Genie3_Format( Protocol *protocol, bit_string *msg, Genie3_cli::MessageInfo *info, ostream *os );
		virtual ~Genie3_Format();

		virtual void format_field
		(
			enum Protocol::field_type,					// the storage type of this field
			enum Protocol::field_format field_format,	// how to format this field
			bit_string *value,							// the value of this field
			int start_pos,								// the starting bit position of this field
			int value_size,								// the bit size	of this field
			const char *ui_group,						// the name of the grouping
			const char *variable_name,					// variable that holds this value
			const char *long_description,				// full description of this field
			const char *value_description,				// name of this value
			Protocol::CustomUIOps *custom_ops = 0
		);

		virtual void format_labelling_prologue
		(
			int start_pos,
			const char *ui_group,	// the name of the grouping
			const char *long_description
		);

		virtual void format_labelling_epilogue
		(
			int start_pos,
			const char *ui_group,	// the name of the grouping
			const char *long_description
		);

		void all_done(void);

private:
		Protocol *protocol;
		RWCString fmt_last_group;
		int fmt_hex_start;
		bit_string *fmt_msg;	// the message that is being formatted

		// the single msg_info entry for the message being decoded
		Genie3_cli::MessageInfo *fmt_msg_info;

		// the name of the'group being formatted
		RWCString fmt_group;
};

#endif /* __UI_HEADER_INCLUDED_ */
