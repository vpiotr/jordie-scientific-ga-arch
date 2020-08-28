/////////////////////////////////////////////////////////////////////////////
// Name:        FitnessScanner.h
// Project:     sgpLib
// Purpose:     Fitness scanning class
// Author:      Piotr Likus
// Modified by:
// Created:     13/07/2013
/////////////////////////////////////////////////////////////////////////////

#include "sgp\FitnessScanner.h"
#include "sc\utils.h"

class sgpFitnessStorage {
public:
  sgpFitnessStorage() {}
  virtual ~sgpFitnessStorage() {}
  virtual void getFitness(uint itemIndex, sgpFitnessValue &output) const = 0;
  virtual double getFitness(uint itemIndex, int objIndex) const = 0;
  virtual double getFitness(uint itemIndex) const = 0;
  virtual uint size() = 0;
  virtual bool empty() { return (size() == 0); }
};

typedef std::pair<uint, double> UIntDoublePair;

// ----------------------------------------------------------------------------
// sgpFitnessStorageForGeneration
// ----------------------------------------------------------------------------
class sgpFitnessStorageForGeneration: public sgpFitnessStorage {
public:
  // construct
  sgpFitnessStorageForGeneration(const sgpGaGeneration *input);
  virtual ~sgpFitnessStorageForGeneration();
  // properties
  virtual void getFitness(uint itemIndex, sgpFitnessValue &output) const;
  virtual double getFitness(uint itemIndex, int objIndex) const;
  virtual double getFitness(uint itemIndex) const;
  virtual uint size();
protected:  
  const sgpGaGeneration *m_fitnessValues;
};

// ----------------------------------------------------------------------------
// sgpFitnessStorageForNode
// ----------------------------------------------------------------------------
class sgpFitnessStorageForNode: public sgpFitnessStorage {
public:
  // construct
  sgpFitnessStorageForNode(const scDataNode *input);
  virtual ~sgpFitnessStorageForNode();
  // properties
  virtual void getFitness(uint itemIndex, sgpFitnessValue &output) const;
  virtual double getFitness(uint itemIndex, int objIndex) const;
  virtual double getFitness(uint itemIndex) const;
  virtual uint size();
protected:  
  const scDataNode *m_fitnessValues;
};

// ----------------------------------------------------------------------------
// sgpFitnessStorageForGeneration
// ----------------------------------------------------------------------------
sgpFitnessStorageForGeneration::sgpFitnessStorageForGeneration(const sgpGaGeneration *input): sgpFitnessStorage()
{
  m_fitnessValues = input;
}

sgpFitnessStorageForGeneration::~sgpFitnessStorageForGeneration()
{
}

void sgpFitnessStorageForGeneration::getFitness(uint itemIndex, sgpFitnessValue &output) const
{
  const sgpEntityBase &element = (*m_fitnessValues)[itemIndex];
  element.getFitness(output);
}

double sgpFitnessStorageForGeneration::getFitness(uint itemIndex, int objIndex) const
{
  const sgpEntityBase &element = (*m_fitnessValues)[itemIndex];
  return element.getFitness(objIndex);
}

double sgpFitnessStorageForGeneration::getFitness(uint itemIndex) const
{
  return getFitness(itemIndex, 0);
}  

uint sgpFitnessStorageForGeneration::size()
{
  return m_fitnessValues->size();
}

// ----------------------------------------------------------------------------
// sgpFitnessStorageForNode
// ----------------------------------------------------------------------------
sgpFitnessStorageForNode::sgpFitnessStorageForNode(const scDataNode *input)
{
  m_fitnessValues = input;
}

sgpFitnessStorageForNode::~sgpFitnessStorageForNode()
{
}

void sgpFitnessStorageForNode::getFitness(uint itemIndex, sgpFitnessValue &output) const
{
  scDataNode helper;
  const scDataNode &element = 
     m_fitnessValues->getNode(itemIndex, helper);
  output.resize(element.size());
  for(uint i=0, epos = output.size(); i != epos; i++)
    output[i] = element.getDouble(i);
}

