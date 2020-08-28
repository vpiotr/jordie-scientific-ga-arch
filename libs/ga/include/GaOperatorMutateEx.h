/////////////////////////////////////////////////////////////////////////////
// Name:        GaOperatorMutateEx.h
// Project:     sgpLib
// Purpose:     Extended mutation operator for GA algorithm
// Author:      Piotr Likus
// Modified by:
// Created:     06/09/2011
/////////////////////////////////////////////////////////////////////////////

#ifndef _SGPGAOPERATORMUTEX_H__
#define _SGPGAOPERATORMUTEX_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file GaOperatorBasic.h
///
/// Basic GA operators

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
//std
#include <set>

//perf
#include "perf/Counter.h"

//sc
#include "sc/utils.h"
#include "sc/txtf/CsvWriter.h"
#include "sc/events/Events.h"

//sgp
#include "sgp/GaOperatorBasic.h"
#include "sgp/EntityIslandTool.h"

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
enum sgpGaMutFeatures {
  gmfIslandMutRateFilter       = 1, // if enabled, keeps mutation ratio on pop level in a given range
  gmfProtectIslandId           = 2  // if enabled, island ID cannot be changed during mutation
};

// mutate operator with yield support & counters
class sgpGaOperatorMutateEx: public sgpGaOperatorMutateBasic {
  typedef sgpGaOperatorMutateBasic inherited;
public:
  // construct
  sgpGaOperatorMutateEx();
  virtual ~sgpGaOperatorMutateEx();
  // properties
  void setYieldSignal(scSignal *value);
  virtual void getCounters(scDataNode &output);
  virtual double getEntityChangeProb();
  void setExperimentParams(const sgpGaExperimentParams *params);
  void setIslandTool(sgpEntityIslandToolIntf *islandTool);
  uint getFeatures();
  void setFeatures(uint value);
protected:
  virtual void invokeNextEntity();
  virtual void invokeEntityChanged(uint entityIndex);
  virtual bool processGenome(uint entityIndex, sgpGaGenome &genome);
  virtual void beforeExecute(sgpGaGeneration &newGeneration);
  void updateIslandId(uint entityIndex);
  virtual uint getChangePointLimit();
private:
  scSignal *m_yieldSignal;
  perf::LocalCounter m_counter;
  const sgpGaExperimentParams *m_experimentParams;
  sgpEntityIslandToolIntf *m_islandTool;
  uint m_activeIslandId;
  uint m_lastIslandId;
  uint m_features;
  double m_entityChangeProb;
  sgpGaGeneration *m_activeGeneration;
};

#endif // _SGPGAOPERATORMUTEX_H__
