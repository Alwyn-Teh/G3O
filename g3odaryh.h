/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+*********************************************************************

	Module Name:		g3odaryh.h (was darray.h)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Dynamic Array Template Class

	History:
		Who			When			Description
	----------	-------------	-----------------------
	Alwyn Teh	94Q4 - 95Q2		Initial Creation

**********************************************************************-*/

#ifndef _DARRAY_HEADER_INCLUDED_
#define _DARRAY_HEADER_INCLUDED_

//--------------------------------ARRAY--------------------------------------------

template<class T>
class DArray
{
public:
		DArray(int s=5);
		virtual ~DArray() { delete[] v; }

		// T operator[](const int i) const { return v[i]; } // rvalue version
		T& operator[](const int i); //	{ return v[i]; } // lvalue version

		void append(const T a);
		T* arrayPtr() { return v; }
		T& first() { return *v; }
		T& last() { return (p == v) ? *p : *(p-1); }

		int size() const { return p-v; }

		T* dup_arrayPtr();	// make a copy of the array and return pointer to it
							// caller is responsible for freeing it with free()
private:
		T* v;
		T* p;
		int sz, incr_sz;
};

#endif /* _DARRAY_HEADER_INCLUDED_ */
