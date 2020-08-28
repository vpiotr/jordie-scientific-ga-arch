/////////////////////////////////////////////////////////////////////////////
// Name:        GaOperatorSelectTourProb.cpp
// Project:     scLib
// Purpose:     Select operator - tournament with probability of winning
// Author:      Piotr Likus
// Modified by:
// Created:     29/12/2009
/////////////////////////////////////////////////////////////////////////////

//std
#include <cmath>
#include <algorithm>

//base
#include "base/rand.h"
#include "base/utils.h"

//perf
#include "perf/Counter.h"
#include "perf/Timer.h"
#include "perf/Log.h"

//sc
#include "sc/smath.h"
#include "sc/alg/kmeans.h"
//sgp
#include "sgp/GaOperatorSelectTourProb.h"
#include "sgp/GaStatistics.h"

#include "sgp/ExperimentConst.h"

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

using namespace dtp;
using namespace perf;

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------
const uint DEF_SHAPE_OBJ_INDEX = 1;
const uint TOUR_PROB_SHAPE_LIMIT = 5;
const uint TOUR_PROB_SHAPE_STEPS = 10;
// level of gravity between input ratio and extreme (-1,1) value
// use (0..1)
const double TOUR_PROB_EXTREME_GRAVITY_RATIO = 0.25; // 0..1
const uint TOUR_LOOP_LIMIT = 1000;
const uint TOUR_MAX_GROUP_SIZE = 2;
const double TOUR_PROB_SHAPE_DISTRIB_DECREASE_FACTOR = 1.0;
const bool TOUR_USE_PROB_ONLY_FOR_NON_DOMIN = true;
// WARNING: below is a key value(!) Do not change if you are not sure!
const double TOUR_DISTRIB_NARROW_FACTOR = 10.0; 
const double TOUR_SHAPE_SEL_PROB_MARGIN = 0.1;

const uint ISLAND_DATA_IDX_SHAPE_COLN = 0;
const uint ISLAND_DATA_IDX_SHAPE_DISTRIB = 1;
const uint ISLAND_DATA_IDX_SIZE = 2;
const uint ISLAND_DATA_IDX_SHAPE_DISTRIB_SUM = 3;
  
// ----------------------------------------------------------------------------
// sgpGaOperatorSelectTourProb
// ----------------------------------------------------------------------------
sgpGaOperatorSelectTourProb::sgpGaOperatorSelectTourProb(): inherited()
{
  m_distanceFactor = SGP_GA_DEF_SELECT_DIST_FACTOR;
  m_tournamentFailedTracer = SC_NULL;
  m_shapeObjIndex = DEF_SHAPE_OBJ_INDEX;
  m_secShapeObjIndex = m_shapeObjIndex;
  m_shapeLimit = TOUR_PROB_SHAPE_LIMIT;
  m_islandLimit = 0;
  m_experimentParams = SC_NULL;
  m_islandTool = SC_NULL;
}

sgpGaOperatorSelectTourProb::~sgpGaOperatorSelectTourProb()
{
}

void sgpGaOperatorSelectTourProb::setObjectiveWeights(const sgpWeightVector &value)
{
  m_objectiveWeights = value;
  m_objectiveWeightsRescaled = value;
  double maxWeight = 0.0;

// skip weight for obj[0]
  for(sgpWeightVector::iterator it=m_objectiveWeightsRescaled.begin()+1,epos = m_objectiveWeightsRescaled.end(); it != epos; ++it)
  {
    if (fabs(*it) > maxWeight)
      maxWeight = fabs(*it);  
  }

// skip weight for obj[0]     
  m_objectiveWeightsRescaledSum = 0.0;   
  for(sgpWeightVector::iterator it=m_objectiveWeightsRescaled.begin()+1,epos = m_objectiveWeightsRescaled.end(); it != epos; ++it)
  {
    *it = 1 + maxWeight - *it;
    m_objectiveWeightsRescaledSum += *it;
  }
}

void sgpGaOperatorSelectTourProb::setMatchFailedTracer(sgpMatchFailedTracer *tracer)
{
  m_matchFailedTracer = tracer;
}

void sgpGaOperatorSelectTourProb::setTournamentFailedTracer(sgpTournamentFailedTracer *tracer)
{
  m_tournamentFailedTracer = tracer;
}

void sgpGaOperatorSelectTourProb::setDistanceFunction(sgpDistanceFunction *value)
{
  m_distanceFunction = value;
}

void sgpGaOperatorSelectTourProb::setDistanceFactor(double value)
{
  m_distanceFactor = value;
}

void sgpGaOperatorSelectTourProb::setStats(const sgpFitnessValue &topAvg)
{
  m_statsTopAvg = topAvg;
}

void sgpGaOperatorSelectTourProb::setShapeObjIndex(uint value)
{
  m_shapeObjIndex = value;
}

void sgpGaOperatorSelectTourProb::setSecShapeObjIndex(uint value)
{
  m_secShapeObjIndex = value;
}

void sgpGaOperatorSelectTourProb::setShapeLimit(uint value)
{
  m_shapeLimit = value;
}

uint sgpGaOperatorSelectTourProb::getShapeLimit()
{
  return m_shapeLimit;
}

void sgpGaOperatorSelectTourProb::setIslandLimit(uint value)
{
  m_islandLimit = value;
}

void sgpGaOperatorSelectTourProb::setExperimentParams(const sgpGaExperimentParams *params)
{
  m_experimentParams = params;
}
  
void sgpGaOperatorSelectTourProb::setExperimentLog(sgpExperimentLog *value)
{
  m_experimentLog = value;
}
  
void sgpGaOperatorSelectTourProb::setIslandTool(sgpEntityIslandToolIntf *value)
{
  m_islandTool = value;
}

void sgpGaOperatorSelectTourProb::prepareShapeCollectionByObj(const sgpGaGeneration &input, 
  scDataNode &output)
{
  double objValue;
  scString shapeName, shapeName2;
  bool oneLevel = (m_shapeObjIndex == m_secShapeObjIndex);

  for(uint i=0, epos = input.size(); i != epos; i++)
  {
    objValue = input[i].getFitness(m_shapeObjIndex);
    shapeName = toString(objValue);
    if (!output.hasChild(shapeName))
      output.addChild(shapeName, new scDataNode());
    if (oneLevel)
    {  
      output[shapeName].addItem(scDataNode(i));  
    } else {
      objValue = input[i].getFitness(m_secShapeObjIndex);
      shapeName2 = toString(objValue);
      if (!output[shapeName].hasChild(shapeName2))
        output[shapeName].addChild(shapeName2, new scDataNode());
      output[shapeName][shapeName2].addItem(scDataNode(i));  
    }  
  }
}  

