/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */
/*+*****************************************************»*************

	Module Name:		g3optini.cxx (was prot_ini.cxx)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Protocol Class Initialisations and Implementations

	Author:				Barry A. Scott
						Scott Concepts Limited
						Tel: [44]-1734-413655

	History:
		Who			When				Description
	-----------	-------------	-------------------------------
	Barry Scott	January 1995	Initial Creation
	Alwyn Teh	21 June 1995	Add bit_width to Table and
								Table_Ref for use with field
								value meanings.
								Add next() to Table.
	Alwyn Teh	4 August 1995	Convert Table_Ref to use DArray
	Alwyn Teh	4 August 1995	Implement Field::operator*()
								in order to add is_header
								PropertyType to Field.
	Alwyn Teh	17 August 1995	Add UIGroup::operator*() for
								Properties per_link and invisible.

********************************************************************-*/

#include <assert.h>

#include <g3oproth.h>
#include <g3obitsh.h>
#include <g3ouvarh.h>

#include <iomanip.h>
#include <iostream.h>
#include <strstream.h>

#include <rw/cstring.h>
#include <rw/hashdict.h>
#include <rw/ordcltn.h>

//
//	10 manipulators
//
inline ostream& left( ostream& o )
{
	o.setf( ios::left, ios::adjustfield );
	return o;
}

inline ostream& right( ostream& o )
{
	o.setf( ios::right, ios::adjustfield );
	return o;
}

//
//
//	RefCount implementation
//
//	Debug output is written to refcount.err in the current directory
//
#if _DEBUG_REFCOUNT
	#include <fstream.h>

	fstream _dbg_rc_err( "refcount.err", ios::out );

	void *bpt_if_this = NULL;

	void bpt()
			{ return; }
#endif

Protocol::RefCount::RefCount(void)
{
#if _DEBUG_REFCOUNT
	_dbg_rc_err << hex << (unsigned int)this << "->Ref Count::RefCount ref_ = 0" << endl;
	if ( this == bpt_if_this ) bpt();
#endif

	ref_ = 0;
}

Protocol::RefCount::~RefCount(void)
{
#if _DEBUG_REFCOUNT
	_dbg_rc_err << hex << (unsigned int)this << "->RefCount::~RefCount ref_ = " << ref_ << endl;
	if( this == bpt_if_this ) bpt();
#endif

	if ( ref_ != 0 )
	  throw InternalError("~RefCount ref_ == %d", ref_ );
}

void Protocol::RefCount::addRef(void)
{
	ref_++;

#if _DEBUG_REFCOUNT
	_dbg_rc_err << hex << (unsigned int)this << "->RefCount::addRef ref_ = " << ref_ << endl;
	if ( this == bpt_if_this ) bpt();
#endif
}

int Protocol::RefCount::removeRef(void)
{
	ref_--;

#if _DEBUG_REFCOUNT
	_dbg_rc_err << hex << (unsigned int)this << "->RefCount::removeRef ref_ = " << ref_ << endl;
	if ( this == bpt_if_this ) bpt();
#endif

	if ( ref_ < 0 )
	  throw InternalError("removeRef ref_ == %d", ref_ );

	return ref_;
}

//
//	operator+ helper macro
//
#define OP_PLUS( def_class, plus_class ) \
Protocol::def_class &Protocol::def_class::operator+( const plus_class & m ) \
{ \
	ref->plus( (const Field &)m ); \
	return *this; \
}

//
//
//	Exception handling constructors
//
//
Protocol::Exception::Exception(void)
{
	code = 0;
}

Protocol::InternalError::InternalError ( const char *m ) : message ( m )
{
}

Protocol::InternalError::InternalError( const char *m, int a )
{
	char buf[128] ;
	sprintf( buf, m, a );
	message = buf;
}

Protocol::InternalError::InternalError( const char *m, const char *a )
{
	char buf[128];
	sprintf( buf, m, a );
	message = buf;
}

Protocol::FieldNotDefined::FieldNotDefined( RWCString &name )
{
	int colon = name.first(':');
	group = name;
	group.remove( colon );
	field = name;
	field.remove( 0, colon+1 );
}

Protocol::FieldTooSmall::FieldTooSmall( RWCString &name ) : FieldNotDefined( name )
{
}

