/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+*******************************************************************

	Module Name:		g3obitsx.cxx (was bit_str.cxx)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Class bit_string Implementation

	Author:				Barry A. Scott
						Scott Concepts Limited
						Tel: [44]-1734-413655

	History:
		Who			When				Description
	-----------	--------------	---------------------------------
	Barry Scott	January 1995	Initial Creation
	Alwyn Teh	13 July 1995	Use zero padding for hex output.
								Add set-fill () to operator<<{).

*******************************************************************-*/

#include <iostream.h>
#include <iomanip.h>

#include <stdlib.h>
#include <memory.h>
#include <malloc.h>
#include <string.h>

#include <g3obitsh.h>

//
//
//
#define _DEBUG_DIRTY_BUFFER	0
#define _DEBUG_FETCH		0
#define _DEBUG_APPEND		0

bit_string::bit_string( int initial_size )
{
	allocated = initial_size;
	bits = (byte *)malloc( initial_size );
#if _DEBUG_DIRTY_BUFFER
	// dirty buffer to prove that it gets filled in correctly
	memset( bits, Oxaa, initial_size );
#else
	memset( bits, 0, initial_size );
#endif
	empty();
}

bit_string::bit_string( bit_string &b )
{
	allocated = b.allocated;
	bits = (byte *)malloc( allocated );

	memcpy( bits, b.bits, allocated );
	len = b.len;
	pos = b.pos;
}

bit_string::bit_string( const char *str )
{
	allocated = strlen( str ) + 1;
	bits = (byte *)malloc( allocated );

	memcpy( bits, str, allocated );
	len = (allocated-1)*8;
	pos = 0;
}

bit_string::~bit_string()
{
	free( bits );
}

bit_string & bit_string::operator=( bit_string &b )
{
	allocated = b.allocated;
	free( bits );
	bits = (byte *)malloc( allocated );

	memcpy( bits, b.bits, allocated );
	len = b.len;
	pos = b.pos;

	return *this;
}

void bit_string::replace_lsb( int pos, int num_bits, int value )
{
	// confirm that the replacement is within the existing bit string
	if ( pos + num_bits > len )
	  throw BadReplace();

	// record len
	int cur_len = len;

	// backup to the replace position
	len = pos;

	// use the append logic to insert the bits
	append_lsb( num_bits, value );

	// restore the bit string to its original length
	len = cur_len;
}

void bit_string::append_lsb( int num_bits, int value )
{
	while ( num_bits > 0 )
	{
		// work out the first byte that will be written into
		int byte_offset = len/8;

		// work out the first free bit in the byte
		int bit_offset = len%8;

		// update the byte - don't worry about moving too much
		bits[byte_offset] &= ~(byte)(0xff << bit_offset); // clear a space
		bits[byte_offset] |= (byte)(value << bit_offset); // insert the value
		//update the len
		int bits_moved = min( 8 - bit_offset, num_bits );
		len += bits_moved;

		num_bits -= bits_moved;
		// shrink the value
		value >>= bits_moved;
	}
}

void bit_string::replace_msb( int pos, int num_bits, int value )
{
	// confirm that the replacement is within the existing bit string
	if ( pos + num_bits > len )
	  throw BadReplace();

	// record len
	int cur_len = len;

	// backup to the replace position
	len = pos;

	// use the append logic to insert the bits
	append_msb( num_bits, value );

	// restore the bit string to its original length
	len = cur_len;
}

void bit_string::append_msb( int num_bits, int value )
{
	// HACK!!!!!
	append_lsb( num_bits, value );
}

void bit_string::replace ( int pos, int num_bits, byte * value )
{
	// confirm that the replacement is within the existing bit string
	if ( pos + num_bits > len )
	  throw BadReplace();

	// record len
	int cur_len = len;

	// backup to the replace position
	len = pos;

	// use the append logic to insert the bits
	append( num_bits, value );

	// restore the bit string to its original length
	len = cur_len;
}

void bit_string::append( int num_bits, byte *value )
{
	if ( len % 8 == 0 )
	{
	  memcpy( &bits[len/8], value, (num_bits+7)/8 ) ;
	  len += num_bits;
	}
	else while( num_bits > 0 )
			append_lsb( max( num_bits, 8 ), (int)*value++ );
}

void bit_string::append( bit_string *value )
{
	for ( int num_bits = value->length(1); num_bits > 0; num_bits -= 8 )
	   append_lsb( 8, value->fetch_lsb( min( 8, num_bits ) ) );
}

int bit_string::fetch_lsb( int num_bits )
{
	if ( num_bits > (len-pos) )
	  throw FetchTooMuch();

	int value = 0;

	int bit_pos_in_value = 0;

	while ( num_bits > 0 )
	{
		// work out the first byte that will be written into
		int byte_offset = pos/8;

		// work out the first free bit in the byte
		int bit_offset = pos%8;

		// we want this number of bits from this byte
		int bits_fetched = min(	num_bits,	// the smaller of what we want
								8-bit_offset // and what is available
								);

		int bit_mask = (1 << bits_fetched) - 1;

		// fetch the bits
		int these_bits = (bits[byte_offset]>>bit_offset) & bit_mask;

		// merge into the value
		value |= these_bits << bit_pos_in_value;

		//update the pos
		pos += bits_fetched;

		bit_pos_in_value += bits_fetched;

		num_bits -= bits_fetched;
	}

#if _DEBUG_FETCH
	cerr << "fetch_lsb returning " << value << " pos is now " « pos << endl;
#endif

	return value;
}

int bit_string::fetch_msb( int num_bits )
{
	if ( num_bits > (len-pos) )
	  throw FetchTooMuch();

	int value = 0;

	int bit_pos_in_value = num_bits;

	while ( num_bits > 0 )
	{
		// work out the first byte that will be written into
		int byte_offset = pos/8;

		// work out the first free bit in the byte
		int bit_offset = pos%8;

		// we want this number of bits from this byte
		int bits_fetched = min(	num_bits,	// the smaller of what we want
								8-bit_offset // and what is available
								) ;
		int bit_mask = (1 << bits_fetched) - 1;

		// fetch the bits
		int these_bits = (bits[byte_offset]>>bit_offset) & bit_mask;

		//update the pos
		pos += bits_fetched;

		bit_pos_in_value -= bits_fetched;

		num_bits -= bits_fetched;

		// merge into the value
		value |= these_bits << bit_pos_in_value ;
	}

	return value;
}

const char *bit_string::fetch_str(void)
{
	return (const char *)bits;
}

void bit_string::fetch( int num_bits, byte *value )
{
	if ( num_bits > (len-pos) )
	  throw FetchTooMuch();

	if ( (pos%8) == 0 && (num_bits%8) == 0 )
	{
	  memcpy( value, bits, num_bits/8 );
	  return;
	}

	while ( num_bits > 0 )
	{
		*value++ = fetch_lsb( min( 8, num_bits ) ) ;
	}
}

void bit_string::fetch( int num_bits, bit_string *value )
{
	if ( num_bits > (len-pos) )
	  throw FetchTooMuch();

	while ( num_bits > 0 )
	{
		value->append_lsb( 8, fetch_lsb( min( 8, num_bits ) ) );
		num_bits -= 8;
	}
}

ostream & operator<< ( ostream &o, const bit_string &b )
{
	const long orig_flags = o.flags();

	o << "(" << dec << b.len << ")";

	for ( int i=0; i<b.len; i += 8 )
	   o << " " << setw(2) << setprecision(2) << setfill('O') << hex << (int)(b.bits[i/8]);

	o.fill(' '); // reset
	o.flags( orig_flags );

	return o;
}