template <class T>
class lessThenPred {    // function object that returns true if value is less then
  private:
    double predArg;       
  public:
    lessThenPred(T value) : predArg(value) {
    }
    bool operator() (T arg) {
        return (arg < predArg);
    }
};

void sgpGaOperatorSelectTourProb::prepareShapeCollectionByObjDistrib(const sgpGaGeneration &input, 
  const scDataNode *itemList, uint islandId,
  scDataNode &shapeList, scDataNode &shapeDistrib, double decFactor)
{ 
  bool oneLevel;
  uint shapeObjIdx;
  double objValue;
  scString shapeName, shapeName2;
  scDataNode shapeListWithPriority;

  shapeList.clear();
  shapeDistrib.clear();
  shapeDistrib.setAsArray(vt_double);
    
  selectShapeObj(islandId, oneLevel, shapeObjIdx);
    
  uint eposj, idx;
  if (itemList != SC_NULL)
    eposj = itemList->size();
  else
    eposj = input.size();
        
  for(uint j=0; j != eposj; j++)
  {
    if (itemList != SC_NULL)
      idx = (*itemList).getUInt(j);
    else
      idx = j;
        
    objValue = input[idx].getFitness(shapeObjIdx);
    
    // lower precision by converting to string which will be a map key
    shapeName = toString(objValue);

    if (!shapeList.hasChild(shapeName))
    {
      if (oneLevel)
        shapeList.addChild(shapeName, new scDataNode(ict_array, vt_uint));
      else
        shapeList.addChild(shapeName, new scDataNode(ict_parent));
      shapeListWithPriority.addChild(shapeName, new scDataNode(objValue));
    }  

    if (oneLevel)
    {  
      shapeList[shapeName].addItem(idx);  
    } else {
      objValue = input[idx].getFitness(m_secShapeObjIndex);
      shapeName2 = toString(objValue);
      if (!shapeList[shapeName].hasChild(shapeName2))
        shapeList[shapeName].addChild(shapeName2, new scDataNode());
      shapeList[shapeName][shapeName2].addItem(idx);  
    }  
  }
  
  prepareShapeDistrib(shapeListWithPriority, decFactor, shapeDistrib);
}  

void sgpGaOperatorSelectTourProb::selectShapeObj(uint islandId, bool &oneLevel, uint &shapeObjIdx)
{
  oneLevel = (m_shapeObjIndex == m_secShapeObjIndex);
  shapeObjIdx = m_shapeObjIndex;

  if ((m_experimentParams != SC_NULL) && !oneLevel)
  {
    double param;
    if (m_experimentParams->getDouble(islandId, SGP_EXP_PAR_BLOCK_IDX_TOUR + SGP_TOUR_PROB_EP_SHAPE_OBJ_PROB, param))
    {
      oneLevel = true;
      if (param < TOUR_SHAPE_SEL_PROB_MARGIN)
        shapeObjIdx = m_shapeObjIndex;
      else if (param > (1.0 - TOUR_SHAPE_SEL_PROB_MARGIN))
        shapeObjIdx = m_secShapeObjIndex;
      else // select first obj idx with prob = rescaled param w/o margins 
        shapeObjIdx = (randomFlip((param - TOUR_SHAPE_SEL_PROB_MARGIN)/(1.0 - 2.0*TOUR_SHAPE_SEL_PROB_MARGIN))?m_shapeObjIndex:m_secShapeObjIndex);    
    }
  }  
}

void sgpGaOperatorSelectTourProb::prepareShapeDistrib(const scDataNode &shapeListWithPriority, double decFactor, scDataNode &shapeDistrib)
{ 
  uint uTotalCount = shapeListWithPriority.size();

  shapeDistrib.clear();
  shapeDistrib.setAsArray(vt_double);
 
  for(uint i=0, epos = uTotalCount; i != epos; i++) {
    shapeDistrib.addItemAsDouble(1.0);
  }

  uint lessCount;
  scDataNode::size_type shapeIndex;
  double prob, fitRate;
  double dTotalCount = static_cast<double>(uTotalCount);
  double priority;

  std::vector<double> shapeVect;

  shapeVect.resize(uTotalCount);
  for(uint i=0; i != uTotalCount; i++)
    shapeVect[i] = shapeListWithPriority.getDouble(i);

  std::sort(shapeVect.begin(), shapeVect.end());

  for(uint i=0, epos = shapeListWithPriority.size(); i != epos; i++)
  {
    lessCount = std::distance(shapeVect.begin(), std::lower_bound(shapeVect.begin(), shapeVect.end(), priority));

    fitRate = static_cast<double>(lessCount)/dTotalCount;

    prob = 
      1.0/dTotalCount +
      (1.0 / (pow(2, ((1.0 - fitRate) * decFactor * TOUR_DISTRIB_NARROW_FACTOR))) * (1.0 - 2.0/dTotalCount));

    shapeDistrib.setDouble(i, prob);
  }
}  

void sgpGaOperatorSelectTourProb::prepareShapeCollectionByCluster(const sgpGaGeneration &input, 
  scDataNode &shapeList)
{
  scDataNode shapeDistrib;
  prepareShapeCollectionByCluster(input, SC_NULL, 0, shapeList, shapeDistrib, 1.0);
}

void sgpGaOperatorSelectTourProb::prepareShapeCollectionByCluster(const sgpGaGeneration &input, 
  const scDataNode *itemList, uint islandId,
  scDataNode &shapeList, scDataNode &shapeDistrib, double decFactor)
{
  bool oneLevel;
  uint shapeObjIdx;

  scKMeansCalculator calc;
  double objValue;
  scDataNode clusterInput, clusterOutput;
  scString shapeName;
  scDataNode shapeListForDistrib;

  selectShapeObj(islandId, oneLevel, shapeObjIdx);

  uint epos, idx;

  if (itemList != SC_NULL)
    epos = itemList->size();
  else
    epos = input.size();

  for(uint i=0; i != epos; i++)
  {
    if (itemList != SC_NULL)
      idx = (*itemList).getUInt(i);
    else
      idx = i;

    objValue = input[idx].getFitness(shapeObjIdx);

    if (oneLevel)    
    {
      clusterInput.addItemAsDouble(objValue);
    } else {
      clusterInput.addChild(new scDataNode());
      clusterInput[i].addItemAsDouble(objValue);
      objValue = input[idx].getFitness(m_secShapeObjIndex);
      clusterInput[i].addItemAsDouble(objValue);
    }  
  }  
  
  calc.execute(clusterInput, clusterOutput, m_shapeLimit, TOUR_PROB_SHAPE_STEPS);

  for(uint i=0, epos = clusterOutput.size(); i != epos; i++)
  {
    if (itemList != SC_NULL)
      idx = (*itemList).getUInt(i);
    else
      idx = i;

    shapeName = toString(clusterOutput.getUInt(i));
    if (!shapeList.hasChild(shapeName)) {
      shapeList.addChild(shapeName, new scDataNode());
      objValue = input[idx].getFitness(shapeObjIdx);
      shapeListForDistrib.addChild(shapeName, new scDataNode(objValue));
    }

    shapeList[shapeName].addItem(scDataNode(idx));      
  }  

  prepareShapeDistrib(shapeListForDistrib, decFactor, shapeDistrib);
}