//
//
//	Protocol - implementation
//
//
Protocol::Protocol( const char *protocol_name ) : prot()
{
//	prot = NULL;

	ProtocolName = protocol_name;
	ProtocolDesc = 0;

	// Construct basic banner title
	ostrstream output;

	output << G3O_tool_name;

	if ( protocol_name != 0 && *protocol_name != '\0' )
	  output << " - " << protocol_name;

	output << ends << flush;

	basic_banner_title = output.str();
	banner.append( basic_banner_title );
	banner.append( "" );
}

Protocol::~Protocol()
{
	// clean up RW stuff...
	msg_type_names.clearAndDestroy();
	group_names.clearAndDestroy();
	optional_group_names.clearAndDestroy();

	msg_type_descriptions.clearAndDestroy();
	msg_type_uservars.clearAndDestroy();
	group_info.clearAndDestroy();

	pointer_info.clearAndDestroy();

	// clean up the msg type group
	{
		RWHashDictionaryIterator next( msg_type_groups );

		while( next() )
		{
			RWOrdered *group_list = (RWOrdered *)next.value();
			group_list->clearAndDestroy();
		}

		msg_type_groups.clearAndDestroy();
	}

	if ( basic_banner_title != 0 )
	  delete basic_banner_title;
}

//
// Protocol Custom Operations Implementation
//
Protocol::CustomUIOps::CustomUIOps( Protocol *p )
{
	vproc = 0;
	cic_filtering_flag = 0;
	cic_filter_uv_name = 0;
	video_fx_flag = 0;
	auto_cic_feature_indicator = 0;

	_protocol = p;
}

Protocol::CustomUIOps::CustomUIOps( Protocol *p, VprocType *_vproc, enum custom_ui_ops_type type )
{
	vproc = 0;
	cic_filtering_flag = 0;
	cic_filter_uv_name = 0;
	video_fx_flag = 0;
	auto_cic_feature_indicator = 0;

	_protocol = p;

	if ( type == vproc_checking )
	  set_vproc( _vproc );
}

Protocol::CustomUIOps::CustomUIOps( Protocol *p, const char *name, enum custom_ui_ops_type type )
{
	vproc = 0;
	cic_filtering_flag = 0;
	cic_filter_uv_name = 0;
	video_fx_flag = 0;
	auto_cic_feature_indicator = 0;

	_protocol = p;

	switch ( type )
	{
		case user_variable_control:
			if ( name != 0 && *name != ’\0’ )
			  decode_with_default_uv( p, name );
			break;
		case cic_filtering:
			cic_filtering_flag = 1;
			cic_filter_uv_name = name;
			break;
		case video_fx:
			video_fx_flag = 1;
			break;
		case auto_cic:
			auto_cic_feature_indicator = 1;
			break;
		default:
			break;
	}
}

Protocol::CustomUIOps::~CustomUIOps( void )
{
	vproc =0;
	cic_filtering_flag = 0;
	cic_filter_uv_name = 0;
	video_fx_flag = 0;
	auto_cic_feature_indicator = 0;

	_protocol = 0;
}

void Protocol::CustomUIOps::decode_with_default_uv( Protocol *p, const char * uv_name )
{
	if ( uv_name != 0 && *uv_name != '\0' )
	{
	  // Insert user variables name, no duplicates will be inserted.
	  p->decode_uv_names.insert( new RWCollectableString( uv_name ) );
	}
}

RWCString Protocol::OverrideName( const char *group_name )
{
	RWCString name( group_name );
	name += ":override_value";

	return name;
}

RWCString Protocol::OptionName( const char *group_name )
{
	RWCString name( group_name );
	name += ":option_enabled";

	return name;
}

RWCString Protocol::MessageTypeName( void )
{
	RWCString name( "MSGTYPE:name" );
	return name;
}

//
//
//	Table - implementation
//
//
Protocol::Table::Table( int init_tab_size ) : initial_table_size( init_tab_size )
{
	// create ref when required
	ref = NULL;
	bit_width = 0;
}

Protocol::Table::Table(const Table &t)
{
	// copy t's ref
	ref = t.ref;

	if ( ref != NULL )
	  // add a reference
	  ref->addRef();
}

