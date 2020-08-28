/////////////////////////////////////////////////////////////////////////////
// Name:        GaOperatorBasic.cpp
// Project:     scLib
// Purpose:     Basic operators for GA algorithm
// Author:      Piotr Likus
// Modified by:
// Created:     22/02/2009
/////////////////////////////////////////////////////////////////////////////
#include "sc/defs.h"

//std
#include <set>
#include <cmath>

//base
#include "base/rand.h"

//perf
#include "perf/Counter.h"
#include "perf/Timer.h"
#include "perf/Log.h"

//sc
#include "sc/defs.h"
#include "sc/utils.h"

//sgp
#include "sgp/GaOperatorBasic.h"
#include "sgp\GaStatistics.h"

#ifdef TRACE_ENTITY_BIO
#include "sgp\GpEntityTracer.h"
#endif

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

//#define GAOPER_TRACE
//#define DEBUG_OPER_EVAL

using namespace dtp;
using namespace perf;

// ----------------------------------------------------------------------------
// sgpGaOperatorInitGen
// ----------------------------------------------------------------------------
sgpGaOperatorInitGen::sgpGaOperatorInitGen(): inherited(), m_operInitEntity(SC_NULL)
{
}

sgpGaOperatorInitGen::~sgpGaOperatorInitGen()
{
}

void sgpGaOperatorInitGen::setOperatorInitEntity(sgpGaOperatorInitEntity *value)
{
  m_operInitEntity = value;
}

sgpGaOperatorInitEntity *sgpGaOperatorInitGen::getOperatorInitEntity()
{
  return m_operInitEntity;
}

void sgpGaOperatorInitGen::setMetaInfo(const sgpGaGenomeMetaList &list)
{
  inherited::setMetaInfo(list);
  if (m_operInitEntity != SC_NULL) 
    m_operInitEntity->setMetaInfo(list);
}

void sgpGaOperatorInitGen::execute(uint limit, sgpGaGeneration &newGeneration)
{
  sgpGaOperatorInitEntity *operInitEntity = getOperatorInitEntity();

  std::auto_ptr<sgpEntityBase> infoGuard;

  for(uint i=0; i != limit; i++) {
    infoGuard.reset(newGeneration.newItem());
    operInitEntity->buildRandomEntity(*infoGuard);
    invokeEntityReady(infoGuard.get());
#ifdef TRACE_ENTITY_BIO
    sgpEntityTracer::handleEntityBorn(newGeneration.size(), infoGuard.get(), "init");
#endif    
    newGeneration.insert(infoGuard.release());
  }  
}

// ----------------------------------------------------------------------------
// sgpGaOperatorMutateBasic
// ----------------------------------------------------------------------------
sgpGaOperatorMutateBasic::sgpGaOperatorMutateBasic()
{
  m_probability = SGP_GA_DEF_OPER_PROB_MUTATE;
}
  
sgpGaOperatorMutateBasic::~sgpGaOperatorMutateBasic()
{
}

double sgpGaOperatorMutateBasic::getProbability()
{
  return m_probability;
}

double sgpGaOperatorMutateBasic::getEntityChangeProb()
{
  return m_probability;
}

uint sgpGaOperatorMutateBasic::getChangePointLimit()
{
  return 1;
}

void sgpGaOperatorMutateBasic::setProbability(double aValue)
{
  m_probability = aValue;
}  
  
void sgpGaOperatorMutateBasic::setMetaInfo(const sgpGaGenomeMetaList &list)
{
  sgpGaOperatorMutate::setMetaInfo(list);
  uint genomeSize = 0;
  
  for(sgpGaGenomeMetaList::const_iterator it =list.begin(), epos = list.end(); it != epos; it++)
  {
    genomeSize += it->genSize;
  }  
  m_genomeSize = genomeSize;
}

void sgpGaOperatorMutateBasic::beforeExecute(sgpGaGeneration &newGeneration)
{
#ifdef _DEBUG
  // this operator works only with single-genome entities
  if (newGeneration.size() > 0) {  
    assert(newGeneration.at(0).getGenomeCount() == 1);
  }
#endif
}

void sgpGaOperatorMutateBasic::execute(sgpGaGeneration &newGeneration)
{
  sgpGaGenome genome;

  beforeExecute(newGeneration);

  for(int i = newGeneration.beginPos(), epos = newGeneration.endPos(); i != epos; i++)
  {
    newGeneration.at(i).getGenome(0, genome);
    processGenome(i, genome);
    newGeneration.at(i).setGenome(0, genome);
    invokeNextEntity();
  }  
}