void sgpGaOperatorSelectTourProb::prepareIslands(const sgpGaGeneration &input, uint totalSize, scDataNode &islandItems, scDataNode &islandData)
{
  prepareIslands(input, 0, 0, totalSize, islandItems, islandData);
}

void sgpGaOperatorSelectTourProb::prepareIslands(const sgpGaGeneration &input, uint beginIslandNo, uint endIslandNo, 
  uint totalSize, scDataNode &islandItems, scDataNode &islandData)
{
  prepareIslandMap(input, beginIslandNo, endIslandNo, islandItems);
  prepareIslandData(input, islandItems, islandData);
  normalizeIslandSize(totalSize, islandItems, islandData);
}

void sgpGaOperatorSelectTourProb::prepareIslandMap(const sgpGaGeneration &input, 
  uint beginIslandNo, uint endIslandNo, 
  scDataNode &islandItems)
{
  if (beginIslandNo != endIslandNo)
  {
    // island range
    scDataNode bufferMap;
    std::auto_ptr<scDataNode> newIsland;
    uint islandId;

    m_islandTool->prepareIslandMap(input, bufferMap);
    islandItems.clear();
    islandItems.setAsParent();

    for(uint i=0, epos = bufferMap.size(); i != epos; i++)
    {
      islandId = stringToUInt(bufferMap.getElementName(i));
      if ((islandId >= beginIslandNo) && (islandId < endIslandNo))
      {
        newIsland.reset(new scDataNode());
        *newIsland = bufferMap[i];
        islandItems.addChild(toString(islandId), newIsland.release());
      }        
    }    
  } else {
    // all
    m_islandTool->prepareIslandMap(input, islandItems);
  }    
}

void sgpGaOperatorSelectTourProb::prepareIslandData(const sgpGaGeneration &input, scDataNode &islandItems, scDataNode &islandData)
{
  dnode shapeCollection, shapeDistrib;
  
  uint islandId; 
  double decFactor = TOUR_PROB_SHAPE_DISTRIB_DECREASE_FACTOR;
  double sizeFactor = 1.0;
  double shapeDistribSum;
  double param;
  std::auto_ptr<scDataNode> islandDataGuard; 

  for(uint i=0, epos = islandItems.size(); i != epos; i++)
  {
    islandId = stringToUIntDef(islandItems.getElementName(i), 0);
    
    if (m_experimentParams != SC_NULL)
    {
      if (m_experimentParams->getDouble(islandId, SGP_EXP_PAR_BLOCK_IDX_TOUR + SGP_TOUR_PROB_EP_DISTRIB_DEC_RATIO, param))
        decFactor = param;
      else
        decFactor = TOUR_PROB_SHAPE_DISTRIB_DECREASE_FACTOR;
      if (m_experimentParams->getDouble(islandId, SGP_EXP_PAR_BLOCK_IDX_TOUR + SGP_TOUR_PROB_EP_ISLAND_SIZE, param))
        sizeFactor = param;
      else
        sizeFactor = 1.0;
    }

    if (islandItems[i].empty())
      sizeFactor = 0.0;
      
    if (getShapeLimit() > 0)
      prepareShapeCollectionByCluster(input, &(islandItems[i]), islandId, shapeCollection, shapeDistrib, decFactor);  
    else
      prepareShapeCollectionByObjDistrib(input, &(islandItems[i]), islandId, shapeCollection, shapeDistrib, decFactor);  

    shapeDistribSum = shapeDistrib.accumulate(0.0);

    islandDataGuard.reset(new dnode());

    // sorted in ISLAND_DATA_IDX_X order
    islandDataGuard->addChild(new dnode(base::move(shapeCollection)));
    islandDataGuard->addChild(new dnode(base::move(shapeDistrib)));
    islandDataGuard->addChild(new dnode(sizeFactor));
    islandDataGuard->addChild(new dnode(shapeDistribSum));

    assert(islandDataGuard->size() - 1 == ISLAND_DATA_IDX_SHAPE_DISTRIB_SUM);
    
    islandData.addChild(islandItems.getElementName(i), islandDataGuard.release());
  }
}

void sgpGaOperatorSelectTourProb::normalizeIslandSize(uint totalSize, const scDataNode &islandItems, scDataNode &islandData)
{
  // normalize size  
  uint itemsLeft, islandSize;
  itemsLeft = totalSize;
  double sizeSum = 0.0;
  
  for(uint i=0, epos = islandData.size(); i != epos; i++)
  {
    if (!islandItems[islandData.getElementName(i)].empty()) 
      sizeSum += islandData[i].getDouble(ISLAND_DATA_IDX_SIZE);
  }
    
  for(uint i=0, epos = islandData.size(); i != epos; i++)
  {
    if (i == epos - 1) {
      if (islandItems[islandData.getElementName(i)].empty()) {
      // find which one can be bigger
        for(uint j=0, eposj = epos - 1; j != eposj; j++)
          if (islandData[j].getUInt(ISLAND_DATA_IDX_SIZE) > 0) {
            islandData[j].setUInt(ISLAND_DATA_IDX_SIZE, islandData[j].getUInt(ISLAND_DATA_IDX_SIZE) + itemsLeft);
            break;
          }
        islandSize = 0;         
      } else {      
        islandSize = itemsLeft;
      }  
    } else { 
      islandSize = round<uint>((islandData[i].getDouble(ISLAND_DATA_IDX_SIZE) / sizeSum) * static_cast<double>(totalSize));
      islandSize = std::min<uint>(islandSize, itemsLeft);
      itemsLeft -= islandSize;
    }  
    islandData[i][ISLAND_DATA_IDX_SIZE].setAsUInt(islandSize); 
  }
}

