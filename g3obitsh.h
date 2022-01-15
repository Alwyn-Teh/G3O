/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+*******************************************************************

	Module Name:		g3obitsh.h (was bitstr.h)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Class bit_string Header Declarations

	Author:				Barry A. Scott
						Scott Concepts Limited
						Tel: [44]-1734-413655

	History:
		Who				When				Description
	-----------		--------------	---------------------------
	Barry Scott		January 1995	Initial Creation

********************************************************************-*/

#ifndef __TUP_BIT_STR_H
#define __TUP_BIT_STR_H

#include <iostream.h>

#include <rw/collect.h>

typedef unsigned char byte;

class bit_string : public RWCollectable
{
public:
		bit_string( int initial_size = 1024 );
		bit_string ( bit_string &bits ) ;
		bit_string( const char *str );

		~bit_string();

		class Exception
		{
			public:
					Exception() { code=0; }
					int code;
		};

		class FetchTooMuch : public Exception { };
		class BadReplace : public Exception { };

		bit_string &operator=( bit_string & );

		int length( int units=8) { return (len+(units-1))/units; }
		const byte * pointer() { return bits; }

		void empty( void ) { pos = 0; len =0; }

		void set_position( int p ) { pos = p; }
		int position( int units=1 ) { return (pos+(units-1))/units; }

		int get_bit_position() { return pos; }
		int get_bit_length() { return len; }

		friend ostream &operator<<( ostream &o, const bit_string &b );

		void append_lsb( int num_bits, int value );
		void append_msb( int num_bits, int value );
		void append( int num_bits, byte *value );
		void append( bit_string *value );

		void append_lsb( unsigned int num_bits, int value )
				{ append_lsb( (int)num_bits, value ); }
		void append_msb( unsigned int num_bits, int value )
				{ append_msb( (int)num_bits, value ); }
		void append( unsigned int num_bits, byte *value )
				{ append( (int)num_bits, value ); }

		void replace_lsb( int pos, int num_bits, int value );
		void replace_msb( int pos, int num_bits, int value );
		void replace( int pos, int num_bits, byte *value );

		void replace_lsb( int pos, unsigned int num_bits, int value )
				{ replace_lsb( pos, (int)num_bits, value ); }
		void replace_msb( int pos, unsigned int num_bits, int value )
				{ replace_msb ( pos, (int) num_bits, value ); }
		void replace( int pos, unsigned int num_bits, byte *value )
				{ replace( pos, (int)num_bits, value ); }

		int fetch_lsb( int num_bits );
		int fetch_msb( int num_bits );
		void fetch( int num_bits, byte *bits );
		void fetch( int num_bits, bit_string *bits );
		const char *fetch_str(void);

protected:
		int len;		// in bits
		int pos;		// in bits
		int allocated;	// in byte
		byte *bits;		// pointer to allocated bytes

public:
		int min( int	a, int b )	{ if ( a < b ) return a; else return b; }
		int max( int	a, int b )	{ if ( a > b ) return a; else return b; }
};

ostream &operator<<( ostream &o, const bit_string &b );

#endif /*  TUP_BIT_STR_H */
