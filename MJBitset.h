// MJBitset.h: interface for the CMJBitset class.
//
// (c) 2004 MarsJupiter LTD
//
//
// You may use and distribute this class freely, 
// but may not remove this header or claim ownership.
//
// If you do use this class we would appreciate an email to
// martin@marsjupiter.com
//
// If you are a decent sized company and use this class then
// some payment would also be appreciated.
//
//
// Version history
//
// 1.00 June 14th 2004 
//
//
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MJBITSET_H__44E16525_5D70_4530_B0C1_5E90AAA4F4CA__INCLUDED_)
#define AFX_MJBITSET_H__44E16525_5D70_4530_B0C1_5E90AAA4F4CA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <use_ansi.h>
#include <istream>

#ifdef  _MSC_VER
/*
 * Currently, all MS C compilers for Win32 platforms default to 8 byte
 * alignment.
 */
#pragma pack(push,8)

#endif  /* _MSC_VER */
//
// When DEBUG_CMMJBITSET is defined, then a shadow bitset is kept and validated against operations.
// This is obviously very slow and uses loads of memory and defeats any purpose in using this class
// and hence should never be used outside of a debug session.
//
//
//#define DEBUG_CMJBITSET

//
// data representation type for bitsets <= 256 CMJBITSET_USE_CHAR should be defined
//

//#define CMJBITSET_USE_CHAR
#define CMJBITSET_USE_SHORT
//#define CMJBITSET_USE_LONG

#include <bitset>
#include <vector>

using namespace std;

// Tweakable parameters
//
// Reserving space usually is effective, as in a sparse bitset it does save the overhead of
// malloc/free. How much space to reserve for best performance will very according to how 
// sparse the average bitset in your program is.
//
// remembering the allocated length, adds memory to each instance and would probably only help
// if the realloc being used was really brain dead.
//
//
// in a similar fashion the ROUNDBITS/ROUNDUP/SMALLEST_ALLOC stuff will have little postive effect
// unless malloc/realloc are pretty bad.
//
#define CMJBITSET_ROUNDBITS 0
#define CMJBITSET_ROUNDUP(len) ((len) | CMJBITSET_ROUNDBITS)
// never allocate less than this amount.
#define CMJBITSET_SMALLEST_ALLOC ((_N/64))

//
// use some space within the class to avoid the need for malloc/free in many cases
//
#define  CMJBITSET_RESERVE_SPACE
#define CMJBITSET_DEFAULT_SIZE 4      // applies only if CMJBITSET_RESERVE_SPACE is set


// check representation type for BS_NORMAL when (check & CMJBITSET_CHECK_NORMAL_MASK)==CMJBITSET_CHECK_NORMAL_MASK
#define CMJBITSET_CHECK_NORMAL_MASK 31

// record allocation length
#undef CMJBITSET_RECORD_ALLOCATION_LENGTH