// generate tournament group using "equal oportunities" algorithm
// shapeCollection is list of groups of entities, each group contains a list of entity indexes in population
void sgpGaOperatorSelectTourProb::genRandomGroupByShape(sgpTournamentGroup &output, const sgpGaGeneration &input, uint limit, 
  const scDataNode &shapeCollection, const scDataNode &shapeDistrib, double shapeDistribSum)
{
  output.clear();
  if (shapeCollection.empty())
    return;

  uint shapeIndex, shapeIndex2, entityIndex;
  uint addedCnt = 0;
  bool useDistrib = !shapeDistrib.empty();
  bool oneLevel = true;
  std::vector<uint> shapeIndexVector;
  uint shapeIndexVectorIdx = 0;
  
  while(addedCnt < limit) 
  {
    if (useDistrib) {
      if (shapeIndexVectorIdx >= shapeIndexVector.size()) {
        shapeIndexVector.clear();
        selectProbItem(shapeDistrib, shapeDistribSum, shapeIndexVector, limit - addedCnt);
        shapeIndexVectorIdx = 0;
      } 
      assert(!shapeIndexVector.empty());
      shapeIndex = shapeIndexVector[shapeIndexVectorIdx];
      shapeIndexVectorIdx++;
    } else {
      shapeIndex = randomUInt(0, shapeCollection.size() - 1);
    }
        
    assert(!shapeCollection[shapeIndex].empty());
    entityIndex = randomUInt(0, shapeCollection[shapeIndex].size() - 1);
    if (oneLevel)
    {
      entityIndex = shapeCollection[shapeIndex].getUInt(entityIndex);
    } else {
      shapeIndex2 = entityIndex;
      entityIndex = randomUInt(0, shapeCollection[shapeIndex][shapeIndex2].size() - 1);
      entityIndex = shapeCollection[shapeIndex][shapeIndex2].getUInt(entityIndex);    
    }  
    if (output.find(entityIndex) == output.end())
    {
      output.insert(entityIndex);
      addedCnt++;
    }  
  }    
}  

// select random island and items from it
void sgpGaOperatorSelectTourProb::genRandomGroupAndIsland(sgpTournamentGroup &output, const sgpGaGeneration &input, uint limit, 
  const scDataNode &islandItems, const scDataNode &islandData, uint &islandId, scDataNode &newItems)
{
  uint islandIdx, islandSize;
  uint useLimit = limit;
  uint islandSpaceLeft;
  int islandLimit;
  scString islandName;

  islandId = islandItems.size();
  
  output.clear();
  if (!islandItems.empty()) {
    do {
      islandIdx = randomUInt(0, islandItems.size() - 1);
      islandName = islandItems.getElementName(islandIdx);
      islandSize = islandItems[islandName].size();
      islandSpaceLeft = islandData[islandName].getUInt(ISLAND_DATA_IDX_SIZE);
      
      if (newItems.hasChild(islandName))   
        islandSpaceLeft = islandSpaceLeft - newItems.getUInt(islandName);   
      if ((islandSize > 0) && (islandSpaceLeft > 0)) {
        if (m_experimentParams != SC_NULL)
          if (m_experimentParams->getInt(islandIdx, SGP_EXP_PAR_BLOCK_IDX_TOUR + SGP_TOUR_PROB_EP_TOUR_SIZE, islandLimit))
            useLimit = static_cast<uint>(islandLimit);        
        useLimit = std::min<uint>(useLimit, islandSize);
        useLimit = std::min<uint>(useLimit, islandSpaceLeft);
        if (useLimit > 0) 
        {
          islandId = stringToIntDef(islandName, 0);
          genRandomGroup(output, input, useLimit, islandId, islandItems, islandData);
          assert(!output.empty());  
          break;
        }  
      }  
    } while(true);   
  } 
}

// select random items from a given island
void sgpGaOperatorSelectTourProb::genRandomGroupFromIsland(sgpTournamentGroup &output, const sgpGaGeneration &input, 
  uint groupLimit, uint targetLimit, uint allocatedCount,
  uint islandId, const scDataNode &islandItems, const scDataNode &islandData)
{
  uint useLimit;
  uint islandSpaceLeft;

  output.clear();

  islandSpaceLeft = targetLimit;

  if (islandSpaceLeft > allocatedCount)
    islandSpaceLeft = islandSpaceLeft - allocatedCount;   
  else  
    islandSpaceLeft = 0;
    
  if (islandSpaceLeft > 0) {                
    useLimit = SC_MIN(groupLimit, islandSpaceLeft);
    
    if (useLimit > 0) 
    {
      genRandomGroup(output, input, useLimit, islandId, islandItems, islandData);
      assert(!output.empty());  
    }  
  }  
}

void sgpGaOperatorSelectTourProb::genRandomGroup(sgpTournamentGroup &output, const sgpGaGeneration &input, 
  uint limit, uint islandId, const scDataNode &islandItems, const scDataNode &islandData)
{
  if (selectTourGroupSelTypeByShape(islandId))
    genRandomGroupByShape(output, input, limit, 
      islandData[ISLAND_DATA_IDX_SHAPE_COLN], 
      islandData[ISLAND_DATA_IDX_SHAPE_DISTRIB], 
      islandData[ISLAND_DATA_IDX_SHAPE_DISTRIB_SUM].getAsDouble()
    );
  else  
    genRandomGroupOnIsland(output, input, islandItems, limit);
}

void sgpGaOperatorSelectTourProb::genRandomGroupOnIsland(sgpTournamentGroup &output, const sgpGaGeneration &input, 
  const scDataNode &islandItems, uint limit)
{
  genRandomGroupFromBlockByIds(output, input, islandItems, limit);
}

void sgpGaOperatorSelectTourProb::updateIslandAllocs(uint islandId, uint groupSize, scDataNode &newItems)
{
  scString islandName(toString(islandId));
  if (!newItems.hasChild(islandName))
    newItems.addChild(islandName, new scDataNode(groupSize));
  else  
    newItems.setUInt(islandName, newItems.getUInt(islandName) + groupSize);
}

void sgpGaOperatorSelectTourProb::genRandomGroupFromBlock(sgpTournamentGroup &output, const sgpGaGeneration &input, uint limit)
{
  uint blockSize = input.size();
  output.clear();
  while(output.size() < limit) 
    output.insert(randomInt(0, blockSize - 1));
}

