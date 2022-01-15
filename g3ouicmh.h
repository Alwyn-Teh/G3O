/* EDITION AA02 (REL001) , ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+********************************************************************

	Module Name:		g3ouicmh.h (was uicmd.h)

	Copyright:	BNR Europe Limited, 1994-1995
	Bell-Northern Research
	Northern Telecom / NORTEL

	Description:	Built-In Command Classes

	History:
		Who			When				Description
	----------	--------------	-------------------------------
	Alwyn Teh	94Q4 - 95Q2		Initial Creation
	Alwyn Teh	7th July 1995	Timestamp received messages
	Alwyn Teh	24th July 1995	Implement filter_cic command
	Alwyn Teh	28 July 1995	Implement preview_msg command
	Alwyn Teh	28 July 1995	Implement set_auto_cic command
	Alwyn Teh	28 July 1995	Implement disp_header command
	Alwyn Teh	16 August 1995	Implement status command

********************************************************************—*/

#ifndef __UICMD_HEADER_INCLUDED_
#define __UICMD_HEADER_INCLUDED_

#include <time.h>
#include <iostream.h>

#include <rw/bitvec.h>

#include <g3ouih.h>

#define G3O_COMMAND_ID 4295

class Command
{
public:
		Command(Genie3_cli *);
		virtual ~Command();

		static Atp_Result Atp2Tcl_cb(ClientData, Tcl_Interp *, int, char **) ;
		// calls
		Atp_Result do_cmd(int, char **);
		// calls
		virtual Atp_Result cmd(int, char **) = 0;

		void Create(const char *name, const char *desc, int help_id = 0);

		Genie3_cli	*ui;
		parmdef		*pd;

		int			Command_ID; // so as not to get mixed up with Tcl procs

private:
		static void delete_command( ClientData cd );
};

class send_msg_cmd : public Command
{
public:
		send_msg_cmd(Genie3_cli *);
		Atp_Result cmd( int arge, char **argv );
		Atp_Result do_cmd( int arge, char **argv, int _preview_only = 0 );

private:
		int preview_only;
};

class send_hex_cmd : public Command
{
public:
		send_hex_cmd(Genie3_cli *) ;
		Atp_Result cmd(int, char **);
};

class def_parm_hex_cmd : public Command
{
public:
		def_parm_hex_cmd(Genie3_cli *);
		Atp_Result cmd(int, char **);
};

class disp_parm_cmd : public Command
{
public:
		disp_parm_cmd(Genie3_cli *);
		Atp_Result cmd(int, char **);
};

class receive_cmd : public Command
{
public:
		receive_cmd(Genie3_cli *);
		Atp_Result cmd(int, char **);

		enum receive_mode { automatic = 0, manual };
		enum receive_option { RCV_LINK_NO = 0, RCV_ALL_LINKS };

		void set_mode( enum receive_mode mode, int set_tcl_result_flag = 1 );

		int is_script_mode( void );

		void IncomingMessage( int link_no, unsigned char *msg, int length, time_t _timestamp = 0, ostream *output = &cout );

		class Message
		{
			public:
					Message();
					Message( int linkno, unsigned char *data, int length, time_t _timestamp = 0 );
					~Message();

					void Insert( Message *position );
					void Remove(void);

					Message *next, *prev;

					int linkno;
					unsigned char *data;
					int length;
					time_t timestamp;
		};

		Message queue;

		int reset_queue ( void );

		static Tcl_CmdProc *OldSourceProc;
		static int SourceProcWrapper(ClientData, Tcl_Interp*, int , char **);

private:
		enum receive_mode mode;
		const char *receive_cmd_name;

		int script_mode;
		char *scriptFile;

		// names for parmdef
		char *_timeout;
		char *_link_option;
		char *_linkno;
		char *_rcv_all_links;

		Tcl_CmdProc *Get_TclCmdProc( const char* cmdname );
};

class set_receive_cmd : public Command
{
public:
		set_receive_cmd(Genie3_cli *, receive_cmd *);
		Atp_Result cmd(int, char **);
private:
		receive_cmd *receive_cmd_ptr;
};

