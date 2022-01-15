/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+*******************************************************************

	Module Name:		g3oproth.h (was protocol.h)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Class Protocol Declaration

	Author:				Barry A. Scott
						Scott Concepts Limited
						Tel: [44]-1734-413655

	History:
		Who			When				Description
	-----------	--------------	----------------------------------
	Barry Scott	January 1995	Initial Creation
	Alwyn Teh	21 June 1995	Add get_table() to
								iterate_fields_of_group.
	Alwyn Teh	3rd July 1995	Protocol information banner
								to become part of protocol
								object.
	Alwyn Teh	10th July 1995	Add vproc field to UIField
								for SPCSIZE to ensure only
								14 or 24 bits. All associated
								classes will also support this.
	Alwyn Teh	11th July 1995	Add virtual UICustomInit() for
								any custom UI initializations
								for a specific protocol.
	Alwyn Teh	19th July 1995	Package vproc and other extra
								stuff in new class CustomUIOps.
	Alwyn Teh	4 August 1995	Convert Table_Ref storage code
								from fixed to dynamic arrays (DArray).
	Alwyn Teh	4 August 1995	Implement disp_header using new
								class Property.
	Alwyn Teh	15 August 1995	Add message_type_name() method.
	Alwyn Teh	28 Sept 1995	Fix range and status bugs for
								CTDP and BTUP using new properties
								add_one_to_control_value and
								add_one_to_control_value_except_zero.

********************************************************************-*/

#ifndef __TUP_PROTOCOL_H
#define __TUP_PROTOCOL_H

#include <rw/cstring.h>
#include <rw/sortvec.h>
#include <rw/hashdict.h>
#include <rw/collect.h>
#include <rw/collstr.h>
#include <rw/rwset.h>

#include <g3opdbgh.h>
#include <g3obboxh.h>

extern char *G3O_tool_name;
extern char *G3O_BNR_Copyright;
extern char *G3O_CCITT_CCS7;

/* these classes are used by g3oproth.h */

class user_variables;
class ProtocolFormat;
class bit_string;
class ostream;

// can't #include <g3ouih.h> due to mutual inclusion
class Genie3_cli;

class Protocol
{
public:
		Protocol( const char *protocol_name = 0 );
		virtual ~Protocol();

		virtual void UICustomInit( Genie3_cli * ) {}

		typedef char * (VprocType)( void *valPtr );

		RWSet decode_uv_names; // only one set per protocol allowed

		// Custom UI Operations to be performed when running application

		class CustomUIOps
		{
		public:
				enum custom_ui_ops_type
				{
					vproc_checking,
					user_variable_control,
					cic_filtering,
					video_fx,
					auto_cic
				};

				CustomUIOps( Protocol *p );
				CustomUIOps( Protocol *p, VprocType *_vproc, enum custom_ui_ops_type _t = vproc_checking );
				CustomUIOps( Protocol *p, const char *name, enum custom_ui_ops_type );
				~CustomUIOps( void );

				// This is the ATP vproc without the Atp_BoolType isUserValue argument
				// since commands and parameters generated are never optional.
				void set_vproc( VprocType *_vproc ) { vproc = _vproc; }
				VprocType * get_vproc( void ) { return vproc; }

				// Some default user variables are required when decoding e.g. SPCSIZE
				// This indicates the ones which should be copied to the decode user variables set
				// to help control decoding.
				void decode_with_default_uv( Protocol *p, const char *uv_name );

				// CIC filtering allows effective sharing of links using different CICs
				int get_cic_filter_flag( void ) { return cic_filtering_flag; }
				const char *get_cic_uv_name( void ) { return cic_filter_uv_name; }

				// Video highlighting effects
				int get_video_fx( void ) { return video_fx_flag; }

				// Auto-CIC feature
				int auto_cic_required( int flag ) { return ( auto_cic_feature_indicator = flag ); }
				int get_auto_cic_flag( void ) { return auto_cic_feature_indicator; }

