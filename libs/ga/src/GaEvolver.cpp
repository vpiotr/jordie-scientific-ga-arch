/////////////////////////////////////////////////////////////////////////////
// Name:        GaEvolver.cpp
// Project:     scLib
// Purpose:     Class that finds a better set of values using GA algorithm.
// Author:      Piotr Likus
// Modified by:
// Created:     22/02/2009
/////////////////////////////////////////////////////////////////////////////
#include "Precomp.h"

#include "base/rand.h"
#include "base/bitstr.h"
#include "base/bmath.h"

#include "perf/counter.h"
#include "perf/log.h"

#include "sc/defs.h"
#include "sc/utils.h"

#include "sgp/GaEvolver.h"

#include "sc/ompdefs.h"

#ifdef USE_OPENMP
#include "sc/process.h"
#endif

#ifdef TRACE_ENTITY_BIO
#include "sgp\GpEntityTracer.h"
#endif

#include "sgp\FitnessScanner.h"

#define COUT_ENABLED
//#define DEBUG_EVOLVER

#ifdef COUT_ENABLED
#include <iostream>
using namespace std;
#endif

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

#define OPT_USE_GRAY_CODE

using namespace dtp;
using namespace perf;

// ----------------------------------------------------------------------------
// Functions
// ----------------------------------------------------------------------------
namespace sgp {
  uint decodeBitStrUInt(const scString &value) {

#ifndef OPT_USE_GRAY_CODE
    return bitStringToInt<uint>(value);
#else  
    return bitStringToIntGray<uint>(value);
#endif

  }

  scString encodeBitStrUInt(uint value, uint len) {
#ifndef OPT_USE_GRAY_CODE
    return intToBitString(value, len);
#else  
    return intToBitStringGray(value, len);
#endif
  }

  double decodeBitStrDouble(const scString &value) {
#ifndef OPT_USE_GRAY_CODE
    return bitStringToFp<double>(value);
#else  
    return bitStringToFpGray<double>(value);
#endif
  }

  void encodeBitStrDouble(double value, uint len, scString &output) {
#ifndef OPT_USE_GRAY_CODE
    fpToBitString(value, len, output);
#else  
    fpToBitStringGray(value, len, output);
#endif
  }

};

// ----------------------------------------------------------------------------
// Private classes
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// sgpGaGenomeMetaBuilder
// ----------------------------------------------------------------------------

void sgpGaGenomeMetaBuilder::addMetaRangedInt(int minValue, int maxValue)
{
  sgpGaGenomeMetaInfo info;
  info.genSize = 1;
  info.minValue = scDataNode(minValue);
  info.maxValue = scDataNode(maxValue);
  info.genType = gagtRanged;
  m_meta.push_back(info);
}

void sgpGaGenomeMetaBuilder::addMetaRangedUInt(uint minValue, uint maxValue)
{
  sgpGaGenomeMetaInfo info;
  info.genSize = 1;
  info.minValue = scDataNode(minValue);
  info.maxValue = scDataNode(maxValue);
  info.genType = gagtRanged;
  m_meta.push_back(info);
}

void sgpGaGenomeMetaBuilder::addMetaRangedDouble(double minValue, double maxValue)
{
  sgpGaGenomeMetaInfo info;
  info.genSize = 1;
  info.minValue = scDataNode(minValue);
  info.maxValue = scDataNode(maxValue);
  info.genType = gagtRanged;
  m_meta.push_back(info);
}

void sgpGaGenomeMetaBuilder::addMetaAlphaString(const scString &charSet, uint maxLen)
{
  sgpGaGenomeMetaInfo info;
  info.genSize = maxLen;
  info.minValue = scDataNode(charSet);
  info.genType = gagtAlphaString;
  m_meta.push_back(info);
}

void sgpGaGenomeMetaBuilder::addMetaConst(const scDataNode &value)
{
  sgpGaGenomeMetaInfo info;
  info.genSize = 0;
  info.minValue = value;
  info.maxValue = value;
  info.genType = gagtConst;
  m_meta.push_back(info);
}

void sgpGaGenomeMetaBuilder::addMetaValue(const scDataNode &value)
{
  sgpGaGenomeMetaInfo info;
  info.genSize = 1;
  info.minValue = value;
  info.maxValue = scDataNode(); // no max = not a ranged value
  info.genType = gagtValue;
//DEBUG:+
try {    
//DEBUG:-
  m_meta.push_back(info);
//DEBUG:+
} catch(const std::exception& e) {
  scString msg = scString("main(): Exception (std): ") + e.what();
  msg += ", ctx: oper: addMetaValue, m_meta.size = ["+toString(m_meta.size())+"], value.type = ["+toString(value.getValueType())+"]";
  throw scError(msg);
}    
//DEBUG:-
}

void sgpGaGenomeMetaBuilder::setLastUserType(uint userType)
{
  m_meta[m_meta.size() - 1].userType = userType;
}

void sgpGaGenomeMetaBuilder::setLastSize(uint aSize)
{
  m_meta[m_meta.size() - 1].genSize = aSize;
}

// ----------------------------------------------------------------------------
// sgpGaExperimentParamsStored
// ----------------------------------------------------------------------------
sgpGaExperimentParamsStored::sgpGaExperimentParamsStored(): sgpGaExperimentParams()
{
}

sgpGaExperimentParamsStored::~sgpGaExperimentParamsStored()
{
}

void sgpGaExperimentParamsStored::clear()
{
  m_paramIndexMap.clear();
  m_paramId2NameMap.clear();
  m_params.clear();
  m_meta.clear();
  m_staticParams.clear();
}
  
uint sgpGaExperimentParamsStored::calcCompactParamId(uint groupId, uint paramId) const
{
  return groupId * 1000 + paramId;
}