class reset_msg_queue_cmd : public Command
{
public:
		reset_msg_queue_cmd(Genie3_cli *, receive_cmd *)
		Atp_Result cmd(int, char **);
private:
		receive_cmd *receive_cmd_ptr;
};

class def_parm_cmd : public Command
{
public:
		def_parm_cmd( Genie3_cli *cli,
					  const char *grp_name, const char *desc,
					  parmdef *p,
					  int props = 0 );

		Atp_Result cmd(int, char **);

		const char *group_name;
		RWCString cmd_name;
		int	properties;
};

class var_cmd : public Command
{
public:
		var_cmd(Genie3_cli *);
		Atp_Result cmd(int, char **);

private:
		enum operator_id
		{
			VAR_SET = 1,
			VAR_DELETE,
			VAR_COPY,
			VAR_PRINT,
			VAR_LIST
		};
		enum print_type
		{
			pt_hex,
			pt_ascii,
			pt_integer
		};

		static char * var_name_vproc	( void *valPtr, Atp_BoolType );
		static char * new_var_name_vproc( void *valPtr, Atp_BoolType isUserValue );

		void set_cmd(	ostrstream *message, const char *name );
		void del_cmd(	ostrstream *message, const char *name );
		void copy_cmd(	ostrstream *message, const char *from, const char *to );
		void print_cmd( ostrstream *message, const char *name );
		void list_cmd(	ostrstream *message, const char *name );

		void parse( const char *to, const char *from );

		RWCString to_v_set;
		RWCString from_v_set, from_v_parm, from_v_part, from_v_name;
};

class filter_cic_cmd : public Command
{
public:
		filter_cic_cmd( Genie3_cli *, int cic_bitwidth = 12 );
		~filter_cic_cmd();

		Atp_Result cmd( int argc, char **argv );

		enum cic_filter
		{
			include, exclude, list,	// filter actions
			range, series, all		// filter options
		};

		void set_range_of_cics( int lower, int upper, enum cic_filter action );
		void set_individual_cic( int cic, enum cic_filter action );

		int get_cic_filter_setting( int cic );

		void list_cic_filter_settings( int lower, int upper,   ostream& output = cout );
		void list_cic_filter_settings( int count, int *series, ostream& output = cout );

		static char * tidyup_series_vproc( void *, Atp_BoolType );

private:
		int cic_bit_width;		// how many bits are there?
		int cic_max_dec_width;	// how many digits in decimal value?
		int cic_max_value;

		RWBitVec *cic_settings;
		static int series_count;
};

class preview_msg_cmd : public Command
{
public:
		preview_msg_cmd( Genie3_cli * );
		Atp_Result cmd( int argc, char **argv );
};

class disp_header_cmd : public Command
{
public:
		disp_header_cmd( Genie3_cli * ) ;
		Atp_Result cmd( int argc, char **argv );
};

class set_auto_cic_cmd : public Command
{
public:
		set_auto_cic_cmd( Genie3_cli *, int no_of_links = 256, int cic_bitwidth =12 );
		~set_auto_cic_cmd( void );

		Atp_Result cmd( int argc, char **argv );

		enum auto_cic_indicators
		{
			auto_cic_on, auto_cic_off, auto_cic_reset, auto_cic_query,
			auto_cic_one_link, auto_cic_al1_1inks
		};

		struct auto_cic_setting
		{
			int on_off_flag;
			int cic_value; // previous incoming message's CIC value
		};

		int set_auto_cic_flag( int link_no );
		int unset_auto_cic_flag( int link_no );
		int reset_auto_cic_value( int link_no );

		int set_auto_cic_value( int link_no, int cic_value );

		int get_auto_cic_flag( int link_no );
		int get_auto_cic_value( int link_no );

		int display_cic_value ( int link_no, ostream& output );

private:
		int num_of_links;
		int max_linkno_dec_width;
		int cic_bit_width;
		int max_cic_value;
		int max_cic_dec_width;
		struct auto_cic_setting *auto_cic_settings;
};

class status_cmd : public Command
{
public:
		status_cmd( Genie3_cli * );
		Atp_Result cmd( int argc, char **argv );
};

#endif /* __UICMD_HEADER_INCLUDED_ */