bool sgpGaOperatorMutateBasic::processGenome(uint entityIndex, sgpGaGenome &genome)
{
  double p;
  bool res = false;
  uint pointCountLimit = getChangePointLimit();
  pointCountLimit = SC_MAX(1u, pointCountLimit);
  uint pointCount = randomUInt(1, pointCountLimit);
  
  while(pointCount--) {
    p = randomDouble(0.0, 1.0);
    if (p < getEntityChangeProb()) {
      uint mutPoint = randomInt(0, m_genomeSize - 1);
      uint curOffset = 0;
      uint mutVar;
      scDataNode element;
    
      for(sgpGaGenomeMetaList::const_iterator it = m_meta.begin(), epos = m_meta.end(); it != epos; it++)
      {
        if ((mutPoint < curOffset + it->genSize) && (it->genSize > 0))
        {
          mutVar = it - m_meta.begin();
          element = genome[mutVar];
          mutateVar(*it, mutPoint-curOffset, element);
          genome[mutVar] = element;
          break;
        } else {
          curOffset += it->genSize;
        } 
      } // for
    
      res = true;
    }
  }
  return res;
}  

inline void sgpGaOperatorMutateBasic::mutateVar(const sgpGaGenomeMetaInfo &metaInfo, uint offset, scDataNode &var)
{
  switch (metaInfo.genType) {
    case gagtConst:
      break;
    case gagtRanged: {
      dnValueType vt = var.getValueType();
      switch (vt) {
      case vt_int:
      case vt_byte:
      case vt_uint: {
        uint range = metaInfo.maxValue.getAsUInt() - metaInfo.minValue.getAsUInt();
        uint bitCnt = getActiveBitSize(range);
        uint bitNo = randomUInt(0, bitCnt - 1);
        uint bitMask;
        if (bitNo > 0)
          bitMask = 1 << bitNo;
        else
          bitMask = 1;
        uint rawValue = var.getAsUInt() - metaInfo.minValue.getAsUInt();
        rawValue = rawValue ^ bitMask;
        var.setAsUInt(rawValue);
        break;
      }
      case vt_int64:
      case vt_uint64: {
        uint64 range = metaInfo.maxValue.getAsUInt64() - metaInfo.minValue.getAsUInt64();
        uint bitCnt = getActiveBitSize(range);
        uint bitNo = randomUInt(0, bitCnt - 1);
        uint64 bitMask;
        if (bitNo > 0)
          bitMask = 1 << bitNo;
        else
          bitMask = 1;
        uint64 rawValue = var.getAsUInt64() - metaInfo.minValue.getAsUInt64();
        rawValue = rawValue ^ bitMask;
        var.setAsUInt64(rawValue);
        break;
      }
      case vt_float:
      case vt_double:
      case vt_xdouble: {
          sgpGaOperatorInit::getRandomRanged(var, metaInfo.minValue, metaInfo.maxValue, var);
          break;
      }
      default:
        throw scError("Unknown var type for mutate/range: "+toString(vt));
    } // switch vt 
      break;
    } // case
    case gagtAlphaString: {
      scString value = var.getAsString();
      scString newChar;
      if (offset > value.length())
        throw scError("Invalid string offset: "+toString(offset));
      randomString(metaInfo.minValue.getAsString(), 1, newChar);  
      value[offset] = newChar[0];      
      var.setAsString(value);
      break;
    }  
    default:
      throw scError("Unknown var type for mutate: "+toString(metaInfo.genType));
  }
}

// ----------------------------------------------------------------------------
// sgpGaOperatorMutateWithYield
// ----------------------------------------------------------------------------
sgpGaOperatorMutateWithYield::sgpGaOperatorMutateWithYield(): inherited()
{
  m_yieldSignal = SC_NULL;
}

sgpGaOperatorMutateWithYield::~sgpGaOperatorMutateWithYield()
{
}

void sgpGaOperatorMutateWithYield::setYieldSignal(scSignal *value)
{
  m_yieldSignal = value;
}

void sgpGaOperatorMutateWithYield::invokeNextEntity()
{
  if (m_yieldSignal != SC_NULL)
    m_yieldSignal->execute();
}


// ----------------------------------------------------------------------------
// sgpGaOperatorEvaluateBasic
// ----------------------------------------------------------------------------
void sgpGaOperatorEvaluateBasic::setFitnessFunc(sgpFitnessFunction *value)
{
  m_fitnessFunc = value;
}

void sgpGaOperatorEvaluateBasic::setOperatorMonitor(sgpGaOperatorEvalMonitorIntf *value)
{
  m_operatorMonitor = value;
}