double sgpFitnessStorageForNode::getFitness(uint itemIndex, int objIndex) const
{
  scDataNode helper;
  const scDataNode &element = 
     m_fitnessValues->getNode(itemIndex, helper);
  //return m_fitnessValues->getElement(itemIndex).getDouble(objIndex);
  return element.getDouble(objIndex);
}

double sgpFitnessStorageForNode::getFitness(uint itemIndex) const
{
  return getFitness(itemIndex, 0);
}  

uint sgpFitnessStorageForNode::size()
{
  return m_fitnessValues->size();
}


// ----------------------------------------------------------------------------
// sgpFitnessScanner
// ----------------------------------------------------------------------------
sgpFitnessScanner::sgpFitnessScanner(const scDataNode *input)
{
  m_storage.reset(new sgpFitnessStorageForNode(input));
}

sgpFitnessScanner::sgpFitnessScanner(const sgpGaGeneration *input)
{
  m_storage.reset(new sgpFitnessStorageForGeneration(input));
}

sgpFitnessScanner::~sgpFitnessScanner()
{
}

void sgpFitnessScanner::getTopGenomesByObjective(int limit, uint objectiveIndex, sgpEntityIndexList &indices)
{
  int leftCnt = std::min<uint>(limit, m_storage->size());
  uint lastIdx = m_storage->size();
  uint maxIdx;
  double maxFit;

  indices.clear();
  if (m_storage->empty()) 
    return;

  maxIdx = getBestGenomeIndexByObjective(objectiveIndex, false);
  
  while(leftCnt > 0) {
    maxFit = m_storage->getFitness(maxIdx, objectiveIndex);
    indices.push_back(maxIdx);
    lastIdx = maxIdx;
    leftCnt--;
    if (!getBestGenomeIndexByObjective(objectiveIndex, maxFit, maxIdx))
      break;
  }
}

uint sgpFitnessScanner::getBestGenomeIndexByObjective(uint objIndex, bool searchForMin) const
{
  double maxFit = 0.0;
  uint maxIdx = m_storage->size();
  double fit;
      
  if (m_storage->size() > 0) {
    maxFit = m_storage->getFitness(0, objIndex);
    maxIdx = 0;
  } 

  if (searchForMin) {
    for(uint i = 0, epos = m_storage->size(); i != epos; i++)
    {
      fit = m_storage->getFitness(i, objIndex);
      if (fit < maxFit) 
      {
        maxFit = fit;
        maxIdx = i;
      }  
    }
  } else {      
    for(uint i = 0, epos = m_storage->size(); i != epos; i++)
    {
      fit = m_storage->getFitness(i, objIndex);
      if (fit > maxFit) 
      {
        maxFit = fit;
        maxIdx = i;
      }  
    }
  }
  return maxIdx;
}

bool sgpFitnessScanner::getBestGenomeIndexByObjective(uint objIndex, double upperLimit, uint &output) const
{
  double maxFit = 0.0;
  uint maxIdx = 0;
  double fit;
  bool res = false;
      
  if (m_storage->size() > 0) {
    maxIdx = 0;
    maxFit = m_storage->getFitness(0, objIndex);
  } 

  for(uint i = 0, epos = m_storage->size(); i != epos; i++)
  {
    fit = m_storage->getFitness(i, objIndex);
    if (fit < upperLimit) {
      if (!res || (fit > maxFit)) {
        maxFit = fit;
        maxIdx = i;
        res = true;
      }
    }
  }
  
  output = maxIdx;
  return res;
}

