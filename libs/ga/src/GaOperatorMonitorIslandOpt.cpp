/////////////////////////////////////////////////////////////////////////////
// Name:        GaOperatorMonitorIslandOpt.cpp
// Project:     sgpLib
// Purpose:     Operator-monitor for island optimization.
// Author:      Piotr Likus
// Modified by:
// Created:     05/06/2010
/////////////////////////////////////////////////////////////////////////////

//perf
#include "perf/log.h"
#include "perf/timer.h"

//sc
#include "sc/utils.h"

//sgp
#include "sgp/GaStatistics.h"
#include "sgp/GaOperatorMonitorIslandOpt.h"

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

using namespace dtp;
using namespace perf;

const uint MAX_ISLAND_TOP_SIZE = 20;

sgpGaOperatorMonitorIslandOpt::sgpGaOperatorMonitorIslandOpt(): inherited()
{
  m_experimentParams = SC_NULL;
  m_experimentLog = SC_NULL;
  m_optimizer = SC_NULL;
  m_islandTool = SC_NULL;
  m_islandLimit = 0;
}

sgpGaOperatorMonitorIslandOpt::~sgpGaOperatorMonitorIslandOpt()
{
}

uint sgpGaOperatorMonitorIslandOpt::getIslandTopSize()
{
  return MAX_ISLAND_TOP_SIZE;
}

void sgpGaOperatorMonitorIslandOpt::setExperimentParams(sgpGaExperimentParams *params)
{
  m_experimentParams = params;
}

void sgpGaOperatorMonitorIslandOpt::setOptimizer(scPsoOptimizer *optimizer)
{
  m_optimizer = optimizer;
}

void sgpGaOperatorMonitorIslandOpt::setIslandLimit(uint value)
{
  m_islandLimit = value;
}

void sgpGaOperatorMonitorIslandOpt::setExperimentLog(sgpExperimentLog *value)
{
  m_experimentLog = value;
}  

void sgpGaOperatorMonitorIslandOpt::setRatingObjs(const sgpObjectiveIndexSet &ratingObjs)
{
  m_ratingObjs = ratingObjs;
}

void sgpGaOperatorMonitorIslandOpt::setIslandTool(sgpEntityIslandToolIntf *value)
{
  m_islandTool = value;
}

void sgpGaOperatorMonitorIslandOpt::intExecute(uint stepNo, const sgpGaGeneration &input)
{
#ifdef TRACE_TIME  
  Timer::start(TIMER_ISLAND_OPT);
#endif
#ifdef DEBUG_OPER_OTHER
  Log::addDebug("IslandOpt::intExecute - begin");
#endif

  m_activeStepNo = stepNo;
  if (m_islandLimit > 0) 
  {
    scDataNode islandRating, islandSize;
    uint bestIslandId;
    
    rateIslands(input, islandRating, bestIslandId);

#ifdef DEBUG_OPER_OTHER
    Log::addDebug("IslandOpt::intExecute - islands rated");
#endif

    calcIslandsSize(input, islandSize);

#ifdef DEBUG_OPER_OTHER
    Log::addDebug("IslandOpt::intExecute - size ready");
#endif

    if ((m_optimizer != SC_NULL) && !islandRating.empty())
      optimizeParams(islandRating, dynamic_cast<sgpGaExperimentParamsStored *>(m_experimentParams));
  
#ifdef DEBUG_OPER_OTHER
    Log::addDebug("IslandOpt::intExecute - islands optimized");
#endif

    uint bestIslandIdByBestItem = getBestIslandIdByBestItem();
    logIslandStatsToFile(stepNo, bestIslandId, bestIslandIdByBestItem, islandRating);
    logIslandParamsToFile(stepNo, islandRating, islandSize);
  }

#ifdef DEBUG_OPER_OTHER
  Log::addDebug("IslandOpt::intExecute - end");
#endif

#ifdef TRACE_TIME  
  Timer::stop(TIMER_ISLAND_OPT);
#endif
}

uint sgpGaOperatorMonitorIslandOpt::getBestIslandIdByBestItem()
{
  return 0;
}

// rate islands using AVG(for each error obj: avg(pos-in-top-by-obj))
void sgpGaOperatorMonitorIslandOpt::rateIslands(const sgpGaGeneration &input, scDataNode &islandRating,  
  uint &bestIslandId)
{
  sgpEntityIndexList topList;
  scDataNode islandSize;
  
  islandRating.clear();
  
  for(uint i=0, epos = m_islandLimit; i != epos; i++)
  {
    islandRating.addChild(toString(i), new scDataNode(0.0));
  }  

  for(sgpObjectiveIndexSet::const_iterator it = m_ratingObjs.begin(), epos = m_ratingObjs.end(); it != epos; ++it)
  { 
    addIslandRatingForObjective(islandRating, input, *it);
  }
  
  double rating;
  double bestRating = 0.0;
  uint bestIslandPos = islandRating.size();
  // find best island
  for(uint i=0, epos = islandRating.size(); i != epos; i++) { 
    rating = islandRating.getDouble(i);
    if ((i == 0) || (rating > bestRating)) {
      bestIslandPos = i;
      bestRating = rating;
    } 
  }      
  
  if (bestIslandPos < islandRating.size()) {
    bestIslandId = stringToInt(islandRating.getElementName(bestIslandPos));
  }  
  else
    bestIslandId = m_islandLimit;  
}