Protocol::Table & Protocol::Table::operator=(const Table &t)
{
	//
	//	The order of events is important,
	//	first increase the ref count of the incoming Table,
	//	then dereference the existing Table.
	//
	//	This is important in the case where out Table
	//	and t's is the same.
	//
	if ( t.ref != NULL )
	  // add a reference
	  t.ref->addRef();

	// if ref exists
	if ( ref != NULL
	  // and the ref count drops to zero
		 && ref->removeRef() == 0 )
	  // delete ref
	  delete ref;

	// copy t's ref
	ref = t.ref;

	this->set_bit_width(t.get_bit_width()); // ACST.175

	return *this;
}

Protocol::Table::~Table(void)
{
	// if ref exists
	if ( ref != NULL
	  // and the ref count drops to zero
		 && ref->removeRef() == 0 )
	  // delete ref
	  delete ref;
}

void Protocol::Table::Setup(void)
{
	// if we do not have one
	if ( ref != NULL )
	  return;

	// then create one
	ref = new Table_Ref( initial_table_size );

	// add say we have a reference to ref
	ref->addRef();
}

void Protocol::Table::set_bit_width( unsigned int width )
{
	bit_width = width;

	if ( ref != 0 )
	  ref->set_bit_width(width);
}

const Protocol::Table::num_str_pair Protocol::Table::next( int mode )
{
	static int index = 0;
	static Protocol::Table::num_str_pair NULL_PAIR = {0};

	if ( ref == NULL )
	  return NULL_PAIR;

	if ( mode == 0 )
	  index = 0; // reset to beginning of table

	if ( index < ref->get_num_items() )
	{
	  Protocol::Table::num_str_pair result;
	  result.value = ref->get_item_value(index);
	  result.string = ref->get_item_strings(index);

	  index += 1;

	  return result;
	}
	else
	  return NULL_PAIR;
}

Protocol::Table_Ref::Table_Ref( int initial_table_size ) :
								item_strings(initial_table_size),
								item_values(initial_table_size)
{
	num_items = 0;
	bit_width = 0;
}

// sets up a range with a single value in ref
Protocol::Table &Protocol::Table::operator+( int value )
{
	Setup();

	ref->plus( value );

	return *this;
}

// sets up a range with a single value in ref
Protocol::Table &Protocol::Table::operator-( int value )
{
	Setup();

	ref->minus( value );

	return *this;
}

// sets up a range with a single value in ref
Protocol::Table &Protocol::Table::operator+( const char *str )
{
	Setup();

	ref->plus( str );

	return *this;
}

const char *Protocol::Table::operator[]( int value )
{
	if ( ref == NULL )
	  return NULL;
	else
	  return ref->index(value);
}

// set the end of the range
void Protocol::Table_Ref::plus( int value )
{
	first = last = value;
}

// set the end of the range
void Protocol::Table_Ref::minus( int value )
{
	last = value;

	// swap first and last if last is not last!
	if ( last < first )
	{
	  int temp = first;
	  first = last;
	  last = temp;
	}
}

void Protocol::Table_Ref::plus( const char *str )
{
	// assign the string to the range
	for ( ; first <= last; first++ )
	{
	   item_values.append( first );
	   item_strings.append( str );
	   num_items++;
	}
}

const char *Protocol::Table_Ref::index( int value )
{
	register int i;

	for ( i = 0; i < num_items; i++ )
	   if ( value == item_values[i] )
		 return item_strings[i];

	return NULL;
}

//
//
//	Field - implementation
//
//
// RWDEFINE_COLLECTABLE(Protocol::Field, Protocol::id_Field)

Protocol::Field::Field()
{
	ref = new Field_Ref( 0, simple, ff_spare, 0, 0, 0, Table(), 0, 0 );
	// say we have a reference to ref
	ref->addRef();
}

// This constructor does not init ref as it was done by a derived class
Protocol::Field::Field( Field_Ref * )
{
}

Protocol::Field::~Field()
{
	if ( ref->removeRef() == 0 )
	  // delete ref
	  delete ref;
}

Protocol::Field::Field( const Field& f )
{
	ref = f.ref;
	ref->addRef();
}