template<size_t _N> class CMJBitset  
{
public:
	enum Format {BS_NORMAL, BS_SPARSE, BS_FULL};
#ifdef CMJBITSET_USE_CHAR
	//typedef unsigned char _TYPE;
	typedef short           _INDEX; // needed if we have _N = 256
	typedef unsigned char * _DATAPTR;
	typedef unsigned char   _DATA;

#endif
#ifdef CMJBITSET_USE_SHORT
	//typedef unsigned long _TYPE;
	typedef unsigned short    _INDEX;  // must be able to index _N hence _N cannot be 0x10000
	typedef unsigned short * _DATAPTR;
	typedef unsigned short  _DATA;

#endif

#ifdef CMJBITSET_USE_LONG
	//typedef unsigned long _TYPE;
	typedef unsigned long     _INDEX;  // must be able to index _N hence _N cannot be 0x10000
	typedef unsigned long  * _DATAPTR;
	typedef unsigned long  _DATA;

#endif


	unsigned char m_type;
	 _INDEX  m_len;
	_DATAPTR m_pData;
#ifdef CMJBITSET_RESERVE_SPACE
	_DATA m_Default[CMJBITSET_DEFAULT_SIZE];
#endif

#ifdef CMJBITSET_RECORD_ALLOCATION_LENGTH
	_INDEX m_alloc;
#endif

#ifdef DEBUG_CMJBITSET
	bitset <_N> m_bitset;
#endif

public:
	

	CMJBitset<_N>(){
#ifdef CMJBITSET_RESERVE_SPACE
		m_pData = (_DATAPTR)&m_Default[0];
#else
		m_pData = NULL;
#endif
		m_len = 0;
		m_type = BS_SPARSE;
#ifdef DEBUG_CMJBITSET
		_ASSERT((*this)== m_bitset);
	
     
#endif

	}

	CMJBitset<_N>( const CMJBitset<_N> &_L)
	{
#ifdef CMJBITSET_RESERVE_SPACE

		m_pData = (_DATAPTR )&m_Default[0];
#else
		m_pData = NULL;
#endif
	
		switch (_L.m_type)
		{
		case BS_SPARSE:
			// all bits set is a special case
			m_type = BS_SPARSE;
			if (_L.m_len == _N)
			{
				m_len = (_INDEX) _N;
			}
			else
			{
				m_len = 0;;
				allocate( _L.m_len);

				memcpy(m_pData, _L.m_pData, m_len * sizeof(_DATA));
			}
			break;
		case BS_FULL:
			m_type = BS_FULL;
			// no bits set is a special case
			if (_L.m_len == _N)
			{
				m_len = (_INDEX) _N;
			}
			else
			{
				m_len = 0;;
				allocate( _L.m_len);

				memcpy(m_pData, _L.m_pData, m_len * sizeof(_DATA));
			}
			break;
		case BS_NORMAL:
			m_type = BS_NORMAL;
			m_len = _L.m_len;
			allocate_normal(m_len);
			memcpy(m_pData, _L.m_pData, m_len);
			break;
		}


#ifdef DEBUG_CMJBITSET
		m_bitset = _L.m_bitset;
		
		_ASSERT((*this)== m_bitset);
		
     
#endif		

	}

	~CMJBitset<_N>(){
#ifdef DEBUG_CMJBITSET
		//_ASSERT((*this)== m_bitset);
	
		//check_heap();   
#endif
#ifdef CMJBITSET_RESERVE_SPACE
		if (m_pData != &m_Default[0]) free(m_pData);
#else
		if (m_pData) free(m_pData);
#endif
	}

	size_t count() const {
	
		switch (m_type)
		{
		case BS_SPARSE:
			return m_len;
		case BS_NORMAL:
			{
				std::bitset<_N> *pData = (std::bitset<_N> *)m_pData;
				return pData->count();
			}
		case BS_FULL:
			return _N - m_len;
		}

		_ASSERT(false);
		return false;
		
	}

	bool test(_INDEX bit) const{

		switch(m_type)
		{
		case BS_SPARSE:
			{
			if (!m_len) return false;
			if (m_len == _N) return true;

			_INDEX i = 0;
			_INDEX pos = 0;
			while (i < m_len)
			{
				_ASSERT(m_pData[i] < _N);
				pos += m_pData[i++];

				if (pos == bit) return true;

				if (pos > bit) return false;
				pos++;

			}
			return false;
			}
		case BS_NORMAL:
			return ((std::bitset<_N> *)m_pData)->test(bit);
		case BS_FULL:
			{
			if (!m_len) return true;
			if (m_len == _N) return false;

			_INDEX i = 0;
			_INDEX pos = 0;
			while (i < m_len)
			{
				_ASSERT(m_pData[i] < _N);
				pos += m_pData[i++];

				if (pos == bit) return false;

				if (pos > bit) return true;
				pos++;

			}
			return true;
			}
		}

		_ASSERT(false);
		return false;
	}

#ifdef DEBUG_CMJBITSET
	void set_debug_bitset()
	{

		switch(m_type)
		{
		case BS_SPARSE:
			{
			this->m_bitset.reset();
			
			_INDEX bit=0;

			for (_INDEX i = 0; i < m_len;i++)
			{
				bit += m_pData[i];
				m_bitset.set(bit);
				bit++;
			}
			break;
			}
		case BS_FULL:
			{
			this->m_bitset.set();
			
			_INDEX bit=0;

			for (_INDEX i = 0; i < m_len;i++)
			{
				bit += m_pData[i];
				m_bitset.set(bit,0);
				bit++;
			}
			break;
			}
		
		case BS_NORMAL:
			{
				std::bitset<_N> *pL = (std::bitset<_N> *)m_pData;
				m_bitset = *pL;
			}
			break;
		}

		_ASSERT((*this) == m_bitset);

	}
#endif
	
	CMJBitset<_N>& operator = (const CMJBitset<_N> &_R){
#ifdef DEBUG_CMJBITSET

	//_ASSERT(_R == _R.m_bitset );



	  //   _ASSERT((*this)== m_bitset);
	
	
	
#endif
		change_type(_R.m_type);

	
		switch (_R.m_type)
		{
		case BS_NORMAL:
			allocate_normal(_R.m_len);

			memcpy(m_pData,_R.m_pData,m_len);

			break;
		default:

			if (_R.m_len == _N)
			{
				m_len = _N;

			}
			else
			{
				allocate(_R.m_len);

				memcpy(m_pData,_R.m_pData,m_len * sizeof(_DATA));
			}
		}
#ifdef DEBUG_CMJBITSET
		
		m_bitset = _R.m_bitset;
		
		//_ASSERT((*this)== m_bitset);
	
     
#endif	
		return (*this);

	}



	CMJBitset<_N>& operator&=(const CMJBitset<_N>& _R){
#ifdef DEBUG_CMJBITSET
		//check_heap();
	
		_ASSERT((*this) == m_bitset);

#endif
		reset_normal_type();

		switch (m_type)
		{
		case BS_SPARSE:
			switch (_R.m_type)
			{
			case BS_SPARSE:
				{
				_INDEX bit =0;

				if (m_len == _R.m_len && m_len == _N)
				{
			
				}
				else 
				{	
					CMJBitset<_N> temp;

					if (m_len < _R.m_len)
					{
						for (_INDEX i = 0; i < m_len;i++)
						{
							bit += m_pData[i];
							if (_R.test(bit)){	temp.set(bit);	}
							bit++;
						}
					}
					else
					{
						for (_INDEX i = 0; i < _R.m_len;i++)
						{
							bit += _R.m_pData[i];
							if (test(bit))	{	temp.set(bit);	}
							bit++;
						}

					}

					*this = temp;
				}
				break;
				}
			default:
				{
				CMJBitset<_N> temp;
				_INDEX bit=0;

				for (_INDEX i = 0; i < m_len;i++)
				{
					bit += m_pData[i];
					if (_R.test(bit)){	temp.set(bit);	}
					bit++;
				}
				*this = temp;
				break;
				}
			}
			break;
		case BS_FULL:
			switch(_R.m_type)
			{
			case BS_FULL:
				{
				_INDEX bit = 0;
				for (_INDEX i = 0; i < _R.m_len;i++)
				{
					bit += _R.m_pData[i];
					set(bit,0);
					bit++;
				}
				}
				break;
			case BS_SPARSE:
				{
				CMJBitset<_N> temp;
				_INDEX bit = 0;
				for (_INDEX i = 0; i < _R.m_len;i++)
				{
					bit += _R.m_pData[i];
					if (test(bit))	{	temp.set(bit);	}
					bit++;
				}
				*this = temp;
				break;
				}

			case BS_NORMAL:
				{
				CMJBitset<_N> temp;

				for (_INDEX bit = 0; bit < _N; bit++)
				{
					if (_R.test(bit)){	temp.set(bit);	}
				}
				*this = temp;
			
				break;
				}
			}
			break;
		case BS_NORMAL:
			switch (_R.m_type)
			{
			case BS_FULL:
				{
				_INDEX bit = 0;
				for (_INDEX i = 0; i < _R.m_len;i++)
				{
					bit += _R.m_pData[i];
					set(bit,0);
					bit++;
				}
				break;
				}
			case BS_NORMAL:
				{

					std::bitset<_N> *pL = (std::bitset<_N> *)m_pData;
					std::bitset<_N> *pR = (std::bitset<_N> *)_R.m_pData;

					*pL &= *pR;


				break;
				}
			case BS_SPARSE:
				{
				CMJBitset<_N> temp;
				_INDEX bit = 0;
				for (_INDEX i = 0; i < _R.m_len;i++)
				{
					bit += _R.m_pData[i];
					if (test(bit))	{	temp.set(bit);	}
					bit++;
				}
				*this = temp;
				break;
				}
			}
			break;
		}
	
#ifdef DEBUG_CMJBITSET
		//check_heap();
		this->m_bitset &= _R.m_bitset;
		_ASSERT((*this) == m_bitset);
#endif

		return (*this);
    }

	friend CMJBitset< _N> operator&(const CMJBitset<_N>& _L,const CMJBitset<_N>& _R)
	{
			return (CMJBitset<_N>(_L) &= _R); 
	}

	friend CMJBitset<_N> operator|(const CMJBitset<_N>& _L,
		const CMJBitset<_N>& _R)
		{return (CMJBitset<_N>(_L) |= _R); }


	CMJBitset<_N> operator<<(size_t _R) const {
		return (CMJBitset<_N>(*this) <<= _R); 
	}
	
	CMJBitset<_N> operator>>(size_t _R) const {
		return (CMJBitset<_N>(*this) >>= _R); 
	}

	CMJBitset<_N>& reset(	CMJBitset<_N> &_R){
	

		switch (m_type)
		{
		case BS_SPARSE:
			switch(_R.m_type)
			{
			case BS_SPARSE:
			
				if (_R.m_len == _N)
				{
					m_len = 0;
				}
				else
				{
					_INDEX bit = 0;

					for (_INDEX i = 0; i < _R.m_len;i++)
					{
						bit += _R.m_pData[i];;					
						set(bit,0);
						bit++;
					}
				}
				break;
			case BS_FULL:
				if (_R.m_len == 0)
				{
					m_len = 0;
				}
				else
				{
				
					_INDEX bit=0;
					_INDEX unset_bit = 0;
					for (_INDEX i = 0; i < _R.m_len; i++)
					{

						 unset_bit += _R.m_pData[i];
						for (;bit < unset_bit; bit++)
						{

							set(bit,0);
						}
						bit = ++unset_bit;
					}
					for (;bit < _N;bit++)
					{
						set(bit,0);
					}
				}
				break;
			case BS_NORMAL:
				for (_INDEX bit = 0; bit < _N;bit++)
				{
					if (_R.test(bit))
					{
						set(bit,0);
					}
				}
				break;
			}
			break;
		case BS_FULL:
			switch (_R.m_type)
			{
			case BS_SPARSE:

				if (_R.m_len == _N)
				{
					m_type = BS_SPARSE;
					m_len = 0;
				}
				else
				{
					_INDEX bit = 0;
					for (_INDEX i = 0; i < _R.m_len;i++)
					{
						bit += _R.m_pData[i];;
						set(bit,0);
						
						bit++;
					}
		
				}
				break;
			case BS_NORMAL:
				{
				for (_INDEX bit = 0; bit < _N;bit++)
				{
					if (_R.test(bit))
					{
						set(bit,0);
					}
				}
				break;
				}
			case BS_FULL:
				if (_R.m_len =0)
				{
					this->m_len = 0;
				}
				else
				{
				_INDEX bit=0;
				_INDEX unset_bit = 0;
				for (_INDEX i = 0; i < _R.m_len; i++)
				{

					 unset_bit += _R.m_pData[i];
					for (;bit < unset_bit; bit++)
					{

						set(bit,0);
					}
					bit = ++unset_bit;
				}
				for (;bit < _N; bit++)
				{
					set(bit,0);
				}
				break;

				}
			}
			break;
		case BS_NORMAL:

			switch (_R.m_type)
			{

			case BS_NORMAL:
				{
				std::bitset<_N> *pL = (std::bitset<_N> *)m_pData;
				std::bitset<_N> *pR = (std::bitset<_N> *)_R.m_pData;

				*pL &= ~(*pR);
				break;
				}
			case BS_SPARSE:
				{
				_INDEX bit = 0;
				for (_INDEX i = 0; i < _R.m_len;i++)
				{
					bit += _R.m_pData[i];;
					set(bit,0);
				
					bit++;
				}
				break;
				}
			case BS_FULL:
				if (_R.m_len == 0)
				{
					m_type = BS_SPARSE;
					m_len = 0;
			
				}
				else
				{
					_INDEX bit=0;
					_INDEX unset_bit = 0;
					for (_INDEX i = 0; i < _R.m_len; i++)
					{

						 unset_bit += _R.m_pData[i];
						for (;bit < unset_bit; bit++)
						{

							set(bit,0);
						}
						bit = ++unset_bit;
					}
					for (;bit < _N;bit++)
					{
						set(bit,0);
					}
				}
				break;

				

			}
			break;
		
		}
#ifdef DEBUG_CMJBITSET
		m_bitset  &= ~_R.m_bitset;
		_ASSERT((*this) == m_bitset);
#endif
		return (*this); 
	}


	CMJBitset<_N>& reset(){
		m_type = BS_SPARSE;
		m_len = 0;
#ifdef DEBUG_CMJBITSET
		m_bitset.reset();
		_ASSERT((*this) == m_bitset);
#endif
		return (*this); 
	}





	CMJBitset(unsigned long _X){
		m_type = BS_SPARSE;
		m_len = 0;
		int pos=0;
		while(_X)
		{
			if (_X & 1) set(pos);
			_X >>=1;
			pos++;
		}
#ifdef DEBUG_CMJBITSET
		m_bitset.bitset(_X);

		_ASSERT((*this)== m_bitset);
	
     
#endif
	}



	bool operator!=(const bitset<_N>& _R) const {
		return (!(*this == _R)); 
	}

	CMJBitset<_N>& operator|=(const CMJBitset<_N>& _R) {
#ifdef DEBUG_CMJBITSET
		_ASSERT(_R == _R.m_bitset );
		_ASSERT((*this) == m_bitset);
#endif
	
		reset_normal_type();

		switch (m_type)
		{
		case BS_SPARSE:
			switch (_R.m_type)
			{
			case BS_SPARSE:
				if (_R.m_len == _N || m_len == _N)
				{
					// nothing to do
				}
				else
				{
					_INDEX bit =0;
					for (int i = 0; i < _R.m_len;i++)
					{
						bit += _R.m_pData[i];
						this->set(bit++);
					}
				}
				break;
			case BS_NORMAL:
				if (m_len == _N)
				{
					// nothing to do
				}
				else
				{
					for (_INDEX bit =0; bit < _N; bit++)
					{
						if (_R.test(bit))
						{
							this->set(bit);
						}
					}

				}
				break;
			case BS_FULL:
				if (m_len == 0)
				{
					// nothing to do
				}
				else
				{
					_INDEX bit=0;
					_INDEX unset_bit = 0;
					for (_INDEX i = 0; i < _R.m_len; i++)
					{

						 unset_bit += _R.m_pData[i];
						for (;bit < unset_bit; bit++)
						{

							set(bit);
						}
						bit = ++unset_bit;
					}
				}
		
			}
			break;
		case BS_NORMAL:
		switch (_R.m_type)
			{
			case BS_SPARSE:
				if (_R.m_len == _N )
				{
					// nothing to do
				}
				else
				{
					_INDEX bit =0;
					for (int i = 0; i < _R.m_len;i++)
					{
						bit += _R.m_pData[i];
						this->set(bit++);
					}
				}
				break;
			case BS_NORMAL:
				{
					std::bitset<_N> *pL = (std::bitset<_N> *)m_pData;
					std::bitset<_N> *pR = (std::bitset<_N> *)_R.m_pData;

				*pL |= *pR;
				break;
				}
			case BS_FULL:
			
				_INDEX bit=0;
				_INDEX unset_bit = 0;
				for (_INDEX i = 0; i < _R.m_len; i++)
				{

					 unset_bit += _R.m_pData[i];
					for (;bit < unset_bit; bit++)
					{

						set(bit);
					}
					bit = ++unset_bit;
				}
				
		
			}
			break;
		case BS_FULL:
			switch (_R.m_type)
			{
			case BS_SPARSE:
				if (_R.m_len == _N || m_len == 0)
				{
					// nothing to do
				}
				else
				{
					_INDEX bit =0;
					for (int i = 0; i < _R.m_len;i++)
					{
						bit += _R.m_pData[i];
						this->set(bit++);
					}
				}
				break;
			case BS_NORMAL:
				if (m_len == 0)
				{
					// nothing to do
				}
				else
				{
					for (_INDEX bit =0; bit < _N; bit++)
					{
						if (_R.test(bit))
						{
							this->set(bit);
						}
					}

				}
				break;
			case BS_FULL:
				if (m_len == 0)
				{
					// nothing to do
				}
				else
				{
					_INDEX bit=0;
					_INDEX unset_bit = 0;
					for (_INDEX i = 0; i < _R.m_len; i++)
					{

						 unset_bit += _R.m_pData[i];
						for (;bit < unset_bit; bit++)
						{

							set(bit);
						}
						bit = ++unset_bit;
					}
					// fill remains
					for (;bit <_N;bit++)
					{
						set(bit);
					}
				}
			}

		}


#ifdef DEBUG_CMJBITSET
		m_bitset |= _R.m_bitset;
		_ASSERT((*this) == m_bitset);
#endif
		return (*this);

	}

	CMJBitset<_N>& operator^=(const CMJBitset<_N>& _R) {
#ifdef DEBUG_CMJBITSET
	
		_ASSERT((*this) == m_bitset);
#endif

		if (m_type == BS_NORMAL && _R.m_type == BS_NORMAL)
		{
			*(bitset<N> *)m_pData) ~= *(bitset<N> *)_R.m_pData);
		}
		else
		{
			for (_INDEX i = 0; i < _N; i++)
			{
				this->set(i, test(i) ^ _R.test(i));
			}
		}

