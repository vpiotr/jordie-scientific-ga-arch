/////////////////////////////////////////////////////////////////////////////
// Name:        OperatorXOverGa.h
// Project:     sgpLib
// Purpose:     XOver operator - GA-specific
// Author:      Piotr Likus
// Modified by:
// Created:     24/07/2011
/////////////////////////////////////////////////////////////////////////////

#ifndef _SGPOPXOVGA_H__
#define _SGPOPXOVGA_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/** \file OperatorXOverGa.h
\brief Short file description

Long description
*/

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "perf\counter.h"

#include "sc\dtypes.h"
#include "sgp\OperatorXOverIslands.h"

// ----------------------------------------------------------------------------
// Simple type definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Forward class definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------
const double SGP_GA_DEF_GEN_CHG_PROB_XOVER = 0.6;

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------
class sgpOperatorXOverGa: public sgpOperatorXOverIslands {
  typedef sgpOperatorXOverIslands inherited;
public:
  // construction
  sgpOperatorXOverGa();
  virtual ~sgpOperatorXOverGa();
  // properties
  double getMatchThreshold();
  void setMatchThreshold(double value);
  double getGenChangeProb();
  void setGenChangeProb(double value);
  void setCompareTool(sgpGaGenomeCompareTool *tool);
  virtual void getCounters(scDataNode &output);
protected:
  virtual bool crossGenomes(sgpGaGeneration &newGeneration, uint first, uint second,
    uint newChildCount, uint replaceParentCount);
  virtual double calcGenomeDiff(const sgpGaGeneration &newGeneration, uint first, uint second, uint genNo);
  virtual bool doCrossGen(uint genNo, sgpEntityBase &firstEntity, sgpEntityBase &secondEntity);
  bool crossGenomesSamePos(uint genNo, sgpGaGenome &genFirst, sgpGaGenome &genSecond);
private:
  double m_matchThreshold;
  double m_genChangeProb;
  sgpGaGenomeCompareTool *m_genomeCompareTool;
  perf::LocalCounter m_counter;
};

#endif // _SGPOPXOVGA_H__