Protocol::Field & Protocol::Field::operator=(const Field &f)
{
	//
	//	The order of events is important;
	//	first increase the ref count of the incoming Field,
	//	then dereference the existing Field.
	//
	//	This is important in the case where out Field
	//	and f's is the same to prevent delete in the ref
	//	prematurely.
	//
	f.ref->addRef();
	if ( ref->removeRef() == 0 )
	  delete ref;

	ref = f.ref;

	return *this;
}

Protocol::Field & Protocol::Field::operator*( const Property &prop )
{
	ref->set_property_masks( prop.get_property_masks() );
	return *this;
}

//----------------------------------------------------------------------------------------------------------

// Fixed width fields, simple, length, indicator

Protocol::Field::Field( const char *varname, enum field_type type, enum field_format fmt,
						int width, const char *controller, const char *longdesc,
						Protocol::CustomUIOps *cops_objptr )
{
	FieldInit( varname, type, fmt, width, Table(), controller, longdesc, cops_objptr );
}

Protocol::Field::Field( const char *varname, enum field_type type, enum field_format fmt,
						int width, const char *longdesc, Protocol::CustomUIOps *cops_objptr )
{
	FieldInit( varname, type, fmt, width, Table(), 0, longdesc, cops_objptr );
}

//-----------------------------------------------------------------------------------------------------------

// ... with a restricted set of valid values

Protocol::Field::Field( const char *varname, enum field_type type, enum field_format fmt,
						int width, int min_val, int max_val, const char *longdesc, Protocol::CustomUIOps *cops_objptr )
{
	FieldInit( varname, type, fmt, width, min_val, max_val, Table(), 0, longdesc, cops_objptr );
}

Protocol::Field::Field( const char *varname, enum field_type type, enum field_format fmt,
						int width, int min_val, int max_val, const char *controller, const char *longdesc,
						Protocol::CustomUIOps *cops_objptr )
{
	FieldInit( varname, type, fmt, width, min_val, max_val, Table(), controller, longdesc, cops_objptr );
}

//-----------------------------------------------------------------------------------------------------------

// ... and Table ( with or without CustomUIOps )

//-----------------------------------------------

Protocol::Field::Field( const char *varname, enum field_type type, enum field_format fmt,
						int width, const char *longdesc, const Table &table )
{
	FieldInit( varname, type, fmt, width, table, 0, longdesc );
}

Protocol::Field::Field( const char *varname, enum field_type type, enum field_format fmt,
						int width, const char *longdesc, Protocol::CustomUIOps *cops_objptr,
						const Table &table )
{
	FieldInit( varname, type, fmt, width, table, 0, longdesc, cops_objptr );
}

//-----------------------------------------------

Protocol::Field::Field( const char *varname, enum field_type type, enum field_format fmt,
						int width, int min_val, int max_val, const char *longdesc, const Table &table )
{
	FieldInit( varname, type, fmt, width, min_val, max_val, table, 0, longdesc );
}

Protocol::Field::Field( const char *varname, enum field_type type, enum field_format fmt,
						int width, int min_val, int max_val, const char *longdesc,
						Protocol::CustomUIOps *cops_objptr, const Table &table )
{
	FieldInit( varname, type, fmt, width, min_val, max_val, table, 0, longdesc, cops_objptr );
}

//-----------------------------------------------

Protocol::Field::Field( const char *varname, enum field_type type, enum field_format fmt,
						int width, const char *controller, const char *longdesc, const Table &table )
{
	FieldInit( varname, type, fmt, width, table, controller, longdesc );
}

Protocol::Field::Field( const char *varname, enum field_type type, enum field_format fmt,
						int width, const char *controller, const char *longdesc, Protocol::CustomUIOps *cops_objptr,
						const Table &table )
{
	FieldInit( varname, type, fmt, width, table, controller, longdesc, cops_objptr );
}

//------------------------------------------------

Protocol::Field::Field( const char *varname, enum field_type type, enum field_format fmt,
						int width, int min_val, int max_val, const char *controller, const char *longdesc, const Table &table )
{
	FieldInit( varname, type, fmt, width, min_val, max_val, table, controller, longdesc );
}

Protocol::Field::Field( const char *varname, enum field_type type, enum field_format fmt,
						int width, int min_val, int max_val, const char *controller, const char *longdesc,
						Protocol::CustomUIOps *cops_objptr, const Table &table )
{
	FieldInit( varname, type, fmt, width, min_val, max_val, table, controller, longdesc, cops_objptr );
}

