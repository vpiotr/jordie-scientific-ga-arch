/////////////////////////////////////////////////////////////////////////////
// Name:        GaGeneration.cpp
// Project:     sgpLib
// Purpose:     Entity container for GA algorithms.
// Author:      Piotr Likus
// Modified by:
// Created:     13/07/2013
/////////////////////////////////////////////////////////////////////////////

#include "sc\utils.h"

#include "sgp\GaGeneration.h"

// ----------------------------------------------------------------------------
// sgpGaGeneration
// ----------------------------------------------------------------------------
void sgpGaGeneration::setItem(uint index, sgpEntityBase &src)
{
  this->at(index) = src;
}

sgpEntityBase *sgpGaGeneration::extractItem(int index) {
  sgpGaGenomeWorkList::auto_type item = m_items.release( m_items.begin() + index );
  return item.release();
}  

void sgpGaGeneration::copyFrom(const sgpGaGeneration &src) {
  clear();
  m_items.reserve(src.size());
  if (!src.empty())
  for(int i=src.endPos() - 1,epos = src.beginPos(); i >= epos; i--) {
    m_items.push_back(newItem(src[i]));
  }
}

void sgpGaGeneration::copyFitnessFrom(const scDataNode &node, size_t sourceOffset, size_t destOffset, size_t limit, uint objectiveCount)
{
  std::auto_ptr<sgpEntityBase> genomeCodeGuard;
  uint addedCnt = 0;
  sgpFitnessValue itemFitness;
  uint currOffset = destOffset;
  itemFitness.resize(objectiveCount);
    
  scDataNode helpNode;
  const scDataNode *nodePtr;

  uint copyCnt = SC_MIN(m_items.size(), SC_MIN(sourceOffset + limit, node.size()));

  for(uint i=sourceOffset, epos = copyCnt; i != epos; i++)
  {
    nodePtr = node.getNodePtrR(i, helpNode);
    for(uint j=0, eposj = itemFitness.size(); j != eposj; j++)
    {
      if (j < nodePtr->size())        
        itemFitness.setValue(j, nodePtr->getDouble(j));
      else
        itemFitness.setValue(j, 0.0);   
    }
    m_items[currOffset].setFitness(itemFitness);
    currOffset++;
  }
}  

uint sgpGaGeneration::addCodeFrom(const scDataNode &node, size_t sourceOffset, size_t limit)
{
  std::auto_ptr<sgpEntityBase> genomeCodeGuard;
  scDataNode helper;

  uint addedCnt(0);
  uint realLimit = node.size();
  if (limit > 0)
    realLimit = SC_MIN(sourceOffset + limit, node.size());

  for(uint i=sourceOffset, epos = realLimit; i != epos; i++)
  {
    genomeCodeGuard.reset(newItem());
    genomeCodeGuard->setGenomeAsNode(*(node.getNodePtrR(i, helper)));
    insert(genomeCodeGuard.release());
    addedCnt++;
  }

  return addedCnt;
}

void sgpGaGeneration::transferItemsFrom(sgpGaGeneration &src) {
  m_items.reserve(m_items.size() + src.size());
  if (!src.empty())
  for(int i=src.endPos() - 1,epos = src.beginPos(); i >= epos; i--) {
    m_items.push_back(src.extractItem(i));
  }
}

base::StructureWriterIntf *sgpGaGeneration::newWriterForCode(const scDataNode &extraValues) const
{
  throw std::runtime_error("Not implemented");
}

base::StructureWriterIntf *sgpGaGeneration::newWriterForFitness(const scDataNode &extraValues) const
{
  throw std::runtime_error("Not implemented");
}
