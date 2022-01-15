/* EDITION AA02 (RELOOI), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+*******************************************************************

	Module Name:		g3obbox.cxx

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Banner Box

	History:
		Who			When				Description
	----------	-------------	------------------------------
	Alwyn Teh	25 May 1995		Initial Creation
	Alwyn Teh	3 July 1995		Add str() method

*******************************************************************-*/

#include <stdio.h>
#include <string.h>
#include <strstream.h>

#include <g3obboxh.h>
#include <g3odaryh.h>

G3O_BannerBox::G3O_BannerBox() : corner('+'), horizontal_edge('-'), vertical_edge('|')
{
	max_len = 0;
	margin = 4;
	strings = 0;
}

G3O_BannerBox::~G3O_BannerBox()
{
	free_strs(strings);
}

void G3O_BannerBox::append(const char *string)
{
	eval_max(string);
	DArray<const char *>::append(string);
}

void G3O_BannerBox::display(void)
{
	const char **banner = 0;
	register int x = 0;

	banner = strs();

	for (x = 0; banner[x] != (const char *)0; x++)
	{
	   printf("\n%s", banner[x]);
	}
	printf("\n");
	fflush(stdout);

	free_strs(strings);
	strings = 0;
}

DArray<const char *> * G3O_BannerBox::get_banner(void)
{
	DArray<const char *> *banner = new DArray<const char *>;
	register int x;

	banner->append(borderline(horizontal_edge, corner));

	int nelem = size();
	for (x = 0; x < nelem; x++)
	{
	   const char *line = borderline (' ', vertical_edge);
	   centralize(line, (*this)[x]);
	   banner->append(line);
	}

	banner->append(borderline(horizontal_edge, corner));

	return banner;
}

const char ** G3O_BannerBox::strs(void)
{
	DArray<const char *> *banner = get_banner();

	free_strs(strings);
	strings = 0;
	strings = banner->dup_arrayPtr();

	delete banner;

	return strings;
}

const char * G3O_BannerBox::str(void)
{
	ostrstream output;
	const char **banner = 0;
	register int x = 0;

	banner = strs();

	for (x = 0; banner[x] != (const char *)0; x++)
	{
	   output << banner[x] << endl;
	}
	output << ends << flush;

	return output.str(); // caller must free return string
}

void G3O_BannerBox::free_strs(const char **strs)
{
	if (strs != 0)
	{
	  for (register int x = 0; strs[x] != 0; x++)
		 delete (void *) strs[x];
	  delete[] strs;
	  strs = 0;
	}
}

void G3O_BannerBox::eval_max(const char *string)
{
	int len = (string == 0) ? 0 : strlen(string);

	if (len > max_len)
	  max_len = len;
}

const char * G3O_BannerBox::borderline(const char inside, const char outside)
{
	char *borderline = 0;
	int length = max_len + (margin * 2) + 2;

	borderline = new char[length+1];
	borderline[length] = '\0';

	memset(borderline, inside, length);
	borderline[0] = borderline[length-1] = outside;

	return borderline;
}

void G3O_BannerBox::centralize(const char * target, const char * insert)
{
	int target_len = strlen (target);
	int insert_len = strlen (insert);
	int offset = 0;

	offset = (target_len - insert_len) / 2;
	memcpy((void *)(target+offset), insert, insert_len);
}