//-------------------------------—-----------—

//-----------------------------------------------------------------------------------------------------------

void Protocol::Field::FieldInit(const char *varname, enum field_type type, enum field_format fmt,
								int width, const Table &table, const char *controller, const char *longdesc,
								Protocol::CustomUIOps *cops_objptr )
{
	switch( type )
	{
		default:
				if ( fmt == ff_int )
				  FieldInit( varname, type, fmt, width, -(1<< (width-1)), (1<<(width-1))-1,
						  	 table, controller, longdesc, cops_objptr );
				else
				FieldInit(	varname, type, fmt, width, 0, (1<<width)-1, table, controller,
							longdesc, cops_objptr );
				break;

		case bcd:
		case bcd_pad8:
				if ( controller != NULL )
				  throw InternalError("No default widths for controlled field %s", varname );
				FieldInit(	varname, type, fmt, width, width/4, width/4, table, controller, longdesc,
							cops_objptr );
				break;

		case octet:
				if ( controller != NULL )
				  throw InternalError("No default widths for controlled field %s", varname );
				FieldInit(	varname, type, fmt, width, width/8, width/8, table, controller, longdesc,
							cops_objptr );
				break;

		case bit8:
				if ( controller != NULL )
				  throw InternalError("No default widths for controlled field %s", varname ) ;
				FieldInit( varname, type, fmt, width, width, width, table, controller, longdesc,
				cops_objptr );
				break;
	}
}

void Protocol::Field::FieldInit(const char *varname, enum field_type type, enum field_format fmt,
								int width, int min_val, int max_val, const Table &table, const char *controller,
								const char *longdesc, Protocol::CustomUIOps *cops_objptr )
{
	ref = new Field_Ref( varname, type, fmt, width, min_val, max_val, table, controller, longdesc, cops_objptr );
	ref->addRef() ;
}

Protocol::Field_Ref::~Field_Ref()
{
	properties = 0;
}

Protocol::Field_Ref::Field_Ref(void) : controlled_by("")
{
	bit_width = 0;
	min_valid = 0;
	max_valid = 0;			// range of values in an integer field
	field_type = simple;	// the storage type of this field
	field_format = ff_int;	// how to format this field
	table_of_names = Table();
	custom_ops = 0;
	properties = 0;
}

Protocol::Field_Ref::Field_Ref(	const char *varname, enum field_type type,
								enum field_format fmt, int width, int min_val, int max_val,
								const Table &table, const char *controller,
								const char *longdesc, Protocol::CustomUIOps *cops_objptr
								) : variable_field( varname ? varname : "" ),
									controlled_by( controller ? controller : "" ),
									long_description( longdesc ? longdesc : ""),
									custom_ops( cops_objptr ? cops_objptr : 0 )
{
	bit_width = width;		// width of the field in bits from min to max
	min_valid = min_val;
	max_valid = max_val;	// range of values in an integer field
	field_type = type;		// the storage type of this field
	field_format = fmt;		// how to format this field
	((Table&)table).set_bit_width(width);
	table_of_names = table;
	properties = 0;
}

Protocol::UIField::UIField(	const char *ui_field_name, enum field_type type,
							enum field_format fmt, int width, int min_val, int max_val,
							int def_value, const char *description, Table &table )
{
	ref = new UIField_Ref( ui_field_name, type, fmt, width, min_val, max_val, def_value, description, table );
	// say we have a reference to ref
	ref->addRef();
}

Protocol::UIField::UIField(	const char *ui_field_name, enum field_type type,
							enum field_format fmt, int width, int min_val, int max_val,
							int def_value, const char *description,
							Protocol::CustomUIOps *cops_objptr, Table &table )
{
	ref = new UIField_Ref( ui_field_name, type, fmt, width, min_val, max_val, def_value, description, cops_objptr, table );
	// say we have a reference to ref
	ref->addRef();
}

Protocol::UIField_Ref::UIField_Ref(	const char *ui_field_name, enum field_type type,
									enum field_format fmt, int width, int min_val, int max_val,
									int def_value, const char *description, Table &table
									) : Field_Ref(	ui_field_name, type, fmt, width,
													min_val, max_val, table, NULL, description )
{
	default_value = def_value;
}