bool sgpGaOperatorEvaluateBasic::execute(uint stepNo, bool isNewGen, sgpGaGeneration &generation)
{
#ifdef DEBUG_OPER_EVAL
  Log::addDebug("ga-oper-eval-a0");  
#endif  

  bool res = false;
  
  if (m_fitnessFunc == SC_NULL) 
    return res;

  if (m_operatorMonitor != SC_NULL)  
    m_operatorMonitor->handleBeforeEvaluate(stepNo, generation);

  sgpFitnessFunctionEx *fitEx = dynamic_cast<sgpFitnessFunctionEx *>(m_fitnessFunc);
  if (fitEx)
    fitEx->initProcess(generation);    

#ifdef DEBUG_OPER_EVAL
  Log::addDebug("ga-oper-eval-a1");  
#endif  
    
  res = evaluateAll(&generation);

#ifdef DEBUG_OPER_EVAL
  Log::addDebug(scString("ga-oper-eval-a2"));  
#endif  
    
  if (m_operatorMonitor != SC_NULL)  
    m_operatorMonitor->handleBeforeEvaluatePostProc(stepNo, generation);

#ifdef DEBUG_OPER_EVAL
  Log::addDebug("ga-oper-eval-a3");  
#endif  

  if (fitEx)
    if (!fitEx->postProcess(generation))
      res = false;
  
#ifdef DEBUG_OPER_EVAL
  Log::addDebug("ga-oper-eval-a4");  
#endif  

  if (m_operatorMonitor != SC_NULL)  
    m_operatorMonitor->handleAfterEvaluate(stepNo, generation);

#ifdef DEBUG_OPER_EVAL
  Log::addDebug("ga-oper-eval-a5");  
#endif  

  return res;
}

bool sgpGaOperatorEvaluateBasic::evaluateAll(sgpGaGeneration *generation)
{
  bool res;

  res = evaluateRange(generation, 0, generation->size() - 1);
  
  return res;
}

bool sgpGaOperatorEvaluateBasic::evaluateRange(sgpGaGeneration *generation, int first, int last)
{
  bool res = true;
  bool evalRes;
  sgpFitnessValue fitValueVector;

  for(int i = first; i <= last; i++)
  {
    evalRes = m_fitnessFunc->calc(i, &(generation->at(i)), fitValueVector);
    generation->at(i).setFitness(fitValueVector);

    invokeNextEntity();
    res = res && evalRes;
  }
  Counter::inc(COUNTER_EVAL, last - first + 1);
  return res;
}

// ----------------------------------------------------------------------------
// sgpGaOperatorEvaluateWithYield
// ----------------------------------------------------------------------------
sgpGaOperatorEvaluateWithYield::sgpGaOperatorEvaluateWithYield(): inherited()
{
  m_yieldSignal = SC_NULL;
}

sgpGaOperatorEvaluateWithYield::~sgpGaOperatorEvaluateWithYield()
{
}

void sgpGaOperatorEvaluateWithYield::setYieldSignal(scSignal *value)
{
  m_yieldSignal = value; 
}

void sgpGaOperatorEvaluateWithYield::invokeNextEntity()
{
  if (m_yieldSignal != SC_NULL)
    m_yieldSignal->execute();
}

// ----------------------------------------------------------------------------
// sgpGaOperatorSelectBasic
// ----------------------------------------------------------------------------
void sgpGaOperatorSelectBasic::execute(sgpGaGeneration &input, sgpGaGeneration &output, uint limit)
{
  if ((input.size() == 0) || (limit == 0))
    return;

  double fitSum = 0.0;
  double fitMin = 0.0;
  double p;
  double fit;

  fitMin = input.at(0).getFitness();
  
  // calculate total fitness sum and min value
  for(int i = input.beginPos(), epos = input.endPos(); i != epos; i++)
  {
    fit = input.at(i).getFitness();
    fitSum += fit;
    if (fit < fitMin)
      fitMin = fit;
  }

  // select new generation  
  uint j;
  uint eposj = input.size() - 1;

  std::vector<double> partSumArr(eposj+1);
  partSumArr[0] = input.at(0).getFitness();
  j = 0; 
  while(j < eposj) {
    j++; 
    partSumArr[j] = partSumArr[j-1] + input.at(j).getFitness();
  } // while j

  for(uint i=0,epos=limit; i != epos; i++)
  { 
    p = randomDouble(0.0, 1.0) * fitSum;
    j = 0; 
    while(j != eposj) {
      if (p <= partSumArr[j]) {
        output.insert(input.cloneItem(j));
#ifdef TRACE_ENTITY_BIO
    sgpEntityTracer::handleEntityMoved(j, output.size() - 1, "select");
#endif                             
        break;
      } else {
        j++; 
      } // if p      
    } // while j
    if (j == eposj) {
      output.insert(input.cloneItem(eposj));    
#ifdef TRACE_ENTITY_BIO
    sgpEntityTracer::handleEntityMoved(eposj, output.size() - 1, "select");
#endif                             
    }  
  } // for i              
}