void sgpFitnessScanner::getTopGenomesByWeights(int limit, 
    const sgpWeightVector &weights, sgpEntityIndexList &indices,
    const sgpEntityIndexSet *requiredItems)
{
  sgpEntityIndexSet workIndices;

  int leftCnt = std::min<uint>(limit, m_storage->size());
  uint maxIdx;

  indices.clear();  
  
  if (m_storage->size() == 0) 
    return;
                                 
  sgpEntityIndexSet ignoreSet;
  maxIdx = findBestWithWeights(weights, requiredItems, ignoreSet, false);  
  
  while(leftCnt > 0) {
    indices.push_back(maxIdx);
    workIndices.insert(maxIdx);
    leftCnt--;
    maxIdx = findBestWithWeights(weights, requiredItems, workIndices, true); 
    if (maxIdx >= m_storage->size()) 
      break;
  }
}    

uint sgpFitnessScanner::findBestWithWeights(const sgpWeightVector &weights, const sgpEntityIndexSet *requiredItems) const
{
  sgpEntityIndexSet ignoreSet;
  return findBestWithWeights(weights, requiredItems, ignoreSet, false);
}

void sgpFitnessScanner::findBestWithWeights(uint &bestIndex, uint &bestCount, const sgpWeightVector &weights) const
{
  sgpEntityIndexSet ignoreSet;
  intFindBestWithWeights(bestIndex, &bestCount, weights, NULL, ignoreSet, false);
}

uint sgpFitnessScanner::findBestWithWeights(const sgpWeightVector &weights, const sgpEntityIndexSet *requiredItems, const sgpEntityIndexSet &ignoreSet, bool includeFront) const
{
  uint bestIndex;

  intFindBestWithWeights(bestIndex, NULL, weights, requiredItems, ignoreSet, includeFront);

  return bestIndex;
}

void sgpFitnessScanner::intFindBestWithWeights(uint &bestIndex, uint *bestCount, const sgpWeightVector &weights, const sgpEntityIndexSet *requiredItems, const sgpEntityIndexSet &ignoreSet, bool includeFront) const
{
  uint locBestCount;

  if ((weights.size() < 3) && (requiredItems == SC_NULL))
    findBestSingleObj(bestIndex, locBestCount, ignoreSet);
  else
    findBestWithWeightsMultiObj(bestIndex, &locBestCount, weights, requiredItems, ignoreSet, includeFront);

  if (bestCount != NULL)
    *bestCount = locBestCount;
}

/// Find best item using just one objective 
void sgpFitnessScanner::findBestSingleObj(uint &bestIndex, uint &bestCount, const sgpEntityIndexSet &ignoreSet) const
{
  if (m_storage->empty()) {
    bestIndex = m_storage->size();
    bestCount = 0;
    return;
  }

  double bestFit, objValue;
  sgpFitnessValue fitVector;

  const uint fitObjIdx = sgpFitnessValue::SGP_OBJ_OFFSET + 0;

  bestFit = m_storage->getFitness(0, fitObjIdx);
  sgpEntityIndexSet::iterator ignoreEpos = ignoreSet.end();
  bool ignoreEmpty = ignoreSet.empty();

  //uint 
  bestIndex = 0;
  bestCount = 1;

  for(uint i = 1, epos = m_storage->size(); i != epos; i++) {
    if (ignoreEmpty || (ignoreSet.find(i) == ignoreEpos))
    {
      objValue = m_storage->getFitness(i, fitObjIdx);
      if (objValue > bestFit)
      {
        bestIndex = i;
        bestFit = objValue;
        bestCount = 1;
      } else if (objValue == bestFit) {
        bestCount++;
      }
    }
  }

}

void sgpFitnessScanner::findBestWithWeightsMultiObj(uint &bestIndex, uint *bestCount, const sgpWeightVector &weights, const sgpEntityIndexSet *requiredItems, const sgpEntityIndexSet &ignoreSet, bool includeFront) const
{
  uint locBestIndex, locBestCount;
  locBestIndex = findBestWithWeightsMultiObj(weights, requiredItems, ignoreSet, includeFront);
  if (locBestIndex >= m_storage->size())
    locBestCount = 0;
  else {
    sgpFitnessValue bestValues;
    m_storage->getFitness(locBestIndex, bestValues);
    locBestCount = countEntitiesWithFitness(bestValues);
  }

  bestIndex = locBestIndex;
  if (bestCount != NULL)
    *bestCount = locBestCount;
}

