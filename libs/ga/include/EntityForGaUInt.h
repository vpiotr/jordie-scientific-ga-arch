/////////////////////////////////////////////////////////////////////////////
// Name:        EntityForGaUInt.h
// Project:     sgpLib
// Purpose:     Single entity storage class for GA algorithms - uint version.
// Author:      Piotr Likus
// Modified by:
// Created:     14/07/2013
/////////////////////////////////////////////////////////////////////////////

#ifndef _SGPENTFORGAUINT_H__
#define _SGPENTFORGAUINT_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/** \file EntityForGaUInt.h
\brief Single entity storage class for GA algorithms - uint version.


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
class sgpEntityForGaUInt: public sgpEntityBase {
  typedef sgpEntityBase inherited;
  typedef std::vector<uint> code_storage_type;
public:  
  sgpEntityForGaUInt():inherited() { }

  sgpEntityForGaUInt(const sgpEntityForGaUInt &src): m_genome(src.m_genome), inherited(src) 
   {}
     
  virtual ~sgpEntityForGaUInt() {}
  
  virtual sgpEntityForGaUInt &operator=(const sgpEntityForGaUInt &src) {
    if (&src != this) {
      m_genome = src.m_genome; 
      m_fitness = src.m_fitness;
    } 
    return *this;
  }
  
  //--> genome access

  virtual void getGenome(int genomeNo, sgpGaGenome &output) const
  { 
     assert(genomeNo >= 0);
     assert(static_cast<uint>(genomeNo) < getGenomeCount());
     getGenome(output);
  }
   
  virtual void setGenome(int genomeNo, const sgpGaGenome &genome) {
     assert(genomeNo >= 0);
     assert(static_cast<uint>(genomeNo) < getGenomeCount());
     setGenome(genome);
  }
  
  virtual uint getGenomeCount() const { return 1; }

  virtual void getGenome(sgpGaGenome &output) const {
    output.resize(m_genome.size());
    for(uint i=0, epos = m_genome.size(); i != epos; ++i)
      output[i].setAsUInt(m_genome[i]);
  }

  virtual const sgpGaGenome &getGenome() const {
    throw std::runtime_error("Not implemented");
  }

  virtual void setGenome(const sgpGaGenome &genome) {
    m_genome.clear();
    m_genome.reserve(genome.size());
    for(uint i=0, epos = genome.size(); i != epos; ++i)
      //m_genome[i] = genome[i].getAsUInt();
      m_genome.push_back(genome[i].getAsUInt());
  }

  virtual void getGenomeItem(int genomeNo, uint itemIndex, scDataNode &output) const {
    output = m_genome[itemIndex]; 
  }

  virtual void setGenomeItem(int genomeNo, uint itemIndex, const scDataNode &value) {
    assert(genomeNo == 0);
    m_genome[itemIndex] = value.getAsUInt();
  }
  
  virtual void getGenomeAsNode(scDataNode &output, int offset = 0, int count = -1) const;
  virtual void setGenomeAsNode(const scDataNode &genome);

  virtual void getGenomeItem(int genomeNo, uint itemIndex, uint &output) const {
    assert(genomeNo == 0);
    output = m_genome[itemIndex]; 
  }

  virtual uint getGenomeSize(int genomeNo) const {
    assert(genomeNo == 0);
    return m_genome.size(); 
  }

protected:
  code_storage_type m_genome;
};


#endif // _SGPENTFORGAUINT_H__
