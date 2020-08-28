/////////////////////////////////////////////////////////////////////////////
// Name:        GaOperatorMonitorIslandOpt.h
// Project:     g4gLib
// Purpose:     Operator-monitor for island optimization.
// Author:      Piotr Likus
// Modified by:
// Created:     05/06/2010
/////////////////////////////////////////////////////////////////////////////

#ifndef _GAOPERATORMONITORISLANDOPT_H__
#define _GAOPERATORMONITORISLANDOPT_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file GaOperatorMonitorIslandOpt.h
///
/// File description

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
//sc
#include "sc/alg/PsoOptimizer.h"
//sgp
#include "sgp/GaEvolver.h"
#include "sgp/ExperimentLog.h"
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
class sgpGaOperatorMonitorIslandOpt: public sgpGaOperatorMonitor {
  typedef sgpGaOperatorMonitor inherited;
public:
  sgpGaOperatorMonitorIslandOpt();
  virtual ~sgpGaOperatorMonitorIslandOpt();
  void setExperimentParams(sgpGaExperimentParams *params);
  void setOptimizer(scPsoOptimizer *optimizer);
  void setIslandLimit(uint value);
  void setExperimentLog(sgpExperimentLog *value);
  void setRatingObjs(const sgpObjectiveIndexSet &ratingObjs);
  void setIslandTool(sgpEntityIslandToolIntf *value);
protected:
  virtual void intExecute(uint stepNo, const sgpGaGeneration &input);
  void rateIslands(const sgpGaGeneration &input, scDataNode &islandRating, uint &bestIslandId);
  virtual void calcIslandsSize(const sgpGaGeneration &input, scDataNode &islandSize);
  void addIslandRatingForObjective(scDataNode &islandRating, const sgpGaGeneration &input, uint objectiveIndex);
  void calcTopIslandStats(const sgpGaGeneration &input, 
    const scDataNode &topIslandIds,
    scDataNode &topIslandSum, scDataNode &topIslandSize);
  virtual void optimizeParams(const scDataNode &islandRating, sgpGaExperimentParamsStored *params);
  void logIslandStatsToFile(uint stepNo, uint topBestIslandId, uint objBestIslandId, 
    const scDataNode &islandRating);
  void logIslandParamsToFile(uint stepNo, const scDataNode &islandRating, const scDataNode &islandSize);
  void prepareIslandIds(const sgpGaGeneration &input, const sgpEntityIndexList &topIdList, 
    scDataNode &topIslandIds);
  virtual void getTopGenomesByObjective(const sgpGaGeneration &input, uint objectiveIndex, 
    scDataNode &topIslandIds);
  uint getIslandTopSize();
  virtual uint getBestIslandIdByBestItem();
protected:  
  uint m_islandLimit;
  uint m_activeStepNo; 
  sgpGaExperimentParams *m_experimentParams;
  scPsoOptimizer *m_optimizer;
  sgpExperimentLog *m_experimentLog;
  sgpEntityIslandToolIntf *m_islandTool;
  sgpObjectiveIndexSet m_ratingObjs; 
};
  
#endif // _GAOPERATORMONITORISLANDOPT_H__