		private:
				VprocType *vproc;
				Protocol *_protocol;
				int cic_filtering_flag;
				const char *cic_filter_uv_name;
				int video_fx_flag;
				int auto_cic_feature_indicator;
				int latest_incoming_cic_value;
		};

		//
		// forward declaration of classes
		//   indentation shows class hierarchy
		class Field;
			class UIField;
			class TypeCode;
			class Group;
				class Message;
				class LabelGroup;
				class SelectOne;
				class UIGroup;
				class LengthGroup;
				class OptionGroup;

		class Field_Ref;
			class UIField_Ref;
			class Group_Ref;
				class Message_Ref;
				class LabelGroup_Ref;
				class SelectOne_Ref;
				class UIGroup_Ref;
				class LengthGroup_Ref;
				class OptionGroup_Ref;

		class Table;
		class Table_Ref;

		enum PropertyType // bit fields
		{
			is_header	= 0x01,
			is_trailer	= 0x02,
			is_optional	= 0x04,
			per_link	= 0x08,
			invisible	= 0x10,

			add_one_to_control_value				= 0x20,	// for BTUP
			add_one_to_control_value_except_zero	= 0x40	// for CTUP
		};

		class Property;

		//
		//	Enums
		//
		enum field_type
		{
			unknown,			// no idea what this one is
			simple,				// fixed width set of bits
			bcd,				// BCD string (no padding)
			bcd_pad8,			// BCD string padded to 8 bit boundary
			bcd_to_end,			// BCD string to end of bit string
			octet,				// octet string
			octet_to_end,		// octet string to the end of the bit string
			bit8,				// bit string padded to 8 bit boundary
			length0,			// this field is the length of another - counts from 0 to n
			length1,			// this field is the length of another - counts from 0, 2 to n
			pos_indicator,		// this field is a positive indicator to include another field
			neg_indicator,		// this field is a negative indicator to include another field
			bcd_odd_indicator,	// this field is true if a bcd string has an odd number of digits
			group,				// a sequential group of fields
			type_code,			// fixed value type code
			pointer,			// ISUP pointer type field
			marker				// where an ISUP pointer points to
		};

		enum field_format
		{
			ff_spare,	// this field is not used in the protocol
			ff_group,	// this field is a grouping of other fields
			ff_int,		// this field is a signed integer
			ff_uint,	// this field is an unsigned integer
			ff_bcd,		// this field is BCD
			ff_ia5,		// this field is IA5
			ff_hex,		// this field is octets formated as HEX
			ff_bit,		// this field is a vector of bits
			ff_ascii	// this field is an ASCII string (for internal use ONLY)
		};

		//
		//	Class used to encapsulate a Field with a name
		//
		class named_field : public RWCollectableString
		{
		public:
				named_field( Field_Ref * );
				named_field( const char * );
				virtual ~named_field();
				Field_Ref *field( void );
				ostream &operator<< ( ostream &o ) ;
		protected:
				Field_Ref *the_field;
		};

		//
		//	iterator classes
		//
		class iterate_message_types
		{
		public:
				iterate_message_types( Protocol *p );
				int operator () () ;
				int operator()(int pos);
				RWCString name(void);
				RWCString description(void);
		private:
				Protocol *p;
				RWCollectableString *item;
				int pos;
		};

		class iterate_groups_of_message_type
		{
		public:
				iterate_groups_of_message_type( Protocol *p, const char *message_name );
				~iterate_groups_of_message_type(void);
				int operator() () ;
				RWCString name(void);
				RWCString description(void);
				int get_properties(void) ;
		private:
				RWCollectableString msg_name;
				Protocol *p;
				RWCollectableString *item;
				RWOrderedIterator *next;
		};

		class iterate_field_groups
		{
		public:
				iterate_field_groups( Protocol *p );
				int operator() ();
				int operator() (int pos);
				RWCString name(void);
				RWCString description(void);
				int get_properties(void);
		private:
				Protocol *p;
				RWCollectableString *item;
				int pos;
		};