// ----------------------------------------------------------------------------
// sgpGaOperatorSelectPrec
// ----------------------------------------------------------------------------
// precise selection, based on local fitness range
void sgpGaOperatorSelectPrec::execute(sgpGaGeneration &input, sgpGaGeneration &output, uint limit)
{
  if ((input.size() == 0) || (limit == 0))
    return;

  const double newRangeMin = 0.01;
  const double newRangeMax = 0.99;  
  const double newRangeDiff = newRangeMax - newRangeMin;
  
  double fitSum = 0.0;
  double fitMin, fitMax, newFit;
  double p;
  double fit;
  
  std::vector<double> rfit;

  fitMin = fitMax = input.at(0).getFitness();
  
  // calculate total fitness sum and min value
  for(int i = input.beginPos(), epos = input.endPos(); i != epos; i++)
  {
    fit = input.at(i).getFitness();
    fitSum += fit;
    if (fit < fitMin) {
      fitMin = fit;
    } else if (fit > fitMax) {
      fitMax = fit;
    }      
  }

  if (!equDouble(fitMax, fitMin)) {
    fitSum = 0.0;
    for(int i = input.beginPos(), epos = input.endPos(); i != epos; i++)
    {
      newFit = (
        (
          ((input.at(i).getFitness() - fitMin) / (fitMax - fitMin))
          *
          newRangeDiff
        )
        +
        newRangeMin
     );   
     
     rfit.push_back(newFit);
     fitSum += newFit;  
    }     
  } else {
    for(int i = input.beginPos(), epos = input.endPos(); i != epos; i++)
    {
      rfit.push_back(input.at(i).getFitness());
    }       
  }  
      
  // select new generation
  uint j;
  uint eposj = input.size() - 1;

  std::vector<double> partSumArr(eposj+1);
  partSumArr[0] = rfit[0];
  j = 0; 
  while(j < eposj) {
    j++; 
    partSumArr[j] = partSumArr[j-1] + rfit[j];
  } // while j

  for(uint i=0,epos=limit; i != epos; i++)
  { 
    p = randomDouble(0.0, 1.0) * fitSum;
    j = 0; 
    while(j != eposj) {
      if (p <= partSumArr[j]) {
        output.insert(input.cloneItem(j));
#ifdef TRACE_ENTITY_BIO
    sgpEntityTracer::handleEntityMoved(j, output.size() - 1, "sel-prec");
#endif                             
        break;
      } else {
        j++; 
      } // if p      
    } // while j
    if (j == eposj) {
      output.insert(input.cloneItem(eposj));    
#ifdef TRACE_ENTITY_BIO
    sgpEntityTracer::handleEntityMoved(eposj, output.size() - 1, "sel-prec");
#endif                             
    }  
  } // for i            
}

// ----------------------------------------------------------------------------
// sgpGaOperatorSelectTournament
// ----------------------------------------------------------------------------
sgpGaOperatorSelectTournament::sgpGaOperatorSelectTournament(): sgpGaOperatorSelect()
{
  setTournamentSize(SGP_GA_DEF_TOURNAMENT_SIZE);
  setTemperature(0.0);
}

uint sgpGaOperatorSelectTournament::getTournamentSize()
{
  return m_tournamentSize;
}

void sgpGaOperatorSelectTournament::setTournamentSize(uint value)
{
  m_tournamentSize = value;
}

void sgpGaOperatorSelectTournament::setTemperature(double value)
{
  m_temperature = value;
}

void sgpGaOperatorSelectTournament::execute(sgpGaGeneration &input, sgpGaGeneration &output, uint limit)
{
  std::set<uint> group;
  uint tourSize = std::min<uint>(input.size(), m_tournamentSize);
  uint idx, maxIdx;
  double maxFit, fit;
    
  for(uint i=0,epos=limit; i != epos; i++)
  { 
    group.clear();
    while(group.size() < tourSize) {
      idx = randomInt(0, input.size() - 1);
      if (group.find(idx) == group.end()) 
        group.insert(idx);
    }
    maxIdx = *group.begin();
    maxFit = input[*group.begin()].getFitness();
    for(std::set<uint>::const_iterator it=group.begin(),epos=group.end(); it != epos; ++it)
    { 
      fit = input[*it].getFitness();
      if (fit > maxFit) {
        maxIdx = *it;
        maxFit = fit;
      }
    }

    if ((m_temperature > 0.0) && randomFlip(m_temperature)) {
    // select random value, not best
      group.erase(group.find(maxIdx));
      uint setIdx = randomInt(0, group.size()-1);
      std::set<uint>::const_iterator it = group.begin();
      while(setIdx > 0) {
        ++it;
        setIdx--;
      }  
      maxIdx = *it;  
    } 
    
    output.insert(input.cloneItem(maxIdx));
#ifdef TRACE_ENTITY_BIO
    sgpEntityTracer::handleEntityMoved(maxIdx, output.size() - 1, "tour");
#endif                             
  } // for i            
}

void sgpGaOperatorSelectTournament::genRandomGroup(sgpTournamentGroup &output, const sgpGaGeneration &input, uint limit)
{
  output.clear();
  while(output.size() < limit) 
    output.insert(randomInt(0, input.size() - 1));
}

void sgpGaOperatorSelectTournament::getRandomElement(uint &output, const sgpTournamentGroup &group)
{
  uint setIdx = randomInt(0, group.size()-1);
  sgpTournamentGroup::const_iterator it = group.begin();
  while(setIdx > 0) {
    ++it;
    setIdx--;
  }  
  output = *it;  
}

