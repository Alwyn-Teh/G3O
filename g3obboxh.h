/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+*******************************************************************

	Module Name:		g3obboxh.h

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Banner Box

	History:
		Who			When			Description
	----------	--------------	---------------------
	Alwyn Teh	25 May 1995		Initial Creation
	Alwyn Teh	3 July 1995		Add str() method

*******************************************************************-*/

#ifndef _G3O_BANNER_BOX_H
#define _G3O_BANNER_BOX_H

#include <g3odaryh.h>

class G3O_BannerBox : public DArray<const char *>
{
public:
		G3O_BannerBox();
		~G3O_BannerBox();

		void append (const char *string);
		void display(void);

		const char * str(void);
		const char **strs(void);

protected:
		void			eval_max(const char *string);
		const char *	borderline(const char inside, const char outside);
		void			centralize(const char * target, const char * insert);
		void			free_strs(const char **);

		DArray<const char *> * get_banner(void);

private:
		int				max_len;
		int				margin;
		const char		corner;
		const char		horizontal_edge;
		const char		vertical_edge;
		const char **	strings;
};
#endif /* _G3O_BANNER_BOX_H */
