/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+******************************************************************

	Module Name:		g3optui.cxx (was prot_ui.cxx)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Protocol Class UI Interface

	Author:				Barry A. Scott
						Scott Concepts Limited
						Tel: [44]-1734-413655

	History:
		Who			When				Description
	-----------	--------------	-----------------------------------
	Barry Scott	January	1995	Initial Creation
	Alwyn Teh	21 June 1995	Add	field_value_meanings()
								to iterate_fields_of_group.
	Alwyn Teh	10 July 1995	Add	get_vproc() to Protocol::
								iterate_fields_of_group to
								support use of UI (ATP) vproc
								for checking of SPCSIZE.
	Alwyn Teh	15 August 1995	Add message_type_name(),
								get_group_name(),
								get_properties().
	Alwyn Teh	16	August 1995	Add	is_parm_of_msg().

*******************************************************************-*/

#include <assert.h>

#include <g3oproth.h>
#include <g3obitsh.h>
#include <g3ouvarh.h>

#include <rw/cstring.h>
#include <rw/hashdict.h>
#include <rw/ordcltn.h>

static RWCString tmp_string;

#if _DEBUG_BUILD_UI
#include <iostream>
using namespace std;
#endif

//
//
//	named_field is helper class for the build_ui_database
//
//
Protocol::named_field::named_field( Field_Ref *f ) : RWCollectableString( f->_variable_field() )
{
	the_field = f;
	f->addRef();
}

Protocol::named_field::named_field( const char *s ) : RWCollectableString( s )
{
	the_field = NULL;
}

Protocol::named_field::~named_field()
{
	if ( the_field != NULL && the_field->removeRef() == 0 )
	  delete the_field;
}

Protocol::Field_Ref *Protocol::named_field::field( void )
{
	return the_field;
}

ostream &Protocol::named_field::operator<<( ostream &o )
{
	return o << ((RWCString *)this);
}

//
//	class pointer_details
//
//RWDEFINE_COLLECTABLE(Protocol::pointer_details,Protocol::id_pointer_details)

Protocol::pointer_details::pointer_details( int s, int w )
{
	start = s;
	width = w;
}

int Protocol::pointer_details::Start(void) { return start; }
int Protocol::pointer_details::Width(void) { return width; }

//
//	Class group info
//
Protocol::GroupInfo::GroupInfo( Protocol::UIGroup_Ref *ui_grp, const char *description )
								: long_description( description ? description : "" )
{
	ui_group = ui_grp;
}

Protocol::GroupInfo::~GroupInfo()
{
	ui_group = NULL;
	fields.clearAndDestroy();
}

//
//	This routine starts a tree walk of prot to build all the interesting
//	databases derived from the protocol information
//

void Protocol::build_ui_database(void)
{
	user_variables v;
	ui_group_index = 0;
	ui_groups_in_header = 1000000;	// init to a very large number

	prot.ref->_build_ui_database( this, &v, NULL );

	/* Add UIGroup EXCESS and PADDING */
	UIGroup excess ( "EXCESS", "Excess data" );
	UIGroup padding( "PADDING", "Spare or padding data" );

	excess.ref->_build_ui_database( this, &v, NULL );
	padding.ref->_build_ui_database( this, &v, NULL );
}

void Protocol::UIGroup_Ref::_build_ui_database( Protocol *p, user_variables *initial_v, const char * )
{
#if _DEBUG_BDILD_UI
	cerr << "At UIGroup " << variable_group << endl;
#endif

	int i;

	// record this group in the ui_groups structure if it's not there yet
	for ( i = 0; i < p->ui_group_index; i++ )
	   if ( variable_group == *p->ui_groups[i] )
		 break;

	if ( i == p->ui_group_index )
	  p->ui_groups[ p->ui_group_index++ ] = &variable_group;

	// find out if we have this group already
	RWCollectableString group ( variable_group );
	GroupInfo *info = (GroupInfo *) (p->group_info.findValue( &group ));
	if ( info == NULL )
	{
	  // This group is not known about yet
	  // allocate a string for each collection
	  RWCollectableString *group1 = new RWCollectableString( group );
	  RWCollectableString *group2 = new RWCollectableString( group );

	  // record the groups name
	  p->group_names.insert( group1 );

	  // record the groups field set data structure
	  info = new GroupInfo( this, long_description );

	  // Get properties from base class Field_Ref and put in a permanent/convenient place.
	  info->set_properties( get_properties() );

	  p->group_info.insertKeyAndValue( group2, info );
	}

	for ( int m = 0; m < num_msgs; m++ )
	   msgs[m]->_build_ui_database( p, initial_v, variable_group );

#if _DEBUG_BUILD_UI
	cerr << "End UIGroup " << variable_group << endl;
#endif
}

