/////////////////////////////////////////////////////////////////////////////
// Name:        GaOperatorSelectTourProb.h
// Project:     scLib
// Purpose:     Select operator - tournament with probability of winning
// Author:      Piotr Likus
// Modified by:
// Created:     29/12/2009
/////////////////////////////////////////////////////////////////////////////
#ifndef _GAOPERATORSELECTTOURPROB_H__
#define _GAOPERATORSELECTTOURPROB_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file GaOperatorSelectTourProb.h
///
/// File description

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
//std
#include <map>
#include <set>

//sc
#include "sc/dtypes.h"
//sgp
#include "sgp/GaOperatorBasic.h"
#include "sgp/ExperimentLog.h"
#include "sgp/EntityIslandTool.h"

// ----------------------------------------------------------------------------
// Simple type definitions
// ----------------------------------------------------------------------------
typedef std::multimap<uint, uint> sgpTraceEntityMoveMap;

// ----------------------------------------------------------------------------
// Forward class definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------
// Tournament that compares objectives with a calculated probability.
// More important objective gives more probability of winning.
// Obj(A,2) - entity A, objective with index = 2
// Objective raw rating:
//   OA = Obj(A,2)
//   OB = Obj(B,2)
//   C = 0.00001 
// RR(x,y,z) - is a rating how much x is better than y in the given objective z
//   If OA < 0 
//     RR(A,B,2) = (-C + OB)/(-C + OA)
//   Else 
//     RR(A,B,2) = (C + OA)/(C + OB)
// Value of RR(A,B) can be 0.0 - +inf
// If RR < 1.0 it means A is worse then B on a selected objective
// If RR > 1.0 it means A is better then B
// n = number of objectives
// lr = log10(RR)
// ls = sign(lr), if >=0 then +1 
// lk = 1*ls - 1/(1*ls + lr)
//   where lk can be (-1.0, +1.0)
// When RR is >= 1.0 for all objectives then A is better or equal to B  
// PW - probability of winning of A against B
// MAXW - max weigth value (=100)
// W(i) - weight of objective i
// PT - total probability of winning of A against B
// LRK - rescaled LK (0.0, 1.0)
// LRK = (LK + 1.0) / 2.0
// WR(i) - rescaled W(i) (1.0 -> MAXW+1)
//   WR(i) = 1 + MAXW - W(i)
// PT - probability of winning of A against B
//   PT = SUM(LRK(i)*WR(i), i=1..n)/SUM(WR(i))
// 
// Tournament in group is performed as follows:
// - select random entity A
// - match it against each other entity B
// - if match won by A then B is removed from work group
// - if match not won then B is assigned again to output group
// - result is one or more entities - all are copied to output 
class sgpGaOperatorSelectTourProb: public sgpGaOperatorSelectTournament {
  typedef sgpGaOperatorSelectTournament inherited;
public:
  // construct
  sgpGaOperatorSelectTourProb();
  virtual ~sgpGaOperatorSelectTourProb();
  // properties
  virtual void setObjectiveWeights(const sgpWeightVector &value);
  void setMatchFailedTracer(sgpMatchFailedTracer *tracer);
  void setTournamentFailedTracer(sgpTournamentFailedTracer *tracer);
  void setDistanceFunction(sgpDistanceFunction *value);
  void setDistanceFactor(double value);
  void setStats(const sgpFitnessValue &topAvg);
  void setShapeObjIndex(uint value);
  void setSecShapeObjIndex(uint value);
  void setShapeLimit(uint value);
  uint getShapeLimit();
  void setIslandLimit(uint value);
  void setExperimentParams(const sgpGaExperimentParams *params);
  void setExperimentLog(sgpExperimentLog *value); 
  void setIslandTool(sgpEntityIslandToolIntf *value);
  // run
  virtual void execute(sgpGaGeneration &input, sgpGaGeneration &output, uint limit);
protected:
  virtual void runMatchInGroup(sgpTournamentGroup &output, const sgpGaGeneration &input, const sgpTournamentGroup &group);
  int runMatch(const sgpGaGeneration &input, uint first, uint second, double gravity, bool useProbOnlyForNonDomin);
  //tracing
  void traceMatchProb(ulong64 stepNo, ulong64 groupNo, 
      ulong64 matchNo,
      const sgpFitnessValue &fitVectorFirst, const sgpFitnessValue &fitVectorSecond, 
      double sumLrk, double matchProb, int matchRes, int compareRes);
  void traceTournamentFailedFor(sgpGaGeneration &input, uint itemIndex);
  virtual void genRandomGroupFromBlock(sgpTournamentGroup &output, const sgpGaGeneration &input, uint limit);
  void genRandomGroupOnIsland(sgpTournamentGroup &output, const sgpGaGeneration &input, 
     const scDataNode &islandItems, uint limit);
  virtual void genRandomGroupWithDistance(sgpTournamentGroup &output, const sgpGaGeneration &input, uint limit);
  virtual double getObjectiveWeight(const sgpGaGeneration &input, uint first, uint second,
    uint objIndex, double defValue);
  void prepareShapeCollectionByObj(const sgpGaGeneration &input, scDataNode &output);
  void prepareShapeCollectionByCluster(const sgpGaGeneration &input, scDataNode &output);
  void prepareShapeCollectionByCluster(const sgpGaGeneration &input, 
    const scDataNode *itemList, uint islandId,
    scDataNode &shapeList, scDataNode &shapeDistrib, double decFactor);
  void genRandomGroupByShape(sgpTournamentGroup &output, const sgpGaGeneration &input, uint limit, 
    const scDataNode &shapeCollection, const scDataNode &shapeDistrib, double shapeDistribSum);
  /// \brief prepare shape collection defined using random objective (1 or 2)
  /// Probability of selecting given shape can be specified in island parameters.
  /// \param input collection of entities
  /// \param itemList (optional) defines item set to be selected from
  /// \param islandId island ID
  /// \param shapeList collection [shape-value] -> item-idx-1, item-idx-2; if 2 shape objs - two levels of shapes: [shape-val1][shape-val2] -> item-idx1, item-idx2...
  /// \param shapeDistrib contains probability of selecting shape no. x (one entry for each shape from lvl 1)
  /// \param decFactor specifies how flat is shape probability distribution depending on shape value
  void prepareShapeCollectionByObjDistrib(const sgpGaGeneration &input, 
    const scDataNode *itemList, uint islandId,
    scDataNode &shapeList, scDataNode &shapeDistrib, double decFactor);
  void genRandomGroupAndIsland(sgpTournamentGroup &output, const sgpGaGeneration &input, uint limit, 
    const scDataNode &islandItems, const scDataNode &islandData, uint &islandId, scDataNode &newItems);
  virtual void prepareIslands(const sgpGaGeneration &input, uint totalSize, scDataNode &islandItems, scDataNode &islandData);
  virtual void prepareIslands(const sgpGaGeneration &input, uint beginIslandNo, uint endIslandNo, 
    uint totalSize, scDataNode &islandItems, scDataNode &islandData);
  virtual void prepareIslandMap(const sgpGaGeneration &input, 
    uint beginIslandNo, uint endIslandNo, scDataNode &islandItems);
  void prepareIslandData(const sgpGaGeneration &input, scDataNode &islandItems, scDataNode &islandData);
  void normalizeIslandSize(uint totalSize, const scDataNode &islandItems, scDataNode &islandData);
  void runMatchInGroupOnIsland(sgpTournamentGroup &output, const sgpGaGeneration &input, 
    const sgpTournamentGroup &group, uint islandId);
  void intRunMatchInGroup(sgpTournamentGroup &output, const sgpGaGeneration &input, const sgpTournamentGroup &group,
    double gravity, bool probOnlyForNonDomin, uint outSizeLimit);
  void updateIslandAllocs(uint islandId, uint groupSize, scDataNode &newItems);
  bool getStaticTourProbForIsland(uint islandId, double &staticTourProb);
  bool selectTourGroupSelTypeByShape(uint islandId);
  void traceItemSelected(uint inputIdx, uint outputIdx, 
    sgpTournamentGroup &traceUsedItems, sgpTraceEntityMoveMap &moveMap);
  void traceHandleMatchResult(const sgpTournamentGroup &inputGroup, const sgpTournamentGroup &outputGroup, 
    sgpTournamentGroup &traceTestedItems);
  void traceAfterSelectMatchRes(sgpGaGeneration &input, const sgpTournamentGroup &traceUsedItems);
  void traceAfterSelectBio(const sgpTournamentGroup &traceUsedItems, const sgpTournamentGroup &traceTestedItems,
    const sgpTraceEntityMoveMap &moveMap);  
  void executeOnIslandList(sgpGaGeneration &input, sgpGaGeneration &output, uint limit, uint firstIslandId, uint lastIslandId);
  void executeOnIsland(sgpGaGeneration &input, sgpGaGeneration &output, uint limit, uint islandId, 
    const scDataNode &islandItems, scDataNode &islandData);
  void genRandomGroupFromIsland(sgpTournamentGroup &output, const sgpGaGeneration &input, 
    uint groupLimit, uint targetLimit, uint allocatedCount,
    uint islandId, const scDataNode &islandItems, const scDataNode &islandData);
  void executeOnAll(sgpGaGeneration &input, sgpGaGeneration &output, uint limit);
  virtual bool canProcessIsland(uint islandId, const scDataNode &islandItems);
  void genRandomGroup(sgpTournamentGroup &output, const sgpGaGeneration &input, 
    uint limit, uint islandId, const scDataNode &islandItems, const scDataNode &islandData);
  void genRandomGroupFromBlockByIds(sgpTournamentGroup &output, const sgpGaGeneration &input, 
    const scDataNode &idList, uint limit);
  void prepareShapeDistrib(const scDataNode &shapeListWithPriority, double decFactor, scDataNode &shapeDistrib);
  void selectShapeObj(uint islandId, bool &oneLevel, uint &shapeObjIdx);
protected:  
  // config
  sgpWeightVector m_objectiveWeights;
  sgpWeightVector m_objectiveWeightsRescaled;
  double m_objectiveWeightsRescaledSum;
  double m_distanceFactor;
  uint m_shapeObjIndex;
  uint m_secShapeObjIndex;
  uint m_shapeLimit;
  uint m_islandLimit;
  // core name: name like 'log_*_20090303.csv' with '*' replaced on use
  std::auto_ptr<scTxtWriterValueFormatter> m_formatter;
  sgpMatchFailedTracer *m_matchFailedTracer;
  sgpTournamentFailedTracer *m_tournamentFailedTracer;
  sgpDistanceFunction *m_distanceFunction;
  const sgpGaExperimentParams *m_experimentParams;
  sgpExperimentLog *m_experimentLog;
  sgpEntityIslandToolIntf *m_islandTool;
  // state
  sgpFitnessValue m_statsTopAvg;
};


#endif // _GAOPERATORSELECTTOURPROB_H__