///-------------------------------------------------------------------------------
void sgpGaExperimentParamsStored::defineParam(const scString &name, uint paramId, const scDataNode &minValue, const scDataNode &maxValue, bool isStatic)
{
  scString groupName = toString(0);
  if (!m_params.hasChild(groupName))
    m_params.addChild(groupName, new scDataNode(ict_parent));

  m_params[groupName].addChild(name, scDataNode());  

  m_paramId2NameMap.insert(ParamId2NameMap::value_type(paramId, name));

  if (!m_meta.isContainer())
    m_meta.setAsParent();

  m_meta.addElement(name, base::move<scDataNode>(
    scDataNode(ict_list).
      addChild(new scDataNode(minValue)).
      addChild(new scDataNode(maxValue))
  ));

  if (isStatic)
    m_staticParams.addChild(name, scDataNode(true));  
}

uint sgpGaExperimentParamsStored::getGroupCount() const
{
  return m_params.size();
}

void sgpGaExperimentParamsStored::getParamList(scDataNode &values) const
{
  scString groupName;
  values.clear();
  for(uint i=0, epos = m_params.size(); i != epos; i++)
  {
    groupName = m_params.getElementName(i);
    values.addChild(groupName, new scDataNode(ict_parent));
    for(uint j=0, eposj = m_params[i].size(); j != eposj; j++)
    {
      values[groupName].addElement(m_params[i].getElementName(j), m_params[i].getElement(j));
    }    
  }  
}

void sgpGaExperimentParamsStored::getDynamicParamList(scDataNode &values) const
{
  scString groupName;
  values.clear();
  for(uint i=0, epos = m_params.size(); i != epos; i++)
  {
    groupName = m_params.getElementName(i);
    values.addChild(groupName, new scDataNode(ict_parent));
    for(uint j=0, eposj = m_params[i].size(); j != eposj; j++)
    {
      if (!m_staticParams.hasChild(m_params[i].getElementName(j)))
        values[groupName].addElement(m_params[i].getElement(j));
    }    
  }  
}

void sgpGaExperimentParamsStored::setParamListValues(const scDataNode &values)
{
  scString groupName;
  
  for(uint i=0, epos = values.size(); i != epos; i++)
  {
    groupName = values.getElementName(i);
    if (m_params.hasChild(groupName))
    { 
      for(uint j=0, eposj = values[groupName].size(); j != eposj; j++)
      {
        assert(m_params[groupName].getElementName(j) == values[groupName].getElementName(j));
        m_params[groupName].setElement(j, values[groupName].getElement(j));
      }
    }
  }  
  //was: m_params = values;
}

void sgpGaExperimentParamsStored::setParamList(const scDataNode &values)
{
  scString groupName, elementName;
  uint paramIdx;
  
  for(uint i=0, epos = values.size(); i != epos; i++)
  {
    groupName = values.getElementName(i);
    if (!m_params.hasChild(groupName))
    {
      m_params.addChild(groupName, new scDataNode(ict_parent));
    }  
    
    for(uint j=0, eposj = values[groupName].size(); j != eposj; j++)
    {
      elementName = values[groupName].getElementName(j);
      if (!m_params[groupName].hasElement(elementName))
      {
        m_params[groupName].addElement(elementName, values[groupName].getElement(j));
      } else {  
        m_params[groupName].setElement(elementName, values[groupName].getElement(j));
      }  
    }
  }  
  //was: m_params = values;
}

/// Set param values, error if param not found
void sgpGaExperimentParamsStored::setParamListCheck(const scDataNode &values)
{
  scString groupName, elementName;
  uint paramIdx;
  
  for(uint i=0, epos = values.size(); i != epos; i++)
  {
    groupName = values.getElementName(i);
    if (!m_params.hasChild(groupName))
      m_params.addChild(groupName, new scDataNode(ict_parent));
    
    for(uint j=0, eposj = values[groupName].size(); j != eposj; j++)
    {
      elementName = values[groupName].getElementName(j);
      assert(m_params[groupName].hasElement(elementName));
      m_params[groupName].setElement(elementName, values[groupName].getElement(j));
    }
  }  
  //was: m_params = values;
}

void sgpGaExperimentParamsStored::setParamListByIndex(const scDataNode &values)
{
  scString groupName;
  
  for(uint i=0, epos = values.size(); i != epos; i++)
  {
    groupName = values.getElementName(i);
    if (!m_params.hasChild(groupName))
      m_params.addChild(groupName, new scDataNode(ict_parent));
    
    assert(m_params[groupName].size() == values[groupName].size());
    for(uint j=0, eposj = values[groupName].size(); j != eposj; j++)
    {
      m_params[groupName].setElementValue(j, values[groupName].getElement(j));
    }
  }  
  //was: m_params = values;
}

void sgpGaExperimentParamsStored::setDynamicParamList(const scDataNode &values)
{
  scString groupName;
  
  for(uint i=0, epos = values.size(); i != epos; i++)
  {
    groupName = values.getElementName(i);
    if (m_params.hasChild(groupName))
    { 
      for(uint j=0, eposj = values[groupName].size(); j != eposj; j++)
      {
        assert(m_params[groupName].getElementName(j) == values[groupName].getElementName(j));
        if (!m_staticParams.hasChild(values[groupName].getElementName(j)))        
        {
          m_params[groupName].setElement(j, values[groupName].getElement(j));
        }  
      }
    }
  }  
}

void sgpGaExperimentParamsStored::getParamMeta(scDataNode &meta) const
{
  meta = m_meta;
}

void sgpGaExperimentParamsStored::getParamValuesForGroup(uint groupId, scDataNode &values) const
{
  scString groupName(toString(groupId));
  values.clear();
  if (m_params.hasChild(groupName))
  {
    const scDataNode &groupContents = m_params[groupName];
    values = groupContents;
  }  
}

bool sgpGaExperimentParamsStored::getParamValueForGroup(uint groupId, uint paramId, scDataNode &value) const
{
  value.clear();

  uint paramIdx;
  scString paramName;
  bool res = getParamName(groupId, paramId, paramName);

  if (res) {
    m_params[toString(groupId)].getElement(paramName, value);
  } 

  return res;
}

void sgpGaExperimentParamsStored::getParamMetaForGroup(uint groupId, scDataNode &meta) const
{
  meta = m_meta;
}


bool sgpGaExperimentParamsStored::isDefined(uint groupId, uint paramId) const
{
  scString name;
  bool res = getParamName(groupId, paramId, name);
  return res;
}