#ifdef DEBUG_CMJBITSET
		this->m_bitset ^= _R.bitset();
		_ASSERT((*this) == m_bitset);
#endif
		return (*this); 
	
	}


	bool operator==(const CMJBitset<_N>& _R) const {
#ifdef DEBUG_CMJBITSET
	
		_ASSERT((*this) == m_bitset);
#endif
		switch m_type)
		{
		case BS_SPARSE:
			switch (_R.m_type)
			{
			case BS_SPARSE:
				if (m_len != _R.m_len) return false;
				if (m_len == _N) return true;

				return (memcmp(m_pData,_R.m_pData, m_len  * sizeof(_DATA)) == 0);
			case BS_NORMAL:
				for (_INDEX i = 0; i < _N; i++)
				{
					if (test(i) != _R.test(i)) return false;
				}

				return true;
			case BS_FULL:
				// bad place to be the types should always
				if (_N - m_len != _R.m_len) return false;
				for (_INDEX i = 0; i < _N; i++)
				{
					if (test(i) != _R.test(i)) return false;
				}
				return true;
			}
			_ASSERT(false);
			break;
		case BS_NORMAL:
			switch (_R.m_type)
			{
			case BS_NORMAL:
				return *(bitset<_N> *) m_pData) == *(bitset<_N> *)_R.m_pData);
			case BS_SPARSE:
			case BS_FULL:
				for (_INDEX i = 0; i < _N; i++)
				{
					if (test(i) != _R.test(i)) return false;
				}
				return true;
			}
			_ASSERT(false);
		case BS_FULL:
			switch (_R.m_type)
			{
				case BS_FULL:
				if (m_len != _R.m_len) return false;
				if (m_len == _N) return true;

				return (memcmp(m_pData,_R.m_pData, m_len  * sizeof(_DATA)) == 0);
			case BS_NORMAL:
				for (_INDEX i = 0; i < _N; i++)
				{
					if (test(i) != _R.test(i)) return false;
				}

				return true;
			case BS_SPARSE:
				// bad place to be the types should always
				if (_N - m_len != _R.m_len) return false;
				for (_INDEX i = 0; i < _N; i++)
				{
					if (test(i) != _R.test(i)) return false;
				}
				return true;
			}
			_ASSERT(false);
			break;

		}
	}

	CMJBitset<_N> operator~() const {
#ifdef DEBUG_CMJBITSET
		_ASSERT((*this) == m_bitset);

#endif
		return (CMJBitset<_N>(*this).flip()); 
	}



	CMJBitset<_N>& flip(size_t _P) {
		this->set(_P,!test(_P));
#ifdef DEBUG_CMJBITSET
		m_bitset.flip(_P);
		_ASSERT((*this) == m_bitset);
#endif		
		return (*this); 
	}

	CMJBitset<_N>& flip(){
#ifdef DEBUG_CMJBITSET
		//check_heap();
		_ASSERT((*this) == m_bitset);
#endif

		switch (m_type)
		{

		case BS_SPARSE:
			
			m_type = BS_FULL;
			
			break;
		case BS_FULL:
			m_type = BS_SPARSE;
			break;
		case BS_NORMAL:

			((std::bitset<_N> *)m_pData)->flip();
		}
#ifdef DEBUG_CMJBITSET
		m_bitset.flip();
		_ASSERT((*this) == m_bitset);
		//check_heap();
#endif
		return (*this);
	}


	CMJBitset<_N>& set(size_t _P, bool _X = true) {
#ifdef DEBUG_CMJBITSET
		//check_heap();
		_ASSERT((*this) == m_bitset);
#endif
		if (_N <= _P)
			_Xran();

		switch(m_type)
		{
		case BS_NORMAL:
			((std::bitset<_N> *)m_pData)->set(_P,_X);
			break;
		default:
			if (_X)
			{
				if (!test(_P))
				{
					set_unset_bit(_P);
				}
			}
			else
			{

				if (test(_P))
				{

					reset_set_bit(_P);
				}
			}
		}
#ifdef DEBUG_CMJBITSET
		m_bitset.set(_P,_X);
		_ASSERT((*this) == m_bitset);
		//check_heap();
#endif
		return (*this);
	}

	CMJBitset<_N>& set(){
		m_type = BS_FULL;
		m_len = 0;
#ifdef DEBUG_CMJBITSET
		m_bitset.set();
		_ASSERT((*this) == m_bitset);
#endif
		return (*this); 
	}
	size_t size() const {
		return (_N); 
	}
	string to_string() const {
		string _S;
		_S.reserve(_N);
		for (size_t _P = _N; 0 < _P; )
			_S += test(--_P) ? '1' : '0';
		return (_S); 
	}
	
	bool none() const {
#ifdef DEBUG_CMJBITSET
		//check_heap();
		_ASSERT((*this) == m_bitset);
#endif
		return (!any());
	}

	bool any() const{
#ifdef DEBUG_CMJBITSET
	
		_ASSERT((*this) == m_bitset);
		//check_heap();
#endif
		switch (m_type)
		{
		case BS_SPARSE:

			return m_len >0;
		case BS_FULL:
			return m_len < _N;
		case BS_NORMAL:
			return ((std::bitset<_N> *)m_pData)->any();
		}

		_ASSERT(false);
		return false;
	}

	bool at(size_t _P) const {
		if (_N <= _P)
			_Xran();
		return (test(_P)); 
	}



	bitset<_N> bitset() const{

	
		switch (m_type)
		{
		case BS_NORMAL:

			return *(bitset<N> *)m_pData);

		case  BS_SPARSE:
			
			bitset<_N> bitmap;

			_INDEX bit =0;
			for (_INDEX  i = 0; i < m_len;i++)
			{
		
			
				bit += m_pData[i];
				bitmap.set(bit);
				bit++;
				

			}
		
			return bitmap;
		case BS_FULL:
			bitset<_N> bitmap;
			bitmap.set();

			_INDEX bit =0;
			for (_INDEX  i = 0; i < m_len;i++)
			{
				 
			
				bit += m_pData[i];
				bitmap.reset(bit);
				bit++;
				
			}
		
			return bitmap;
		}
	}
	

	void change_type(unsigned char type)
	{

		if (m_type == type) return;


		if (type == BS_NORMAL)
		{
#ifndef CMJBITSET_RESERVE_SPACE
			free(m_pData);

			m_pData = NULL;
#else
			if (m_pData != &m_Default[0])
			{
				free(m_pData);
				m_pData = &m_Default[0];
			}


#endif
		}
		m_type = type;
	}

	bool save(FILE *fp)
	{
		if (fwrite(&mask->m_bitset.m_type, 1,1, fp) !=1) goto save_error;

		if (fwrite(&mask->m_bitset.m_len, sizeof(unsigned short),1,fp) != 1) goto save_error;

		if (mask->m_bitset.m_len  < BITS_PER_DICT && mask->m_bitset.m_len )
		{
			switch(m_bitset.m_type)
			{
			case CMJBitset<BITS_PER_DICT>::BS_SPARSE:
			case CMJBitset<BITS_PER_DICT>::BS_FULL:
#ifdef CMJBITSET_USE_SHORT		
				if (fwrite((void *)mask->m_bitset.m_pData,sizeof(unsigned short), mask->m_bitset.m_len) != m_len) goto save_error;
#else
				if (fwrite((void *)mask->m_bitset.m_pData,sizeof(unsigned char), mask->m_bitset.m_len) != m_len) goto save_error;

#endif
				break;
			case CMJBitset<BITS_PER_DICT>::BS_NORMAL:
				if (fwrite((void *)this->m_bitset.m_pData,1, this->m_bitset.m_len)!= m_len) goto save_error;
				break;
			default:
				_ASSERT(FALSE);
			}


		}
		return true;
save_error:
		return false;
	}


	bool load(FILE *fp)
	{
		unsigned short len;
		if (fread(&this->m_bitset.m_type,1,1,fp) != 1) goto load_error;
		if (fread(&len, sizeof(unsigned short),1,fp) != 1) goto load_error;
		switch(m_bitset.m_type)
		{
		case CMJBitset<BITS_PER_DICT>::BS_SPARSE:
		case CMJBitset<BITS_PER_DICT>::BS_FULL:
			if (len  < BITS_PER_DICT && len )
			{
				this->m_bitset.allocate(len);
#ifdef CMJBITSET_USE_SHORT
				if (fread(this->m_bitset.m_pData,sizeof(unsigned short), len,fp) != len) goto load_error
#else
				if (fread(this->m_bitset.m_pData,sizeof(unsigned char), len,fp)) != len) goto load_error;
#endif

			}
			else
			{
				this->m_bitset.m_len = len;
			}
			break;
		case CMJBitset<BITS_PER_DICT>::BS_NORMAL:
			this->m_bitset.allocate_normal(len);
			if (fread(this->m_bitset.m_pData,1, len,fp)) != len) goto load_error;
			break;
		
		}
