/////////////////////////////////////////////////////////////////////////////
// Name:        EvalFltNormProb.cpp
// Project:     sgpLib
// Purpose:     Normalization with objective probability filter.
// Author:      Piotr Likus
// Modified by:
// Created:     11/06/2011
/////////////////////////////////////////////////////////////////////////////


#include "base/rand.h"
#include "base/bmath.h"

#include "sc/defs.h"

#include "sgp/EvalFltNormProb.h"

sgpEvalFltNormProb::sgpEvalFltNormProb(sgpGaOperatorEvaluate *prior):
  sgpGaOperatorEvaluate()
{
}

sgpEvalFltNormProb::~sgpEvalFltNormProb()
{
}

void sgpEvalFltNormProb::setObjectiveProbs(const scVectorOfDouble &value)
{
  m_objectiveProbs = value;
}

void sgpEvalFltNormProb::setObjectiveWeights(const sgpWeightVector &value)
{
  m_objectiveWeights = value;
}

uint sgpEvalFltNormProb::getObjectiveCount()
{
  return m_objectiveWeights.size();
}

void sgpEvalFltNormProb::setPrior(sgpGaOperatorEvaluate *prior)
{
  m_prior = prior;
}

bool sgpEvalFltNormProb::execute(uint stepNo, bool isNewGen, sgpGaGeneration &generation)
{
  bool res = m_prior->execute(stepNo, isNewGen, generation);
  filterFitness(generation);
  return res;
}

void sgpEvalFltNormProb::filterFitness(sgpGaGeneration &generation)
{
  if (generation.empty())
    return;

  if (!getObjectiveCount()) 
    return;

  scVectorOfBool objectiveFlags;

  if (!m_objectiveProbs.empty()) {
    selectObjectives(m_objectiveProbs, objectiveFlags);
  } else {
    objectiveFlags.resize(getObjectiveCount());
    for(uint i=0, epos = objectiveFlags.size(); i != epos; i++) objectiveFlags[i] = true;
  }

  normObjectives(generation, objectiveFlags);
  calcTotalFitness(generation, objectiveFlags);
  normTotalFitness(generation);
}

void sgpEvalFltNormProb::selectObjectives(const scVectorOfDouble &objectiveProbs, scVectorOfBool &objectiveFlags)
{
  for(uint i=0,epos = objectiveProbs.size(); i != epos; i++)
    objectiveFlags[i] = randomFlip(objectiveProbs[i]);  
}

void sgpEvalFltNormProb::normObjectives(sgpGaGeneration &newGeneration, const scVectorOfBool &objectiveFlags) {
  double fit;
  double useWeight, fitSign;
  
  if (!newGeneration.size())
    return;
  
  // normalize fitness value
  for(uint i=1, epos = getObjectiveCount(); i != epos; i++)
  {
    if (!objectiveFlags[i]) 
      continue;      

    fitSign = newGeneration.at(0).getFitness(i);

    normObjectiveByIndex(newGeneration, i);
          
    useWeight = m_objectiveWeights[i];
    if (fitSign < 0.0)
      useWeight = -useWeight;          

    for(uint j=0, eposj = newGeneration.size(); j != eposj; j++)
    {
      fit = newGeneration[j].getFitness(i);
      if (isnan(fit)) {
#ifdef COUT_ENABLED      
      cout << "NAN-base: " << i << ": " << fit[i] << endl;
#endif         
      fit = -1e+100; // max error   
      }
      assert(!isnan(fit));
      newGeneration[j].setFitness(i, fit+useWeight);
    } // for j 
  } // for i
}
  
void sgpEvalFltNormProb::calcTotalFitness(sgpGaGeneration &newGeneration, const scVectorOfBool &objectiveFlags) {    
  // calc total fitness
  double totalPlus;
  double totalMinus;
  double fit;
  sgpFitnessValue fitnessValue;
  bool filteredObjectives = (!objectiveFlags.empty());

  for(uint j=0, eposj = newGeneration.size(); j != eposj; j++)
  {
    newGeneration[j].getFitness(fitnessValue);
    if (filteredObjectives)
      filterObjectives(objectiveFlags, fitnessValue);
    calcTotalFitnessWithMidVals(fitnessValue, totalPlus, totalMinus, fit);
    newGeneration[j].setFitness(0, fit);      
  } // for j
} // function

void sgpEvalFltNormProb::normTotalFitness(sgpGaGeneration &newGeneration) 
{    
  normObjectiveByIndex(newGeneration, 0);
}