bool sgpGaExperimentParamsStored::getParamName(uint groupId, uint paramId, scString &value) const
{
  ParamId2NameMap ::const_iterator it = m_paramId2NameMap.find(paramId);
  if (it != m_paramId2NameMap.end()) {
    value = it->second;
    return true;
  } else {
    return false;
  }
}

bool sgpGaExperimentParamsStored::getParamIndex(uint groupId, uint paramId, uint &value) const
{
  bool res;
  uint storedParamId = calcCompactParamId(groupId, paramId);
  ParamIndexMap::const_iterator it = m_paramIndexMap.find(storedParamId);
  if (it != m_paramIndexMap.end())
  {
    value = it->second;
    res = true;
  } else {
    scString name;
    if (!getParamName(groupId, paramId, name))
      return false;

    value = m_params[toString(groupId)].indexOfName(name);
    m_paramIndexMap.insert(make_pair(storedParamId, value));
    res = true;
  }

  return res;
}

bool sgpGaExperimentParamsStored::getInt(uint groupId, uint paramId, int &value) const
{
  scString paramName;
  bool res = getParamName(groupId, paramId, paramName);
  if (res) {
    value = m_params[toString(groupId)].getInt(paramName);
  }  
  return res;
}

bool sgpGaExperimentParamsStored::getDouble(uint groupId, uint paramId, double &value) const
{
  scString paramName;
  bool res = getParamName(groupId, paramId, paramName);
  if (res) {
    value = m_params[toString(groupId)].getDouble(paramName);
  }  
  return res;
}

void sgpGaExperimentParamsStored::setInt(uint groupId, uint paramId, int value)
{
  scString paramName;
  bool res = getParamName(groupId, paramId, paramName);
  if (res) {
    m_params[toString(groupId)].setInt(paramName, value);
  }  
}

void sgpGaExperimentParamsStored::setDouble(uint groupId, uint paramId, double value)
{
  scString paramName;
  bool res = getParamName(groupId, paramId, paramName);
  if (res) {
    m_params[toString(groupId)].setDouble(paramName, value);
  }  
}

// ----------------------------------------------------------------------------
// sgpGaOperatorElite
// ----------------------------------------------------------------------------
uint sgpGaOperatorElite::calcEliteLimit(uint eliteMode, double eliteRatio, uint popSize)
{
  uint eliteLimit;
  
  switch (eliteMode) {
    case 0: 
      eliteLimit = 0;
      break;
    case 1:
      eliteLimit = 1;
      break;
    default:  
      eliteLimit = round<uint>(popSize * eliteRatio);
      break;
  }    
  return eliteLimit;
}

// ----------------------------------------------------------------------------
// sgpGaOperatorXOver
// ----------------------------------------------------------------------------
bool sgpGaOperatorXOver::canExecute()
{
  if (m_meta.size() > 1) 
    return true;
  else
    return false;  
}

void sgpGaOperatorXOver::executeWithProb(double aProb, sgpGaGeneration &newGeneration)
{
  double p;
  int parentNo = 0;
  int first = 0;
  
  if (canExecute()) 
  for(int i = newGeneration.beginPos(), epos = newGeneration.endPos(); i != epos; i++)
  {
    p = randomDouble(0.0, 1.0);
    if (p < aProb) {
      if(parentNo % 2 == 1) {
        if (crossGenomes(newGeneration, first, i)) {
          parentNo = 0;
        }  
      } else {
        parentNo++;
        first = i;
      }        
    }
  }
}

// ----------------------------------------------------------------------------
// sgpGaOperatorMonitor
// ----------------------------------------------------------------------------
void sgpGaOperatorMonitor::execute(uint stepNo, const sgpGaGeneration &input)
{
  performBeforeExecute(stepNo, input);
  intExecute(stepNo, input);
  performAfterExecute(stepNo, input);
}

void sgpGaOperatorMonitor::addBeforeStep(sgpGaOperatorMonitor *gaOperator)
{
  m_beforeSteps.push_back(gaOperator);
  m_beforeStepsExt.push_back(gaOperator);
}

void sgpGaOperatorMonitor::addAfterStep(sgpGaOperatorMonitor *gaOperator)
{
  m_afterSteps.push_back(gaOperator);
  m_afterStepsExt.push_back(gaOperator);
}

void sgpGaOperatorMonitor::addBeforeStepExt(sgpGaOperatorMonitor *gaOperator)
{
  m_beforeStepsExt.push_back(gaOperator);
}

void sgpGaOperatorMonitor::addAfterStepExt(sgpGaOperatorMonitor *gaOperator)
{
  m_afterStepsExt.push_back(gaOperator);
}

void sgpGaOperatorMonitor::performBeforeExecute(uint stepNo, const sgpGaGeneration &input)
{
  for(sgpGaOperatorListExt::iterator it = m_beforeStepsExt.begin(), epos = m_beforeStepsExt.end(); it != epos; ++it)
    checked_cast<sgpGaOperatorMonitor *>(*it)->execute(stepNo, input);
}

void sgpGaOperatorMonitor::performAfterExecute(uint stepNo, const sgpGaGeneration &input)
{
  for(sgpGaOperatorListExt::iterator it = m_afterStepsExt.begin(), epos = m_afterStepsExt.end(); it != epos; ++it)
    checked_cast<sgpGaOperatorMonitor *>(*it)->execute(stepNo, input);
}


// ----------------------------------------------------------------------------
// sgpGaOperatorInit
// ----------------------------------------------------------------------------
void sgpGaOperatorInit::buildRandomGenome(const sgpGaGenomeMetaList &metaList, scDataNode &output)
{
  scDataNode varValue;
  scString str;
  
  output.clear();
  output.setAsList();
  
  for(sgpGaGenomeMetaList::const_iterator it =metaList.begin(), epos = metaList.end(); it != epos; it++)
  {
    switch (it->genType) {
      case gagtConst: {
        output.addChild(new scDataNode(it->minValue));        
        break;
      }
      case gagtRanged: {
        getRandomRanged(it->minValue, it->maxValue, varValue);
        output.addChild(new scDataNode(varValue));        
        break;
      }
      case gagtAlphaString: {
        randomString(it->minValue.getAsString(), it->genSize, str);
        varValue.setAsString(str);
        output.addChild(new scDataNode(varValue));        
        break;
      }
      default:
        throw scError("Unknown variable type inside genome: "+toString(it->genType));
    }
  }
}