#ifdef DEBUG_CMJBITSET
		this->m_bitset.set_debug_bitset();
#endif
		return true;
load_error:
		return false;

	}

#ifdef CMJBITSET_RESERVE_SPACE
	void allocate(_INDEX len)
	{
		
	
			_ASSERT(m_type != BS_NORMAL);
			_ASSERT(len < _N);
			if (m_pData == &m_Default[0] && len < CMJBITSET_DEFAULT_SIZE)
			{

			}
			else if (m_pData == &m_Default[0] )
			{
				
				_ASSERT(m_len < CMJBITSET_DEFAULT_SIZE || m_len == _N);
#ifdef CMJBITSET_RECORD_ALLOCATION_LENGTH		
				m_alloc = (len>CMJBITSET_SMALLEST_ALLOC?len :CMJBITSET_SMALLEST_ALLOC);
				m_pData = (_DATAPTR ) malloc(sizeof(_DATA) * m_alloc);
#else
				m_pData = (_DATAPTR ) malloc(sizeof(_DATA) *(len>CMJBITSET_SMALLEST_ALLOC?len :CMJBITSET_SMALLEST_ALLOC));
#endif
				if (m_len != _N)
				{
					memcpy(m_pData,&m_Default[0], m_len * sizeof(_DATA));
	}

			}
#ifdef CMJBITSET_RECORD_ALLOCATION_LENGTH
			else if (len > m_alloc)
			
#else
			else if (len > m_len)
#endif
			{
#ifdef CMJBITSET_RECORD_ALLOCATION_LENGTH
				m_alloc = (len>CMJBITSET_SMALLEST_ALLOC? CMJBITSET_ROUNDUP(len) :CMJBITSET_SMALLEST_ALLOC);
				m_pData = (_DATAPTR ) realloc(m_pData, sizeof(_DATA) * m_alloc);

#else
				m_pData = (_DATAPTR ) realloc(m_pData, sizeof(_DATA) *(len>CMJBITSET_SMALLEST_ALLOC? CMJBITSET_ROUNDUP(len) :CMJBITSET_SMALLEST_ALLOC));
#endif
			}
			m_len = len;
	

	

	}


	void allocate_normal(_INDEX len)
	{
		
	
			_ASSERT(m_type == BS_NORMAL);
			_ASSERT(len == sizeof(std::bitset<_N>));
			if (m_pData != &m_Default[0] )
			{
				free(m_pData);
			}		
			
			m_pData = (_DATAPTR )calloc(1,len);
			
		
			m_len = len;
	

	

	}