		class iterate_fields_of_group
		{
		public:
				iterate_fields_of_group( Protocol *p, const char *group_name );
				~iterate_fields_of_group(void);
				int operator() ();
				RWCString variable_name(void);
				RWCString variable_group(void);
				RWCString variable_field(void);
				enum field_type type(void);
				enum field_format format(void);
				RWCString long_description(void);
				RWCString controlled_by(void);
				int min_valid(void);
				int max_valid(void);
				int get_bitwidth(void);
				const Table * get_table(void);
				Protocol::CustomUIOps * get_custom_ops( void );
				int get_properties();
		private:
				RWCollectableString grp_name;
				Protocol *p;
				Protocol::named_field *item;
				Field_Ref *field;
				RWOrderedIterator *next;
		};

		class iterate_optional_groups
		{
		public:
				iterate_optional_groups( Protocol *p );
				~iterate_optional_groups(void);
				int operator() () ;
				RWCString name(void);
		private:
				Protocol *p;
				RWCollectableString *item;
				RWSortedVectorIterator next;
		};

		int message_type_exists( const char *name );
		int group_exists( const char *name );
		int optional_group_exists( const char *name );
		int is_parm_of_msg( const char *msg_name, const char *parm_name );

		int message_type_index( const char *name );
		const char * message_type_name ( int index );

		int group_index( const char *name );
		const char * get_group_name( int index );

		int message_type_entries();
		int group_entries();
		int header_entries();

		friend class iterate_message_types;
		friend class iterate_groups_of_message_type;
		friend class iterate_field_groups;
		friend class iterate_fields_of_group;
		friend class iterate_optional_groups;

		// these work on whole messages
		void Encode( const char *msg_type_name, user_variables *uv, bit_string *msg );
		void Decode( bit_string *msg, user_variables *uv );
		void Format( bit_string *msg, user_variables *uv, ProtocolFormat *pf );

		// these overloads work on single UIGroup part of a message
		void Encode( user_variables *uv, bit_string *msg, const char *ui_group_name );

		void Decode( bit_string *msg, user_variables *uv, const char *ui_group_naine );
		void Format( bit_string *msg, user_variables *uv, ProtocolFormat *pf, const char *ui_group_name );

		// Make name strings with special meanings
		static RWCString OverrideName( const char *group_name );
		static RWCString OptionName( const char *group_name );
		static RWCString MessageTypeName( void );

		// used for debugging
#if _DEBUG_DESCRIBE
		void _describe(void);
		void _print_rw_vars(void);
#endif

		//
		//	Exception classes
		//
		class Exception
		{
		public:
				Exception(void);
				int code;
		};

		class FieldNotDefined : public Exception
		{
		public:
				RWCString group;
				RWCString field;
				FieldNotDefined( RWCString &name );
		};

		class FieldTooSmall : public FieldNotDefined
		{
		public:
				FieldTooSmall( RWCString &name );
		};

		class InternalError : public Exception
		{
		public:
				RWCString message;
				InternalError( const char *m );
				InternalError( const char *m, int a );
				InternalError( const char *m, const char *a );
		};

		class MessageShort : public Exception { };
		class MessageLong : public Exception { };
		class UnknownMessageType : public Exception { };
		class TypeCodeMismatch : public Exception { };

public:
		//
		//	RefCount class use in xxx_Ref classes
		//
		class RefCount
		{
		public:
				RefCount(void); // { ref_ = 0; }
				virtual ~RefCount(void);
				void addRef(void); // { ref_++; }
				int removeRef(void); // { ref_--; return ref_; }
		private:
				int ref_;
		};

		//
		//	these classes are used by Protocol only
		//
		class Table_Ref;
		class Table
		{
		public:
				Table( int init_tab_size = 64 );
				virtual ~Table();

				Table(const Table &);
				Table &operator=( const Table &t );

				Table &operator+( int msg_type );
				Table &operator-( int msg_type );
				Table &operator+( const char *string );

				const char *operator[]( int value );

				// ACST.175
				typedef struct
				{
					int	value;
					const char * string;
				} num_str_pair;

				const num_str_pair next( int mode = -1 ); // 0 resets to beginning of table

