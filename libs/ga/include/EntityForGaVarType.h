/////////////////////////////////////////////////////////////////////////////
// Name:        EntityForGa.h
// Project:     sgpLib
// Purpose:     Single entity storage class for GA algorithms.
// Author:      Piotr Likus
// Modified by:
// Created:     13/07/2013
/////////////////////////////////////////////////////////////////////////////

#ifndef _SGPENTFORGAVARTP_H__
#define _SGPENTFORGAVARTP_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/** \file EntityForGa.h
\brief Single entity storage class for GA algorithms.


*/

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sgp\EntityBase.h"

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

/// GA-specific genome - single vector of data
class sgpEntityForGaVarType: public sgpEntityBase {
public:  
  sgpEntityForGaVarType() {m_fitness.resize(1);}
  sgpEntityForGaVarType(const sgpEntityForGaVarType &src): m_genome(src.m_genome), sgpEntityBase(src) {}  
  virtual ~sgpEntityForGaVarType() {}
  virtual sgpEntityForGaVarType &operator=(const sgpEntityForGaVarType &src) {if (&src != this) {m_genome = src.m_genome; m_fitness = src.m_fitness;} return *this;}
  //--> genome access
  virtual void getGenome(int genomeNo, sgpGaGenome &output) const
  { 
     assert(genomeNo >= 0);
     assert(static_cast<uint>(genomeNo) < getGenomeCount());
     output = m_genome;
  } 
  virtual void setGenome(int genomeNo, const sgpGaGenome &genome) {
     assert(genomeNo >= 0);
     assert(static_cast<uint>(genomeNo) < getGenomeCount());
     m_genome = genome;     
  }
  virtual uint getGenomeCount() const { return 1; }

  virtual void getGenome(sgpGaGenome &output) const {output = m_genome;}
  virtual const sgpGaGenome &getGenome() const {return m_genome;}
  virtual void setGenome(const sgpGaGenome &genome) {m_genome = genome;}

  virtual void getGenomeItem(int genomeNo, uint itemIndex, scDataNode &output) const {
    output = m_genome[itemIndex]; 
  }

  virtual void setGenomeItem(int genomeNo, uint itemIndex, const scDataNode &value) {
    assert(genomeNo == 0);
    m_genome.at(itemIndex).copyFrom(value);
  }
  
  virtual void getGenomeAsNode(scDataNode &output, int offset = 0, int count = -1) const;
  virtual void setGenomeAsNode(const scDataNode &genome);
protected:
  sgpGaGenome m_genome;
};


#endif // _SGPENTFORGAVARTP_H__
