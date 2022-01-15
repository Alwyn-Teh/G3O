/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */
/*+*******************************************************************

	Module Name:		g3oprmdf.cxx (was parm_def.cxx)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Dynamic ATP Parmdef Implementation

	History:
		Who			When				Description
	----------	-------------	-------------------------------------
	Alwyn Teh	94Q4 - 95Q2		Initial Creation
	Alwyn Teh	21 June 1995	Add KeyTabPtr field	to end
								of NumDef() & UnsignedNumDef()
								to contain value meanings of
								protocol fields.
	Alwyn Teh	4th July 1995	Complete the API

*******************************************************************-*/

#include <atph.h>
#include <g3opmdfh.h>

static Atp_ParmDefEntry zero_pde = {0};

void parmdef::BeginParms()
{
	Atp_ParmDefEntry pde = zero_pde;
	pde.parmcode = ATP_BPM;
	pde.Name = "BEGIN_PARMS";
	append(pde);
}

void parmdef::EndParms()
{
	Atp_ParmDefEntry pde = zero_pde;
	pde.parmcode = ATP_EPM;
	pde.Name = "END_PARMS";
	append(pde);
}

void parmdef::BeginList(const char *name, const char *desc)
{
	Atp_ParmDefEntry pde = zero_pde;
	pde.parmcode = ATP_BLS;
	pde.parser = Atp_ProcessListConstruct;
	pde.Name = (char *)name;
	pde.Desc = (char *)desc;
	append(pde);
}

void parmdef::BeginList(const char *name, const char *desc, void *def_list_ptr)
{
	Atp_ParmDefEntry pde = zero_pde;
	pde.parmcode = Atp_SetOptParm(ATP_BLS);
	pde.parser = Atp_ProcessListConstruct;
	pde.Name = (char *)name;
	pde.Desc = (char *)desc;
	pde.DataPointer = def_list_ptr;
	append(pde);
}

void parmdef::EndList(opt_flag f)
{
	Atp_ParmDefEntry pde = zero_pde;
	if (f == OPTL)
	{
	  pde.parmcode = Atp_SetOptParm(ATP_ELS);
	  pde.Name = "END_OPT_LIST";
	}
	else
	{
	  pde.parmcode = ATP_ELS;
	  pde.Name = "END_LIST";
	}
	append(pde);
}

void parmdef::BeginRepeat(const char *name, const char *desc, int min, int max,
Atp_VprocType vproc)
{
	Atp_ParmDefEntry pde = zero_pde;
	pde.parmcode = ATP_BRP;
	pde.parser = Atp_ProcessRepeatBlockConstruct;
	pde.Name = (char *)name;
	pde.Desc = (char *)desc;
	pde.Min = min;
	pde.Max = max;
	pde.vproc = vproc;
	append(pde);
}

void parmdef::BeginRepeat(const char *name, const char *desc,
						  Atp_DataDescriptor *def_rptblk_desc,
						  int min, int max, Atp_VprocType vproc)
{
	Atp_ParmDefEntry pde = zero_pde;
	pde.parmcode = Atp_SetOptParm(ATP_BRP);
	pde.parser = Atp_ProcessRepeatBlockConstruct;
	pde.Name = (char *)name;
	pde.Desc = (char *)desc;
	pde.Min = min;
	pde.Max = max;
	pde.vproc = vproc;
	pde.DataPointer = def_rptblk_desc;
	append(pde);
}

void parmdef::EndRepeat(opt_flag f)
{
	Atp_ParmDefEntry pde = zero_pde;
	if (f == OPTL)
	{
	  pde.parmcode = Atp_SetOptParm(ATP_ERP);
	  pde.Name = "END_OPT_REPEAT";
	}
	else
	{
	  pde.parmcode = ATP_ERP;
	  pde.Name = "END_REPEAT";
	}
	pde.parser = (Atp_ParserType)Atp_ParseRptBlkMarker;
	append(pde);
}

void parmdef::BeginChoice(const char *name, const char *desc,
						  Atp_VprocType vproc)
{
	Atp_ParmDefEntry pde = zero_pde;
	pde.parmcode = ATP_BCH;
	pde.parser = Atp_ProcessChoiceConstruct;
	pde.Name = (char *)name;
	pde.Desc = (char *)desc;
	pde.vproc = vproc;
	append(pde);
}

void parmdef::BeginChoice(const char *name, const char *desc,
						  Atp_ChoiceDescriptor *def_choice_desc,
						  Atp_VprocType vproc)
{
	Atp_ParmDefEntry pde = zero_pde;
	pde.parmcode = Atp_SetOptParm(ATP_BCH);
	pde.parser = Atp_ProcessChoiceConstruct;
	pde.Name = (char *)name;
	pde.Desc = (char *)desc;
	pde.vproc = vproc;
	pde.DataPointer = def_choice_desc;
	append(pde);
}