void Protocol::Message_Ref::_build_ui_database( Protocol *p, user_variables *initial_v, const char *var_grp )
{
#if _DEBUG_BUILD_UI
	cerr << "At Message " << (var_grp ? var_grp : "") << " " << variable_field << endl;
#endif

	p->msg_type_names.insert( new RWCollectableString( variable_field ) );

	p->msg_type_descriptions.insertKeyAndValue(	new RWCollectableString( variable_field ),
												new RWCollectableString( long_description ));

#if _DEBUG_BUILD_UI
	cerr << "MsgType " << variable_field << *initial_v << endl;
#endif

	p->msg_type_uservars.insertKeyAndValue(	new RWCollectableString( variable_field ),
											new user_variables( initial_v ) );

	// remember the ui group index that we start with
	int start_index = p->ui_group_index;
	if ( p->ui_group_index < p->ui_groups_in_header )
	  p->ui_groups_in_header = p->ui_group_index;

	for ( int m = 0; m < num_msgs; m++ )
	   msgs[m]->_build_ui_database( p, initial_v, var_grp );

	RWOrdered *group_list = new RWOrdered( p->ui_group_index );
	for ( int g = 0; g < p->ui_group_index; g++ )
	   group_list->append( new RWCollectableString( *p->ui_groups[g] ) );

	// wind back to the start
	p->ui_group_index = start_index;
	p->msg_type_groups.insertKeyAndValue(new RWCollectableString( variable_field ), group_list );

#if _DEBUG_BUILD_UI
	cerr << "End Message " << (var_grp ? var_grp : "") << " " << variable_field << endl;
#endif
}

void Protocol::OptionGroup_Ref::_build_ui_database( Protocol *p, user_variables *initial_v, const char *var_grp )
{
#if _DEBUG_BUILD_UI
	cerr << "At OptionGroup " << variable_group << endl;
#endif

	p->optional_group_names.insert( new RWCollectableString( variable_group ) );

	for ( int m = 0; m < num_msgs; m++ )
	   msgs[m]->_build_ui_database( p, initial_v, var_grp );

#if _DEBUG_BUILD_UI
	cerr << "End OptionGroup " << variable_group << endl;
#endif
}

void Protocol::LabelGroup_Ref::_build_ui_database( Protocol *p, user_variables *initial_v, const char *var_grp )
{
#if _DEBUG_BUILD_UI
	cerr << "At LabelGroup " << long_description << endl;
#endif

	if ( var_grp != NULL )
	  variable_group = var_grp;

	for ( int m = 0; m < num_msgs; m++ )
	   msgs[m]->_build_ui_database( p, initial_v, var_grp );

#if _DEBUG_BUILD_UI
	cerr << "End LabelGroup " << long_description << endl;
#endif
}

void Protocol::SelectOne_Ref::_build_ui_database( Protocol *p, user_variables *initial_v, const char *var_grp )
{
#if _DEBUG_BUILD_UI
	cerr << "At SelectOne " << (var_grp ? var_grp : "") << " " << variable_field << endl;
#endif

	Field_Ref::_build_ui_database( p, initial_v, var_grp );

	user_variables v( initial_v );

	for ( int m = 0; m < num_msgs; m++ )
	{
	   bit_string b;
	   b.append_lsb( bit_width ? bit_width : 32, msg_types[m] );

	   // define the value of the selector
	   v.define( variable_name, b, bit_width ? bit_width : 32, field_type, field_format, long_description );

	   msgs[m] ->_build_ui_database( p, &v, var_grp );
	}

#if _DEBUG_BUILD_UI
	cerr << "End SelectOne " << (var_grp ? var_grp : "") << " " << variable_field << endl;
#endif
}