Protocol::UIField_Ref::UIField_Ref(	const char *ui_field_name, enum field_type type,
									enum field_format fmt, int width, int min_val, int max_val,
									int def_value, const char *description,
									Protocol::CustomUIOps *cops_objptr, Table &table
									) : Field_Ref(	ui_field_name, type, fmt, width,
													min_val, max_val, table, NULL,
													description, cops_objptr )
{
	default_value = def_value;
}

//
//
//	TypeCode - implementation
//
//
Protocol::TypeCode::TypeCode( const char *description, int code, int width,
							  enum field_format format )
							 : Field( 0, type_code, format, width, code, code, description )
{
}

//
//
//	Group - implementation
//
//
static Protocol::Field_Ref *null_ref = 0;

Protocol::Group::Group() : Field( null_ref )
{
	ref = new Group_Ref();
	ref->addRef();
}

// This constructor does not init ref as it was done by a derived class,
// call Fields constructor that also supresses the init of x
Protocol::Group::Group( Field_Ref * ) : Field( null_ref )
{
}

Protocol::Group &Protocol::Group::operator=( const Group &g )
{
	//
	//	The order of events is important;
	//	first increase the ref count of the incoming Group,
	//	then dereference the existing Group
	//
	//	This is important in the case where out Group
	//	and g's is the same to prevent delete in the ref
	//	prematurely.
	//
	g.ref->addRef();
	if ( ref->removeRef() == 0 )
	  delete ref;

	ref = g.ref;

	return *this;
}

//Protocol::Group::Group( const char *varname, enum field_type type, enum field_format fmt,
//						int width, const char *longdesc ) : Field( ref )
//{
//	ref = new Group_Ref( varname, type, fmt, width, longdesc, Table() );
//	ref->addRef() ;
//}

//Protocol::Group::Group( const char *vamame, enum field_type type, enum field_format fmt,
//						int width, const char *longdesc, Table &table ) : Field( ref )
//{
//	ref = new Group_Ref( varname, type, fmt, width, longdesc, table );
//	ref->addRef();
//}

Protocol::Group::Group( const char *controller ) : Field( null_ref )
{
	ref = new Group_Ref( controller );
	ref->addRef();
}

OP_PLUS( Group, UIGroup )
OP_PLUS( Group, UIField )
OP_PLUS( Group, LabelGroup )
OP_PLUS( Group, SelectOne )
OP_PLUS( Group, Group )
OP_PLUS( Group, LengthGroup )
OP_PLUS( Group, OptionGroup )
OP_PLUS( Group, Message )
OP_PLUS( Group, Field )

Protocol::Group_Ref::~Group_Ref()
{
	for ( int i = 0; i < num_msgs; i++ )
	{
		assert( msgs[i] != NULL );
		if ( msgs[i]->removeRef() == 0 )
		  delete msgs[i];
		msgs[i] = NULL;
	}

	num_msgs = 0;
}

Protocol::Group_Ref::Group_Ref( const char *varname, enum field_type type,
								enum field_format fmt, int width, const char *longdesc,
								const Table &table )
								: Field_Ref(varname, type, fmt, width, 0, (1<<width)-1,
											table, 0, longdesc )
{
	num_msgs = 0;
}

Protocol::Group_Ref::Group_Ref( void ) : Field_Ref()
{
	num_msgs = 0;
}

Protocol::Group_Ref::Group_Ref ( const char *controller ) : Field_Ref()
{
	num_msgs = 0;
	controlled_by = controller;
}

void Protocol::Group_Ref::plus( const Field &f )
{
	msgs[num_msgs++] = f.ref;
	f.ref->addRef();
}

//
//
//	SelectOne - implementation
//
//
Protocol::SelectOne::SelectOne(	const char *varname, enum field_format fmt,
								int width, const char *longdesc ) : Group( null_ref )
{
	ref = new SelectOne_Ref( varname, fmt, width, longdesc, Table() );
	ref->addRef();
}

Protocol::SelectOne::SelectOne( const char *varname, enum field_format fmt,
								int width, const char *longdesc, const Table &table )
								: Group( null_ref )
{
	ref = new SelectOne_Ref( varname, fmt, width, longdesc, table );
	ref->addRef();
}