void sgpGaOperatorInit::getRandomRanged(const scDataNode &minValue, const scDataNode &maxValue, scDataNode &output)
{
  switch (minValue.getValueType()) {
    case vt_int: {
      int iMinValue = minValue.getAsInt();
      int iMaxValue = maxValue.getAsInt();
      int outValue = randomInt(iMinValue, iMaxValue);
      output.setAsInt(outValue);
      break;
    }
    case vt_uint: {
      uint iMinValue = minValue.getAsUInt();
      uint iMaxValue = maxValue.getAsUInt();
      uint outValue = randomUInt(iMinValue, iMaxValue);
      output.setAsUInt(outValue);
      break;
    }
    case vt_double: {
      double dMinValue = minValue.getAsDouble();
      double dMaxValue = maxValue.getAsDouble();
      double outValue = randomDouble(dMinValue, dMaxValue);
      output.setAsDouble(outValue);
      break;
    }
    default:
      throw scError("Wrong variable type: "+toString(minValue.getValueType()));
  }
}

void sgpGaOperatorInit::getRandomRanged(const scDataNode &startValue, const scDataNode &minValue, const scDataNode &maxValue, scDataNode &output)
{
  switch (minValue.getValueType()) {
    case vt_int: {
      int iMinValue = minValue.getAsInt();
      int iMaxValue = maxValue.getAsInt();
      int iStart = startValue.getAsInt();

      double randomFrac = randomDouble(0.0, 1.0) - 0.5;
      int outValue;
      
      if (randomFrac < 0.0) {
        outValue = (int)round<int>(double(iStart) + (iStart - iMinValue) * randomFrac);
      } else {
        outValue = (int)round<int>(double(iStart) + (iMaxValue - iStart) * randomFrac);
      }   
      
      output.setAsInt(outValue);
      break;
    }
    case vt_double: {
      double dMinValue = minValue.getAsDouble();
      double dMaxValue = maxValue.getAsDouble();
      double dStart = startValue.getAsDouble();
      double outValue = randomDouble(0.0, 1.0) - 0.5;
      if (outValue < 0.0) {
        outValue = dStart + (dStart - dMinValue) * outValue;
      } else {
        outValue = dStart + (dMaxValue - dStart) * outValue;
      }   
      output.setAsDouble(outValue);
      break;
    }
    default:
      throw scError("Wrong variable type: "+toString(minValue.getValueType()));
  }
}

// ----------------------------------------------------------------------------
// sgpGaEvolver
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Construction
// ----------------------------------------------------------------------------
sgpGaEvolver::sgpGaEvolver()
{
  m_populationSize = SGP_GA_DEF_POPSIZE;
  m_eliteLimit = 1;
}

sgpGaEvolver::~sgpGaEvolver()
{
}

// ----------------------------------------------------------------------------
// Properties
// ----------------------------------------------------------------------------
class sgpGaOperatorSetMeta: public scObjectVisitorIntf {
public:
  sgpGaOperatorSetMeta(const sgpGaGenomeMetaList &metaList): m_metaList(&metaList) {}
  virtual ~sgpGaOperatorSetMeta() {}

  virtual void visit(scObject *obj) {
    checked_cast<sgpGaOperator *>(obj)->setMetaInfo(*m_metaList);
  }
private:
  const sgpGaGenomeMetaList *m_metaList;
};

void sgpGaEvolver::setGenomeMeta(const sgpGaGenomeMetaList &metaList)
{
  m_genomeMeta = metaList;

  sgpGaOperatorSetMeta setMeta(metaList);
  m_operatorReg.forEach(&setMeta);
}

class sgpGaOperatorInitializer: public scObjectVisitorIntf {
public:
  virtual void visit(scObject *obj) {
    checked_cast<sgpGaOperator *>(obj)->init();
  }
};

void sgpGaEvolver::initOperators()
{
  // init registered
  sgpGaOperatorInitializer initOperatorInReg;
  m_operatorReg.forEach(&initOperatorInReg);
}

void sgpGaEvolver::setOperator(const scString &operatorType, sgpGaOperator *gaOperator, bool owned)
{
  if (owned)
    m_operatorMap.insert(const_cast<scString &>(operatorType), gaOperator); 

  m_operatorReg.registerObject(operatorType, gaOperator);
}

void sgpGaEvolver::setFitnessFunction(sgpFitnessFunction *function)
{
  m_fitnessFunction.reset(function);
}

sgpFitnessFunction *sgpGaEvolver::getFitnessFunction() const
{
  return m_fitnessFunction.get();
}

void sgpGaEvolver::setPopulationSize(uint a_size)
{
  m_populationSize = a_size;
}

uint sgpGaEvolver::getPopulationSize() const
{
  return m_populationSize;
}

void sgpGaEvolver::getOldGenomes(sgpGaGenomeList &output) const
{
  output.clear();
  output.setAsList();
  
  scDataNode entity;

  for(int i=m_generation->beginPos(), epos = m_generation->endPos(); i != epos; i++)
  {
    m_generation->at(i).getGenomeAsNode(entity);
    output.addElement(entity);
  }
}

void sgpGaEvolver::getNewGenomes(sgpGaGenomeList &output) const
{
  output.clear();
  output.setAsList();
  
  scDataNode entity;
  for(int i = m_newGeneration->beginPos(), epos = m_newGeneration->endPos(); i != epos; i++)
  {
    m_newGeneration->at(i).getGenomeAsNode(entity);
    output.addElement(entity);
  }
}

void sgpGaEvolver::getAllGenomes(sgpGaGenomeList &output) const
{
  output.clear();
  output.setAsList();

  scDataNode entity;

  for(int i = m_generation->beginPos(), epos = m_generation->endPos(); i != epos; i++)
  {
    m_generation->at(i).getGenomeAsNode(entity);
    output.addElement(entity);
  }
  
  for(int i = m_newGeneration->beginPos(), epos = m_newGeneration->endPos(); i != epos; i++)
  {
    m_newGeneration->at(i).getGenomeAsNode(entity);
    output.addElement(entity);
  }
}