void Protocol::Group_Ref::_build_ui_database( Protocol *p, user_variables *initial_v, const char *var_grp )
{
#if _DEBUG_BUILD_UI
	cerr << "At Group " << (var_grp ? var_grp : "") << " " << variable_field << endl;
#endif

	if ( bit_width != 0 )
	  throw InternalError ( "_build_ui_database Group type group has width\n" ) ;

	for ( int m = 0; m < num_msgs; m++ )
	   msgs[m]->_build_ui_database( p, initial_v, var_grp );

#if _DEBUG_BUILD_UI
	cerr << "end Group " << (var_grp ? var_grp : "") << " " << variable_field << endl;
#endif
}

void Protocol::LengthGroup_Ref::_build_ui_database( Protocol *p, user_variables *initial_v, const char *var_grp )
{
#if _DEBUG_BUILD_UI
	cerr << "At LengthGroup " << (var_grp ? var_grp : "") << " " << variable_field << endl;
#endif

	Field_Ref::_build_ui_database( p, initial_v, var_grp );

	for ( int m = 0; m < num_msgs; m++ )
	   msgs[m]->_build_ui_database( p, initial_v, var_grp );

#if _DEBUG_BUILD_UI
	cerr << "end LengthGroup " << (var_grp ? var_grp : "") << " " << variable_field << endl;
#endif
}

void Protocol::Field_Ref::_build_ui_database( Protocol *p, user_variables *, const char *var_grp )
{
#if _DEBUG_BUILD_UI
	cerr << "At Field " << (var_grp ? var_grp : "") << " " << variable_field << endl;
#endif

	// must setup variable group if possible
	if ( var_grp != NULL )
	  // save the group name
	  variable_group = var_grp;

	// we are only interested in fields with user interface variables attached
	if ( variable_field.isNull() )
	  return;

	if ( var_grp == NULL )
	{
	  // variable name is the field name
	  variable_name = variable_field;
	}
	else
	{
	  // variable name is group:field
	  variable_name = var_grp;
	  variable_name += ":";
	  variable_name += variable_field;

	  RWCollectableString group( variable_group );
	  RWCollectableString field( variable_field );

	  GroupInfo *info = (GroupInfo *) (p->group_info.findValue( &group ) );

	// avoid duplicate entries
	if ( info->fields.find( &field ) == NULL )
	  // add this field to the group
	  info->fields.insert( new named_field( this ) );
	}

#if _DEBUG_BUILD_UI
	cerr << "End Field " << (var_grp ? var_grp : "") << " " << variable_field << endl;
#endif
}

void Protocol::UIField_Ref::_build_ui_database( Protocol *p, user_variables *vars, const char *var_grp )
{
#if _DEBUG_BUILD_UI
	cerr << "At UIField " << (var_grp ? var_grp : "") << " " << variable_field << endl;
#endif

	Field_Ref::_build_ui_database( p, vars, var_grp );

#if _DEBUG_BUILD_UI
	cerr << "End UIField " << (var_grp ? var_grp : "") << " " << variable_field << endl;
#endif
}

//
//
//	User Interface interface routines
//
//
Protocol::iterate_message_types::iterate_message_types( Protocol *_p ) : p(_p)
{
	pos = -1;
}

int Protocol::iterate_message_types::operator()(void)
{
	pos++;

	if ( pos >= p->msg_type_names.entries() )
	  return 0;

	item = (RWCollectableString *)p->msg_type_names( pos );

	return 1;
}

int Protocol::iterate_message_types::operator()( int position )
{
	pos = position;

	if ( pos >= p->msg_type_names.entries() )
	  return 0;

	item = (RWCollectableString *)p->msg_type_names( pos );

	return 1;
}

RWCString Protocol::iterate_message_types::name()
{
	return *item;
}