Protocol::SelectOne::SelectOne( const char *varname ) : Group( null_ref )
{
	ref = new SelectOne_Ref( varname, ff_group, 0, 0, Table () ) ;
	ref->addRef();
}

Protocol::SelectOne &Protocol::SelectOne::operator+( int msg_type )
{
	ref->plus( msg_type );
	return *this;
}

OP_PLUS( SelectOne, UIGroup )
OP_PLUS( SelectOne, UIField )
OP_PLUS( SelectOne, Group )
OP_PLUS( SelectOne, LengthGroup )
OP_PLUS( SelectOne, OptionGroup )
OP_PLUS( SelectOne, SelectOne )
OP_PLUS( SelectOne, LabelGroup )
OP_PLUS( SelectOne, Message )
OP_PLUS( SelectOne, Field )

Protocol::SelectOne_Ref::SelectOne_Ref( const char *varname, enum field_format fmt,
										int width, const char *longdesc, const Table &table )
										: Group_Ref( varname, simple, fmt, width, longdesc, table )
{
}

void Protocol::SelectOne_Ref::plus( int msg_type )
{
	msg_types[num_msgs] = msg_type;
}

//
//
//	Message - implementation
//
//
Protocol::Message::Message( const char *msg_name, const char *description ) : Group( null_ref )
{
	ref = new Message_Ref( msg_name, description );
	ref->addRef();
}

OP_PLUS( Message, UIGroup )
OP_PLUS( Message, UIField )
OP_PLUS( Message, Group )
OP_PLUS( Message, LengthGroup )
OP_PLUS( Message, OptionGroup )
OP_PLUS( Message, LabelGroup )
OP_PLUS( Message, SelectOne )
OP_PLUS( Message, Field )

Protocol::Message_Ref::Message_Ref( const char *msg_name, const char *description )
									: Group_Ref( 0, simple, ff_uint, 0, 0, Table() )
{
	variable_field = msg_name;
	long_description = description;
}

//
//
//	UIGroup - implementation
//
//
Protocol::UIGroup::UIGroup( const char *ui_group_name, const char *description ) : Group( null_ref )
{
	ref = new UIGroup_Ref( ui_group_name, description );
	ref->addRef();
}

Protocol::UIGroup & Protocol::UIGroup::operator=( const UIGroup &g )
{
	//
	//	The order of events is important,
	//	first increase the ref count of the incoming Group,
	//	then dereference the existing Group

	//	This is important in the case where out Group
	//	and g's is the same to prevent delete in the ref
	//	prematurely.
	//
	g.ref->addRef();
	if ( ref->removeRef() == 0 )
	  delete ref;

	ref = g.ref;

	return *this;
}

Protocol::UIGroup & Protocol::UIGroup::operator*( const Property & prop )
{
	ref->set_property_masks( prop.get_property_masks() ); // ref is of type Field_Ref *
	return *this;
}

Protocol::UIGroup_Ref::UIGroup_Ref( const char *ui_group_name, const char *description )
									: Group_Ref( 0, simple, ff_uint, 0, 0, Table() )
{
	if ( ui_group_name == NULL )
	  throw InternalError( "UIGroup constructor requires a ui_group_name\n" );

	if ( description == NULL )
	  throw InternalError( "UIGroup constructor requires a long_description\n" );

	variable_group = ui_group_name;
	long_description = description;
}

OP_PLUS( UIGroup, Group )
OP_PLUS( UIGroup, LengthGroup )
OP_PLUS( UIGroup, OptionGroup )
OP_PLUS( UIGroup, LabelGroup )
OP_PLUS( UIGroup, SelectOne )
OP_PLUS( UIGroup, UIGroup )
OP_PLUS( UIGroup, UIField )
OP_PLUS( UIGroup, Field )

//
//
//	LabelGroup - implementation
//
//
Protocol::LabelGroup::LabelGroup( const char *description, const char *var_grp ) : Group( null_ref )
{
	ref = new LabelGroup_Ref( description, var_grp );
	ref->addRef();
}

Protocol::LabelGroup &Protocol::LabelGroup::operator=( const LabelGroup &g )
{
	//
	//	The order of events is important,
	//	first increase the ref count of the incoming Group,
	//	then dereference the existing Group
	//
	//	This is important in the case where out Group
	//	and g's is the same to prevent delete in the ref
	//	prematurely.
	//
	g.ref->addRef();
	if ( ref->removeRef() == 0 )
	  delete ref;

	ref = g.ref;

	return *this;
}