				Table_Ref *ref;

				void set_bit_width( unsigned int width );
				unsigned int get_bit_width(void) const { return bit_width; }

		protected:
				void Setup(void);
		private:
				int initial_table_size;
				unsigned int bit_width;
		};

		class Table_Ref : public RefCount
		{
		public:
				Table_Ref( int initial_table_size = 64 );

				void plus( int msg_type );
				void minus( int msg_type );
				void plus( const char *string );

				const char *index( int value );

				void set_bit_width( unsigned int width ) { bit_width = width; }
				unsigned int get_bit_width(void) { return bit_width; }

				int get_num_items(void) { return num_items; }
				int get_item_value( int index ) { return item_values[index]; }
				const char * get_item_strings( int index ) { return item_strings[index]; }

		protected:
				int num_items; // the number of messages in the Table
				DArray<const char *> item_strings;
				DArray<int> item_values;
		private:
				int first, last;	// that range of values to set
				unsigned int bit_width;
		};

		// base message class
		class Field // : public RWCollectable
		{
		public:
				Field(void);
				virtual ~Field();
				Field( const Field & );
				Field & operator=( const Field & );
				Field & operator*( const Property & );

				// fixed width fields, simple, length, indicator ( with or without CustomUIOps )
				Field( const char *varname, enum field_type type, enum field_format fmt,
						int width, const char *longdesc,
						Protocol::CustomUIOps *cops_objptr = 0 );
				Field( const char *varname, enum field_type type, enum field_format fmt,
						int width, const char *controller, const char *longdesc,
						Protocol::CustomUIOps *cops_objptr = 0 );

				// ... with a restricted set of valid values ( with or without CustomUIOps )
				Field( const char *varname, enum field_type type, enum field_format fmt,
						int width, int min_val, int max_val, const char *longdesc,
						Protocol::CustomUIOps *cops_objptr = 0 );
				Field( const char *varname, enum field_type type, enum field_format fmt,
						int width, int min_val, int max_val, const char *controller, const char *longdesc,
						Protocol::CustomUIOps *cops_objptr = 0 );

				// ... and Table ( with or without CustomUIOps )
				Field( const char *varname, enum field_type type, enum field_format fmt,
						int width, const char *longdesc, const Table Stable );
				Field( const char *varname, enum field_type type, enum field_format fmt,
						int width, const char *longdesc, Protocol::CustomUIOps *cops_objptr,
						const Table Stable );

				Field( const char *varname, enum field_type type, enum field_format fmt,
						int width, int min_val, int max_val, const char *longdesc, const Table Stable );
				Field( const char *varname, enum field_type type, enum field_format fmt,
						int width, int min_val, int max_val, const char *longdesc,
						Protocol::CustomUIOps *cops_objptr, const Table Stable );

				Field( const char *varname, enum field_type type, enum field_format fmt,
						int width, int min_val, int max_val, const char *controller,
						const char *longdesc, const Table stable );
				Field( const char *varname, enum field_type type, enum field_format fmt,
						int width, int min_val, int max_val, const char *controller,
						const char *longdesc, Protocol::CustomUIOps *cops_objptr, const Table Stable );

				Field( const char *varname, enum field_type type, enum field_format fmt,
						int width, const char *controller, const char *longdesc, const Table Stable );
				Field( const char *varname, enum field_type type, enum field_format fmt,
						int width, const char *controller, const char *longdesc,
						Protocol::CustomUIOps *cops_objptr, const Table Stable );

				Field_Ref *ref;

		protected:
				// this constructor is used to suppress init'ing ref
				Field( Field_Ref * );

				// constructor helper function
				void FieldInit(	const char *varname, enum field_type type, enum field_format fmt,
								int width, const Table &table,
								const char *controller, const char *longdesc,
								Protocol::CustomUIOps *cops_objptr = 0 );

				void FieldInit( const char *varname, enum field_type type, enum field_format fmt,
								int width, int min_val, int max_val, const Table &table,
								const char *controller, const char *longdesc,
								Protocol::CustomUIOps *cops_objptr = 0 );
		};