#else
	void allocate(_INDEX len)
	{
		
	
			_ASSERT(m_type != BS_NORMAL);
			_ASSERT(len < _N);
			if (!m_pData  )
			{
		
#ifdef CMJBITSET_RECORD_ALLOCATION_LENGTH
				m_alloc = (len> CMJBITSET_SMALLEST_ALLOC?len :CMJBITSET_SMALLEST_ALLOC);
				m_pData = (_DATAPTR ) malloc(sizeof(_DATA) * m_alloc);
#else
				m_pData = (_DATAPTR ) malloc(sizeof(_DATA) *(len> CMJBITSET_SMALLEST_ALLOC?len :CMJBITSET_SMALLEST_ALLOC));
#endif			
				
			}
#ifdef CMJBITSET_RECORD_ALLOCATION_LENGTH
			else if (len > m_alloc)
#else
			else if (len > m_len)
#endif
			{
#ifdef CMJBITSET_RECORD_ALLOCATION_LENGTH

				m_alloc = (len>CMJBITSET_SMALLEST_ALLOC? CMJBITSET_ROUNDUP(len) :CMJBITSET_SMALLEST_ALLOC);
				m_pData = (_DATAPTR ) realloc(m_pData, sizeof(_DATA) * m_alloc);
#else
				m_pData = (_DATAPTR ) realloc(m_pData, sizeof(_DATA) *(len>CMJBITSET_SMALLEST_ALLOC? CMJBITSET_ROUNDUP(len) :CMJBITSET_SMALLEST_ALLOC));

#endif
			}
			m_len = len;
	

	

	}


	void allocate_normal(_INDEX len)
	{
		
	
			_ASSERT(m_type == BS_NORMAL);
			_ASSERT(len == sizeof(std::bitset<_N>));
			if (m_pData)
			{
				free(m_pData);
			}		
			
			m_pData = (_DATAPTR )calloc(1,len);
			
		
			m_len = len;
	

	

	}