RWCString Protocol::iterate_message_types::description()
{
	RWCollectableString *desc = (RWCollectableString *)p->msg_type_descriptions.findValue( item );
	return *desc;
}

Protocol::iterate_groups_of_message_type::iterate_groups_of_message_type
		(Protocol *_p, const char *message_name )
			: p(_p), msg_name( (tmp_string = message_name, tmp_string.toUpper(), tmp_string) )
{
	RWOrdered *set = (RWOrdered *)_p->msg_type_groups.findValue( &msg_name );

	if ( set == NULL )
	  next = NULL;
	else
	  next = new RWOrderedIterator( *set );
}

Protocol::iterate_groups_of_message_type::~iterate_groups_of_message_type(void)
{
	if ( next )
	  delete next;
}

int Protocol::iterate_groups_of_message_type::operator()(void)
{
	if ( next == NULL )
	  return 0;

	item = (RWCollectableString *)(*next)();
	return item != 0;
}

RWCString Protocol::iterate_groups_of_message_type::name()
{
	return *item;
}

RWCString Protocol::iterate_groups_of_message_type::description()
{
	GroupInfo *info = (GroupInfo *)p->group_info.findValue( item );
	return info->long_description;
}

int Protocol::iterate_groups_of_message_type::get_properties()
{
	GroupInfo *info = (GroupInfo *)p->group_info.findValue( item );
	int prop = info->ui_group->get_properties();
	return prop;
}

Protocol::iterate_field_groups::iterate_field_groups( Protocol *_p ) : p(_p)
{
	pos = -1;
}

int Protocol::iterate_field_groups::operator()(void)
{
	pos++;

	if ( pos >= p->group_names.entries() )
	  return 0;

	item = (RWCollectableString *)p->group_names( pos );

	return 1;
}

int Protocol::iterate_field_groups::operator()(int position)
{
	pos = position;

	if ( pos >= p->group_names.entries() )
	  return 0;

	item = (RWCollectableString *)p->group_names( pos );

	return 1;
}

RWCString Protocol::iterate_field_groups::name()
{
	return *item;
}

RWCString Protocol::iterate_field_groups::description()
{
	GroupInfo *info = (GroupInfo *)p->group_info.findValue( item );
	return info->long_description;
}

int Protocol::iterate_field_groups::get_properties( void )
{
	GroupInfo *info = (GroupInfo *)p->group_info.findValue( item );
	return info->get_properties();
}

Protocol::iterate_fields_of_group::iterate_fields_of_group( Protocol *_p, const char *group_name )
			: p(_p), grp_name( (tmp_string = group_name, tmp_string.toUpper(), tmp_string) )
{
	GroupInfo *gi = (GroupInfo *)_p->group_info.findValue( &grp_name );
	if ( gi == NULL )
	  next = NULL;
	else
	  next = new RWOrderedIterator( gi->fields );

	item = 0;
	field = 0;
}

Protocol::iterate_fields_of_group::~iterate_fields_of_group(void)
{
	if ( next )
	  delete next;
}

int Protocol::iterate_fields_of_group::operator()(void)
{
	if ( next == NULL )
	  return 0;

	item = (named_field *)(*next)();
	if ( item != 0 )
	  field = item->field();

	return item != 0;
}

RWCString Protocol::iterate_fields_of_group::variable_name(void)
{
	return item->field()->variable_name;
}

RWCString Protocol::iterate_fields_of_group::variable_group(void)
{
	return item->field()->variable_group;
}

RWCString Protocol::iterate_fields_of_group::variable_field(void)
{
	enum Protocol::field_type ui_field_type = item->field()->field_type;

	return item->field()->variable_field;
}

RWCString Protocol::iterate_fields_of_group::long_description(void)
{
	return item->field()->long_description;
}

RWCString Protocol::iterate_fields_of_group::controlled_by(void)
{
	return item->field()->controlled_by;
}

int Protocol::iterate_fields_of_group::min_valid(void)
{
	return item->field()->min_valid;
}