		class Field_Ref : public RefCount
		{
		public:
				Field_Ref(void);
				virtual ~Field_Ref();
				Field_Ref( const char *varname, enum field_type type, enum field_format fmt,
							int width, int min_val, int max_val, const Table &table,
							const char *controller, const char *longdesc, Protocol::CustomUIOps *cops_objptr = 0 );

				virtual void Encode( Protocol *p, user_variables *uv, bit_string *msg );
				virtual void Decode( Protocol *p, bit_string *msg, user_variables *uv );
				virtual void Format( Protocol *p, bit_string *msg, user_variables *uv, ProtocolFormat *pf );

#if _DEBUG_DESCRIBE
				virtual void _describe(int depth);
#endif

				virtual void _build_ui_database( Protocol *p, user_variables *uv, const char *group_name );

				RWCString _variable_field(void) { return variable_field; }

				const char *Description(void) { return long_description; }

				virtual void plus( int ) { throw InternalError( "Field_Ref::plus called" ); }
				virtual void plus( const Field & ) { throw InternalError{ "Field_Ref::plus called" }; }

				const Table * get_table(void) { return (const Table *) &table_of_names; }

				Protocol::CustomUIOps * get_custom_ops( void ) { return custom_ops; }

				friend class Protocol::iterate_fields_of_group;

				void set_properties( int prop ) { properties = prop; }
				int get_properties(void) { return properties; }
				int set_property_masks( int masks ) { return ( properties |= masks ); }

		protected:
				int bit_width;						// width of the field in bits
				int min_valid, max_valid;			// range of values in an integer field
				RWCString variable_group;			// name of variable group that this field is a part of
				RWCString variable_field;			// name	of variable	field within the group
				RWCString variable_name;			// full	name of the	variable to hold this fields value
				RWCString long_description;			// full	description	of this field
				enum field_type field_type;			// the storage type	of this field
				enum field_format field_format;		// how to format this field
				RWCString controlled_by;			// the field that controls inclusion of this one
				Table table_of_names;
				ProtocolCustomUIOps *custom_ops;	// contains any custom UI operations

		private:
				int properties;						// bit flags of PropertyType
		};

		class UIField : public Field
		{
		public:
				UIField(const char *ui_field_name, enum field_type type, enum field_format fmt,
						int width, int min_val, int max_val, int def_value,
						const char *description, Table &table);
				UIField(const char *ui_field_name, enum field_type type, enum field_format fmt,
						int width, int min_val, int max_val, int def_value,
						const char *description, Protocol::CustomUIOps *cops_objptr, Table &table );
		};

		class UIField_Ref : public Field_Ref
		{
		public:
				UIField_Ref(const char *ui_field_name, enum field_type type, enum field_format fmt,
							int width, int min_val, int max_val, int def_value,
							const char *description, Table &table);
				UIField_Ref(const char *ui_field_name, enum field_type type, enum field_format fmt,
							int width, int min_val, int max_val, int def_value,
							const char *description, Protocol::CustomUIOps *cops_objptr, Table &table);

				virtual void Encode( Protocol *p, user_variables *uv, bit_string *msg );
				virtual void Decode( Protocol *p, bit_string *msg, user_variables *uv );
				virtual void Format( Protocol *p, bit_string *msg, user_variables *uv, ProtocolFormat *pf );

#if _DEBUG_DESCRIBE
				virtual void _describe(int depth);
#endif
				virtual void _build_ui_database( Protocol *p, user_variables *uv, const char *group_name );

		private:
				int default_value;
		};

		class TypeCode : public Field
		{
		public:
				TypeCode( const char *description, int code, int width = 8, enum field_format field_format = ff_uint );
		};

		class Group : public Field
		{
		public:
				Group(void);
				Group( const Group & );
				Group & operator= ( const Group & );

				Group( const char *controller );
				//	Group( const char *varname, enum field_type type, enum field_format fmt,
				//			int width, const char *longdesc );
				//	Group( const char *varname, enum field_type type, enum field_format fmt,
				//			int width, const char *longdesc, Table &table );