void sgpGaOperatorMonitorIslandOpt::calcIslandsSize(const sgpGaGeneration &input, scDataNode &islandSize)
{
  islandSize.clear();

  for(uint i=0, epos = m_islandLimit; i != epos; i++)
  {
    islandSize.addChild(new scDataNode(toString(i), 0));
  }  

  sgpEntityBase *entity;
  scString islandName;
  uint islandId;

  for(uint i=0, epos = input.size(); i != epos; i++)  
  {
    entity = &(const_cast<sgpEntityBase &>(input[i]));
    if (m_islandTool->getIslandId(*entity, islandId))
    {
      islandName = toString(islandId);   

      islandSize.setUInt(islandName, islandSize.getUInt(islandName, 0) + 1);
    }
  }    
}

// calculate avg pos in top for a given objective for each island, add result to existing island rating
void sgpGaOperatorMonitorIslandOpt::addIslandRatingForObjective(scDataNode &islandRating, const sgpGaGeneration &input, uint objectiveIndex)
{
  const double DIV_HELPER = 1.0;
  scDataNode topIslandSum, topIslandSize;
  scDataNode topIslandIds;
  
  getTopGenomesByObjective(input, objectiveIndex, topIslandIds);
    
  // add positions & calc number of items per each island
  calcTopIslandStats(input, topIslandIds, topIslandSum, topIslandSize);
  double objRating;
  scString islandName;

  std::auto_ptr<scDataNode> rowGuard;
  
  // update total rating
  for(uint i=0, epos = topIslandSum.size(); i != epos; i++)
  {
    islandName = topIslandSum.getElementName(i);
    // island sum contains positions, 0 - best, 19 - worst, so we need to invert it
    // 0 -> 1
    // 1 -> 1/2
    // 2 -> 1/3    
    objRating = 1.0/(DIV_HELPER + (topIslandSum.getDouble(i) / static_cast<double>(topIslandSize.getUInt(islandName))));
    islandRating.setDouble(islandName, islandRating.getDouble(islandName) + objRating);

    rowGuard.reset(new scDataNode);
    rowGuard->setAsParent();    
    rowGuard->addChild("step", new scDataNode(m_activeStepNo));
    rowGuard->addChild("obj-idx", new scDataNode(objectiveIndex));
    rowGuard->addChild("island", new scDataNode(islandName));
    rowGuard->addChild("sum", new scDataNode(topIslandSum.getDouble(i)));
    rowGuard->addChild("size", new scDataNode(topIslandSize.getUInt(islandName)));     
    rowGuard->addChild("obj-rating", new scDataNode(objRating));     
    rowGuard->addChild("curr-total-rating", new scDataNode(islandRating.getDouble(islandName)));     
    m_experimentLog->addLineToCsvFile(*rowGuard, "rating_stats", "csv");
  }
}

void sgpGaOperatorMonitorIslandOpt::getTopGenomesByObjective(const sgpGaGeneration &input, uint objectiveIndex, 
  scDataNode &topIslandIds)
{
  sgpEntityIndexList topIdList;
  sgpGaEvolver::getTopGenomesByObjective(input, MAX_ISLAND_TOP_SIZE, objectiveIndex, topIdList); 
  prepareIslandIds(input, topIdList, topIslandIds);
}  

void sgpGaOperatorMonitorIslandOpt::prepareIslandIds(const sgpGaGeneration &input, const sgpEntityIndexList &topIdList, 
  scDataNode &topIslandIds)
{
  sgpEntityBase *entity; 
  uint islandId;

  topIslandIds.setAsArray(vt_uint);
  for(uint i=0, epos = topIdList.size(); i != epos; i++)  
  {
    entity = &(const_cast<sgpEntityBase &>(input[topIdList[i]]));
          
    if (!m_islandTool->getIslandId(*entity, islandId))
      islandId = m_islandLimit;

    topIslandIds.addItem(scDataNode(islandId));  
  }  
}