void sgpGaOperatorSelectTourProb::genRandomGroupFromBlockByIds(sgpTournamentGroup &output, const sgpGaGeneration &input, 
  const scDataNode &idList, uint limit)
{
  uint itemIdx, cnt;
  uint blockSize = idList.size();
  
  output.clear();
  cnt = 0;
  
  if (!idList.empty())
  while(cnt < limit) 
  {
    itemIdx = randomUInt(0, blockSize - 1);
    itemIdx = idList.getUInt(itemIdx);
    output.insert(itemIdx);
    cnt++;
  }  
}

// generate tournament group using distance function based on position in population
void sgpGaOperatorSelectTourProb::genRandomGroupWithDistance(sgpTournamentGroup &output, const sgpGaGeneration &input, uint limit)
{
  output.clear();
  if (!m_distanceFunction) { 
    while(output.size() < limit) 
      output.insert(randomInt(0, input.size() - 1));
  } else {      
    output.insert(randomInt(0, input.size() - 1));
    uint firstPos = *output.begin();
    uint secondPos;
    double dist;
    double distRatio;
    
    while(output.size() < limit) {
      secondPos = static_cast<uint>(randomInt(0, input.size() - 1));
      m_distanceFunction->calcDistanceRel(firstPos, secondPos, dist);
      // m_distanceFactor - maximum useful range, all farther entities are ignored
      dist = dist / m_distanceFactor; 
      if (dist > 1.0) 
        dist = 1.0;
      distRatio = 1.0 - 1.0 / (2.0 - dist);
      if (randomFlip(distRatio))      
        output.insert(secondPos);
    }  
  }   
}

// return <true> if tournament type is calculated basing on probability
bool sgpGaOperatorSelectTourProb::getStaticTourProbForIsland(uint islandId, double &staticTourProb)
{
  bool res = false;
  double param;
  
  staticTourProb = 0.0;
  if (m_experimentParams != SC_NULL)
    if (m_experimentParams->getDouble(islandId, SGP_EXP_PAR_BLOCK_IDX_TOUR + SGP_TOUR_PROB_EP_TOUR_TYPE, param))
    {
      staticTourProb = param;
      res = true;
    }  
  
  return res;    
}

// returns <true> if group contents selection should be based on shape
// default: <true>
bool sgpGaOperatorSelectTourProb::selectTourGroupSelTypeByShape(uint islandId)
{
  const double MARGIN_VALUE = 0.1;
  bool res = true;
  double param, probValue;
  
  probValue = 0.0;
  if (m_experimentParams != SC_NULL)
    if (m_experimentParams->getDouble(islandId, SGP_EXP_PAR_BLOCK_IDX_TOUR + SGP_TOUR_PROB_EP_TOUR_GROUP_TYPE_PROB, param))
    {
      probValue = param;
      if (probValue <= MARGIN_VALUE) 
        res = false;
      else if (probValue >= 1.0 - MARGIN_VALUE)
        res = true;
      else {
        probValue = (probValue - MARGIN_VALUE) / (1.0 - 2.0 * MARGIN_VALUE);
        res = randomFlip(probValue);
      }          
    }  
  
  return res;    
}

void sgpGaOperatorSelectTourProb::execute(sgpGaGeneration &input, sgpGaGeneration &output, uint limit)
{
  if (m_islandLimit > 0) 
    executeOnIslandList(input, output, limit, 0, m_islandLimit - 1);
  else 
    executeOnAll(input, output, limit);
}

void sgpGaOperatorSelectTourProb::executeOnIslandList(sgpGaGeneration &input, sgpGaGeneration &output, uint limit, uint firstIslandId, uint lastIslandId)
{
  scDataNode islandItems;
  scDataNode islandData;

  prepareIslands(input, firstIslandId, lastIslandId + 1, limit, islandItems, islandData);
  scString islandName;
  uint islandLimit;
  
  for(uint i=firstIslandId, epos = lastIslandId + 1; i != epos; i++)
  {
    islandName = toString(i);
    if (islandItems.hasChild(islandName) && islandData.hasChild(islandName))
    {
      islandLimit = islandData[islandName].getUInt(ISLAND_DATA_IDX_SIZE);
      executeOnIsland(input, output, islandLimit, i, islandItems[islandName], islandData[islandName]);
    }  
  }    
}

void sgpGaOperatorSelectTourProb::executeOnIsland(sgpGaGeneration &input, sgpGaGeneration &output, uint limit, uint islandId, 
  const scDataNode &islandItems, scDataNode &islandData)
{
  sgpTournamentGroup group, workGroup;
  uint tourSize = std::min<uint>(input.size(), m_tournamentSize);
  uint addedSize;

#if defined(TRACE_MATCH_PROB) 
  Counter::reset("gx-tour-match-count");
  Counter::reset("gx-tour-match-failed-count");
  Counter::inc("gx-tour-step-no");
#endif

#ifdef GAOPER_TRACE
  Log::addDebug("Tournament-prob started");  
#endif

#if defined(TRACE_MATCH_PROB) || defined(TRACE_ENTITY_BIO)
  sgpTournamentGroup traceUsedItems, traceTestedItems;
  sgpTraceEntityMoveMap moveMap;
#endif
    
  double staticTourProb;  
  bool dynamicTourType;

  uint islandLimit = limit;  
  
  if (m_experimentParams != SC_NULL)
  {
    int tourSizeOnIsland;
    if (m_experimentParams->getInt(islandId, SGP_EXP_PAR_BLOCK_IDX_TOUR + SGP_TOUR_PROB_EP_TOUR_SIZE, tourSizeOnIsland))
      tourSize = static_cast<uint>(tourSizeOnIsland);    
  }    
  
  tourSize = SC_MIN(tourSize, islandItems.size());
  
  addedSize = 0;

  bool canProcess = false;
  if ((tourSize > 0) && canProcessIsland(islandId, islandItems))
    canProcess = true;
  
  if (canProcess) {
    dynamicTourType = getStaticTourProbForIsland(islandId, staticTourProb);
  } else {
    // just for warnings
    dynamicTourType = false;
    staticTourProb = 0.0;
  }

  if (canProcess)
  while(addedSize < islandLimit)
  { 
    genRandomGroupFromIsland(group, input, tourSize, islandLimit, addedSize, islandId, islandItems, islandData);

    if (dynamicTourType && randomFlip(staticTourProb)) {
      workGroup.clear();
      workGroup.insert(sgpGaOperatorSelectTournamentMF::findBestInGroup(input, group, m_objectiveWeights));
    } else {
      runMatchInGroupOnIsland(workGroup, input, group, islandId);
    }  
    
#ifdef TRACE_MATCH_PROB
  Counter::inc("gx-tour-group-no");
#endif

#ifdef TRACE_ENTITY_BIO
    traceHandleMatchResult(group, workGroup, traceTestedItems);
#endif
        
    for(sgpTournamentGroup::const_iterator it = workGroup.begin(), epos = workGroup.end(); it != epos; ++it) {
      if (addedSize < islandLimit) {
        output.insert(input.cloneItem(*it));  
        addedSize++;
#if defined(TRACE_MATCH_PROB) || defined(TRACE_ENTITY_BIO)
        traceItemSelected(*it, output.size() - 1, traceUsedItems, moveMap);
#endif
      }  
    }  
  } // for i            
  
#if defined(TRACE_MATCH_PROB) 
  traceAfterSelectMatchRes(traceUsedItems, traceTestedItems, moveMap);
#endif

#if defined(TRACE_ENTITY_BIO)
  traceAfterSelectBio(traceUsedItems, traceTestedItems, moveMap);
#endif
  
#ifdef GAOPER_TRACE
  Log::addDebug("Tournament-prob ended");  
#endif  
}