				Group &operator+( const Field& m );
				Group &operator+( const UIField& m );
				Group &operator+( const Group& m );
				Group &operator+( const LengthGroup& m );
				Group &operator+( const OptionGroup& m );
				Group &operator+( const UIGroup& m );
				Group &operator+( const LabelGroup& m );
				Group &operator+( const SelectOne& m );
				Group &operator+( const Message& m );

		protected:
				// this constructor is used to suppress init'ing ref
				Group( Field_Ref * );
			};

		enum { Group_max_msgs = 100 };

		class Group_Ref : public Field_Ref
		{
		public:
				virtual ~Group_Ref();

				Group_Ref( void );
				Group_Ref( const char *controller );

				virtual void Encode( Protocol *p, user_variables *uv, bit_string *msg );
				virtual void Decode( Protocol *p, bit_string *msg, user_variables *uv );
				virtual void Format( Protocol *p, bit_string *msg, user_variables *uv, ProtocolFormat *pf );

#if _DEBUG_DESCRIBE
				virtual void _describe(int depth);
#endif
				virtual void _build_ui_database( Protocol *p, user_variables *uv, const char *group_name );

				virtual void plus( int a ) { Field_Ref::plus( a ); }
				virtual void plus( const Field & );

		protected:
				Group Ref( const char *varname, enum field_type type, enum field_format fmt,
							int width, const char *longdesc, const Table &table );
				int num_msgs;						// the number of messages in the group
				Field_Ref *msgs[Group_max_msgs];	// pointer to the group of message parts
		};

		class UIGroup : public Group
		{
		public:
				UIGroup( const UIGroup & );
				UIGroup & operator=( const UIGroup & );
				UIGroup & operator*( const Property & );

				UIGroup( const char *ui_group_name, const char *description );

				UIGroup &operator+( const Field& m );
				UIGroup &operator+( const UIField& m );
				UIGroup &operator+( const Group& m );
				UIGroup &operator+( const LengthGroup& m );
				UIGroup &operator+( const OptionGroup& m );
				UIGroup &operator+( const UIGroup& m );
				UIGroup &operator+( const LabelGroup& m );
				UIGroup &operator+( const SelectOne& m );
				UIGroup &operator+( const Message& m );
		};

		class UIGroup_Ref : public Group_Ref
		{
		public:
				UIGroup_Ref( const char *ui_group_name, const char *description );

				virtual void Encode( Protocol *p, user_variables *uv, bit_string *msg );
				virtual void Decode( Protocol *p, bit_string *msg, user_variables *uv );
				virtual void Format( Protocol *p, bit_string *msg, user_variables *uv, ProtocolFormat *pf );
#if _DEBUG_DESCRIBE
				virtual void _describe(int depth);
#endif
				virtual void _build_ui_database( Protocol *p, user_variables *uv, const char *group_name );
		};

		class LabelGroup : public Group
		{
		public:
				LabelGroup(const LabelGroup &);
				LabelGroup &operator=(const LabelGroup &);

				LabelGroup( const char *description, const char *ui_group = NULL );

				LabelGroup &operator+( const Field& m );
				LabelGroup &operator+( const UIField& m );
				LabelGroup &operator+( const Group& m );
				LabelGroup &operator+( const Message& m );
				LabelGroup &operator+( const LengthGroup& m );
				LabelGroup &operator+( const OptionGroup& m );
				LabelGroup &operator+( const UIGroup& m );
				LabelGroup &operator+( const SelectOne& m );
				LabelGroup &operator+( const LabelGroup& m );
		};

		class LabelGroup_Ref : public Group_Ref
		{
		public:
				LabelGroup_Ref( const char *description, const char *ui_group );

				virtual void Encode( Protocol *p, user_variables *uv, bit_string *msg );
				virtual void Decode( Protocol *p, bit_string *msg, user_variables *uv );
				virtual void Format( Protocol *p, bit_string *msg, user_variables *uv, ProtocolFormat *pf );

#if _DEBUG_DESCRIBE
				virtual void _describe(int depth);
#endif
				virtual void _build_ui_database( Protocol *p, user_variables *uv, const char *group_name );
		protected:
		};

