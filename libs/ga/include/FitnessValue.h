/////////////////////////////////////////////////////////////////////////////
// Name:        FitnessValue.h
// Project:     sgpLib
// Purpose:     Container for entity fitness value(s).
// Author:      Piotr Likus
// Modified by:
// Created:     13/07/2013
/////////////////////////////////////////////////////////////////////////////

#ifndef _SGPFITVAL_H__
#define _SGPFITVAL_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/** \file FitnessValue.h
\brief Container for entity fitness value(s).

*/

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include <vector>
#include <cassert>

#include "sc/dtypes.h"

// ----------------------------------------------------------------------------
// Simple type definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Forward class definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------

//typedef std::vector<double> sgpFitnessValue;

class sgpFitnessValue {
public:
  typedef uint size_type;

  // objective index offset
  enum { 
     SGP_OBJ_OFFSET = 0
  };

  sgpFitnessValue(): m_valueList(NULL) { }

  sgpFitnessValue(const sgpFitnessValue &src) { 
    
    if (src.m_valueList == NULL)
    {
      m_singleValue = src.m_singleValue;
      m_valueList = NULL;
    } else {
      m_valueList = new std::vector<double>();
      (*m_valueList) = (*src.m_valueList);
    }

  }

  ~sgpFitnessValue() { 
     if (m_valueList != NULL) 
     {
       delete m_valueList;
       m_valueList = NULL;
     }
  }

  sgpFitnessValue& operator=(const sgpFitnessValue& rhs)
  {
    if (&rhs != this)
    {
      if (rhs.m_valueList == NULL)
      {
        resize(1);
        m_singleValue = rhs.m_singleValue;
      } else {
        resize(rhs.size());
        (*m_valueList) = (*rhs.m_valueList);
      }
    }
    return *this;
  }

  const double &operator[](size_type idx) const {
    if (m_valueList == NULL)
    {
      assert(idx < 2);
      return m_singleValue;
    } else {
      return (*m_valueList)[idx];
    }
  }

  double &operator[](size_type idx) {
    if (m_valueList == NULL)
    {
      assert(idx < 2);
      return m_singleValue;
    } else {
      return (*m_valueList)[idx];
    }
  }

  double getValue(size_type index) const {
    if (m_valueList == NULL) {
      assert(index < 2);
      return m_singleValue;
    } else {
      return (*m_valueList)[index];
    }
  }

  void setValue(size_type index, double value) {
    if (m_valueList == NULL) {
      assert(index < 2);
      m_singleValue = value;
    } else {
      (*m_valueList)[index] = value;
    }
  }

  size_type size() const { 
    if (m_valueList == NULL)
      return 1;
    else
      return m_valueList->size();
  }

  void resize(size_type newSize)
  {
    if (newSize > SGP_OBJ_OFFSET + 1)
    {
      if (m_valueList == NULL)
        m_valueList = new std::vector<double>();
      m_valueList->resize(newSize);
    } else {
      if (m_valueList != NULL) {
        delete m_valueList;
        m_valueList = NULL;
      }
    }
  }

  void clear()
  {
     if (m_valueList != NULL) 
       m_valueList->clear();
  }

  /// Compare fitness values
  /// \return Returns:
  ///  0 - if values are equal,
  /// -2 - if val1[i] <= val2[i]
  ///  2 - if val1[i] >= val2[i]
  ///  1 - if val1[i] <> val2[i] (relation undefined, some less, some greater than)
  int compare(const sgpFitnessValue &rhs) const {
    int res;
    int minusCnt = 0;
    int plusCnt = 0;
    int zeroCnt = 0;
    int maxCnt = size();
    
    for(uint i=SGP_OBJ_OFFSET,epos = maxCnt; i!=epos; i++)
    {
      if ((*this)[i] < rhs[i])
        minusCnt++;
      else if ((*this)[i] > rhs[i])  
        plusCnt++;
      else 
        zeroCnt++;  
    }
    
    if (zeroCnt == maxCnt) {
      res = 0;
    } else if (plusCnt == 0) {
      res = -2;
    } else if (minusCnt == 0) {
      res = 2; 
    } else {
      res = 1; // undecided
    }

    return res;    
  }

private:
  double m_singleValue;
  std::vector<double> *m_valueList;
};


#endif // _SGPFITVAL_H__