uint sgpFitnessScanner::findBestWithWeightsMultiObj(const sgpWeightVector &weights, const sgpEntityIndexSet *requiredItems, const sgpEntityIndexSet &ignoreSet, bool includeFront) const
{
  double wLevel = 0.0;
  double nextLevel = 0.0;
  bool wLevelIsNull = true;
  bool nextLevelIsNull;
  uint objCnt = weights.size();
  std::set<uint> workGroup;
  std::set<uint> newWorkGroup;
  sgpFitnessValue bestValues, fitVector;
  uint bestIndex;    
  int minusCnt;
  int plusCnt;
  int zeroCnt;
  int maxCnt;
  int compRes;
  
  for(uint i = 0, epos = m_storage->size(); i != epos; i++) {
    if (ignoreSet.find(i) == ignoreSet.end())
      workGroup.insert(i);
  }
  
  if (requiredItems != SC_NULL)
  {
    newWorkGroup.clear();
    for(std::set<uint>::iterator it = workGroup.begin(), epos = workGroup.end(); it != epos; ++it)
    {
      if (requiredItems->find(*it) != requiredItems->end())
        newWorkGroup.insert(*it);
    }
    workGroup = newWorkGroup;    
  }
  
  while(workGroup.size() > 1) {
    // find minimal weight value > current wLevel
    nextLevelIsNull = true;
    maxCnt = 0;
    for(uint i=sgpFitnessValue::SGP_OBJ_OFFSET,epos = objCnt; i!=epos; i++) {
      if (wLevelIsNull || (wLevel < weights[i])) {
        if (nextLevelIsNull || (nextLevel > weights[i])) {
          nextLevel = weights[i];
          nextLevelIsNull = false;
        }  
      }
    }
    
    if (nextLevelIsNull)
      break; // last level processed
      
    // next level found
    wLevel = nextLevel;
    wLevelIsNull = false;
    
    // find element with best set of values in work group
    bestIndex = *workGroup.begin();
    bestValues.clear();
    m_storage->getFitness(bestIndex, bestValues);
    
    // find set of best objective values
    for(std::set<uint>::const_iterator it = workGroup.begin(), epos=workGroup.end(); it != epos; ++it)
    {
      m_storage->getFitness(*it, fitVector);
      
      maxCnt = minusCnt = plusCnt = zeroCnt = 0;
      
      for(uint i=sgpFitnessValue::SGP_OBJ_OFFSET,epos = objCnt; i!=epos; i++) {
        if (weights[i] <= wLevel) {
          maxCnt++;
          compRes = fpComp(fitVector[i], bestValues[i]);

          if (compRes < 0) {
            minusCnt++;
          } else if (compRes > 0) {  
            plusCnt++;
          } else {
            zeroCnt++;
          }
        }
      }
      
      if ((minusCnt == 0) && (plusCnt > 0)) 
      {
        bestValues = fitVector; 
        bestIndex = *it;
      }
    } // for whole workgroup
    
    // find all items with compatible with found bestValues
    newWorkGroup.clear();
    for(std::set<uint>::const_iterator it = workGroup.begin(),epos=workGroup.end(); it != epos; ++it)
    {
      m_storage->getFitness(*it, fitVector);

      minusCnt = plusCnt = zeroCnt = 0;
      
      for(uint i=sgpFitnessValue::SGP_OBJ_OFFSET,epos = objCnt; i!=epos; i++) {
        if (weights[i] <= wLevel) {
          compRes = fpComp(fitVector[i], bestValues[i]);

          if (compRes < 0) {
            minusCnt++;
          } else if (compRes > 0) {  
            plusCnt++;
          } else {
            zeroCnt++;
          }
        }
      }
      
      if (
          (zeroCnt == maxCnt) 
          || 
          (!includeFront && (minusCnt == 0))
          ||
          (includeFront && (plusCnt > 0))
         )
      // includeFront - use "in some places better, in some worse" items
      {
        newWorkGroup.insert(*it);
      }
    } // for whole workgroup

    workGroup = newWorkGroup;    
  } // while not one in workgroup
  
  if (workGroup.empty())
    return m_storage->size();
  else  
    return *workGroup.begin();  
}