		class SelectOne : public Group
		{
		public:
				SelectOne( const char *varname );
				SelectOne( const char *varname, enum field_format fmt,
							int width, const char *longdesc );
				SelectOne( const char *varname, enum field_format fmt,
							int width, const char *longdesc, const Table Stable );

				SelectOne &operator+( int msg_type );
				SelectOne &operator+( const Field& m );
				SelectOne &operator+( const UIField& m );
				SelectOne &operator+( const Group& m );
				SelectOne &operator+( const LengthGroup& m );
				SelectOne &operator+( const OptionGroup& m );
				SelectOne &operator+( const UIGroup& m );
				SelectOne &operator+( const Message& m );
				SelectOne &operator+( const LabelGroup& m );
				SelectOne &operator+( const SelectOne& m );
		};

		class SelectOne_Ref : public Group_Ref
		{
		public:
				SelectOne_Ref( const char *varname, enum field_format fmt,
								int width, const char *longdesc, const Table &table );

				virtual void plus( int );
				virtual void plus( const Field &f ) { Group_Ref::plus( f ); }

				virtual void Encode( Protocol *p, user_variables *uv, bit_string *msg );
				virtual void Decode( Protocol *p, bit_string *msg, user_variables *uv );
				virtual void Format( Protocol *p, bit_string *msg, user_variables *uv, ProtocolFormat *pf );
#if _DEBUG_DESCRIBE
				virtual void _describe(int depth) ;
#endif
				virtual void _build_ui_database( Protocol *p, user_variables *uv, const char *group_name );
		protected:
				int msg_types[Group_max_msgs];	// the msg_type values that select the above messages
		};

		class LengthGroup : public Group
		{
		public:
				LengthGroup( const LengthGroup & );
				LengthGroup & operator=( const LengthGroup & );

				LengthGroup( const char *var_name, int width, const char *description );

				LengthGroup &operator+( const Field& m );
				LengthGroup &operator+( const UIField& m );
				LengthGroup &operator+( const Group& m );
				LengthGroup &operator+( const UIGroup& m );
				LengthGroup &operator+( const OptionGroup& m );
				LengthGroup &operator+( const LengthGroup& m );
				LengthGroup &operator+( const LabelGroup& m );
				LengthGroup &operator+( const SelectOne& m );
				LengthGroup &operator+( const Message& m );
		};

		class LengthGroup_Ref : public Group_Ref
		{
		public:
				LengthGroup_Ref( const char *var_name, int width, const char *description );

				virtual void Encode( Protocol *p, user_variables *uv, bit_string *msg );
				virtual void Decode( Protocol *p, bit_string *msg, user_variables *uv );
				virtual void Format( Protocol *p, bit_string *msg, user_variables *uv, ProtocolFormat *pf );
#if _DEBUG_DESCRIBE
				virtual void _describe(int depth);
#endif
				virtual void _build_ui_database( Protocol *p, user_variables *uv, const char *group_name );
		};

		class OptionGroup : public Group
		{
		public:
				OptionGroup( const OptionGroup & );
				OptionGroup & operator=( const OptionGroup & );

				OptionGroup( const char *ui_group_name, const char *description );

				OptionGroup &operator+( const Field& m );
				OptionGroup &operator+( const UIField& m );
				OptionGroup &operator+( const Group& m );
				OptionGroup &operator+( const UIGroup& m );
				OptionGroup &operator+( const OptionGroup& m );
				OptionGroup &operator+( const LengthGroup& m );
				OptionGroup &operator+( const LabelGroup& m );
				OptionGroup &operator+( const SelectOne& m );
				OptionGroup &operator+( const Message& m );
		};

		class OptionGroup_Ref : public Group_Ref
		{
		public:
				OptionGroup_Ref( const char *ui_group_name, const char *description );