Protocol::LabelGroup_Ref::LabelGroup_Ref( const char *description, const char *var_grp )
											: Group_Ref( 0, simple, ff_uint, 0, 0, Table() )
{
	if ( description == NULL )
	  throw InternalError( "LabelGroup constructor requires a long_description\n" );

	long_description = description;

	if ( var_grp )
	  variable_group = var_grp;
}

OP_PLUS( LabelGroup, UIGroup )
OP_PLUS( LabelGroup, UIField )
OP_PLUS( LabelGroup, LabelGroup )
OP_PLUS( LabelGroup, SelectOne )
OP_PLUS( LabelGroup, Group )
OP_PLUS( LabelGroup, LengthGroup )
OP_PLUS( LabelGroup, OptionGroup )
OP_PLUS( LabelGroup, Field )

//
//
//	OptionGroup - implementation
//
//
Protocol::OptionGroup::OptionGroup( const char *ui_group_name, const char *description ) : Group( null_ref )
{
	ref = new OptionGroup_Ref( ui_group_name, description );
	ref->addRef();
}

Protocol::OptionGroup &Protocol::OptionGroup::operator=( const OptionGroup &g )
{
	//
	//	The order of events is important,
	//	first increase the ref count of the incoming Group
	//	then dereference the existing Group
	//
	//	This is important in the case where out Group
	//	and g's is the same to prevent delete in the ref
	//	prematurly.
	//
	g.ref->addRef();
	if ( ref->removeRef() == 0 )
	  delete ref;

	ref = g.ref;

	return *this;
}

Protocol::OptionGroup_Ref::OptionGroup_Ref( const char *ui_group_name, const char *description )
											: Group_Ref( 0, simple, ff_uint, 0, 0, Table() )
{
	if ( ui_group_name == NULL )
	  throw InternalError( "OptionGroup constructor requires a UIGroup name\n" );

	if ( description == NULL )
	  throw InternalError( "OptionGroup constructor requires a long_description\n" );

	variable_group = ui_group_name;
	long_description = description;
}

OP_PLUS( OptionGroup, UIGroup )
OP_PLUS( OptionGroup, UIField )
OP_PLUS( OptionGroup, LabelGroup )
OP_PLUS( OptionGroup, SelectOne )
OP_PLUS( OptionGroup, Group )
OP_PLUS( OptionGroup, LengthGroup )
OP_PLUS( OptionGroup, OptionGroup )
OP_PLUS( OptionGroup, Field )

//
//
//	LengthGroup - implementation
//
//
Protocol::LengthGroup::LengthGroup( const char *varname, int width, const char *description ) : Group( null_ref )
{
	ref = new LengthGroup_Ref( varname, width, description );
	ref->addRef();
}

Protocol::LengthGroup &Protocol::LengthGroup::operator=( const LengthGroup &g )
{
	//
	//	The order of events is important,
	//	first increase the ref count of the incoming Group,
	//	then dereference the existing Group
	//
	//	This is important in the case where out Group
	//	and g’s is the same to prevent delete in the ref
	//	prematurely.
	//
	g.ref->addRef();
	if ( ref->removeRef() == 0 )
	  delete ref;

	ref = g.ref;

	return *this;
}

Protocol::LengthGroup_Ref::LengthGroup_Ref( const char *varname, int width, const char *description )
											: Group_Ref( varname, length0, ff_uint, width, description, Table() )
{
}

OP_PLUS( LengthGroup, UIGroup )
OP_PLUS( LengthGroup, UIField )
OP_PLUS( LengthGroup, LabelGroup )
OP_PLUS( LengthGroup, SelectOne )
OP_PLUS( LengthGroup, Group )
OP_PLUS( LengthGroup, LengthGroup )
OP_PLUS( LengthGroup, OptionGroup )
OP_PLUS( LengthGroup, Field )

Protocol::Property::Property( enum Protocol::PropertyType mask ) : property_masks(0)
{
	property_masks |= mask;
}

Protocol::Property::Property( int properties ) : property_masks(0)
{
	property_masks = properties;
}

Protocol::Property::~Property( void )
{
	property_masks = 0;
}
