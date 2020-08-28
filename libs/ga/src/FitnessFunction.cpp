/////////////////////////////////////////////////////////////////////////////
// Name:        FitnessFunction.h
// Project:     sgpLib
// Purpose:     Abstract function class which returns fitness for given params.
// Author:      Piotr Likus
// Modified by:
// Created:     14/07/2013
/////////////////////////////////////////////////////////////////////////////

#include "sgp\FitnessFunction.h"

// ----------------------------------------------------------------------------
// sgpFitnessFunction
// ----------------------------------------------------------------------------
void sgpFitnessFunction::getObjectiveWeights(sgpWeightVector &output) const
{
  // base version returns all weights equal, so all objectives are equally important
  output.resize(getObjectiveCount());
  for(uint i = 0,epos = getObjectiveCount(); i!=epos; i++)
    output[i] = 100.0;
}

void sgpFitnessFunction::getObjectiveSet(sgpObjectiveSet &output) const
{
  output.resize(getObjectiveCount());
  for(uint i = 0,epos = getObjectiveCount(); i!=epos; i++)
    output[i] = 1;
}

void sgpFitnessFunction::setObjectiveSet(const sgpObjectiveSet &value)
{
  throw scNotImplementedError();
}

void sgpFitnessFunction::getObjectiveSigns(sgpObjectiveSigns &output) const
{
  throw scNotImplementedError();
}