bool sgpGaOperatorSelectTourProb::canProcessIsland(uint islandId, const scDataNode &islandItems)
{
  return (!islandItems.empty());
}

void sgpGaOperatorSelectTourProb::executeOnAll(sgpGaGeneration &input, sgpGaGeneration &output, uint limit)
{
  sgpTournamentGroup group, workGroup;
  uint tourSize = std::min<uint>(input.size(), m_tournamentSize);
  uint addedSize = 0;
  scDataNode shapeDistrib;

#if defined(TRACE_MATCH_PROB) 
  Counter::reset("gx-tour-match-count");
  Counter::reset("gx-tour-match-failed-count");
  Counter::inc("gx-tour-step-no");
#endif

#ifdef GAOPER_TRACE
  Log::addDebug("Tournament-prob started");  
#endif

#if defined(TRACE_MATCH_PROB) || defined(TRACE_ENTITY_BIO)
  sgpTournamentGroup traceUsedItems, traceTestedItems;
  sgpTraceEntityMoveMap moveMap;
#endif
    
#ifdef TRACE_TIME
  Timer::reset(TIMER_SHAPE_DETECT);    
  Timer::start(TIMER_SHAPE_DETECT);    
#endif  
  scDataNode shapeCollection;  
  scDataNode islandItems, newIslandItems;
  scDataNode islandData;
  uint islandId;
  
  if (m_islandLimit > 0) 
    prepareIslands(input, input.size(), islandItems, islandData);
  else if (getShapeLimit() > 0)
    prepareShapeCollectionByCluster(input, shapeCollection);  
  else 
    prepareShapeCollectionByObjDistrib(input, SC_NULL, 0, shapeCollection, shapeDistrib, TOUR_PROB_SHAPE_DISTRIB_DECREASE_FACTOR);  
#ifdef TRACE_TIME
  Timer::stop(TIMER_SHAPE_DETECT);    
#endif  

  double shapeDistribSum = shapeDistrib.accumulate(0.0);
  double staticTourProb;  
  bool dynamicTourType;
  
  while(addedSize < limit)
  { 
    // generate random group of selected size
    if (m_islandLimit > 0) {
      genRandomGroupAndIsland(group, input, tourSize, islandItems, islandData, islandId, newIslandItems);
      dynamicTourType = getStaticTourProbForIsland(islandId, staticTourProb);

      if (dynamicTourType && randomFlip(staticTourProb)) {
        workGroup.clear();
        workGroup.insert(sgpGaOperatorSelectTournamentMF::findBestInGroup(input, group, m_objectiveWeights));
      } else {
        runMatchInGroupOnIsland(workGroup, input, group, islandId);
      }  
      updateIslandAllocs(islandId, workGroup.size(), newIslandItems);
    } else { 
      genRandomGroupByShape(group, input, tourSize, shapeCollection, shapeDistrib, shapeDistribSum);
      runMatchInGroup(workGroup, input, group);
    }  
    
#ifdef TRACE_MATCH_PROB
  Counter::inc("gx-tour-group-no");
#endif

#ifdef TRACE_ENTITY_BIO
    traceHandleMatchResult(group, workGroup, traceTestedItems);
#endif
        
    for(sgpTournamentGroup::const_iterator it = workGroup.begin(), epos = workGroup.end(); it != epos; ++it) {
      if (addedSize < limit) {
        output.insert(input.cloneItem(*it));  
        addedSize++;
#if defined(TRACE_MATCH_PROB) || defined(TRACE_ENTITY_BIO)
        traceItemSelected(*it, output.size() - 1, traceUsedItems, moveMap);
#endif
      }  
    }  
  } // for i            
  
#if defined(TRACE_MATCH_PROB) 
  traceAfterSelectMatchRes(traceUsedItems, traceTestedItems, moveMap);
#endif

#if defined(TRACE_ENTITY_BIO)
  traceAfterSelectBio(traceUsedItems, traceTestedItems, moveMap);
#endif
  
#ifdef GAOPER_TRACE
  Log::addDebug("Tournament-prob ended");  
#endif  
}

void sgpGaOperatorSelectTourProb::traceItemSelected(uint inputIdx, uint outputIdx, 
  sgpTournamentGroup &traceUsedItems, sgpTraceEntityMoveMap &moveMap)
{
  traceUsedItems.insert(inputIdx);
#ifdef TRACE_ENTITY_BIO
  moveMap.insert(std::make_pair<uint, uint>(sgpEntityTracer::getEntityHandle(inputIdx), outputIdx));
#endif  
}  

void sgpGaOperatorSelectTourProb::traceHandleMatchResult(const sgpTournamentGroup &inputGroup, const sgpTournamentGroup &outputGroup, 
  sgpTournamentGroup &traceTestedItems)
{
  scDataNode traceNodeWinners, traceNodeLoosers;
  sgpTournamentGroup traceLoosers;

  for(sgpTournamentGroup::const_iterator it = outputGroup.begin(), epos = outputGroup.end(); it != epos; ++it) {
    traceTestedItems.insert(*it);
  }  
  traceLoosers.clear();
  for(sgpTournamentGroup::const_iterator it = inputGroup.begin(), epos = inputGroup.end(); it != epos; ++it) {
    if (outputGroup.find(*it) == outputGroup.end()) {
      traceLoosers.insert(*it);
    }  
  }  
  traceNodeWinners.clear();
  for(sgpTournamentGroup::const_iterator it = outputGroup.begin(), epos = outputGroup.end(); it != epos; ++it) {
    traceNodeWinners.addChild(new scDataNode(*it));  
  }
  traceNodeLoosers.clear();
  for(sgpTournamentGroup::const_iterator it = traceLoosers.begin(), epos = traceLoosers.end(); it != epos; ++it) {
    traceNodeLoosers.addChild(new scDataNode(*it));  
  }
#ifdef TRACE_ENTITY_BIO
  sgpEntityTracer::handleMatchResult(traceNodeWinners, traceNodeLoosers);
#endif  
}