void sgpGaOperatorMonitorIslandOpt::calcTopIslandStats(const sgpGaGeneration &input, 
  const scDataNode &topIslandIds,
  scDataNode &topIslandSum, scDataNode &topIslandSize)
{  
  scString islandName;
  uint islandId, islandPos;

  for(uint i=0, epos = topIslandIds.size(); i != epos; i++)  
  {
    if (topIslandIds.size() > 0) {
      islandId = topIslandIds.getElement(i).getUInt(0);    
      islandPos = topIslandIds.getElement(i).getUInt(1);    
    } else {
      islandId = topIslandIds.getUInt(i);    
      islandPos = i;
    }  
    islandName = toString(islandId);  

    if (!topIslandSum.hasChild(islandName))
    {
      topIslandSum.addChild(islandName, new scDataNode(0.0));
      topIslandSize.addChild(islandName, new scDataNode((uint)0));        
    }

    topIslandSum.setDouble(islandName, topIslandSum.getDouble(islandName) + static_cast<double>(islandPos));
    topIslandSize.setUInt(islandName, topIslandSize.getUInt(islandName) + 1);
  }  
}

void sgpGaOperatorMonitorIslandOpt::optimizeParams(const scDataNode &islandRating, sgpGaExperimentParamsStored *params)
{
  scDataNode paramsData;
  //params->getDynamicParamList(paramsData);
  // we need all params to be read:
  params->getParamList(paramsData);
  m_optimizer->execute(islandRating, paramsData);
  // but only dynamic will be updated:
  params->setDynamicParamList(paramsData);
}

void sgpGaOperatorMonitorIslandOpt::logIslandStatsToFile(uint stepNo, uint topBestIslandId, uint objBestIslandId, 
  const scDataNode &islandRating)
{
  scDataNode params;
  m_experimentParams->getParamList(params);    
 
  scDataNode statsLog; 
  statsLog.addChild("step-no", new scDataNode(stepNo));
  statsLog.addChild("best-by-top-id", new scDataNode(topBestIslandId));
  statsLog.addChild("best-by-obj-id", new scDataNode(objBestIslandId));
  if (params.size()) 
  {
    uint paramCount = params[0].size();
    uint islandCount = params.size();
    scDataNode stats;
    
    double minValue, maxValue, sumValue, paramValue;
    for(uint i=0; i != paramCount; i++)
    {
      minValue = maxValue = sumValue = 0.0; 
      for(uint j=0; j != islandCount; j++)
      {
        paramValue = params[j].getDouble(i);
        if(j == 0) {
          minValue = maxValue = sumValue = paramValue;
        } else {
          if (paramValue < minValue)
            minValue = paramValue;
          else if (paramValue > maxValue)
            maxValue = paramValue;
          sumValue += paramValue;    
        }  
      }
      
      stats.addChild(scString("min-param-")+toString(i), new scDataNode(minValue));
      stats.addChild(scString("max-param-")+toString(i), new scDataNode(maxValue));
      stats.addChild(scString("avg-param-")+toString(i), new scDataNode(sumValue / static_cast<double>(islandCount)));
    } 
      
    for(uint i=0; i != paramCount; i++)
      statsLog.addChild(scString("min-p")+toString(i)+"-"+params[0].getElementName(i), new scDataNode(stats[scString("min-param-")+toString(i)]));
    for(uint i=0; i != paramCount; i++)
      statsLog.addChild(scString("max-p")+toString(i)+"-"+params[0].getElementName(i), new scDataNode(stats[scString("max-param-")+toString(i)]));
    for(uint i=0; i != paramCount; i++)
      statsLog.addChild(scString("avg-p")+toString(i)+"-"+params[0].getElementName(i), new scDataNode(stats[scString("avg-param-")+toString(i)]));
  }

  assert(m_experimentLog != SC_NULL);
  m_experimentLog->addLineToCsvFile(statsLog, "island_stats", "csv");
}

void sgpGaOperatorMonitorIslandOpt::logIslandParamsToFile(uint stepNo, const scDataNode &islandRating, const scDataNode &islandSize)
{
  scDataNode params;
  m_experimentParams->getParamList(params);    
 
  scDataNode logLine; 
  
  scString islandName;
  scDataNode logLineList;
  
  for(uint i=0, epos = islandRating.size(); i != epos; i++)
  {
    logLine.clear();
    logLine.addChild("step-no", new scDataNode(stepNo));
    islandName = islandRating.getElementName(i);
    logLine.addChild("island", new scDataNode(islandName));
    logLine.addChild("rating", new scDataNode(islandRating.getDouble(islandName)));
    logLine.addChild("pop-size", new scDataNode(islandSize.getUInt(islandName)));
    
    if (params.size()) 
    {
      uint paramCount = params[i].size();      
      for(uint j=0; j != paramCount; j++)
      {
        logLine.addChild(
          scString("p")+toString(j)+"-"+params[i].getElementName(j), 
          new scDataNode(
            params[i].getString(j)
          )
        );
      } 
    }
    logLineList.addChild(new scDataNode(logLine));
  }
  assert(m_experimentLog != SC_NULL);
  m_experimentLog->addLineListToCsvFile(logLineList, "island_params", "csv");
}