void sgpGaEvolver::getGeneration(sgpGaGeneration &output) const
{
  output.copyFrom(*m_generation);
}

void sgpGaEvolver::getBestGenome(sgpGaGenome &output) const
{
  uint maxIdx = sgpFitnessScanner(m_generation.get()).getBestGenomeIndex();
  if (m_generation->size() > maxIdx) {
    // this function works only with single-genome entities
    assert(m_generation->at(maxIdx).getGenomeCount() == 1);

    m_generation->at(maxIdx).getGenome(0, output);

    scDataNodeValue val;
    val.setAsDouble(m_generation->at(maxIdx).getFitness());
    output.push_back(val);
  } else {
    output.clear();
  }
}

void sgpGaEvolver::getBestGenome(scDataNode &output) const
{
  uint maxIdx = sgpFitnessScanner(m_generation.get()).getBestGenomeIndex();
  if (m_generation->size() > maxIdx) {
    m_generation->at(maxIdx).getGenomeAsNode(output);
  } else {
    output.clear();
  }
}

void sgpGaEvolver::getBestGenomeByObjective(uint objIndex, bool searchForMin, scDataNode &output) const
{
  uint maxIdx = sgpFitnessScanner(m_generation.get()).getBestGenomeIndexByObjective(objIndex, searchForMin);
  if (m_generation->size() > maxIdx) {
    m_generation->at(maxIdx).getGenomeAsNode(output);
  } else {
    output.clear();
  }
}

const sgpGaGeneration *sgpGaEvolver::getGeneration() const
{
  return m_generation.get();
}

void sgpGaEvolver::getTopGenomes(int limit, sgpGaGenomeList &output) const
{
  sgpGaEvolver::getTopGenomes(*m_generation, limit, output);
}

void sgpGaEvolver::getTopGenomes(const sgpGaGeneration &input, int limit, 
    sgpGaGenomeList &output)
{
  sgpEntityIndexList indices;
  getTopGenomes(input, limit, output, indices);
}  

void sgpGaEvolver::getTopGenomes(const sgpGaGeneration &input, int limit, 
    sgpGaGenomeList &output, sgpEntityIndexList &indices)
{
  int leftCnt = limit;
  uint lastIdx = input.size();
  uint maxIdx;
  double maxFit;

  scDataNode genome;
  
  output.clear();  
  output.setAsList();
  
  if (input.size() == 0) 
    return;

  sgpFitnessScanner scanner(&input);
  maxIdx = scanner.getBestGenomeIndex();
  
  while(leftCnt > 0) {
    input.at(maxIdx).getGenomeAsNode(genome);
    maxFit = input.at(maxIdx).getFitness();
    output.addElement(genome);
    indices.push_back(maxIdx);
    lastIdx = maxIdx;
    leftCnt--;
    if (!scanner.getBestGenomeIndex(lastIdx, maxFit, maxIdx))
      break;
  }
}

void sgpGaEvolver::getTopGenomesByObjective(const sgpGaGeneration &input, int limit, 
  uint objectiveIndex, sgpEntityIndexList &indices)
{
  sgpFitnessScanner(&input).getTopGenomesByObjective(limit, objectiveIndex, indices);
}    

void sgpGaEvolver::getTopGenomesByObjective(const sgpGaGeneration &input, int limit, 
    uint objectiveIndex, sgpGaGenomeList &output, sgpEntityIndexList &indices)
{
  sgpFitnessScanner(&input).getTopGenomesByObjective(limit, objectiveIndex, indices);

  scDataNode genome;

  output.clear();
  output.setAsList();
    
  for(sgpEntityIndexList::const_iterator it = indices.begin(), epos = indices.end(); it != epos; ++it)
  {
    input.at(*it).getGenomeAsNode(genome);
    output.addElement(genome);
  }  
}

void sgpGaEvolver::getTopGenomesByWeights(const sgpGaGeneration &input, int limit, 
  const sgpWeightVector &weights, sgpGaGenomeList &output, sgpEntityIndexList &indices)
{
  getTopGenomesByWeights(input, limit, weights, &output, indices, SC_NULL);
}

void sgpGaEvolver::getTopGenomesByWeights(const sgpGaGeneration &input, int limit, 
  const sgpWeightVector &weights, sgpEntityIndexList &indices)
{
  getTopGenomesByWeights(input, limit, weights, SC_NULL, indices, SC_NULL);
}

void sgpGaEvolver::getTopGenomesByWeights(const sgpGaGeneration &input, int limit, 
    const sgpWeightVector &weights, sgpGaGenomeList *output, sgpEntityIndexList &indices,
    const sgpEntityIndexSet *requiredItems)
{
  sgpFitnessScanner(&input).getTopGenomesByWeights(limit, weights, indices, requiredItems);
  
  scDataNode genome;

  if (output != SC_NULL)
  {
    output->clear();
    output->setAsList();
    
    for(uint i=0, epos = indices.size(); i != epos; i++)
    {
      input.at(indices[i]).getGenomeAsNode(genome);
      output->addElement(genome);
    }      
  }
}    

void sgpGaEvolver::setEliteLimit(uint a_limit)
{
  m_eliteLimit = a_limit;
}

sgpGaOperator *sgpGaEvolver::getOperator(const scString &operatorType) const
{
  sgpGaOperator *res = getOperatorFromReg(operatorType);
  return res;
}

sgpGaOperator *sgpGaEvolver::getOperatorFromReg(const scString &operatorType) const
{
  return static_cast<sgpGaOperator *>(m_operatorReg.findObject(operatorType));
}

sgpGaOperator *sgpGaEvolver::getOperatorOwned(const scString &operatorType) const
{
  if (m_operatorMap.find(operatorType) != m_operatorMap.end())
    return const_cast<sgpGaOperator *>(&(m_operatorMap.at(operatorType)));
  else
    return SC_NULL;  
}

void sgpGaEvolver::setFitness(uint genomeIndex, double value)
{
  m_generation->at(genomeIndex).setFitness(value);
}