// ----------------------------------------------------------------------------
// sgpGaOperatorSelectTournamentMF
// ----------------------------------------------------------------------------
sgpGaOperatorSelectTournamentMF::sgpGaOperatorSelectTournamentMF(): sgpGaOperatorSelectTournament()
{ 
}

sgpGaOperatorSelectTournamentMF::~sgpGaOperatorSelectTournamentMF()
{
}

void sgpGaOperatorSelectTournamentMF::setObjectiveWeights(const sgpWeightVector &value)
{
  m_objectiveWeights = value;
}

void sgpGaOperatorSelectTournamentMF::execute(sgpGaGeneration &input, sgpGaGeneration &output, uint limit)
{
  sgpTournamentGroup group;
  uint tourSize = std::min<uint>(input.size(), m_tournamentSize);
  uint maxIdx;
    
  for(uint i=0,epos=limit; i != epos; i++)
  { 
    // generate random group of selected size
    genRandomGroup(group, input, tourSize);
    
    // find best entity in group
    maxIdx = findBestInGroup(input, group, m_objectiveWeights);
    
    if ((m_temperature > 0.0) && randomFlip(m_temperature)) 
    {    
    // select random value, not best
      group.erase(group.find(maxIdx));
      getRandomElement(maxIdx, group);     
    } 
    
    output.insert(input.cloneItem(maxIdx));
#ifdef TRACE_ENTITY_BIO
    sgpEntityTracer::handleEntityMoved(maxIdx, output.size() - 1, "tour-mf");
#endif                             
  } // for i            
}

// Returns index of best entity. 
// Uses objective weights to search first only using most important objectives.
// Algorithm:
// a) find lowest weight value
// b) find best entity(-ies) using only objectivies with the selected weight value
// c) find next weight - minimal, > last value
// d) if work group size > 1 go to (b)
uint sgpGaOperatorSelectTournamentMF::findBestInGroup(const sgpGaGeneration &input, const sgpTournamentGroup &group, const sgpWeightVector &weights)
{
  double wLevel = 0.0;
  double nextLevel = 0.0;
  bool wLevelIsNull = true;
  bool nextLevelIsNull;
  uint objCnt = weights.size();
  const sgpTournamentGroup *workGroupPtr = &group;
  sgpTournamentGroup workGroupData;
  sgpTournamentGroup newWorkGroup;
  bool match;
  sgpFitnessValue bestValues, fitVector;
  uint bestIndex;
  const uint UNSET_IDX = -1;
  uint lastFound = UNSET_IDX;

  while(workGroupPtr->size() > 1) {
    // find minimal weight value > current wLevel
    nextLevelIsNull = true;
    for(uint i=sgpFitnessValue::SGP_OBJ_OFFSET,epos = objCnt; i!=epos; i++) {
      if (wLevelIsNull || (wLevel < weights[i])) {
        if (nextLevelIsNull || (nextLevel > weights[i])) {
          nextLevel = weights[i];
          nextLevelIsNull = false;
        }  
      }
    }
    
    if (nextLevelIsNull)
      break; // last level processed
      
    // next level found
    wLevel = nextLevel;
    wLevelIsNull = false;
    
    // find element with best set of values in work group
    bestIndex = *workGroupPtr->cbegin();
    bestValues.clear();
    input.at(bestIndex).getFitness(bestValues);
    
    for(sgpTournamentGroup::const_iterator it = workGroupPtr->begin(),epos=workGroupPtr->end(); it != epos; ++it)
    {
      input.at(*it).getFitness(fitVector);
      
      match = true;
      for(uint i=sgpFitnessValue::SGP_OBJ_OFFSET,epos = objCnt; i!=epos; i++) {
        if (weights[i] <= wLevel) {
          if (bestValues[i] > fitVector[i]) {
            match = false;
            break;
          }  
        }
      }
      
      if (match) {
        bestValues = fitVector; 
        bestIndex = *it;
      }
    } // for whole workgroup
    
    // find all items with found bestValues
    newWorkGroup.clear();
    for(sgpTournamentGroup::const_iterator it = workGroupPtr->begin(),epos=workGroupPtr->end(); it != epos; ++it)
    {
      input.at(*it).getFitness(fitVector);
      
      match = true;
      for(uint i=sgpFitnessValue::SGP_OBJ_OFFSET,epos = objCnt; i!=epos; i++) {
        if (weights[i] <= wLevel) {
          if (bestValues[i] > fitVector[i]) {
            match = false;
            break;
          }  
        }    
      }
      
      if (match) {
        newWorkGroup.insert(*it);
        lastFound = *it;
      }
    } // for whole workgroup

    workGroupPtr = &workGroupData;
    workGroupData = newWorkGroup;    
  } // while not one in workgroup

  if (lastFound == UNSET_IDX)
    if (group.size() == 1)
      lastFound = *group.begin();

  //assert(workGroup.size() > 0);
  assert(lastFound < UNSET_IDX);
  return lastFound;  
}