#endif

	// 4 4 (0000100001) << 5  = 4   (00001)
    // 0 0 0 0 (1111) < 3 = 0  (1)

	// 
	CMJBitset<_N >& operator>>=(size_t _P){
#ifdef DEBUG_CMJBITSET
		
		_ASSERT((*this) == m_bitset);
		m_bitset>>= _P;
		//fprintf(stderr,"CMJBitset::%s\n",this->to_string().c_str());
		//check_heap();
#endif

		switch (m_type)
		{
		case BS_SPARSE:
			if (m_len == _N)
			{
				m_type = BS_FULL;
				allocate(1);
				m_pData[0] = _N - _P ;
			}
			else
			{
				while (m_len && _P>0)
				{
					_INDEX c = m_pData[0];
					if (c >= _P)
					{
						m_pData[0] -= _P;
						break;
					}
					else
					{
						m_len--;
						_P -= c+1;
						memmove(&m_pData[0], &m_pData[1], m_len * sizeof(_DATA));
					}
				}
			}
			break;
		case BS_NORMAL:
			*(std::bitset<_N> *)m_pData>>=_P;
			break;
		case BS_FULL:
			if (m_len == 0)
			{
				allocate(1);
				m_pData[0] = _N - _P;
			}
			else
			{
				while ( _P>0)
				{	
					_INDEX c = m_pData[0];
				
					if (c >= _P)
					{
						m_pData[0] -= _P;
						break;
					}
					else 
					{

						memmove(&m_pData[0], &m_pData[1], (m_len -1) * sizeof(_DATA));
						m_len --;
					
						_P -= c+1;
					
					}
				
				}
				// just one problem all top bits are now set
				_INDEX i = 0;
				_INDEX bit = 0;
				while (i < m_len);
				{
					bit += m_pData[i]; //unset bit

					bit++; // position after unset bit
					i++;
				}
				if (bit < _N-_P)
				{
					allocate(m_len+1);
					m_pData[m_len-1] = (_N- _P) - bit;
					bit = _N - _P +1;
				}
				while (bit < _N)
				{
					allocate(m_len+1);
					m_pData[m_len -1] = 0;
					bit++;

				}

			}


			break;
		}
		
#ifdef DEBUG_CMJBITSET
	
		_ASSERT((*this) == m_bitset);
#endif
		return (*this);
		
	}

	CMJBitset<_N >& operator<<=(size_t _P){
#ifdef DEBUG_CMJBITSET
		//fprintf(stderr,"CMJBitset::%s\n",this->to_string().c_str());
		_ASSERT((*this) == m_bitset);
		
#endif	
		if (!_P) return (*this);
		switch (m_type)
		{

		case BS_SPARSE:
			if (m_len == _N)
			{
				allocate(1);
				m_type = BS_FULL;
				m_pData[0] = _P-1;
			}
			else
			{
				m_pData[0] += _P;
				// remove overflow
				_INDEX bit = 0;
				for (_INDEX i = 0; i < m_len; i++)
				{
					bit += m_pData[i]  ;  // position of set bit
					if (bit >= _N)
					{
						m_len =i;
						break;
					}
					bit++;
				}
			}
			break;
		case BS_NORMAL:
			*(std::bitset<_N> *)m_pData<<=_P;
			break;
		case BS_FULL:
			if (m_len == 0){
				allocate(_P);
				memset(&m_pData[0], 0, m_len * sizeof(_DATA));
			}
			else 
			{
				//if (m_pData[0] + _P < _N)
				//{
					// remove overflow
					_INDEX bit = 0;
					for (_INDEX i = 0; i < m_len; i++)
					{
						bit += m_pData[i] ; // unset bit pos
						if (bit >= _N-_P)
						{
							m_len = i;;
							break;
						}
						bit++;
					}
					if (m_len + _P >= _N)
					{
						m_len = _N;
						
					}
					else
					{
						allocate(m_len +_P);
						memmove(&m_pData[_P], &m_pData[0], sizeof(_DATA) * (m_len - _P));
						memset(&m_pData[0], 0, _P *sizeof(_DATA));
					}
					//for (;bit < _N;bit++)
					//{
					//	reset_set_bit(bit);
					//}
				//}
				//else
				//{
				//	m_len = _N;
				//}

			}

			break;
	
		}
			
#ifdef DEBUG_CMJBITSET
		m_bitset <<= _P;
		_ASSERT((*this) == m_bitset);
	
#endif
		return (*this);
	}

	bool operator==(const bitset<_N>& _R) const {

		_INDEX count = this->count() ;
		if (count != _R.count()) {
#ifdef DEBUG_CMJBITSET
			//
			// example code of the type useful when regressing the CMJBitset class
			//
			int rc = _R.count();
			fprintf(stderr,"CMjBitset::%s\n",this->to_string().c_str());
			fprintf(stderr,"   Bitset::%s\n",this->m_bitset.to_string().c_str());
#endif
			return false;
		}

		if (count == 0 || count == _N) return true;

		switch (m_type)
		{
		case BS_SPARSE:
			{
			_INDEX bit=0;
			for (_INDEX i = 0; i < m_len; i++)
			{
				bit += m_pData[i];

				if (!_R.test(bit)){
					return false;
				}
				bit++;
			}
			}
			return true;
		case BS_NORMAL:
			{
			std::bitset<_N> *pL = (std::bitset<_N> *)m_pData;

			return *pL == _R;
			}
		case BS_FULL:
			{

			_INDEX bit=0;
			for (_INDEX i = 0; i < m_len; i++)
			{
				bit += m_pData[i];

				if (_R.test(bit)) {
					return false;
				}
				bit++;
			}

			return true;
			}
			

		}
		return false;
			
	}

	void make_sparse()
	{

		_ASSERT(m_type != BS_SPARSE);

		CMJBitset<_N> temp;
		switch (m_type)
		{
		case BS_NORMAL:
				{
				for (_INDEX i = 0; i < _N; i++)
				{
					if (test(i))
					{
						temp.set_unset_bit(i, false);
					}
				}
				break;
				}
			case BS_FULL:
				{
					_INDEX bit= 0;
					_INDEX set_bit = 0;
					_INDEX i = 0;
					while(i < m_len)
					{
						bit += m_pData[i]; // unset bit
						for (_INDEX j = set_bit; j < bit; j++)
						{
							temp.set_unset_bit(j, false);
						}
						bit++;
						set_bit = bit;
						i++;

					}
					for (_INDEX j = bit ; j < _N; j++)
					{
						for (_INDEX j = set_bit; j < bit; j++)
						{
							temp.set_unset_bit(j, false);
						}
					}
					break;
				}

		}
#ifdef DEBUG_CMJBITSET
		temp.m_bitset =this->m_bitset; 
#endif
		*this = temp;
			
#ifdef DEBUG_CMJBITSET
	
		_ASSERT((*this) == m_bitset);
	
#endif
	}


	void make_full()
	{

		_ASSERT(m_type != BS_FULL);

		CMJBitset<_N> temp;

		temp.m_type = BS_FULL;
		temp.m_len = 0;
		switch (m_type)
		{
		case BS_NORMAL:
				{
				for (_INDEX i = 0; i < _N; i++)
				{
					if (!test(i))
					{
						temp.reset_set_bit(i, false);
					}
				}
				break;
				}
			case BS_SPARSE:
				{
					_INDEX bit= 0;
					_INDEX unset_bit = 0;
					_INDEX i = 0;
					while(i < m_len)
					{
						bit += m_pData[i]; 
						for (_INDEX j = unset_bit; j < bit; j++)
						{
							temp.reset_set_bit(j, false);
						}
						bit++;
						unset_bit = bit;
						i++;

					}
					for (_INDEX j = bit ; j < _N; j++)
					{
						for (_INDEX j = unset_bit; j < bit; j++)
						{
							temp.reset_set_bit(j, false);
						}
					}
					break;
				}

		}
#ifdef DEBUG_CMJBITSET
		temp.m_bitset =this->m_bitset; 
#endif
		*this = temp;
			
#ifdef DEBUG_CMJBITSET
	
		_ASSERT((*this) == m_bitset);
	
#endif
	}



	void make_normal()
	{

		_ASSERT(m_type != BS_NORMAL);
			
#ifdef DEBUG_CMJBITSET
	
		//_ASSERT((*this) == m_bitset);
	
#endif
		CMJBitset<_N> temp;

		temp.m_type = BS_NORMAL;
		temp.allocate_normal(sizeof(std::bitset<_N>));

		switch (m_type)
		{
		case BS_FULL:
				{
				std::bitset<_N> *pL = (std::bitset<_N> *)temp.m_pData;
					_INDEX bit= 0;
					_INDEX i = 0;
					pL->set();

					while(i < m_len)
					{
						bit += m_pData[i]; // unset bit
					
						
						pL->set(bit,false);
						
						bit++;
						i++;

					}
				
					break;
				}
			case BS_SPARSE:
				{
					std::bitset<_N> *pL = (std::bitset<_N> *)temp.m_pData;
					pL->reset();
		
					_INDEX bit= 0;
					_INDEX i = 0;
					while(i < m_len)
					{
						bit += m_pData[i]; 
					
							pL->set(bit);
						bit++;
						i++;
			
					}
				
					break;
				}

		}
#ifdef DEBUG_CMJBITSET
		temp.m_bitset =this->m_bitset; 
		//_ASSERT(temp == temp.m_bitset);
#endif
		*this = temp;
			
#ifdef DEBUG_CMJBITSET
	
		//_ASSERT((*this) == m_bitset);
	
#endif
	}


	friend ostream& operator<<(ostream& _O, const CMJBitset<_N>& _R)	{
		for (size_t _P = _N; 0 < _P; )
		_O << _R.test(--_P) ? '1' : '0';
		return (_O); 
	}
		// TEMPLATE operator>>
	friend istream& operator>>(istream& _I, CMJBitset<_N>& _R)	{
		ios_base::iostate _St = ios_base::goodbit;
		bool _Chg = false;
		string _X;
		const istream::sentry _Ok(_I);
		if (_Ok)
		{
			_TRY_IO_BEGIN
				int _C = _I.rdbuf()->sgetc();
				for (size_t _M = _R.size(); 0 < _M;
				_C = _I.rdbuf()->snextc(), --_M)
				{
					if (_C == EOF)
					{
						_St |= ios_base::eofbit;
						break;
					}
					else if (_C != '0' && _C != '1')
						break;
					else if (_X.max_size() <= _X.size())
					{
						_St |= ios_base::failbit;
						break; 
					}
					else
						_X.append(1, (char)_C), _Chg = true; 
				}
				_CATCH_IO_(_I); 
		}
		if (!_Chg)
			_St |= ios_base::failbit;
		_I.setstate(_St);
		_R = _X;
		return (_I);
	}