class sgpGaOperatorDescDynParms: public scObjectVisitorIntf {
public:
  sgpGaOperatorDescDynParms(sgpGaGenomeMetaList &output, scDataNode &nameList, const scDataNode &filter): 
    m_output(&output),
    m_nameList(&nameList),
    m_filter(&filter)
    {}
  virtual ~sgpGaOperatorDescDynParms() {}

  virtual void visit(scObject *obj) {
    checked_cast<sgpGaOperator *>(obj)->describeDynamicParams(*m_output, *m_nameList, *m_filter);
  }
private:
  sgpGaGenomeMetaList *m_output;
  scDataNode *m_nameList;
  const scDataNode *m_filter;
};

bool sgpGaEvolver::describeDynamicParams(sgpGaGenomeMetaList &output, scDataNode &nameList, const scDataNode &filter)
{
  sgpGaOperatorDescDynParms descVisitor(output, nameList, filter);
  m_operatorReg.forEach(&descVisitor);

  intDescribeDynamicParams(output, nameList, filter);
  
  if (m_fitnessFunction.get() != SC_NULL) 
  {
    sgpFitnessFunctionEx *fitEx = dynamic_cast<sgpFitnessFunctionEx *>(m_fitnessFunction.get());
    if (fitEx)
      fitEx->describeDynamicParams(output, nameList, filter);
  }
  
  return output.size() > 0;
}

class sgpGaOperatorUseDynParms: public scObjectVisitorIntf {
public:
  sgpGaOperatorUseDynParms(const sgpGaGenomeMetaList &values, const scDataNode &nameList): 
    m_values(&values),
    m_nameList(&nameList)
    {}
  virtual ~sgpGaOperatorUseDynParms() {}

  virtual void visit(scObject *obj) {
    checked_cast<sgpGaOperator *>(obj)->useDynamicParams(*m_values, *m_nameList);
  }
private:
  const sgpGaGenomeMetaList *m_values;
  const scDataNode *m_nameList;
};

void sgpGaEvolver::useDynamicParams(const sgpGaGenomeMetaList &values, const scDataNode &nameList)
{
  sgpGaOperatorUseDynParms useVisitor(values, nameList);
  m_operatorReg.forEach(&useVisitor);

  intUseDynamicParams(values, nameList);
  
  if (m_fitnessFunction.get() != SC_NULL) 
  {
    sgpFitnessFunctionEx *fitEx = dynamic_cast<sgpFitnessFunctionEx *>(m_fitnessFunction.get());
    if (fitEx)
      fitEx->useDynamicParams(values, nameList);
  }
}

// ----------------------------------------------------------------------------
// Execution
// ----------------------------------------------------------------------------
ulong64 sgpGaEvolver::run(ulong64 limit)
{
#ifdef DEBUG_EVOLVER
  Log::addDebug("ga-evolver-begin");  
#endif  
  checkState();
  ulong64 stepsToPerform = limit;
  ulong64 stepNo;
  do {
    stepNo = 1 + limit - stepsToPerform;
    signalStepChanged(stepNo);
#ifdef DEBUG_EVOLVER
  Log::addDebug(scString("ga-evolver-1, step: ")+toString(stepNo));  
#endif  
#ifdef TRACE_ENTITY_BIO
    sgpEntityTracer::handleStepBegin(*m_generation.get());
#endif    
#ifdef DEBUG_EVOLVER
  Log::addDebug("ga-evolver-1a");  
#endif  
    if (!runOperators(stepNo, true))
      break;

#ifdef DEBUG_EVOLVER
  Log::addDebug("ga-evolver-2");  
#endif  

#ifdef DEBUG_EVOLVER
  Log::addDebug("ga-evolver-3");  
#endif  

    if (limit > 0)
      stepsToPerform--;

#ifdef TRACE_ENTITY_BIO
    sgpEntityTracer::handleStepEnd();
#endif    
    if (stepsToPerform > 0) {
      useNewGeneration();
    }
  } while (stepsToPerform > 0);
#ifdef DEBUG_EVOLVER
  Log::addDebug("ga-evolver-end");  
#endif  
  return stepNo;
}

// run evolution step without evaluation
void sgpGaEvolver::runStep()
{
  checkState();
  runOperators(1, false);
  if (m_newGeneration->size() > 0)
    useNewGeneration();
}

void sgpGaEvolver::stop()
{
  if (m_newGeneration->size() > 0)
    useNewGeneration();
}

bool sgpGaEvolver::runOperators(uint stepNo, bool runEval)
{
  bool res = true;
  m_newGeneration->clear();
#ifdef DEBUG_EVOLVER
  Log::addDebug("ga-evolver-oper-before-monitor");  
#endif    
  runMonitorExecute(stepNo);
#ifdef DEBUG_EVOLVER
  Log::addDebug("ga-evolver-oper-before-elite");  
#endif    
  runElite();
#ifdef DEBUG_EVOLVER
  Log::addDebug("ga-evolver-oper-before-selection");  
#endif    
  runSelection(stepNo);
#ifdef TRACE_ENTITY_BIO
  sgpEntityTracer::flushMoveBuffer();
#endif  
#ifdef DEBUG_EVOLVER
  Log::addDebug("ga-evolver-oper-before-mutate");  
#endif    
  runMutate();
#ifdef DEBUG_EVOLVER
  Log::addDebug("ga-evolver-oper-before-xover");  
#endif    
  runXOver();
#ifdef DEBUG_EVOLVER
  Log::addDebug("ga-evolver-oper-before-terminate");  
#endif    
  terminateOutdated();
#ifdef DEBUG_EVOLVER
  Log::addDebug("ga-evolver-oper-before-eval");  
#endif    
  if (runEval) {
    if (!runEvaluate(stepNo)) 
      res = false;
  }    
#ifdef DEBUG_EVOLVER
  Log::addDebug("ga-evolver-oper-end");  
#endif    
  return res;
}

void sgpGaEvolver::reset()
{
  m_generation.reset();  
  m_newGeneration.reset();
  if (m_fitnessFunction.get() != SC_NULL) {      
    m_fitnessFunction->reset();
  }  
}

void sgpGaEvolver::checkPrepared()
{
  if (!isPrepared())
    prepare();
}