// ----------------------------------------------------------------------------
// sgpGaOperatorEliteBasic
// ----------------------------------------------------------------------------
void sgpGaOperatorEliteBasic::execute(sgpGaGeneration &input, sgpGaGeneration &output, uint limit)
{
  if ((input.size() == 0) || (limit == 0))
    return;
    
  double maxFit, newMaxFit;
  uint maxIdx, newMaxIdx, addedCnt;
  double fit;
  bool found;
  
  maxFit = input.at(0).getFitness();
  maxIdx = 0;
  addedCnt = 0;

  for(uint i = 0, epos = input.size(); i != epos; i++)
  {
    fit = input.at(i).getFitness();
    if (fit > maxFit) {
      maxFit = fit;
      maxIdx = i;
    }
  }
    
  output.insert(output.newItem(input.at(maxIdx)));
  addedCnt++;
    
  while(addedCnt < limit) {
    found = false;

    newMaxFit = input.at(0).getFitness();
    newMaxIdx = 0;
    
    for(uint i = 0, epos = input.size(); i != epos; i++)
    {
      fit = input.at(i).getFitness();
      if (!found || (fit > newMaxFit))
      {
        if (
          (fit < maxFit)
          ||
          (
            equDouble(fit, maxFit)
            &&
            (i < maxIdx)
          )
        )
        {
          newMaxFit = fit;
          newMaxIdx = i;
          found = true;
        }    
      }    
    } // for i
    
    if (found) {
      maxIdx = newMaxIdx;
      maxFit = newMaxFit;      
      output.insert(output.newItem(input.at(maxIdx)));
#ifdef TRACE_ENTITY_BIO
    sgpEntityTracer::handleEntityMoved(maxIdx, output.size() - 1, "elite");
#endif                  
      addedCnt++;    
    } else {
      break;
    }
  } // while    
}

// ----------------------------------------------------------------------------
// sgpGaOperatorEliteByObjective
// ----------------------------------------------------------------------------
void sgpGaOperatorEliteByObjective::setPrimaryObjectiveIndex(uint value)
{
  m_primaryObjectiveIndex = value;
}

void sgpGaOperatorEliteByObjective::execute(sgpGaGeneration &input, sgpGaGeneration &output, uint limit)
{
  if ((input.size() == 0) || (limit == 0))
    return;

  sgpGaGenomeList topList;
  sgpEntityIndexList idList;
  uint addedCnt = 0;

  sgpGaEvolver::getTopGenomesByObjective(input, limit, m_primaryObjectiveIndex, topList, idList);  
  uint entityIndex;

  if (!idList.empty()) {
    while(addedCnt < limit) 
    {
      entityIndex = idList[addedCnt % idList.size()];      
      output.insert(output.newItem(input.at(entityIndex)));
#ifdef TRACE_ENTITY_BIO
    sgpEntityTracer::handleEntityMovedBuf(entityIndex, output.size() - 1, "elite-obj");
#endif            
      addedCnt++;
    }
  }
}

// ----------------------------------------------------------------------------
// sgpGaOperatorEliteByWeights
// ----------------------------------------------------------------------------
sgpGaOperatorEliteByWeights::sgpGaOperatorEliteByWeights(): sgpGaOperatorElite() {
}

void sgpGaOperatorEliteByWeights::setObjectiveWeights(const sgpWeightVector &value)
{
  m_objectiveWeights = value;
}

void sgpGaOperatorEliteByWeights::execute(sgpGaGeneration &input, sgpGaGeneration &output, uint limit)
{
  if ((input.size() == 0) || (limit == 0))
    return;

  sgpGaGenomeList topList;
  sgpEntityIndexList idList;
  uint addedCnt = 0;

  sgpGaEvolver::getTopGenomesByWeights(input, limit, m_objectiveWeights, topList, idList);  
  uint entityIndex;

  if (!idList.empty()) {
    while(addedCnt < limit) 
    {
      entityIndex = idList[addedCnt % idList.size()];
      output.insert(output.newItem(input.at(entityIndex)));
#ifdef TRACE_ENTITY_BIO
    sgpEntityTracer::handleEntityMovedBuf(entityIndex, output.size() - 1, "elite-weights");
#endif      
      addedCnt++;
    }
  }
}

// ----------------------------------------------------------------------------
// sgpGaOperatorXOverBasic
// ----------------------------------------------------------------------------
sgpGaOperatorXOverBasic::sgpGaOperatorXOverBasic()
{
  m_probability = SGP_GA_DEF_OPER_PROB_XOVER;
}

sgpGaOperatorXOverBasic::~sgpGaOperatorXOverBasic()
{
}

void sgpGaOperatorXOverBasic::setProbability(double aValue)
{
  m_probability = aValue;
}  

