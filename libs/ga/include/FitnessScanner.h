/////////////////////////////////////////////////////////////////////////////
// Name:        FitnessScanner.h
// Project:     sgpLib
// Purpose:     Fitness scanning class
// Author:      Piotr Likus
// Modified by:
// Created:     13/07/2013
/////////////////////////////////////////////////////////////////////////////

#ifndef _SGPFITSCANNER_H__
#define _SGPFITSCANNER_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/** \file FitnessScanner.h
\brief Fitness scanning class

*/

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sgp\GaGeneration.h"
#include "sgp\EntityBase.h"
#include "sgp\FitnessDefs.h"

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
class sgpFitnessStorage;

class sgpFitnessScanner {
  typedef std::pair<uint, uint> UIntPair;
public: 
  sgpFitnessScanner(const scDataNode *input);
  sgpFitnessScanner(const sgpGaGeneration *input);
  virtual ~sgpFitnessScanner();
  void getTopGenomesByObjective(int limit, uint objectiveIndex, sgpEntityIndexList &indices);  
  uint getBestGenomeIndexByObjective(uint objIndex, bool searchForMin = false) const;
  uint findBestWithWeights(const sgpWeightVector &weights, const sgpEntityIndexSet *requiredItems, const sgpEntityIndexSet &ignoreSet, bool includeFront) const;
  uint findBestWithWeights(const sgpWeightVector &weights, const sgpEntityIndexSet *requiredItems = SC_NULL) const;
  void findBestWithWeights(uint &bestIndex, uint &bestCount, const sgpWeightVector &weights) const;
  void getGenomeIndicesSortedDesc(uint objectiveIndex, sgpEntityIndexList &output);    
  void getTopGenomesByWeights(int limit, const sgpWeightVector &weights, sgpEntityIndexList &indices,
    const sgpEntityIndexSet *requiredItems = SC_NULL);
  uint findEqual(const scDataNode &searchItem);  
  bool getBestGenomeIndex(uint aLastMaxIdx, double aLastMaxFit, uint &foundIdx) const;
  uint getBestGenomeIndex() const;
  uint countGenomesByObjectiveValue(uint objIndex, double objValue) const;
protected:  
  bool getBestGenomeIndexByObjective(uint objIndex, double upperLimit, uint &output) const;
  void findBestSingleObj(uint &bestIndex, uint &bestCount, const sgpEntityIndexSet &ignoreSet) const;
  void findBestWithWeightsMultiObj(uint &bestIndex, uint *bestCount, const sgpWeightVector &weights, const sgpEntityIndexSet *requiredItems, const sgpEntityIndexSet &ignoreSet, bool includeFront) const;
  uint findBestWithWeightsMultiObj(const sgpWeightVector &weights, const sgpEntityIndexSet *requiredItems, const sgpEntityIndexSet &ignoreSet, bool includeFront) const;
  void intFindBestWithWeights(uint &bestIndex, uint *bestCount, const sgpWeightVector &weights, const sgpEntityIndexSet *requiredItems, const sgpEntityIndexSet &ignoreSet, bool includeFront) const;
  uint countEntitiesWithFitness(const sgpFitnessValue &fitValue) const;
protected:
  std::auto_ptr<sgpFitnessStorage> m_storage;  
};
  

#endif // _SGPFITSCANNER_H__