void sgpEvalFltNormProb::normObjectiveByIndex(sgpGaGeneration &newGeneration, uint index) {    
  double min0, max0, sum0, fit;
  
  calcObjectiveStats(newGeneration, index, min0, max0, sum0);
   
#ifdef DEBUG_FIT_BASE
  bool testSign = 
     ((min0 <= 0.0) && (max0 <= 0.0)) 
     ||
     ((min0 >= 0.0) && (max0 >= 0.0)); 
     
  if (!testSign)
  {  
    cout << "obj: [" << index << "], min: [" << min0 << "], max: [" << max0 << "]" << endl;
    assert(
      ((min0 <= 0.0) && (max0 <= 0.0)) 
       ||
      ((min0 >= 0.0) && (max0 >= 0.0))
    );
  }  
#endif
   
  if (min0 != max0) { 
    const double newRangeMin = 0.01;
    const double newRangeMax = 0.99;
    const double newRangeDiff = newRangeMax - newRangeMin;  
    double avgFit;  

    for(uint j=0, eposj = newGeneration.size(); j != eposj; j++)
    {
      fit = newGeneration[j].getFitness(index);
      avgFit = (1.0/double(newGeneration.size()))*sum0;
#ifdef HIPREC_SELECT          
      fit = (fit - min0)/(max0 - min0);
#else
#ifdef NORM_TYPE_SIGM 
        if (fit >= 0.0) {
          //fit = fit / avgFit * smath_const_e;
          fit = (1.0E-100+fit) / (1.0E-100+avgFit) * smath_const_e;
          fit = 2.0*(1.0/(1.0+exp(-fit)))-1.0;
        }    
        else {
          //fit = fit / avgFit * smath_const_e;
          fit = (-1.0E-100+fit) / (-1.0E-100+avgFit) * smath_const_e;
          fit = 2.0*(1.0/(1.0+exp(-fit)))-1.0;
          fit = -fit;
        }  
#else 
      if (fit >= 0.0) {
        fit = fit / max0;
        fit = fit * newRangeDiff + newRangeMin;
      }  
      else {
        fit = - (fit / min0);  
        fit = fit * newRangeDiff - newRangeMin;
      }  
#endif
#endif        
      newGeneration[j].setFitness(index, fit);
    } // for
  } // min0 != max0
  else {
    fit = 1.0 / newGeneration.size();
    if (min0 < 0.0)
      fit = -fit;
    for(uint j=0, eposj = newGeneration.size(); j != eposj; j++)
    {
      newGeneration[j].setFitness(index, fit);
    } // for
  }
}

void sgpEvalFltNormProb::filterObjectives(const scVectorOfBool &objectiveFlags, sgpFitnessValue &fitnessValue)
{
  for(uint i=1, epos = fitnessValue.size(); i != epos; i++)
    if (!objectiveFlags[i])
      fitnessValue[i] = 0.0;
}  

void sgpEvalFltNormProb::calcTotalFitnessWithMidVals(const sgpFitnessValue &fitnessValue, 
  double &totalPlus, double &totalMinus, double &fitValue) 
{    
  totalPlus = totalMinus = 1.0;

  double fit;
  
  for(uint i=1, epos = fitnessValue.size(); i != epos; i++)
  {
    fit = fitnessValue[i];
    if (fit < 0.0)
      totalMinus = totalMinus * (1.001 - fit);
    else
      totalPlus = totalPlus * (0.001 + fit);   
  }  
            
  fitValue = totalPlus / (1.0 + totalMinus);
} // function

void sgpEvalFltNormProb::calcObjectiveStats(sgpGaGeneration &newGeneration, uint objectiveIndex, 
  double &minValue, double &maxValue, double &sumValue) 
{    
  double min0, max0, sum0, fit;
  
  if (newGeneration.empty()) {
    minValue = maxValue = sumValue = 0.0;
  } else {  
    min0 = max0 = sum0 = newGeneration[0].getFitness(objectiveIndex);

    // rescale total fitness
    for(uint j=1, eposj = newGeneration.size(); j != eposj; j++)
    {
      fit = newGeneration[j].getFitness(objectiveIndex);
      sum0 += fit;
      if (fit < min0)
        min0 = fit;
      else if (fit > max0)
        max0 = fit;  
    }
    
    minValue = min0;
    maxValue = max0;
    sumValue = sum0;
  }  
}