void sgpGaOperatorXOverBasic::setMetaInfo(const sgpGaGenomeMetaList &list)
{
  sgpGaOperatorXOver::setMetaInfo(list);
  uint genomeSize = 0;
  
  for(sgpGaGenomeMetaList::const_iterator it =list.begin(), epos = list.end(); it != epos; it++)
  {
    genomeSize += it->genSize;
  }  
  m_genomeSize = genomeSize;
}

void sgpGaOperatorXOverBasic::execute(sgpGaGeneration &newGeneration)
{
  if (m_meta.size() > 1) 
    executeWithProb(m_probability, newGeneration);
}

bool sgpGaOperatorXOverBasic::crossGenomes(sgpGaGeneration &newGeneration, uint first, uint second)
{
  uint point, beg, end;
  
  if (m_meta.size() == 2) 
    point = m_meta[0].genSize;
  else 
    point = randomInt(0, m_genomeSize - 1);  
  
  beg = point;
  end = m_genomeSize;

  uint i = 0, epos = m_meta.size();
  uint xpoint = 0;
  uint endPoint;
  
  while((i < epos) && ((xpoint < beg) || (m_meta[i].genSize == 0))) {
    xpoint += m_meta[i].genSize;
    i++;
  }

  // this operator works only with single-genome entities
  assert(newGeneration.at(first).getGenomeCount() == 1);

  sgpGaGenome firstGenome, secondGenome;
  newGeneration.at(first).getGenome(0, firstGenome);
  newGeneration.at(second).getGenome(0, secondGenome);
       
  while((i < epos) && (xpoint < end)) {
    if (xpoint + m_meta[i].genSize >= end)
      endPoint = end - xpoint;
    else 
      endPoint = xpoint + m_meta[i].genSize;    
    if (m_meta[i].genSize > 0)  
      crossVars(firstGenome, secondGenome, i, endPoint);  
    xpoint += m_meta[i].genSize;
    i++;  
  }    

  newGeneration.at(first).setGenome(0, firstGenome);
  newGeneration.at(second).setGenome(0, secondGenome);
  
  return true;
}

void sgpGaOperatorXOverBasic::crossVars(sgpGaGenome &firstGenome, sgpGaGenome &secondGenome, uint genIndex, uint endPoint)
{
   switch (m_meta[genIndex].genType) {
     case gagtRanged: {
       scDataNodeValue tmp;
       tmp.copyFrom(firstGenome[genIndex]); 
       firstGenome[genIndex] = secondGenome[genIndex];
       secondGenome[genIndex] = tmp;
       break;
     }
     case gagtAlphaString: {
       scString firstVal = firstGenome[genIndex].getAsString(); 
       scString secondVal = secondGenome[genIndex].getAsString(); 
       if (endPoint < m_meta[genIndex].genSize) {
         scString tmp1 = firstVal.substr(0, endPoint)+secondVal.substr(endPoint);
         scString tmp2 = secondVal.substr(0, endPoint)+firstVal.substr(endPoint);
         firstGenome[genIndex].setAsString(tmp1); 
         secondGenome[genIndex].setAsString(tmp2); 
       } else {
         firstGenome[genIndex].setAsString(secondVal); 
         secondGenome[genIndex].setAsString(firstVal); 
       }
       break;
     }  
     default:
       break;  
   }
}

// ----------------------------------------------------------------------------
// sgpGaOperatorXOverSpecies
// ----------------------------------------------------------------------------

void sgpGaOperatorXOverSpecies::setMatchThreshold(double aValue) 
{
  m_matchThreshold = aValue;
}  

void sgpGaOperatorXOverSpecies::setCompareTool(sgpGaGenomeCompareTool *tool)
{
  m_compareTool.reset(tool);
}

void sgpGaOperatorXOverSpecies::setMetaInfo(const sgpGaGenomeMetaList &list)
{
  sgpGaOperatorXOverBasic::setMetaInfo(list);   

  uint maxLen = 0;
  
  for(sgpGaGenomeMetaList::const_iterator it = m_meta.begin(), epos = m_meta.end(); it != epos; it++)
  {
    if ((*it).genType == gagtAlphaString) {
      if ((*it).genSize > maxLen)
        maxLen = (*it).genSize;        
    }    
  }  
  checkCompareTool();
  m_compareTool->setMaxLength(maxLen);
  m_compareTool->setMetaInfo(list);
}

void sgpGaOperatorXOverSpecies::init()
{
  checkCompareTool();
}
  
void sgpGaOperatorXOverSpecies::checkCompareTool()
{
  if (m_compareTool.get() == SC_NULL)
    m_compareTool.reset(new sgpGaGenomeCompareToolForGa());  
}

bool sgpGaOperatorXOverSpecies::crossGenomes(sgpGaGeneration &newGeneration, uint first, uint second)
{
  if ((m_genomeSize == 0) || (m_compareTool->calcGenomeDiff(newGeneration, first, second) <= m_matchThreshold))
    return sgpGaOperatorXOverBasic::crossGenomes(newGeneration, first, second);
  else
    return false;
}      


