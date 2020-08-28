/////////////////////////////////////////////////////////////////////////////
// Name:        EntityForGa.h
// Project:     sgpLib
// Purpose:     Single entity storage class for GA algorithms.
// Author:      Piotr Likus
// Modified by:
// Created:     13/07/2013
/////////////////////////////////////////////////////////////////////////////

#include "sgp/EntityForGaVarType.h"

// ----------------------------------------------------------------------------
// sgpEntityForGaVarType
// ----------------------------------------------------------------------------
void sgpEntityForGaVarType::getGenomeAsNode(scDataNode &output, int offset, int count) const
{
 // --
 // -- Note: whole genome is returned, because offset is related to genome no, not values inside genome
 // --

  std::auto_ptr<scDataNode> childGuard; 
  output.clear();
  output.setAsList();

  for(sgpGaGenome::const_iterator it = m_genome.begin(), epos = m_genome.end(); it != epos; it++) {
    childGuard.reset(new scDataNode());
    (*childGuard) = *it;
    output.addChild(childGuard.release());
  }        
}
    
void sgpEntityForGaVarType::setGenomeAsNode(const scDataNode &genome) {
  m_genome.clear();
  m_genome.reserve(genome.size());
  for(int i = 0, epos = genome.size(); i != epos; ++i) {
    m_genome.push_back(genome[i]);
  }
}

