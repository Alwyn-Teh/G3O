/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+*******************************************************************

	Module Name:		g3ouvarh.h (was uservar.h)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Class user_variables Declaration

	Author:				Barry A. Scott
						Scott Concepts Limited
						Tel: [44]-1734-413655

	History:
		Who			When				Description
	-----------	--------------	---------------------------------
	Barry Scott	January	1995	Initial Creation
	Alwyn Teh	13 July	1995	Store extra info about user
								variables for Tcl variables
								when using define().
	Alwyn Teh	19 July	1995	Add decode_uv_set() to extract
								subset of default user variables
								for decoding e.g. SPCSIZE.

a******************************************************************-*/

#ifndef __TUP_USER_VARIABLES_H
#define __TUP_USER_VARIABLES_H

#include <iostream.h>

#include <rw/cstring.h>
#include <rw/hashdict.h>
#include <rw/collect.h>
#include <rw/rwset.h>

class bit_string;
class RWCRegexp;

class user_variables : public RWCollectable
{
public:
		user_variables (void);
		user_variables(user_variables &);
		user_variables(user_variables *);
		~user_variables(void);

		user_variables & operator=( user_variables & );
		user_variables & operator+= ( user_variables & );

		void define(const char *var_name, bit_string kbits, int bitwidth,
					// int field_type, int field_format, const char *long_description );
					int field_type = 0, int field_format = 0, const char *long_description = 0 );
		void define(const char *var_name, int width, int value,
					// int field_type, int field_format, const char *long_description );
					int field_type = 0, int field_format = 0, const char *long_description = 0 );
		void undefine( const char *var_name );
		void undefine( RWCRegexp &pattern );

		int exists( const char *var_name );

		friend ostream &operator<< ( ostream & o, user_variables &uv );

		void merge( user_variables &uv );
		void merge( user_variables &from, RWCRegexp &pattern );

		friend class user_variables_iterator;

		class uv_info : public RWCollectable
		{
		public:
				uv_info( void );
				uv_info( bit_string& b, int bitwidth, int ft, int ff, const char *desc );
				uv_info( uv_info& from_uv_info );
				~uv_info ( void );

				bit_string *uv_bits;
				int bit_width;
				int field_type;
				int field_format;
				const char *long_description;
		};

		user_variables::uv_info * value( const char *var_name );
		int value( const char *var_name, int width );
		void print_value( ostream &output, user_variables::uv_info *value );

		user_variables& decode_uv_set( user_variables& _uv, RWSet& decode_members );

		//
		//	Exception classes
		//
		class Exception
		{
		public:
				Exception() { code = 0; }
				int code;
		};

		class UndefinedVariable : public Exception
		{
		public:
				RWCString name;
				UndefinedVariable( const char *n ) : name( n ) { };
		};

		class InternalError : public Exception
		{
		public:
				RWCString message;
				InternalError( const char *m );
				InternalError( const char *m, int a );
		};

		RWHashDictionary& get_variables( void ) { return variables; }

protected:
		RWHashDictionary variables;
};

class user_variables_iterator
{
public:
		user_variables_iterator( user_variables &it );
		const char *operator()(void);
protected:
		RWHashDictionaryIterator next;
};

ostream &operator<<( ostream & o, user_variables &uv );

#endif /* __TUP_USER_VARIABLES_H */
