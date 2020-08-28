/////////////////////////////////////////////////////////////////////////////
// Name:        EvalFltClearNan.cpp
// Project:     sgpLib
// Purpose:     Removes NaN values from fitness.
// Author:      Piotr Likus
// Modified by:
// Created:     11/06/2011
/////////////////////////////////////////////////////////////////////////////

#include "sc/defs.h"

#include "sc/smath.h"
#include "sgp/EvalFltClearNan.h"

sgpEvalFltClearNan::sgpEvalFltClearNan(sgpGaOperatorEvaluate *prior):
  sgpGaOperatorEvaluate(), m_prior(prior)
{
}

sgpEvalFltClearNan::~sgpEvalFltClearNan()
{
}

void sgpEvalFltClearNan::setObjectiveSigns(const scVectorOfInt &value)
{
  m_objectiveSigns = value;
}

void sgpEvalFltClearNan::setPrior(sgpGaOperatorEvaluate *prior)
{
  m_prior = prior;
}

bool sgpEvalFltClearNan::execute(uint stepNo, bool isNewGen, sgpGaGeneration &generation)
{
  bool res = m_prior->execute(stepNo, isNewGen, generation);
  filterFitness(generation);
  return res;
}

void sgpEvalFltClearNan::filterFitness(sgpGaGeneration &generation)
{
  sgpFitnessValue fitness;

  for(uint i=0, epos = generation.size(); i != epos; i++)
  {
    generation.at(i).getFitness(fitness);

    for(uint fidx=sgpFitnessValue::SGP_OBJ_OFFSET,epos=fitness.size(); fidx != epos; fidx++) {
      if (isnan(fitness[fidx])) {
#ifdef COUT_ENABLED
        cout << "NAN: " << fidx << ": " << fitness[fidx] << "\n";
#endif      
        if (m_objectiveSigns[fidx] > 0) 
            fitness[fidx] = 0.0;
        else
            fitness[fidx] = -1e+100;      

        assert(!isnan(fitness[fidx]));
      }
    }  

    generation.at(i).setFitness(fitness);

  }  
}

