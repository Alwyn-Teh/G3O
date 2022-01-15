/* EDITION AA02 (RELOOl), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+*******************************************************************

	Module Name:		g3opmdfh.h (was parmdef.h)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Dynamic ATP Parameter Definition Table Class

	History:
		Who			When				Description
	----------	--------------	-----------------------------
	Alwyn Teh	94Q4 - 95Q2		Initial Creation
	Alwyn Teh	4th July 1995	Complete the API

********************************************************************-*/

#ifndef _PARMDEF_HEADER_INCLUDED_
#define _PARMDEF_HEADER_INCLUDED_

#include <atph.h>
#include <g3odaryh.h>

enum opt_flag { MAND = 1, OPTL = 0 };

class parmdef : public DArray<Atp_ParmDefEntry>
{
public:
		parmdef() {}
		~parmdef() {}

		void BeginParms();
		void EndParms();

		void BeginList(const char *name, const char *desc);

		// Optional list parameter with default value
		void BeginList(const char *name, const char *desc, void *def_list_ptr);
		void EndList(opt_flag f = MAND);

		void BeginRepeat(const char *name, const char *desc,
						 int min, int max,
						 Atp_VprocType vproc);

		// Optional repeat block parameter with default value
		void BeginRepeat(const char *name, const char *desc,
						 Atp_DataDescriptor *def_rptblk_desc,
						 int min, int max, Atp_VprocType vproc);
		void EndRepeat (opt_flag f = MAND);

		void BeginChoice(const char *name, const char *desc, Atp_VprocType vproc);

		// Optional choice parameter with default value
		void BeginChoice(const char *name, const char *desc,
						 Atp_ChoiceDescriptor *def_choice_desc,
						 Atp_VprocType vproc);
		void EndChoice(opt_flag f = MAND);

		void Case(const char *name, const char *desc, int case_value);

		void BeginCase(const char *name, const char *desc, int case_value);
		void EndCase(opt_flag f = MAND);

		void NumDef(const char *name, const char *desc,
					Atp_NumType min, Atp_NumType max,
					Atp_VprocType vproc, Atp_KeywordType *KeyTabPtr = 0);

		// Optional number parameter with default value
		void NumDef(const char *name, const char *desc,
					Atp_NumType defval, Atp_NumType min, Atp_NumType max,
					Atp_VprocType vproc, Atp_KeywordType *KeyTabPtr = 0);

		void UnsignedNumDef(const char *name, const char *desc,
							Atp_UnsNumType min, Atp_UnsNumType max,
							Atp_VprocType vproc, Atp_KeywordType *KeyTabPtr = 0);

		// Optional unsigned number parameter with default value
		void UnsignedNumDef(const char *name, const char *desc,
							Atp_UnsNumType defval,
							Atp_UnsNumType min, Atp_UnsNumType max,
							Atp_VprocType vproc, Atp_KeywordType *KeyTabPtr = 0);

		void StrDef(const char *name, const char *desc, int minlen, int maxlen,
					Atp_VprocType vproc);

		// Optional string parameter with default value
		void StrDef(const char *name, const char *desc,
					const char *defstr, int minlen, int maxlen, Atp_VprocType vproc);

		void BcdDigitsDef(const char *name, const char *desc, int min, int max,
						  Atp_VprocType vproc);

		// Optional BCD digits parameter with default value
		void BcdDigitsDef(const char *name, const char *desc,
						  Atp_DataDescriptor *def_digs, int minlen, int maxlen,
						  Atp_VprocType vproc);

		void BoolDef(const char *name, const char *desc, Atp_VprocType vproc);

		// Optional boolean parameter with default value
		void BoolDef(const char *name, const char *desc, Atp_BoolType defval,
					 Atp_VprocType vproc);

		void KeywordDef(const char *name, const char *desc, Atp_KeywordType *keys,
						Atp_VprocType vproc);

		// Optional keyword parameter with default value
		void KeywordDef(const char *name, const char *desc, int default_key_value,
						Atp_KeywordType *keys, Atp_VprocType vproc);

		void DatabytesDef(const char *name, const char *desc, int minlen, int maxlen,
						  Atp_VprocType vproc);

		// Optional databytes parameter with optional value
		void DatabytesDef(const char *name, const char *desc,
						  Atp_DataDescriptor *def_databytes_desc,
						  int minlen, int maxlen,
						  Atp_VprocType vproc);

		void NullDef(const char *name, const char *desc);

protected:

private:

};

#endif /* _PARMDEF_HEADER_INCLUDED_ */
