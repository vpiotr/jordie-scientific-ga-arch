/////////////////////////////////////////////////////////////////////////////
// Name:        GaOperatorMutateEx.cpp
// Project:     sgpLib
// Purpose:     Extended mutation operator for GA algorithm
// Author:      Piotr Likus
// Modified by:
// Created:     06/09/2011
/////////////////////////////////////////////////////////////////////////////
#include "sc/defs.h"

//std
#include <set>
#include <cmath>

//base
#include "base/rand.h"

//perf
//#include "perf/Counter.h"
//#include "perf/Timer.h"
//#include "perf/Log.h"

//sc
#include "sc/defs.h"
#include "sc/utils.h"
#include "sc/smath.h"

//sgp
#include "sgp/GaOperatorMutateEx.h"
#include "sgp\GaStatistics.h"
#include "sgp\ExperimentConst.h"

#ifdef TRACE_ENTITY_BIO
#include "sgp\GpEntityTracer.h"
#endif

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

// ----------------------------------------------------------------------------
// sgpGaOperatorMutateEx
// ----------------------------------------------------------------------------
sgpGaOperatorMutateEx::sgpGaOperatorMutateEx(): inherited()
{
  m_yieldSignal = SC_NULL;
  m_experimentParams = SC_NULL;
  m_islandTool = SC_NULL;
  m_features = 0;
}

sgpGaOperatorMutateEx::~sgpGaOperatorMutateEx()
{
}

uint sgpGaOperatorMutateEx::getFeatures()
{
  return m_features;
}

void sgpGaOperatorMutateEx::setFeatures(uint value)
{
  m_features = value;
}

void sgpGaOperatorMutateEx::setYieldSignal(scSignal *value)
{
  m_yieldSignal = value;
}

void sgpGaOperatorMutateEx::getCounters(scDataNode &output)
{
  m_counter.getAll(output);
}

void sgpGaOperatorMutateEx::setIslandTool(sgpEntityIslandToolIntf *islandTool)
{
  m_islandTool = islandTool;
}

void sgpGaOperatorMutateEx::beforeExecute(sgpGaGeneration &newGeneration)
{
  m_activeGeneration = &newGeneration;
  m_activeIslandId = 0;
  m_lastIslandId = static_cast<uint>(-1);
}

void sgpGaOperatorMutateEx::updateIslandId(uint entityIndex)
{
  assert(m_islandTool != SC_NULL);
  m_islandTool->getIslandId(m_activeGeneration->at(entityIndex), m_activeIslandId);
}

double sgpGaOperatorMutateEx::getEntityChangeProb()
{
  if ((m_features & gmfIslandMutRateFilter) == 0) {
    return inherited::getProbability();
  }
  
  if (m_activeIslandId != m_lastIslandId) {
    double entityChangeProb;

    m_lastIslandId = m_activeIslandId;

    entityChangeProb = 1.0;

    if (m_experimentParams != SC_NULL)
    {  
      if (m_experimentParams->getDouble(m_activeIslandId, SGP_EXP_PAR_BLOCK_IDX_MUT + SGP_MUT_EP_BASE_CHG_PROB, entityChangeProb))
      {
        entityChangeProb = entityChangeProb * 2.0; // can increase or descrease ratio
      } 
    }

    m_entityChangeProb = inherited::getProbability() * entityChangeProb;
  }

  return m_entityChangeProb;
}

uint sgpGaOperatorMutateEx::getChangePointLimit()
{
  double changeLimit;
  uint res;
  if (m_experimentParams->getDouble(m_activeIslandId, SGP_EXP_PAR_BLOCK_IDX_MUT + SGP_MUT_EP_POINT_RANGE, changeLimit))
  {
    res = SC_MAX(1u, round<uint>(changeLimit));
  } else {
    res = 1;
  }
  return res;
}

void sgpGaOperatorMutateEx::setExperimentParams(const sgpGaExperimentParams *params)
{
  m_experimentParams = params;
}

void sgpGaOperatorMutateEx::invokeNextEntity()
{
  if (m_yieldSignal != SC_NULL)
    m_yieldSignal->execute();
}

bool sgpGaOperatorMutateEx::processGenome(uint entityIndex, sgpGaGenome &genome) {
  updateIslandId(entityIndex);

  bool protectIslandId = ((m_features & gmfProtectIslandId) != 0);
  uint oldIslandId = 0;

  if (protectIslandId)
    m_islandTool->getIslandId(m_activeGeneration->at(entityIndex), oldIslandId);

  bool res = inherited::processGenome(entityIndex, genome);
  if (res) {
    if (protectIslandId) {
      uint newIslandId;
      m_islandTool->getIslandId(m_activeGeneration->at(entityIndex), newIslandId);
      if (newIslandId != oldIslandId)
        m_islandTool->setIslandId(m_activeGeneration->at(entityIndex), oldIslandId);
    }

    invokeEntityChanged(entityIndex);
  }
    
  return res;  
}

void sgpGaOperatorMutateEx::invokeEntityChanged(uint entityIndex)
{
  m_counter.inc("gx-mut-f-chg-entities");
}

