/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+********************************************************************

	Module Name:		g3ouivar.cxx (was ui_var.cxx)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Built-in command "var" for user variables

	History:
		Who			When				Description
	-----------	------------	--------------------------
	Alwyn Teh	94Q4 - 95Q2		Initial Creation
	Barry Scott	95Q2			Implement methods

******************************************<<**************************-*/

// uivar.cxx

#include <strstream.h>

#include <g3ouih.h>
#include <g3ouicmh.h>

#include <g3obitsh.h>
#include <g3ouvarh.h>
#include <g3opmdfh.h>

#include <gal.h>

#include <rw/cstring.h>
#include <rw/ctoken.h>
#include <rw/regexp.h>

char * var_cmd::var_name_vproc( void * /*valPtr*/, Atp_BoolType )
{
	// may wish to restrict string symbol set
	return 0;
}

char * var_cmd::new_var_name_vproc( void * /*valPtr*/, Atp_BoolType /*isUserValue*/ )
{
	// may wish to restrict string symbol set
	return 0;
}

// var command
var_cmd::var_cmd( Genie3_cli *cli ) : Command( cli )
{
	static Atp_KeywordType print_keyword [] =
	{
		{"int", pt_integer, "print as integer"},
		{"hex", pt_hex, "print as hex"},
		{"ascii", pt_ascii, "print as ASCII"},
		{NULL}
	};

	pd = new parmdef;

	pd->BeginParms();
	  pd->BeginChoice( "operations", "operations on user variables sets", 0 );
	    pd->BeginCase( "set", "set current user variables set name", VAR_SET );
	      pd->StrDef( "name", "user variables set", 1, 32, new_var_name_vproc );
	    pd->EndCase() ;
	    pd->BeginCase( "del", "delete user variables set", VAR_DELETE );
	      pd->StrDef( "name", "user variables set to be deleted",
	    		  	  1, 32, var_name_vproc );
	    pd->EndCase() ;
	    pd->BeginCase( "copy", "copy user variables set", VAR_COPY );
	      pd->StrDef( "name", "original user variables set",
	    		  	  1, 32, var_name_vproc );
	      pd->StrDef( "new_name", "destination user variables set",
	    		  	  1, 32, new_var_name_vproc );
	    pd->EndCase() ;
	    pd->BeginCase( "print", "print user variables", VAR_PRINT );
	      pd->KeywordDef ("format", "format to print in", print_keyword, 0);
	      pd->StrDef( "name", "user variables set", 1, 32, var_name_vproc );
	    pd->EndCase();
	    pd->BeginCase( "list", "list contents of the user variables", VAR_LIST );
	      pd->StrDef( "name", "user variables set", 1, 32, var_name_vproc );
	    pd->EndCase();
	  pd->EndChoice();
	pd->EndParms();

	Create( "var", "Set, delete, copy, list or print user variables set(s)", Gal_GetHelpAreaSessionId() );
}

Atp_Result var_cmd::cmd( int, char ** )
{
	char *uv_name = Atp_Str( "name" );
	ostrstream output;

	switch( (enum operator_id)Atp_Num( "operations" ) )
	{
		case VAR_SET:		set_cmd( &output, uv_name ); break;
		case VAR_DELETE:	del_cmd( &output, uv_name ); break;
		case VAR_COPY:		copy_cmd( &output, uv_name, Atp_Str( "new_name" ) ); break;
		case VAR_PRINT:		print_cmd( &output, uv_name ); break;
		case VAR_LIST:		list_cmd( &output, uv_name ) ; break;
		default:			break;
	}

	output << ends;
	ui->Tcl.SetResult( output.str(), TCL_DYNAMIC );

	return ATP_OK;
}

void var_cmd::parse( const char *from, const char *to )
{
	{
		RWCString it( from );
		RWCTokenizer next( it );

		from_v_set = from_v_parm = from_v_part = from_v_name = "";

			from_v_set = next(".");

			from_v_parm = next(".");
			if ( !from_v_parm.isNull() )
			  from_v_parm.toUpper();

			from_v_part = next(".");
			if ( !from_v_part.isNull() )
			  from_v_part.toLower();

			from_v_name = next(".");

			if ( !from_v_name.isNull() )
			  throw Genie3_SyntaxError("Too many \".\"s in %s", from);

			from_v_name = from_v_parm;
			if ( ! from_v_part.isNull() )
			{
			  from_v_name += ":";
			  from_v_name += from_v_part;
			}
	}

	{
		RWCString it( to );
		RWCTokenizer next( it );
		RWCString last;

		to_v_set = "";

		to_v_set = next(".");

		last = next(".");
		if ( !last.isNull() )
		  throw Genie3_SyntaxError("Too many \".\"s in %s", to);
	}
}

void var_cmd::set_cmd( ostrstream *message, const char *name )
{
	parse( "", name );

	if ( !ui->uv_exists( to_v_set ) )
	{
	  *message << "Creating user variables " << to_v_set << endl;
	}
	*message << "Setting default user variables to \"" << to_v_set << "\"" << endl;
	ui->uv_set( to_v_set );
}

void var_cmd::del_cmd( ostrstream *message, const char *name )
{
	parse( name, "" );

	if ( !ui->uv_exists( from_v_set ) )
	{
	  *message << "User variables \"" << from_v_set << "\" does not exist" << endl;
	  return;
	}

	if ( from_v_name.isNull() )
	// work on the whole set
	{
	  *message << "Deleting user variables \"" << name << "\"" << endl;
	  ui->uv_del( name );
	}
	else
	// work on a single part
	{
	  if ( !ui->uv()->exists( from_v_name ) )
	  {
		*message << "User variable \"" << from_v_name << "\" in set \"" << from_v_set << "\" does not exist" << endl;
		return;
	  }

	  if ( from_v_part.isNull() )
	  {
		from_v_name += ":.*";

		RWCRegexp re( from_v_name );

		ui->uv()->undefine( re );
	  }
	  else
		ui->uv()->undefine( from_v_name.data() );
	}
}