// ----------------------------------------------------------------------------
// sgpGaGenomeCompareToolForGa
// ----------------------------------------------------------------------------
void sgpGaGenomeCompareToolForGa::setMaxLength(uint value)
{
  sgpGaGenomeCompareTool::setMaxLength(value);
  if (value > 0)
    m_diffFunct.reset(new strDiffFunctor(value));
  else  
    m_diffFunct.reset(); 
}  
  
void sgpGaGenomeCompareToolForGa::setMetaInfo(const sgpGaGenomeMetaList &list)
{
  uint genomeSize = 0;
  
  for(sgpGaGenomeMetaList::const_iterator it =list.begin(), epos = list.end(); it != epos; it++)
  {
    genomeSize += it->genSize;
  }  
  m_genomeSize = genomeSize;
}
  
// returns value between 0.0 and 1.0 
double sgpGaGenomeCompareToolForGa::calcGenomeDiff(const sgpGaGeneration &newGeneration, uint first, uint second)
{
  double res;
  uint mutVar;
  
  res = 0.0;
  scDataNode var1, var2;

  // this function works only with single-genome entities
  assert(newGeneration.at(first).getGenomeCount() == 1);

  sgpGaGenome genome1;
  sgpGaGenome genome2;
  
  newGeneration.at(first).getGenome(0, genome1);
  newGeneration.at(second).getGenome(0, genome2);  

  for(sgpGaGenomeMetaList::const_iterator it = m_meta.begin(), epos = m_meta.end(); it != epos; it++)
  {
    mutVar = it - m_meta.begin();

    switch ((*it).genType) {
      case gagtConst:
        break;
      case gagtRanged: {
        var1 = genome1[mutVar];
        var2 = genome2[mutVar];
        
        res += 
          (
            calcRangedDiff(var1, var2, (*it).minValue, (*it).maxValue)
            *
            (*it).genSize
            /
            m_genomeSize
          );
          
        break;
      }  
      case gagtAlphaString: {
        scString str1 = genome1[mutVar].getAsString();
        scString str2 = genome2[mutVar].getAsString();
        res += 
          (
            m_diffFunct.get()->calc(str1, str2)
            / str1.length()
            *
            (*it).genSize
            /
            m_genomeSize
           );
        break;
      }  
      default:
        throw scError("Unknown var type for mutate: "+toString((*it).genType));
    }
  }
  return res;
}

double sgpGaGenomeCompareToolForGa::calcGenomeDiff(const sgpGaGeneration &newGeneration, uint first, uint second, uint genNo)
{
  return calcGenomeDiff(newGeneration, first, second);
}

double sgpGaGenomeCompareToolForGa::calcRangedDiff(const scDataNode &var1, const scDataNode &var2, const scDataNode &minValue, const scDataNode &maxValue)
{
  double res;
  switch (minValue.getValueType()) {
    case vt_int: {
      int iMinValue = minValue.getAsInt();
      int iMaxValue = maxValue.getAsInt();
      int iRange = iMaxValue - iMinValue;
      int iDiff = var1.getAsInt() - var2.getAsInt();
      if (iDiff < 0) 
        iDiff = -iDiff;
        
      res = iDiff / iRange;
      break;
    }
    case vt_double: {
      double dMinValue = minValue.getAsDouble();
      double dMaxValue = maxValue.getAsDouble();
      double dRange = dMaxValue - dMinValue;
      double dDiff = var1.getAsDouble() - var2.getAsDouble();
      if (dDiff < 0.0) dDiff = -dDiff;

      res = dDiff / dRange;
      break;
    }
    default:
      throw scError("Wrong variable type: "+toString(minValue.getValueType()));
  }
  return res;
}

// select an item index using specified probabilities, handling 0.0 probs is included
uint selectProbItem(const scDataNode &list)
{
  if (list.empty())
    return 0;
    
  double sum = 0.0;

  uint endPos = list.size();

  sum = list.accumulate(0.0);
    
  double p = randomDouble(0.0, 1.0) * sum;
  double rprob = list.getDouble(0);
  uint res = 0;
  endPos--;
  
  while ((p >= rprob) && (res < endPos)) {
    res++;
    rprob += list.getDouble(res);
  }
  
  return res;
}

void selectProbItem(const scDataNode &list, double probSum, std::vector<uint> &output, uint limit)
{
  output.clear();

  if (list.empty())
    return;
    
  uint endPos = list.size();
  endPos--;
    
  double p;
  double rprob, firstProb;
  uint idx; 
  
  firstProb = list.getDouble(0);

  while(limit > 0)
  {
    p = randomDouble(0.0, 1.0) * probSum;
    rprob = firstProb; 
    idx = 0;
  
    while ((p >= rprob) && (idx < endPos)) {
      idx++;
      rprob += list.getDouble(idx);
    }  

    output.push_back(idx);
    --limit;
  }
}