void parmdef::EndChoice(opt_flag f)
{
	Atp_ParmDefEntry pde = zero_pde;
	if (f == OPTL)
	{
	  pde.parmcode = Atp_SetOptParm(ATP_ECH);
	  pde.Name = "END_OPT_CHOICE" ;
	}
	else
	{
	  pde.parmcode = ATP_ECH;
	  pde.Name = "END_CHOICE";
	}
	append(pde);
}

void parmdef::Case(const char *name, const char *desc, int case_value)
{
	parmdef::BeginCase(name, desc, case_value);
	parmdef::EndCase(MAND);
}

void parmdef::BeginCase(const char *name, const char *desc, int case_value)
{
	Atp_ParmDefEntry pde = zero_pde;
	pde.parmcode = ATP_BCS;
	pde.parser = Atp_ProcessCaseConstruct;
	pde.Name = (char *)name;
	pde.Desc = (char *)desc;
	pde.Default = case_value;
	append(pde);
}

void parmdef::EndCase(opt_flag)
{
	Atp_ParmDefEntry pde = zero_pde;
	pde.parmcode = ATP_ECS;
	append(pde);
}

void parmdef::NumDef(const char *name, const char *desc,
					 Atp_NumType min, Atp_NumType max,
					 Atp_VprocType vproc, Atp_KeywordType *KeyTabPtr)
{
	Atp_ParmDefEntry pde = zero_pde;
	pde.parmcode = ATP_NUM;
	pde.parser = Atp_ProcessNumParm;
	pde.Name = (char *)name;
	pde.Desc = (char *) desc;
	pde.Min = min;
	pde.Max = max;
	pde.vproc = vproc;
	pde.KeyTabPtr = KeyTabPtr;
	append(pde);
}

void parmdef::NumDef(const char *name, const char *desc,
					 Atp_NumType defval, Atp_NumType min, Atp_NumType max,
					 Atp_VprocType vproc, Atp_KeywordType *KeyTabPtr)
{
	Atp_ParmDefEntry pde = zero_pde;
	pde.parmcode = Atp_SetOptParm(ATP_NUM);
	pde.parser = Atp_ProcessNumParm;
	pde.Name = (char *)name;
	pde.Desc = (char *)desc;
	pde.Min = min;
	pde.Max = max;
	pde.Default = defval;
	pde.vproc = vproc;
	pde.KeyTabPtr = KeyTabPtr;
	append(pde);
}

void parmdef::UnsignedNumDef(const char *name, const char *desc,
							 Atp_UnsNumType min, Atp_UnsNumType max,
							 Atp_VprocType vproc,
							 Atp_KeywordType * KeyTabPtr)
{
	Atp_ParmDefEntry pde = zero_pde;
	pde.parmcode = ATP_UNS_NUM;
	pde.parser = Atp_ProcessUnsNumParm;
	pde.Name = (char *)name;
	pde.Desc = (char *)desc;
	pde.Min = min;
	pde.Max = max;
	pde.vproc = vproc;
	pde.KeyTabPtr = KeyTabPtr;
	append(pde);
}

void parmdef::UnsignedNumDef(const char *name, const char *desc,
							 Atp_UnsNumType defval,
							 Atp_UnsNumType min, Atp_UnsNumType max,
							 Atp_VprocType vproc,
							 Atp_KeywordType * KeyTabPtr)
{
	Atp_ParmDefEntry pde = zero_pde;
	pde.parmcode = Atp_SetOptParm(ATP_UNS_NUM) ;
	pde.parser = Atp_ProcessUnsNumParm;
	pde.Name = (char *)name;
	pde.Desc = (char *)desc;
	pde.Min = min;
	pde.Max = max;
	pde.Default = defval;
	pde.vproc = vproc;
	pde.KeyTabPtr = KeyTabPtr;
	append(pde);
}

void parmdef::StrDef(const char *name, const char *desc, int minlen, int maxlen,
					 Atp_VprocType vproc)
{
	Atp_ParmDefEntry pde = zero_pde;
	pde.parmcode = ATP_STR;
	pde.parser = Atp_ProcessStrParm;
	pde.Name = (char *)name;
	pde.Desc = (char *)desc;
	pde.Min = minlen;
	pde.Max = maxlen;
	pde.vproc = vproc;
	append(pde);
}

void parmdef::StrDef(const char *name, const char *desc,
					 const char *defstr, int minlen, int maxlen,
					 Atp_VprocType vproc)
{
	Atp_ParmDefEntry pde = zero_pde;
	pde.parmcode = Atp_SetOptParm(ATP_STR);
	pde.parser = Atp_ProcessStrParm;
	pde.Name = (char *)name;
	pde.Desc = (char *)desc;
	pde.Min = minlen;
	pde.Max = maxlen;
	pde.DataPointer = (void *)defstr;
	pde.vproc = vproc;
	append(pde);
}