uint sgpFitnessScanner::findEqual(const scDataNode &searchItem)
{
  uint srcObjSize = searchItem.size();
  bool found;

  uint res = m_storage->size();
  sgpFitnessValue fitVector;
    
  for(uint i=0, epos = m_storage->size(); i != epos; i++) 
  {
    m_storage->getFitness(i, fitVector);      

    found = true;
    for(uint j=0, eposj = SC_MIN(fitVector.size(), srcObjSize); (j != eposj) && found; j++)
    { 
      if (fitVector[j] != searchItem.getDouble(j))
      {
        found = false;
      }  
    }
    
    if (found) {
      res = i;
      break;
    }  
  }  
  
  return res;
}

uint sgpFitnessScanner::getBestGenomeIndex() const
{
  double maxFit = 0.0;
  uint maxIdx = 0;
  double fit;
      
  if (m_storage->size() > 0) {
    maxFit = m_storage->getFitness(0, 0);      
    maxIdx = 0;
  } 
      
  for(uint i = 0, epos = m_storage->size(); i != epos; i++)
  {
    //fit = at(i).getFitness();
    fit = m_storage->getFitness(i, 0);      
    if (fit > maxFit) 
    {
      maxFit = fit;
      maxIdx = i;
    }  
  }
  
  return maxIdx;
}

bool sgpFitnessScanner::getBestGenomeIndex(uint aLastMaxIdx, double aLastMaxFit, uint &foundIdx) const
{
  double maxFit = 0.0;
  uint maxIdx = 0;
  double fit;
  bool found = false;
      
  for(uint i = 0, epos = m_storage->size(); i != epos; i++)
  {
    fit = m_storage->getFitness(i, 0);      
    if (!found || (fit > maxFit))
    {
      if ((fit < aLastMaxFit) || ((fit == aLastMaxFit) && (i < aLastMaxIdx)))
      {
        maxFit = fit;
        maxIdx = i;
        found = true;
      }
    }  
  }
  
  foundIdx = maxIdx;
  return found;
}

bool fitscan_sort_second_pred_gt(const UIntDoublePair& left, const UIntDoublePair& right)
{
  return left.second > right.second;
}

void sgpFitnessScanner::getGenomeIndicesSortedDesc(uint objectiveIndex, sgpEntityIndexList &output)
{
  std::vector<UIntDoublePair> sortVect; 
  double fitValue;
    
  for(uint i=0, epos = m_storage->size(); i != epos; i++) 
  {
    fitValue = m_storage->getFitness(i, objectiveIndex);      
    sortVect.push_back(std::make_pair(i, fitValue));
  }   

  std::sort(sortVect.begin(), sortVect.end(), fitscan_sort_second_pred_gt);         
  
  output.clear();
  for(uint i=0, epos = sortVect.size(); i != epos; i++)
    output.push_back(sortVect[i].first);
}

uint sgpFitnessScanner::countGenomesByObjectiveValue(uint objIndex, double objValue) const
{
  uint res = 0;
  double fitValue;
      
  for(uint i=0, epos = m_storage->size(); i != epos; i++) 
  {
    fitValue = m_storage->getFitness(i, objIndex);      
    // note: this will work fully correct only if the fitness function input is the same
    if (fitValue == objValue) 
      res++;
  }
  return res;
}

uint sgpFitnessScanner::countEntitiesWithFitness(const sgpFitnessValue &fitValue) const
{
  uint res = 0;

  sgpFitnessValue checkValue;

  for(uint i=0, epos = m_storage->size(); i != epos; i++) 
  {
    m_storage->getFitness(i, checkValue);
    if (fitValue.compare(checkValue) == 0)
      res++;
  }
  return res;
}

