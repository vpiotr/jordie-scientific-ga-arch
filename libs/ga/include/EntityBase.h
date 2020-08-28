/////////////////////////////////////////////////////////////////////////////
// Name:        EntityBase.h
// Project:     sgpLib
// Purpose:     Base entity class for GA/GP algorithms.
// Author:      Piotr Likus
// Modified by:
// Created:     13/07/2013
/////////////////////////////////////////////////////////////////////////////

#ifndef _SGPENTBASE_H__
#define _SGPENTBASE_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/** \file EntityBase.h
\brief Base entity class for GA/GP algorithms.

*/

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include <vector>
#include <set>

#include "sc/dtypes.h"

#include "sgp\FitnessValue.h"

// ----------------------------------------------------------------------------
// Simple type definitions
// ----------------------------------------------------------------------------
typedef std::vector<scDataNodeValue> sgpGaGenome;
typedef std::vector<uint> sgpEntityIndexList;
typedef std::set<uint> sgpEntityIndexSet;

// ----------------------------------------------------------------------------
// Forward class definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------

/// Abstract base for Gx entities, multi-chromosome, multi-objective 
/// Keeps DNA - info data, code & fitness value.
class sgpEntityBase /*: boost::noncopyable*/ {
public:  
  sgpEntityBase() {m_fitness.resize(1);}
  sgpEntityBase(const sgpEntityBase &src): m_fitness(src.m_fitness) {}  
  virtual ~sgpEntityBase() {}
  virtual sgpEntityBase &operator=(const sgpEntityBase &src) {if (&src != this) {m_fitness = src.m_fitness;} return *this;}
// properties
  // in fact n-th genome is in DNA science is called "chromosome"
  virtual void getGenome(int genomeNo, sgpGaGenome &output) const = 0; //{ getGenome(0, output); } 
  virtual void setGenome(int genomeNo, const sgpGaGenome &genome) = 0; // { setGenome(0, genome); }
  virtual uint getGenomeCount() const = 0; //{ return 1; }

  /// dump contents of entity (code, dna) as scDataNode
  virtual void getGenomeAsNode(scDataNode &output, int offset = 0, int count = -1) const = 0; //{ output = getGenomeAsNode(); }
  virtual void setGenomeAsNode(const scDataNode &genome) = 0;

  virtual void getGenomeItem(int genomeNo, uint itemIndex, scDataNode &output) const = 0;
  virtual void setGenomeItem(int genomeNo, uint itemIndex, const scDataNode &value) = 0;

  /// returns all fitness values
  void getFitness(sgpFitnessValue &output) const {
    uint aSize = output.size();
    if (aSize == m_fitness.size())
    {
      output = m_fitness;
    } else {
      output = m_fitness;
    }
  }
  /// returns total/main fitness value
  double getFitness() const {return m_fitness.getValue(0);}
  /// returns specified fitness scalar value
  double getFitness(int index) const {return m_fitness.getValue(index);}
  /// returns fitness values as vector
  const sgpFitnessValue &getFitnessVector() const {return m_fitness;}  
  /// returns number of objectives in fitness
  uint getFitnessSize() const { return m_fitness.size(); }

  /// modifies all fitness values
  void setFitness(const sgpFitnessValue &fitness) {m_fitness = fitness;}
  /// modifies global / main fitness values
  void setFitness(double value) {m_fitness.setValue(0, value);}
  /// modifies specified fitness value
  void setFitness(int index, double value) {m_fitness.setValue(index, value);}
protected:
  sgpFitnessValue m_fitness;      
};

#endif // _SGPENTBASE_H__