void sgpGaOperatorSelectTourProb::traceAfterSelectMatchRes(sgpGaGeneration &input, const sgpTournamentGroup &traceUsedItems)  
{
    for(uint i = 0, epos = input.size(); i != epos; ++i) {
      if (traceUsedItems.find(i) == traceUsedItems.end()) {
        traceTournamentFailedFor(input, i);
      }
    }    
}

void sgpGaOperatorSelectTourProb::traceAfterSelectBio(
  const sgpTournamentGroup &traceUsedItems, 
  const sgpTournamentGroup &traceTestedItems,
  const sgpTraceEntityMoveMap &moveMap
)  
{
#ifdef TRACE_ENTITY_BIO
    for(uint i = 0, epos = input.size(); i != epos; ++i) {
      if (traceUsedItems.find(i) == traceUsedItems.end()) {
        if (!sgpEntityTracer::hasBufferedActionFor(i)) {
          if (traceTestedItems.find(i) == traceTestedItems.end()) {
            sgpEntityTracer::handleEntityDeathBuf(i, eecTourNotSel, "tour-not-sel");
          } else {
            sgpEntityTracer::handleEntityDeathBuf(i, eecTourLost, "tour-match-lost");
          }  
        }
      } // if not used  
    } // for
    
   uint lastHandle = ET_UNKNOWN_ENTITY_IDX;
   for(sgpTraceEntityMoveMap::const_iterator it = moveMap.begin(), epos = moveMap.end(); it != epos; ++it) {
      if ((it->first != lastHandle) && !sgpEntityTracer::hasBufferedActionForHandle(it->first)) {
        sgpEntityTracer::handleEntityMovedByHandleBuf(it->first, it->second, "tour-prob");    
      } else {
        sgpEntityTracer::handleEntityCopiedByHandleBuf(it->first, it->second, "tour-prob");    
      }  
      lastHandle = it->first;
   }   
#endif   
}
  
void sgpGaOperatorSelectTourProb::traceTournamentFailedFor(sgpGaGeneration &input, uint itemIndex)
{
  if (m_tournamentFailedTracer)
    m_tournamentFailedTracer->execute(input, itemIndex);
}

void sgpGaOperatorSelectTourProb::runMatchInGroup(sgpTournamentGroup &output, const sgpGaGeneration &input, const sgpTournamentGroup &group)
{
  intRunMatchInGroup(output, input, group, TOUR_PROB_EXTREME_GRAVITY_RATIO, TOUR_USE_PROB_ONLY_FOR_NON_DOMIN, TOUR_MAX_GROUP_SIZE);
}

void sgpGaOperatorSelectTourProb::runMatchInGroupOnIsland(sgpTournamentGroup &output, const sgpGaGeneration &input, 
  const sgpTournamentGroup &group, uint islandId)
{
  double gravity;
  uint groupLimit;

  groupLimit = TOUR_MAX_GROUP_SIZE;
  gravity = TOUR_PROB_EXTREME_GRAVITY_RATIO;

  if (m_experimentParams != SC_NULL) {
    double gravityParam;
    int groupLimitParam;
    
    if (m_experimentParams->getDouble(islandId, SGP_EXP_PAR_BLOCK_IDX_TOUR + SGP_TOUR_PROB_EP_TOUR_REL_PRES, gravityParam))
    {
      gravity = gravityParam;
    } 
    
    if (m_experimentParams->getInt(islandId, SGP_EXP_PAR_BLOCK_IDX_TOUR + SGP_TOUR_PROB_EP_MATCH_OUT_LIMIT, groupLimitParam))
    {
      groupLimit = static_cast<uint>(groupLimitParam);
    } 
  }
  intRunMatchInGroup(output, input, group, gravity, TOUR_USE_PROB_ONLY_FOR_NON_DOMIN, groupLimit);
}

void sgpGaOperatorSelectTourProb::intRunMatchInGroup(sgpTournamentGroup &output, const sgpGaGeneration &input, const sgpTournamentGroup &group,
  double gravity, bool probOnlyForNonDomin, uint outSizeLimit)
{
  uint first;
  sgpTournamentGroup workGroup = group; // group of entities where we are choosing winners
  int matchRes;
  uint stepCnt = 0;
  uint totalCnt, equalCnt;
  bool onlyDiff = false;

  do {
    output.clear();
    getRandomElement(first, workGroup);
    if (workGroup.size() == 1) {
      output.insert(first);
      break;
    } else {  
      equalCnt = totalCnt = 0;
      for(sgpTournamentGroup::const_iterator it = workGroup.begin(), epos = workGroup.end(); it != epos; ++it)
      {
        if (*it != first) {
          matchRes = runMatch(input, first, *it, gravity, probOnlyForNonDomin);

          if (matchRes == 0) {
            equalCnt++;
          }

          totalCnt++;

          if (matchRes <= 0) {
            output.insert(*it);
          }
              
          if (matchRes >= 0) {
            output.insert(first);
          }
        } 
      } 
    }

    if ((!output.empty()) && (totalCnt == equalCnt))
    {
      getRandomElement(first, output);
      output.clear();
      output.insert(first);
      break;
    }

    workGroup = output;
    stepCnt++;
  // while output is too big we need to choose inside workGroup
  } while ((output.size() > outSizeLimit) || (stepCnt < TOUR_LOOP_LIMIT));    

  if (stepCnt >= TOUR_LOOP_LIMIT)
    perf::Log::addText("Tournament match too long", lmlWarning, "OSTP01");
}

