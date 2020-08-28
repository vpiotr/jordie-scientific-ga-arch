/////////////////////////////////////////////////////////////////////////////
// Name:        EntityForGaUInt.h
// Project:     sgpLib
// Purpose:     Single entity storage class for GA algorithms.
// Author:      Piotr Likus
// Modified by:
// Created:     13/07/2013
/////////////////////////////////////////////////////////////////////////////

#include "sgp/EntityForGaUInt.h"

using namespace dtp;

// ----------------------------------------------------------------------------
// sgpEntityForGaUInt
// ----------------------------------------------------------------------------
void sgpEntityForGaUInt::getGenomeAsNode(scDataNode &output, int offset, int count) const
{
 // --
 // -- Note: whole genome is returned, because offset is related to genome no, not values inside genome
 // --


  output.clear();
  output.setAsArray(vt_uint);

  //uint i = 0;
  for(code_storage_type::const_iterator it = m_genome.begin(), epos = m_genome.end(); it != epos; ++it) {
    output.push_back(*it);
  }        
}
    
void sgpEntityForGaUInt::setGenomeAsNode(const scDataNode &genome) {
  m_genome.clear();
  m_genome.reserve(genome.size());
  for(int i = 0, epos = genome.size(); i != epos; ++i) {
    m_genome.push_back(genome.get<uint>(i));
  }
}