void parmdef::BcdDigitsDef(const char *name, const char *desc,
						   int minlen, int maxlen,
						   Atp_VprocType vproc)
{
	Atp_ParmDefEntry pde = zero_pde;
	pde.parmcode = ATP_BCD;
	pde.parser = Atp_ProcessBcdDigitsParm;
	pde.Name = (char *)name;
	pde.Desc = (char *)desc;
	pde.Min = minlen;
	pde.Max = maxlen;
	pde.vproc = vproc;
	append(pde);
}

void parmdef::BcdDigitsDef(const char *name, const char *desc,
						   Atp_DataDescriptor *def_digs, int minlen, int maxlen,
						   Atp_VprocType vproc)
{
	Atp_ParmDefEntry pde = zero_pde;
	pde.parmcode = Atp_SetOptParm(ATP_BCD);
	pde.parser = Atp_ProcessBcdDigitsParm;
	pde.Name = (char *)name;
	pde.Desc = (char *)desc;
	pde.Min = minlen;
	pde.Max = maxlen;
	pde.DataPointer = def_digs;
	pde.vproc = vproc;
	append(pde);
}

void parmdef::BoolDef(const char *name, const char *desc, Atp_VprocType vproc)
{
	Atp_ParmDefEntry pde = zero_pde;
	pde.parmcode = ATP_BOOL;
	pde. parser = Atp_ProcessBoolParm;
	pde.Name = (char *)name;
	pde.Desc = (char *)desc;
	pde.Min = 0;
	pde.Max = 1;
	pde.vproc = vproc;
	append(pde);
}

void parmdef::BoolDef(const char *name, const char *desc,
					  Atp_BoolType defval, Atp_VprocType vproc)
{
	Atp_ParmDefEntry pde = zero_pde;
	pde.parmcode = Atp_SetOptParm(ATP_BOOL);
	pde.parser = Atp_ProcessBoolParm;
	pde.Name = (char *)name;
	pde.Desc = (char *)desc;
	pde.Min = 0;
	pde.Max = 1;
	pde.Default = defval;
	pde.vproc = vproc;
	append(pde);
}

void parmdef::KeywordDef(const char *name, const char *desc,
						 Atp_KeywordType *keys,
						 Atp_VprocType vproc)
{
	Atp_ParmDefEntry pde = zero_pde;
	pde.parmcode = ATP_KEYS;
	pde.parser = Atp_ProcessKeywordParm;
	pde.Name = (char *)name;
	pde.Desc = (char *)desc;
	pde.KeyTabPtr = keys;
	pde.vproc = vproc;
	append(pde);
}

void parmdef::KeywordDef(const char *name, const char *desc,
						 int default_key_value,
						 Atp_KeywordType *keys, Atp_VprocType vproc)
{
	Atp_ParmDefEntry pde = zero_pde;
	pde.parmcode = Atp_SetOptParm(ATP_KEYS);
	pde.parser = Atp_ProcessKeywordParm;
	pde. Name = (char *)name;
	pde.Desc = (char *)desc;
	pde.KeyTabPtr = keys;
	pde.Default = default_key_value;
	pde.vproc = vproc;
	append(pde);
}

void parmdef::DatabytesDef(const char *name, const char *desc,
						   int minlen, int maxlen,
						   Atp_VprocType vproc)
{
	Atp_ParmDefEntry pde = zero_pde;
	pde.parmcode = ATP_DATA;
	pde.parser = Atp_ProcessDatabytesParm;
	pde.Name = (char *)name;
	pde.Desc = (char *)desc;
	pde.Min = minlen;
	pde. Max = maxlen;
	pde.vproc = vproc;
	append(pde);
}

void parmdef::DatabytesDef(const char *name, const char *desc,
						   Atp_DataDescriptor *def_databytes_desc,
						   int minlen, int maxlen,
						   Atp_VprocType vproc)
{
	Atp_ParmDefEntry pde = zero_pde;
	pde.parmcode = Atp_SetOptParm(ATP_DATA);
	pde.parser = Atp_ProcessDatabytesParm;
	pde.Name = (char *)name;
	pde.Desc = (char *)desc;
	pde.Min = minlen;
	pde.Max = maxlen;
	pde.DataPointer = def_databytes_desc;
	pde.vproc = vproc;
	append(pde);
}

void parmdef::NullDef(const char *name, const char *desc)
{
	Atp_ParmDefEntry pde = zero_pde;
	pde.parmcode = ATP_NULL;
	pde. parser = Atp_ProcessNullParm;
	pde.Name = (char *)name;
	pde.Desc = (char *)desc;
	append(pde);
}
