/* EDITION AA02 (REL001), ITD ACST.175 (95/07/13 13:00:00} -- OPEN */

/*+*******************************************************************

	Module Name:		g3otcluv.cxx

	Copyright:			BNR Europe Limited,	1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Tcl User Variables

						For mapping G3O user variables of a received
						message into Tcl string variables. Used for
						integrated automation.

	History:
		Who			When				Description
	----------	---------------	------------------------------
	Alwyn Teh	13-21 July 1995	Initial	Creation
	Alwyn Teh	20 August 1995	Move switch statement in
								user_tcl_variables() to
								print_value() in g3ouvarx.cxx

********************************************************************-*/

#include <malloc.h>
#include <iostream>
#include <strstream.h>

#include <g3oproth.h>
#include <g3otclvh.h>

using namespace std;

user_tcl_variables::user_tcl_variables( Tcl_Interp *iptr, const char *uv_name, user_variables &uv ) : utv_name( uv_name )
{
	interp = iptr;
	tcl_rtn_str = 0;
	char *tmp = 0;

	RWHashDictionaryIterator contents( uv.get_variables() );

	ostrstream varnames_out;

	varnames_out << endl << "Tcl variables contained in array " << uv_name << ":" << endl;

	while ( contents () )
	{
		// Get existing user variable name and value
		RWCollectableString *key = (RWCollectableString *)contents.key();
		user_variables::uv_info *curr_value = (user_variables::uv_info *)contents.value();

		// Convert to Tcl variables - e.g. $rcvmsg(group.field)
		RWCString varname = utv_name; // i.e. "rcvmsg"
		varname += "(" ;
		varname += *key;
		RWRegexp re(":");
		varname(re) = ".";
		varname += ")";
		varname.toLower();

		// cout << varname << " - " << curr_value->long_description << " [" << *curr_value->uv_bits << "] " << endl;

		ostrstream tcl_var_value;

		uv.print_value( tcl_var_value, curr_value );

		tcl_var_value << flush << ends;
		tmp = tcl_var_value.str();

		set_var( varname.data() , tmp );

		varnames_out << "	$" << varname << " = " << tmp << endl;

		if ( tmp != 0 )
		  free( tmp ) ;
	}

	RWCString varlist( uv_name );
	varlist += "_contents";
	varnames_out << endl << endl << flush << ends;
	tmp = varnames_out.str();
	set_var( varlist, tmp );
	if ( tmp != 0 )
	  free( tmp );
}

user_tcl_variables::~user_tcl_variables( void )
{
	if ( tcl_rtn_str != 0 )
	  free( tcl_rtn_str );

	tcl_rtn_str = 0;

	unset_var( utv_name ); // can only delete all variables when utv_name is a Tcl array variable

	interp = 0; // zero this last, but not mine to free
}

void user_tcl_variables::OutputError( void )
{
	cerr << endl << "Error: " << interp->result;
	if ( inside_script() )
	  cerr << "(script file: " << tcl_rtn_str << ")";
	cerr << endl;
}

char * user_tcl_variables::set_var( const char *varName, const char *newValue )
{
	char *rs = Tcl_SetVar( interp, (char *)varName, (char *)newValue, TCL_GLOBAL_ONLY|TCL_LEAVE_ERR_MSG );

	if ( rs == 0 )
	  OutputError();

	return rs;
}

char * user_tcl_variables::get_var( const char *varName )
{
	char *rs = Tcl_GetVar( interp, (char *)varName, TCL_GLOBAL_ONLY|TCL_LEAVE_ERR_MSG );

	if ( rs == 0 )
	  OutputError();

	return rs;
}

int user_tcl_variables::unset_var( const char *varName )
{
	int rc = Tcl_UnsetVar( interp, (char *)varName, TCL_GLOBAL_ONLY|TCL_LEAVE_ERR_MSG );

	if ( rc == TCL_ERROR )
	  OutputError();

	return rc;
}

int user_tcl_variables::var_exists( const char *varName )
{
	char *rs = Tcl_GetVar( interp, (char *)varName, TCL_GLOBAL_ONLY );

	return (rs) ? 1 : 0;
}

char * user_tcl_variables::inside_script( void )
{
	if ( tcl_rtn_str != 0 )
	  free( tcl_rtn_str );

	tcl_rtn_str = 0;

	int rc = Tcl_Eval( interp, "info script" );

	if ( rc == TCL_OK && interp->result != 0 && *interp->result != '\0' )
	  return tcl_rtn_str = strdup(interp->result);
	else
	  return 0;
}

void user_tcl_variables::clearAndDestroy( void )
{
	cout << "user_tcl_variables::clearAndDestroy() called" << endl;
}