private:
	// 1 (01) (1=0) = 00 (11)
	// 01 (101) (01=1) =  000 (111)
	

	void set_unset_bit(_INDEX bit,bool retype=true){
	if (retype)
		{
			switch (m_type)
			{

			case BS_FULL:
				if ( m_len > _N - ((_N/8)/sizeof(_DATA)))
				{
					make_sparse();
				}
				else if(m_len -1  > ((_N/8)/sizeof(_DATA)))
				{
					make_normal();
				}
				break;
			case BS_NORMAL:
				{
					static int check =0;
					check++;
					if (check & CMJBITSET_CHECK_NORMAL_MASK == CMJBITSET_CHECK_NORMAL_MASK)
					{
						std::bitset<_N> *pL = (std::bitset<_N> *)m_pData;
			
						_INDEX count = pL->count();
						if (count - 1 < ((_N/8)/sizeof(_DATA)))
						{
							make_sparse();
						}
						else if (count > _N - ((_N/8)/sizeof(_DATA)))
						{

							make_full();
						}
			

					}


				}
				break;
			case BS_SPARSE:
				{
				if (m_len != _N-1 && m_len > _N - ((_N/8)/sizeof(_DATA)))
				{
					make_full();
				}
				else if(m_len != _N-1 && m_len +1  > ((_N/8)/sizeof(_DATA)))
				{
					make_normal();
				}
				break;	


				}

			}

		}

		switch (m_type)
		{

		case BS_SPARSE:
			{
			_INDEX pos = 0;
			_INDEX prevpos = 0;
			_INDEX i = 0;	

			if (m_len == _N -1)
			{
				m_len = _N;
			}
			else
			{
				while (i < m_len)
				{
					if (bit <= pos + m_pData[i] )
					{
						allocate(m_len +1);
						memmove(&m_pData[i+1], &m_pData[i], (m_len -1 - i) * sizeof(_DATA));
						m_pData[i] = bit - pos ;
						m_pData[i+1] = m_pData[i+1] + pos - bit -1;
						_ASSERT(test(bit));


						return;

					}
					pos += m_pData[i++] +1;
		
				}
	
				allocate(m_len +1);
				m_pData[m_len-1] = bit - pos;
			}
			break;
			}
		case BS_NORMAL:
			((std::bitset<_N> *)m_pData)->set(bit);
			break;

		case BS_FULL:
			{
				if (m_len == 0)
				{
					m_type = BS_SPARSE;
					allocate(1);
					m_pData[0]= bit;
				}
				else
				{
					_INDEX pos = 0;
					_INDEX i = 0;
					do{

						pos += m_pData[i];
			
						if (pos == bit)
						{
							_INDEX offset = m_pData[i];
							m_len--;
							memmove(&m_pData[i],&m_pData[i+1],(m_len -i) * sizeof(_DATA) );
							m_pData[i] += offset+1;
							return;
						}
						_ASSERT(pos < bit);
						pos++;
						i++;
						_ASSERT(i < _N);
					
					}while(1);
				}
			


			break;
			}
		}


		_ASSERT(test(bit));
		
	}


	void reset_normal_type()
	{

		static int check=0;
		if (m_type == BS_NORMAL)
		{
			check++;
			if ((check & CMJBITSET_CHECK_NORMAL_MASK) == CMJBITSET_CHECK_NORMAL_MASK)
			{
					std::bitset<_N> *pL = (std::bitset<_N> *)m_pData;
			
					_INDEX count = pL->count();
					if (count < ((_N/8)/sizeof(_DATA)))
					{
						make_sparse();
					}
					else if (count > _N - ((_N/8)/sizeof(_DATA)))
					{
						make_full();
					}
			}
		}
	}

	// 01 (101) 0 = 2 (001)

	void reset_set_bit(_INDEX bit, bool retype = true){

		if (retype)
		{
			switch (m_type)
			{

			case BS_FULL:
				if (m_len != _N - 1 && m_len > _N - ((_N/8)/sizeof(_DATA)))
				{
					make_sparse();
				}
				else if(m_len != _N - 1 && m_len +1  > ((_N/8)/sizeof(_DATA)))
				{
					make_normal();
				}
				break;
			case BS_NORMAL:
				{
					static int check =0;
					check++;
					if ((check & CMJBITSET_CHECK_NORMAL_MASK) == CMJBITSET_CHECK_NORMAL_MASK)
					{
						std::bitset<_N> *pL = (std::bitset<_N> *)m_pData;
			
						_INDEX count = pL->count();
						if (count - 1 < ((_N/8)/sizeof(_DATA)))
						{
							make_sparse();
						}
						else if (count > _N - ((_N/8)/sizeof(_DATA)))
						{

							make_full();
						}
			

					}


				}
				break;
			case BS_SPARSE:
				{
				if ( m_len > _N - ((_N/8)/sizeof(_DATA)))
				{
					make_full();
				}
				else if(m_len -1  > ((_N/8)/sizeof(_DATA)))
				{
					make_normal();
				}
				break;	


				}

			}

		}

		switch (m_type)
		{
		case BS_SPARSE:
			if (m_len == _N)	
			{
				allocate(_N -1);
				memset((void *)(m_pData ), 0, (_N -1) * sizeof(_DATA));
				m_pData[bit] = 1;
				//_ASSERT(!test(bit));
			}
			else
			{
				_INDEX pos = 0;
				_INDEX i = 0;
				do{
					pos += m_pData[i];
			
					if (pos == bit)
					{
						_INDEX offset = m_pData[i];
						m_len--;
						memmove(&m_pData[i],&m_pData[i+1],(m_len -i) * sizeof(_DATA) );
						m_pData[i] += offset+1;
						return;
					}
					_ASSERT(pos < bit);
					pos++;
					i++;
					_ASSERT(i < _N);

				}while(1);
			}
			break;
		case BS_NORMAL:
			{
				((std::bitset<_N> *)m_pData)->set(bit,0);
				break;

			}
		case BS_FULL:

			{
			_INDEX pos = 0;
			_INDEX prevpos = 0;
			_INDEX i = 0;	

			if (m_len == _N -1)
			{
				m_len = _N;
			}
			else
			{
				while (i < m_len)
				{
					if (bit <= pos + m_pData[i] )
					{
						allocate(m_len +1);
						memmove(&m_pData[i+1], &m_pData[i], (m_len -1 - i) * sizeof(_DATA));
						m_pData[i] = bit - pos ;
						m_pData[i+1] = m_pData[i+1] + pos - bit -1;
						_ASSERT(!test(bit));


						return;

					}
					pos += m_pData[i++] +1;
		
				}
	
				allocate(m_len +1);
				m_pData[m_len-1] = bit - pos;
			}
			break;
			}
		}
		_ASSERT(!test(bit));

	}
#ifdef DEBUG_CMJBITSET
	void check_heap() const
	{
		_ASSERTE(_CrtCheckMemory());
	
		if (m_pData != &m_Default[0])
		{
			_ASSERTE(_CrtIsValidHeapPointer(m_pData));
		}
	}
#endif

	void _Xinv() const
		{/*_THROW(invalid_argument("invalid CMJBitset<N> char"));*/ }
	void _Xoflo() const
		{/*_THROW(overflow_error(
			"CMJBitset<N> conversion overflow"));*/ }
	void _Xran() const
		{/*_THROW(out_of_range("invalid CMJBitset<N> position"));*/ }

};


#ifdef  _MSC_VER
#pragma pack(pop)
#endif  /* _MSC_VER */

#endif // !defined(AFX_MJBITSET_H__44E16525_5D70_4530_B0C1_5E90AAA4F4CA__INCLUDED_)

/*
 * 
 * Copyright (c) 2004 by MarsJupiter LTD.  ALL RIGHTS RESERVED
 * based on bitset
 * Copyright (c) 1994 by P.J. Plauger.  ALL RIGHTS RESERVED. 
 * Consult your license regarding permissions and restrictions.
 */