				virtual void Encode( Protocol *p, user_variables *uv, bit_string *msg );
				virtual void Decode( Protocol *p, bit_string *msg, user_variables *uv );
				virtual void Format( Protocol *p, bit_string *msg, user_variables *uv, ProtocolFormat *pf );
#if _DEBUG_DESCRIBE
				virtual void _describe(int depth);
#endif
				virtual void _build_ui_database( Protocol *p, user_variables *uv, const char *group_name );
		};

		class Message : public Group
		{
		public:
				Message( const char *msg_name, const char *longdesc );

				Message &operator+( const Field& m );
				Message &operator+( const UIField& m );
				Message &operator+( const Group& m );
				Message &operator+( const UIGroup& m );
				Message &operator+( const LengthGroup& m );
				Message &operator+( const OptionGroup& m );
				Message &operator+( const LabelGroup& m );
				Message &operator+( const SelectOne& m );
				// not valid to add a Message to a Message
		};

		class Message_Ref : public Group_Ref
		{
		public:
				Message_Ref( const char *msg_name, const char *longdesc );

				virtual void Encode( Protocol *p, user_variables *uv, bit_string *msg );
				virtual void Decode( Protocol *p, bit_string *msg, user_variables *uv );
				virtual void Format( Protocol *p, bit_string *msg, user_variables *uv, ProtocolFormat *pf );
#if _DEBUG_DESCRIBE
				virtual void _describe(int depth);
#endif
				virtual void _build_ui_database( Protocol *p, user_variables *uv, const char *group_name );
		};

		//
		//	Class used to hold information about a UI Group
		//
		class GroupInfo : public RWCollectable
		{
		public:
				GroupInfo( UIGroup_Ref *ui, const char *desc );
				virtual ~GroupInfo();

				void set_properties( int prop ) { properties = prop; }
				int get_properties(void) { return properties; }
				int set_property_masks( int masks ) { return ( properties |= masks ); }

		public:
				UIGroup_Ref *ui_group;
				RWCString long_description;
				RWOrdered fields;

		private:
				int properties; // easier to keep/search properties here than in Field_Ref for UIGroup !
		};

		//
		//	Class used to hold the details of a pointer
		//
		class pointer_details : public RWCollectable
		{
		public:
				pointer_details( int s, int w );
				int Start(void);
				int Width(void);
		protected:
				int start, width;
		};

		class Property
		{
		public:
				Property( enum Protocol::PropertyType mask );
				Property( int properties );
				~Property();

				int get_property_masks() const { return property_masks; }

		private:
				int property_masks;
		};

		friend void Field_Ref::_build_ui_database( Protocol *p, user_variables *uv, const char *group_name );
		friend void Group_Ref::_build_ui_database( Protocol *p, user_variables *uv, const char *group_name );
		friend void UIGroup_Ref::_build_ui_database( Protocol *p, user_variables *uv, const char *group_name );

		void build_ui_database(void);
		void format_excess_data( bit_string *msg, ProtocolFormat *pf );

		// sort list of names for the UI interface routines to use
		RWSortedVector msg_type_names;
		RWSortedVector group_names;
		RWSortedVector optional_group_names;

		// This dictionary is a map between msg_type_names and their long descriptions
		RWHashDictionary msg_type_descriptions;

		// The entries in this dictionary hold the user_vars needed for encoding
		RWHashDictionary msg_type_uservars;

		// The entries in this dictionary hold a RWOrdered list of UIgroups used a message
		RWHashDictionary msg_type_groups;

		// The entries in this dictionary are a RWOrderVector of Fields
		RWHashDictionary group_info;

		// The Fointer resolution dictionary
		RWHashDictionary pointer_info;

		// These variables are used to record all the UIGroups seen
		enum { num_ui_groups = 100 };
		int ui_group_index;
		RWCString *ui_groups[num_ui_groups];

		int ui_groups_in_header;

		// the protocol
		Group prot;

		// protocol information
		const char *ProtocolName;
		const char *ProtocolDesc;
		G3O_BannerBox banner;
		char *basic_banner_title;
};

#endif /* _TUP_PROTOCOL_H */
