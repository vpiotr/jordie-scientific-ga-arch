/////////////////////////////////////////////////////////////////////////////
// Name:        GaGeneration.h
// Project:     sgpLib
// Purpose:     Entity container for GA algorithms.
// Author:      Piotr Likus
// Modified by:
// Created:     13/07/2013
/////////////////////////////////////////////////////////////////////////////

#ifndef _SGPGAGENER_H__
#define _SGPGAGENER_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/** \file GaGeneration.h
\brief Entity container for GA algorithms.

*/

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sgp\EntityBase.h"
#include "base\StructureWriter.h"

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

class sgpGaGeneration {
public:
  sgpGaGeneration() {}
  virtual ~sgpGaGeneration() {}
  sgpEntityBase &at(int index) {return m_items[index];}   
  const sgpEntityBase &at(int index) const {return m_items[index];}   
  sgpEntityBase *atPtr(int index) {return &(m_items[index]);}   
  const sgpEntityBase *atPtr(int index) const {return &(m_items[index]);}   
  int beginPos() const { return 0; }
  int endPos() const { return m_items.size(); }

  const sgpEntityBase &operator[](int index) const { return m_items[index]; }
  sgpEntityBase &operator[](int index) { return m_items[index]; }

  void insert(sgpEntityBase *item) {m_items.push_back(item);}
  void setItem(uint index, sgpEntityBase &src);

  sgpEntityBase *extractItem(int index);

  virtual base::StructureWriterIntf *newWriterForCode(const scDataNode &extraValues) const;
  virtual base::StructureWriterIntf *newWriterForFitness(const scDataNode &extraValues) const;

  virtual sgpEntityBase *cloneItem(int index) const = 0;
  virtual sgpEntityBase *newItem() const = 0;
  virtual sgpEntityBase *newItem(const sgpEntityBase &src) const = 0;

  void copyFrom(const sgpGaGeneration &src);
  void copyFitnessFrom(const scDataNode &node, size_t sourceOffset, size_t destOffset, size_t limit, uint objectiveCount);
  uint addCodeFrom(const scDataNode &node, size_t sourceOffset = 0, size_t limit = 0);
  
  void transferItemsFrom(sgpGaGeneration &src);
  
  size_t size() const { return m_items.size(); }
  bool empty() const { return m_items.empty(); }
  void clear() { m_items.clear(); }
  virtual sgpGaGeneration *newEmpty() const = 0;

protected:
  typedef boost::ptr_vector<sgpEntityBase> sgpGaGenomeWorkList;
  sgpGaGenomeWorkList m_items;
};
  

#endif // _SGPGAGENER_H__