int Protocol::iterate_fields_of_group::max_valid(void)
{
	return item->field()->max_valid;
}

int Protocol::iterate_fields_of_group::get_bitwidth(void)
{
	return item->field()->bit_width;
}

// ACST.175
const Protocol::Table * Protocol::iterate_fields_of_group::get_table(void)
{
	return field->get_table();
}

Protocol::CustomUIOps * Protocol::iterate_fields_of_group::get_custom_ops(void)
{
	return field->get_custom_ops();
}

int Protocol::iterate_fields_of_group::get_properties(void)
{
	return field->get_properties();
}

Protocol::iterate_optional_groups::iterate_optional_groups( Protocol *_p )
												: next(_p->optional_group_names), p(_p)
{ }

Protocol::iterate_optional_groups::~iterate_optional_groups( void )
{ }

int Protocol::iterate_optional_groups::operator()(void)
{
	item = (RWCollectableString *)next();
	return item != 0;
}

RWCString Protocol::iterate_optional_groups::name()
{
	return *Protocol::iterate_optional_groups::item;
}

enum Protocol::field_type Protocol::iterate_fields_of_group::type(void)
{
	enum Protocol::field_type ui_field_type = field->field_type;

	switch( ui_field_type )
	{
		case Protocol::length0:
		case Protocol::length1:
		case Protocol::pos_indicator:
		case Protocol::neg_indicator:
				ui_field_type = Protocol::simple;
				break;

		case Protocol::bcd_pad8:
				ui_field_type = Protocol::bcd;
				break;

		default:
				ui_field_type = field->field_type;
	}

	return ui_field_type;
}

enum Protocol::field_format Protocol::iterate_fields_of_group::format(void)
{
	return field->field_format;
}

int Protocol::message_type_exists( const char *name )
{
	RWCollectableString msg_name( name );

	msg_name.toUpper();

	return msg_type_names.contains( &msg_name );
}

int Protocol::group_exists( const char *name )
{
	RWCollectableString grp_name( name );

	grp_name.toUpper();

	return group_names.contains( &grp_name );
}

int Protocol::optional_group_exists( const char *name )
{
	RWCollectableString grp_name( name );

	grp_name.toUpper();

	return optional_group_names.contains( &grp_name );
}

int Protocol::is_parm_of_msg( const char *msg_name, const char *parm_name )
{
	if ( ! message_type_exists( msg_name ) )
	  return 0;

	if ( ! group_exists( parm_name ) )
	  return 0;

	int is_parm = 0;
	RWCString parm = parm_name;
	parm.toUpper();
	iterate_groups_of_message_type next_group( this, msg_name );
	while ( next group() )
	{
		RWCString name = next_group.name();
		name.toUpper();
		if ( name == parm )
		{
		  is_parm = 1;
		  break;
		}
	}

	return is_parm;
}

int Protocol::message_type_index( const char *name )
{
	RWCollectableString s( name );
	s.toUpper();

	int index = msg_type_names.index( &s );
	if ( index == RW_NPOS )
	  throw InternalError ("message_type_index passed bad name \"%s\"", name );

	return index;
}

const char * Protocol::message_type_name( int index )
{
	if ( index >= msg_type_names.entries() )
	  return 0;

	const RWCollectableString *msg_name = (const RWCollectableString *) msg_type_names[ index ];
	const char *name = msg_name->data();

	return name;
}

int Protocol::group_index( const char *name )
{
	RWCollectableString s( name );
	s.toUpper();

	int index = group_names.index( &s );
	if ( index == RW_NPOS )
	  throw InternalError ("group_index passed bad name \"%s\"", name );

	return index;
}

const char * Protocol::get_group_name( int index )
{
	if ( index >= group_names.entries() )
	  return 0;

	const RWCollectableString *grp_name = (const RWCollectableString *) group_names[ index ];
	const char *name = grp_name->data();

	return name;
}

int Protocol::message_type_entries(void)
{
	return msg_type_names.entries();
}

int Protocol::group_entries()
{
	return group_names.entries();
}

int Protocol::header_entries()
{
	return ui_groups_in_header;
}
