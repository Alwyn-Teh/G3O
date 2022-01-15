/* EDITION AA02 (REL001), ITD ACST.175 (95/05/28 21:07:52) -- OPEN */

/*+*****************************************************************

	Module Name:		g3odaryx.cxx (was darray.cxx)

	Copyright:			BNR Europe Limited, 1994-1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Dynamic Array Template Implementation
	History:
		Who			when				Description
	----------	-------------	-----------------------------
	Alwyn Teh	94Q4 - 95Q2		Initial Creation

*******************************************************************-*/

#include <iostream.h>
#include <string.h>
#include <g3odaryh.h>

template<class T> DArray<T>::DArray(int s) : sz(s)
{
	v = p = new T [s] ;
	memset(v,0,s*sizeof(T)); // always initialize to zero, null terminator required
	incr_sz = 256;
}

/* Not sure if all this index checking is necessary... */
template<class T> T& DArray<T>::operator[](const int i)
{
	static T zero = { 0 };

	if (i < 0) {
	  cout << "Error: negative array index of " << i << endl;
	  return zero;
	}

	int s = size();

	if (s == 0) {
	  cout << "Error: Array is empty." << endl;
	  return zero;
	} else if (i >= s) {
	  cout << "Error: Array index " << i << " out of range." << endl;
	  return zero;
	} else
	  return v[i];
}

template<class T> void DArray<T>::append(const T a)
{
	// realloc to bigger DArray
	if (p-v == sz-2) // ensure enough room for a null terminator
	{
	  int i;
	  T* t=new T [sz+=incr_sz];
	  memset(t,0,sz*sizeof(T)); // Important: some arrays depend on null termination
	  for (i = 0; i < size(); i++)
		 t[i] = v[i];
	  delete[] v;
	  v = t;
	  p = &v[i];
	}
	*p++ = a; // append T to array
}

template<class T> T* DArray<T>::dup_arrayPtr()
{
	int nelem = size();
	T *array = new T[nelem+1];
	for (int x = 0; x < nelem; x++)
	   array [x] = v[x] ;
	static T zero_terminator = {0};
	array[nelem] = zero_terminator; // NULL-terminate!!
	return array;
}
