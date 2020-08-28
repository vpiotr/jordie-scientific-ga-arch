/////////////////////////////////////////////////////////////////////////////
// Name:        FitnessFunction.h
// Project:     sgpLib
// Purpose:     Abstract function class which returns fitness for given params.
// Author:      Piotr Likus
// Modified by:
// Created:     14/07/2013
/////////////////////////////////////////////////////////////////////////////


#ifndef _SGPFITFUNC_H__
#define _SGPFITFUNC_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file FitnessFunction.h
///
/// Abstract function class which returns fitness for given params.

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sgp\FitnessValue.h"
#include "sgp\FitnessDefs.h"
#include "sgp\EntityBase.h"

enum sgpGpStopStatus {
  gssNull = 0,
  gssHitFound = 1,
  gssRestartOnSize = 2,
  gssRestartOnDynamic = 4,
  gssRestartOnProgress = 8
};  

typedef std::vector<bool> sgpObjectiveSet; 
typedef std::vector<int> sgpObjectiveSigns; 

class sgpFitnessFunction {
public:
  // construct
  sgpFitnessFunction(): m_stopStatus(gssNull) {}
  virtual ~sgpFitnessFunction() {};

  // properties
  uint getStopStatus() {return m_stopStatus;}
  void setStopStatus(uint value) { m_stopStatus = value; }  
  virtual int getFunctionId() const = 0;

  // run
  virtual bool calc(uint entityIndex, const sgpEntityBase *entity, sgpFitnessValue &fitness) const = 0;
  
  /// Reset any internal state variables to initial state, used for evolution restarts.
  virtual void reset() { m_stopStatus = gssNull; }

  /// returns number of objectives - values inside fitness vector, including fake #0
  virtual uint getObjectiveCount() const = 0;
  /// returns default weights of each objective, including fake #0
  virtual void getObjectiveWeights(sgpWeightVector &output) const;
  /// returns on/off flags for each defined objective 
  virtual void getObjectiveSet(sgpObjectiveSet &output) const;
  /// modifies on/off flags for each defined objective 
  virtual void setObjectiveSet(const sgpObjectiveSet &value);
  /// returns sign for each of objective (positive or negative)
  virtual void getObjectiveSigns(sgpObjectiveSigns &output) const;

private:
   uint m_stopStatus;
};

#endif // _SGPFITFUNC_H__