// Run match for two entities.
// PT - probability of winning of A against B
//   PT = SUM(LRK(i)*WR(i), i=1..n)/SUM(WR(i))
// Returns:
//   -1 if first is worse
//   +1 if first is better
//    0 if both are equal
// Scaling:
// 
int sgpGaOperatorSelectTourProb::runMatch(const sgpGaGeneration &input, uint first, uint second, double gravity, bool useProbOnlyForNonDomin)
{
  int res = 0;
  sgpFitnessValue fitVectorFirst, fitVectorSecond;

  input.at(first).getFitness(fitVectorFirst);
  input.at(second).getFitness(fitVectorSecond);
  
  if (useProbOnlyForNonDomin)
  {
    res = fitVectorFirst.compare(fitVectorSecond);
    if (res != 1)
    {
      if (res > 0)
        res = 1;
      else if (res < 0)
        res = -1;
      else
        res = 0;
      return res;       
    }  
  }

  const double C_DIV_HELPER = 0.000001;
  const double MAX_LR = 100.0;
  const double MIN_LR = 1.0/MAX_LR;
  
  double objA, objB, rr, lr, lk, lrkVal, pt, sumLrk, weightSum;  
  double objWeight;

  
#ifdef TRACE_MATCH_PROB
  Counter::inc("gx-tour-match-no");
  Counter::inc("gx-tour-match-count");
#endif
  
  sumLrk = 0.0;
  weightSum = 0.0;
  for(uint i=1,epos=m_objectiveWeightsRescaled.size(); i!=epos; i++)
  {
    objA = fitVectorFirst[i];
    objB = fitVectorSecond[i];
    // if objectives equal - skip them (improves speed of search)
    if (objA == objB) 
      continue;
      
    objWeight = getObjectiveWeight(input, first, second, i, m_objectiveWeightsRescaled[i]);
      
    // calc rr - how much objA is better than objB  
    if (objA < 0.0) {
      rr = (-C_DIV_HELPER + objB) / (-C_DIV_HELPER + objA);
    } else { 
      rr = (C_DIV_HELPER + objA) / (C_DIV_HELPER + objB);
    }  
    lr = fabs(rr);

    //-------------------
    if (lr > MAX_LR)
      lr = MAX_LR;
    else if (lr < MIN_LR)
      lr = MIN_LR; 
        
    if (lr >= 1.0)
    // >1.0 => A is better
      lk = 1.0 - 1.0 / lr;
    else 
    // 0..1 => B is better
      lk = -1.0 + lr;  
      
    // calculate relation related to avg(top); lk is (-1,+1)
    // if any objX is close to avg(top) then lk will be 2x closer to extreme value
    // otherwise it will stay as-is
    // this will increase local searches - more pressure for close-to-best solutions

    // here lk is -1..+1
    if (gravity < 0.0)
    {
      lk = lk * (1.0 / pow(2.0, 10.0*((-gravity) - 0.5)));
    } else {
      if (lk < 0.0)
        lk = lk - (lk - (-1.0)) * gravity; //TOUR_PROB_EXTREME_GRAVITY_RATIO;
      else 
        lk = lk + (1.0 - lk) * gravity; //TOUR_PROB_EXTREME_GRAVITY_RATIO;  
    }  
    lk = SC_MAX(-1.0, lk);  
    lk = SC_MIN(1.0, lk);  
          
    //-------------------
    lrkVal = (lk + 1.0) / 2.0; 
    sumLrk += lrkVal * objWeight;
    weightSum += objWeight;
  }
  
  if (equDouble(weightSum, 0.0))
    weightSum = 1.0;
     
  pt = sumLrk / weightSum;
  
  if (randomFlip(pt))
    res++;
    
  if (randomFlip(1.0 - pt))
    res--;

#ifdef TRACE_MATCH_PROB
  int compareRes = fitVectorFirst.compare(fitVectorSecond);

  if ((compareRes * res < 0) && (std::abs(compareRes != 1)))
    Counter::inc("gx-tour-match-failed-count");
    
#ifdef TRACE_MATCH_TO_LOG    
  traceMatchProb(Counter::getTotal("gx-tour-step-no"), Counter::getTotal("gx-tour-group-no"), 
    Counter::getTotal("gx-tour-match-no"),
    fitVectorFirst, fitVectorSecond, sumLrk, pt, res, compareRes);    
#endif    
  if (
      ((fitVectorFirst[23] > -2000.0) || (fitVectorSecond[23] > -2000.0))
      &&
      (
        ((fitVectorFirst[23] > fitVectorSecond[23]) && (res < 0))
        ||
        ((fitVectorFirst[23] < fitVectorSecond[23]) && (res > 0))
      )  
     ) 
  {
  // match failed & worse item won
    if (m_matchFailedTracer)
      m_matchFailedTracer->execute(Counter::getTotal("gx-tour-step-no"), Counter::getTotal("gx-tour-group-no"), 
        Counter::getTotal("gx-tour-match-no"), input, first, second, res);
  }  
#endif
    
  return res;
}

double sgpGaOperatorSelectTourProb::getObjectiveWeight(const sgpGaGeneration &input, uint first, uint second,
  uint objIndex, double defValue)
{
  double res = defValue;
  double weight;
  uint islandId;
  
  const sgpEntityBase *firstWorkInfo = dynamic_cast<const sgpEntityBase *>(input.atPtr(first));
  
  if ((m_experimentParams != SC_NULL) && m_islandTool->getIslandId(*firstWorkInfo, islandId))
    if (m_experimentParams->getDouble(islandId, SGP_EXP_PAR_BLOCK_IDX_TOUR + SGP_TOUR_PROB_EP_DYN_ERROR_OBJS + objIndex, weight))
      res = weight;        
              
  return res;
}

void sgpGaOperatorSelectTourProb::traceMatchProb(ulong64 stepNo, ulong64 groupNo, 
    ulong64 matchNo,
    const sgpFitnessValue &fitVectorFirst, const sgpFitnessValue &fitVectorSecond, 
    double sumLrk, double matchProb, int matchRes, int compareRes)
{
  scDataNode logLine, logLineA, logLineB;
  logLine.addChild("step-no", new scDataNode(stepNo));
  logLine.addChild("group-no", new scDataNode(groupNo));
  logLine.addChild("match-no", new scDataNode(matchNo));
  logLine.addChild("side", new scDataNode("A"));
  logLine.addChild("comp-val", new scDataNode(compareRes));
  logLine.addChild("match-res", new scDataNode(matchRes));
  logLine.addChild("match-prob", new scDataNode(matchProb));
  logLine.addChild("t-sum-lrk", new scDataNode(sumLrk));
  logLine.addChild("match-failed", new scDataNode((matchRes*compareRes < 0)?1:0));
  logLineA = logLine;
  logLineB = logLine;
  logLineB["side"].setAsString("B");
  for(uint i=1, epos=fitVectorFirst.size(); i!=epos; i++) {
    logLineA.addChild(scString("fit-")+toString(i), new scDataNode(fitVectorFirst[i]));
    logLineB.addChild(scString("fit-")+toString(i), new scDataNode(fitVectorSecond[i]));
  }  
  
  m_experimentLog->addLineToCsvFile(logLineA, "match_res", "csv");
  m_experimentLog->addLineToCsvFile(logLineB, "match_res", "csv");
}