void sgpGaEvolver::checkState()
{
  checkPrepared();
    
  if (m_newGeneration->size() > 0)
    useNewGeneration();
  
  if (!m_generation->size()) {
    runInit();
  }
  
  checkGenerationSize();        
}

void sgpGaEvolver::checkGenerationSize()
{
  if (m_generation->size() <= m_eliteLimit) 
    throw scError("Elite limit too big");
}

bool sgpGaEvolver::canBuildGeneration() 
{
  return (m_genomeMeta.size() > 0);
}

void sgpGaEvolver::runDefaultInit() 
{    
  if (canBuildGeneration()) {
    buildGeneration();
  } else {
    throw scError("Not enough information for initial generation");  
  }  
}  

void sgpGaEvolver::setGeneration(const sgpGaGenomeList &genomeList, const sgpGaFitnessList &fitnessList)
{
  std::auto_ptr<sgpEntityBase> workInfoGuard;
  
  checkPrepared();
  m_generation->clear();
  
  for(uint i=0, epos = genomeList.size(); i != epos; i++) {
    workInfoGuard.reset(m_generation->newItem());
    workInfoGuard->setGenomeAsNode(genomeList.getElement(i));
    if (i < fitnessList.size())
      workInfoGuard->setFitness(fitnessList.getElement(i).getAsDouble());
    m_generation->insert(workInfoGuard.release());
  } 
  
  // ensure current generation has fitness filled-in
  if (fitnessList.size() == 0)
    runEvaluate(0, true);
}

void sgpGaEvolver::buildGeneration()
{
  buildGeneration(true);
}

void sgpGaEvolver::buildGeneration(bool evaluate)
{
  if (getOperator(SGP_GA_OPERATOR_INIT) != SC_NULL) {
    buildGenerationWithInitOperator(evaluate);
  } else {  
    scDataNode genomeNode;
    sgpGaGenomeList templateList;
    templateList.setAsList();

    for(uint i=0; i < m_populationSize; i++) {
      sgpGaOperatorInit::buildRandomGenome(m_genomeMeta, genomeNode);
      templateList.addItem(genomeNode);
    }  
    
    buildGeneration(templateList, evaluate);
  }
}

void sgpGaEvolver::buildGenerationWithInitOperator(bool evaluate)
{
  runInitOperator();
  processInitGeneration(evaluate);
  useNewGeneration();
}

void sgpGaEvolver::runInitOperator()
{
  sgpGaOperator *genOperator = getOperator(SGP_GA_OPERATOR_INIT);
  if (genOperator != SC_NULL) {
    sgpGaOperatorInitBase *oper = checked_cast<sgpGaOperatorInitBase *>(genOperator);
    oper->execute(m_populationSize, *m_newGeneration);
  } 
}

void sgpGaEvolver::buildGeneration(const sgpGaGenomeList &templateList, bool evaluate)
{
  uint modLen = templateList.size();
  uint srcPos;
  std::auto_ptr<sgpEntityBase> workInfoGuard;

  checkPrepared();
  m_newGeneration->clear();

  for(uint i=0, epos = m_populationSize; i != epos; i++) {
    srcPos = i % modLen;  
    workInfoGuard.reset(m_newGeneration->newItem());
    workInfoGuard->setGenomeAsNode(templateList.getElement(srcPos));
    m_newGeneration->insert(workInfoGuard.release());
  }
  processInitGeneration(evaluate);
  useNewGeneration();
}

void sgpGaEvolver::processInitGeneration(bool evaluate)
{
  runMutate();
  runXOver();
  if (evaluate) 
    runEvaluate(0);
}

void sgpGaEvolver::useNewGeneration()
{
  if (!m_newGeneration->empty())
  for(int i=m_newGeneration->endPos() - 1,epos = m_newGeneration->beginPos(); i >= epos; i--) {
    m_generation->insert(m_newGeneration->extractItem(i));
#ifdef TRACE_ENTITY_BIO
  sgpEntityTracer::handleEntityMovedBuf(i, m_generation->size() - 1, "use");    
#endif      
  }
#ifdef TRACE_ENTITY_BIO
  sgpEntityTracer::flushMoveBuffer();
#endif  
  m_newGeneration->clear();
}

void sgpGaEvolver::buildRandomGenome(sgpGaGenome &output)
{
  scDataNode varValue;
  scString str;
  
  output.clear();
  
  for(sgpGaGenomeMetaList::const_iterator it =m_genomeMeta.begin(), epos = m_genomeMeta.end(); it != epos; it++)
  {
    switch (it->genType) {
      case gagtConst: {
        output.push_back(it->minValue);
        break;
      }
      case gagtRanged: {
        sgpGaOperatorInit::getRandomRanged(it->minValue, it->maxValue, varValue);
        output.push_back(varValue);        
        break;
      }
      case gagtAlphaString: {
        randomString(it->minValue.getAsString(), it->genSize, str);
        varValue.setAsString(str);
        output.push_back(varValue);        
        break;
      }
      default:
        throw scError("Unknown variable type inside genome: "+toString(it->genType));
    }
  }
}

void sgpGaEvolver::runMutate()
{
  sgpGaOperator *genOperator = getOperator(SGP_GA_OPERATOR_MUTATE);
  if (genOperator != SC_NULL) {
    sgpGaOperatorMutate *oper = checked_cast<sgpGaOperatorMutate *>(genOperator);
    oper->execute(*m_newGeneration);
  }    
}

void sgpGaEvolver::runSelection(uint stepNo)
{
#ifdef TRACE_ENTITY_BIO
  Log::addInfo("Selection start, step #"+toString(stepNo));  
#endif  
  sgpGaOperator *genOperator = getOperator(SGP_GA_OPERATOR_SELECT);
    
  if (genOperator != SC_NULL) {
    sgpGaOperatorSelect *oper = checked_cast<sgpGaOperatorSelect *>(genOperator);
    oper->execute(*m_generation, *m_newGeneration, getNewGenerationSpaceLeft());
  } else {
  // if no selector - just copy
    performDummySelect();
  }   
#ifdef TRACE_ENTITY_BIO
  Log::addInfo("Selection stop, step #"+toString(stepNo));  
#endif  
}