void var_cmd::copy_cmd( ostrstream *message, const char *from, const char *to )
{
	parse( from, to );

	if ( !ui->uv_exists( from_v_set ) )
	{
	  *message <<"User variables \"" << from_v_set << "\" does not exist" << endl;
	  return;
	}

	if ( from_v_name.isNull() )
	{
	  if ( ui->uv_exists( from_v_set ) )
		*message << "Merging User variables \"" << from_v_set << "\" into \"" << to_v_set << "\"" << endl;
	  else
		*message << "Copying User variables \"" << from_v_set << "\" into \"" << to_v_set << "\"" << endl;
	  ui->uv_copy( from_v_set, to_v_set );
	}
	else
	{
	  if ( ui->uv_exists( to_v_set ) )
		*message << "Merging User variable \"" << from << "\" into \"" << to_v_set << "\"" << endl;
	  else
	  {
		*message << "Copying User variable \n" << from << "\" into \"" << to_v_set << "\"" << endl;
		ui->uv( to_v_set );
	  }

	  // get the two user variables sets
	  user_variables *from_set = ui->uv(from_v_set);
	  user_variables *to_set = ui->uv(to_v_set);

	  if ( from_v_part.isNull() )
	  {
		from_v_name += ":.*";
		RWCRegexp re( from_v_name );
		to_set->merge( *from_set, re );
	  }
	  else
	  {
		// copy the one variable over
		user_variables::uv_info *uvip = from_set->value( from_v_name );
		to_set->define( from_v_name, *uvip->uv_bits, uvip->bit_width, uvip->field_type, uvip->field_format, uvip->long_description );
	  }
	}
}

void var_cmd::print_cmd( ostrstream *message, const char *name )
{
	parse( name, "" );

	user_variables *u = ui->uv( from_v_set );

	bit_string *b = u->value( from_v_name )->uv_bits;

	enum print_type pt = (enum print_type)Atp_Num("format");

	switch (pt)
	{
		case pt_integer:
			if (b->length(1) <= 32)
				*message << dec << b->fetch_lsb(b->length(1)) << endl;
			else
				throw Genie3_SyntaxError(
						"Unable to print %s as an integer its more than 32 bits wide",
						from_v_name);
			break;

		case pt_hex:
		{
			int num_bytes = b->length(8);

			for (int i = 0; i < num_bytes; i++) {
				*message << hex << setw(2) << b->fetch_lsb(8);
				if (i < (num_bytes - 1))
					*message << ' ';
			}
		}
		break;

		case pt_ascii:
		{
			int num_bytes = b->length(8);
			char ch[2];

			ch[1] = 0;
			for (int i = 0; i < num_bytes; i++)
			{
				ch[0] = (char) b->fetch_lsb(8);

				*message << ch;
			}
		}
		break;

		default:
			throw Genie3_InternalError("Unknown format type in var print");
	}
}

void var_cmd::list_cmd( ostrstream *message, const char *name )
{
	parse( name, "" );

	user_variables *u = ui->uv( from_v_set );

	user_variables list_set;

	if ( from_v_name.isNull() )
	  // the whole set
	  list_set. merge ( *u );
	else
	if( from_v_part.isNull() )
    // all of one groups variables
	{
	  from_v_name += ":.*";
	  RWCRegexp re( from_v_name );
	  list_set.merge( *u, re );
	}
	else
	// a single variable
	{
	  RWCRegexp re( from_v_name );
	  list_set.merge( *u, re );
	}

	*message << list_set << endl;
}

user_variables *Genie3_cli::uv( const char *name )
{
	if ( name == 0 )
	  return current_uv;

	RWCollectableString s( name );

	user_variables *v = (user_variables *)variables.findValue( &s );
	if ( v == 0 )
	{
	  v = new user_variables;
	  variables.insertKeyAndValue( new RWCollectableString( s ), v);
	}

	return v;
}

int Genie3_cli::uv_exists( const char *name )
{
	RWCollectableString s( name );

	user_variables *v = (user_variables *)variables.findValue( &s );

	return v != 0;
}

void Genie3_cli::uv_set( const char *name )
{
	user_variables *v = uv( name );

	current_uv_name = name;
	current_uv = v;

	RWCString prompt;
	prompt = prompt += name; prompt += "] ";

	SetPrompt(prompt);
}

void Genie3_cli::uv_del( const char *name )
{
	RWCollectableString s( name );
	variables.removeAndDestroy( &s );

	// if all user variables have been deleted create default again
	if ( variables.isEmpty() )
	  uv_set( default_uv_name );
	else
	// if we have deleted the current uv then set to default
	if ( current_uv_name == s )
	  uv_set( default_uv_name );
}

void Genie3_cli::uv_copy( const char *from, const char *to )
{
	RWCollectableString from_s( from );
	user_variables *from_v = (user_variables *)variables.findValue( &from_s );

	RWCollectableString to_s( to );
	user_variables *to_v = (user_variables *)variables.findValue( &to_s );

	if ( to_v == 0 )
	{
	  to_v = new user_variables;

	  variables.insertKeyAndValue( new RWCollectableString( to_s ), to_v);
	}

	// merge in variables
	*to_v += *from_v;
}