uint sgpGaEvolver::getNewGenerationSize()
{
  return m_newGeneration->size(); 
}

uint sgpGaEvolver::getNewGenerationSpaceLeft()
{
  return m_populationSize - getNewGenerationSize();
}

bool sgpGaEvolver::isEliteActive()
{
  sgpGaOperator *genOperatorElite = getOperator(SGP_GA_OPERATOR_ELITE);
  bool res;
  
  if ((genOperatorElite != SC_NULL) && (m_eliteLimit > 0))
    res = true;
  else  
    res = false;
    
  return res;   
}

// just copy entities
void sgpGaEvolver::performDummySelect()
{
  uint currSize = getNewGenerationSize();

  if (m_populationSize > currSize)
  {
    uint targetSize = m_populationSize - currSize;
    if (!m_generation->empty())
    for(int i = m_generation->endPos() - 1; 
       (i >= 0) && (m_newGeneration->size() < targetSize); 
       i--) 
    {
      m_newGeneration->insert(m_generation->extractItem(i));
    }
  }
}

void sgpGaEvolver::runElite()
{
  sgpGaOperator *genOperator = getOperator(SGP_GA_OPERATOR_ELITE);
  if (genOperator != SC_NULL) {
    sgpGaOperatorElite *oper = checked_cast<sgpGaOperatorElite *>(genOperator);
    oper->execute(*m_generation, *m_newGeneration, m_eliteLimit);
  }    
}

void sgpGaEvolver::runXOver()
{
  sgpGaOperator *genOperator = getOperator(SGP_GA_OPERATOR_XOVER);
  if (genOperator != SC_NULL) {
    sgpGaOperatorXOver *oper = checked_cast<sgpGaOperatorXOver *>(genOperator);
    uint oldSize = m_newGeneration->size();
    oper->execute(*m_newGeneration);
    uint newSize = m_newGeneration->size();
    if (newSize > oldSize)
      newGenerationSizeIncreased(newSize - oldSize);
  }    
}

void sgpGaEvolver::newGenerationSizeIncreased(uint growCnt)
{
  // do nothing here
}

// leave n genomes from the end of list (newest)
void sgpGaEvolver::terminateOutdated()
{
  m_generation->clear();
  return;
}

void sgpGaEvolver::runMonitorExecute(uint stepNo)
{
  sgpGaOperator *genOperator = getOperator(SGP_GA_OPERATOR_MONITOR);
  if (genOperator != SC_NULL) {
    sgpGaOperatorMonitor *oper = checked_cast<sgpGaOperatorMonitor *>(genOperator);
    oper->execute(stepNo, *m_generation);
  }    
}

void sgpGaEvolver::runMonitorBeforeEvaluate(uint stepNo, const sgpGaGeneration *target)
{
  sgpGaOperator *genOperator = getOperator(SGP_GA_OPERATOR_MONITOR);
  if (genOperator != SC_NULL) {
    sgpGaOperatorMonitor *oper = checked_cast<sgpGaOperatorMonitor *>(genOperator);
    oper->handleBeforeEvaluate(stepNo, *target);
  }    
}

void sgpGaEvolver::runMonitorBeforeEvaluatePostProc(uint stepNo, const sgpGaGeneration *target)
{
  sgpGaOperator *genOperator = getOperator(SGP_GA_OPERATOR_MONITOR);
  if (genOperator != SC_NULL) {
    sgpGaOperatorMonitor *oper = checked_cast<sgpGaOperatorMonitor *>(genOperator);
    oper->handleBeforeEvaluatePostProc(stepNo, *target);
  }    
}

void sgpGaEvolver::runMonitorAfterEvaluate(uint stepNo, const sgpGaGeneration *target)
{
  sgpGaOperator *genOperator = getOperator(SGP_GA_OPERATOR_MONITOR);
  if (genOperator != SC_NULL) {
    sgpGaOperatorMonitor *oper = checked_cast<sgpGaOperatorMonitor *>(genOperator);
    oper->handleAfterEvaluate(stepNo, *target);
  }    
}

void sgpGaEvolver::runInit()
{
#ifdef TRACE_ENTITY_BIO
    sgpEntityTracer::handleStepBegin(*m_generation.get());
#endif    
  buildGenerationWithInitOperator(true);
#ifdef TRACE_ENTITY_BIO
    sgpEntityTracer::handleStepEnd();
#endif    
}

bool sgpGaEvolver::runEvaluate(uint stepNo, bool useCurrGener)
{
#ifdef DEBUG_EVOLVER
  Log::addDebug("ga-evolver-e-begin");  
#endif  
  bool res = true;
  sgpGaGeneration *target;
  if (useCurrGener)
    target = m_generation.get();
  else
    target = m_newGeneration.get();
  assert(target != SC_NULL);    

  sgpGaOperator *genOperator = getOperator(SGP_GA_OPERATOR_EVAL);
  if (genOperator != SC_NULL) {
    sgpGaOperatorEvaluate *oper = checked_cast<sgpGaOperatorEvaluate *>(genOperator);
    res = oper->execute(stepNo, !useCurrGener, *target);
  }    
    
#ifdef DEBUG_EVOLVER
  Log::addDebug("ga-evolver-e-end");  
#endif  
  return res;
}

bool sgpGaEvolver::isPrepared()
{
  return (m_generation.get() != SC_NULL);
}

void sgpGaEvolver::prepare()
{
  m_generation.reset(newGeneration());
  m_newGeneration.reset(newGeneration());
  initOperators();
}

sgpGaGeneration *sgpGaEvolver::newGeneration()
{
  std::auto_ptr<sgpGaGeneration> res;

  sgpGaOperator *genOperator = getOperator(SGP_GA_OPERATOR_ALLOC);
  assert(genOperator != SC_NULL);

  if (genOperator != SC_NULL) {
    sgpGaOperatorAlloc *oper = checked_cast<sgpGaOperatorAlloc *>(genOperator);
    res.reset(oper->execute());
  }    

  return res.release();
}

void sgpGaEvolver::signalStepChanged(uint stepNo)
{ // empty here
}
