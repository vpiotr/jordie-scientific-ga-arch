/////////////////////////////////////////////////////////////////////////////
// Name:        GaOperatorBasic.h
// Project:     scLib
// Purpose:     Basic operators for GA algorithm
// Author:      Piotr Likus
// Modified by:
// Created:     22/02/2009
/////////////////////////////////////////////////////////////////////////////

#ifndef _GAOPERATORBASIC_H__
#define _GAOPERATORBASIC_H__

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

#include "base/strcomp.h"

//sc
#include "sc/utils.h"
#include "sc/txtf/CsvWriter.h"
#include "sc/events/Events.h"

//sgp
#include "sgp/GaEvolver.h"

// ----------------------------------------------------------------------------
// Simple type definitions
// ----------------------------------------------------------------------------
typedef std::set<uint> sgpTournamentGroup;

// ----------------------------------------------------------------------------
// Forward class definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------
const double SGP_GA_DEF_OPER_PROB_MUTATE = 0.033;
const double SGP_GA_DEF_OPER_PROB_XOVER = 0.06;
const double SGP_GA_DEF_SPECIES_THRESHOLD = 0.5;
const uint SGP_GA_DEF_TOURNAMENT_SIZE = 4;
const double SGP_GA_DEF_SELECT_DIST_FACTOR = 0.33;

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------
typedef std::auto_ptr<strDiffFunctor> strDiffFunctorGuard;

class sgpGenomeChangedTracer: public scInterface {
public:
   virtual void execute(const sgpGaGeneration &input, uint itemIndex, const scString &sourceName) = 0;  
};

// performs initialization using "init entity" operator
class sgpGaOperatorInitGen: public sgpGaOperatorInitBase {
  typedef sgpGaOperatorInitBase inherited;
public:
  // construct
  sgpGaOperatorInitGen();
  virtual ~sgpGaOperatorInitGen();
  // properties
  void setOperatorInitEntity(sgpGaOperatorInitEntity *value);
  sgpGaOperatorInitEntity *getOperatorInitEntity();
  virtual void setMetaInfo(const sgpGaGenomeMetaList &list);
  // run
  virtual void execute(uint limit, sgpGaGeneration &newGeneration);
protected:
  virtual void invokeEntityReady(sgpEntityBase *entity) {}
private:
  sgpGaOperatorInitEntity *m_operInitEntity;
};

class sgpGaGenomeCompareToolForGa: public sgpGaGenomeCompareTool {
public:
  sgpGaGenomeCompareToolForGa():sgpGaGenomeCompareTool() {}
  virtual ~sgpGaGenomeCompareToolForGa() {};
  virtual void setMaxLength(uint value);
  virtual void setMetaInfo(const sgpGaGenomeMetaList &list);
  virtual double calcGenomeDiff(const sgpGaGeneration &newGeneration, uint first, uint second);
  virtual double calcGenomeDiff(const sgpGaGeneration &newGeneration, uint first, uint second, uint genNo);
protected:  
  double calcRangedDiff(const scDataNode &var1, const scDataNode &var2, const scDataNode &minValue, const scDataNode &maxValue);
protected:  
  strDiffFunctorGuard m_diffFunct;
  uint m_genomeSize;
};

class sgpOperatorMutateBase: public sgpGaOperatorMutate {
public:
  virtual void getCounters(scDataNode &output) = 0;
};
 
class sgpGaOperatorMutateBasic: public sgpOperatorMutateBase {
public:
  sgpGaOperatorMutateBasic();
  virtual ~sgpGaOperatorMutateBasic();
  virtual void execute(sgpGaGeneration &newGeneration);

  virtual double getProbability();
  void setProbability(double aValue);
  virtual void setMetaInfo(const sgpGaGenomeMetaList &list);
  virtual void getCounters(scDataNode &output) {}
protected:  
  inline void mutateVar(const sgpGaGenomeMetaInfo &metaInfo, uint offset, scDataNode &var);
  virtual void invokeNextEntity() {} 
  virtual bool processGenome(uint entityIndex, sgpGaGenome &genome);
  virtual void beforeExecute(sgpGaGeneration &newGeneration);
  virtual double getEntityChangeProb();
  virtual uint getChangePointLimit();
protected:
  double m_probability;  
  uint m_genomeSize;
};

// mutate operator with yield support
class sgpGaOperatorMutateWithYield: public sgpGaOperatorMutateBasic {
  typedef sgpGaOperatorMutateBasic inherited;
public:
  // construct
  sgpGaOperatorMutateWithYield();
  virtual ~sgpGaOperatorMutateWithYield();
  // properties
  void setYieldSignal(scSignal *value);
protected:
  virtual void invokeNextEntity();
private:
  scSignal *m_yieldSignal;
};

class sgpGaOperatorEvaluateBasic: public sgpGaOperatorEvaluate {
public:
  void setFitnessFunc(sgpFitnessFunction *value);
  void setOperatorMonitor(sgpGaOperatorEvalMonitorIntf *value);
  virtual bool execute(uint stepNo, bool isNewGen, sgpGaGeneration &generation);
protected: 
  virtual bool evaluateAll(sgpGaGeneration *generation);
  virtual bool evaluateRange(sgpGaGeneration *generation, int first, int last);
  virtual void invokeNextEntity() {}
protected:  
  sgpFitnessFunction *m_fitnessFunc;
  sgpGaOperatorEvalMonitorIntf *m_operatorMonitor;
};

class sgpGaOperatorEvaluateWithYield: public sgpGaOperatorEvaluateBasic {
  typedef sgpGaOperatorEvaluateBasic inherited;
public:
  // construct
  sgpGaOperatorEvaluateWithYield();
  virtual ~sgpGaOperatorEvaluateWithYield();
  // properties
  void setYieldSignal(scSignal *value);
protected:
  virtual void invokeNextEntity();
private:
  scSignal *m_yieldSignal;
};

class sgpGaOperatorSelectBasic: public sgpGaOperatorSelect {
public:
  virtual void execute(sgpGaGeneration &input, sgpGaGeneration &output, uint limit);
};

class sgpGaOperatorSelectPrec: public sgpGaOperatorSelect {
public:
  virtual void execute(sgpGaGeneration &input, sgpGaGeneration &output, uint limit);
};

class sgpGaOperatorSelectTournament: public sgpGaOperatorSelect {
public:
  // construct
  sgpGaOperatorSelectTournament();
  virtual ~sgpGaOperatorSelectTournament() {}
  // properties
  uint getTournamentSize(); 
  void setTournamentSize(uint value);
  void setTemperature(double value);
  // run
  virtual void execute(sgpGaGeneration &input, sgpGaGeneration &output, uint limit);  
protected:
  inline void genRandomGroup(sgpTournamentGroup &output, const sgpGaGeneration &input, uint limit);  
  inline void getRandomElement(uint &output, const sgpTournamentGroup &group);  
protected:
  uint m_tournamentSize;  
  double m_temperature;
};

// tournament that works on middle values of fitness objectives
// no normalization is required 
// uses weights to prioritize objectives (most important are compared first)
class sgpGaOperatorSelectTournamentMF: public sgpGaOperatorSelectTournament {
public:
  // construct
  sgpGaOperatorSelectTournamentMF();
  virtual ~sgpGaOperatorSelectTournamentMF();
  // properties
  void setObjectiveWeights(const sgpWeightVector &value);
  // run
  virtual void execute(sgpGaGeneration &input, sgpGaGeneration &output, uint limit);
  static uint findBestInGroup(const sgpGaGeneration &input, const sgpTournamentGroup &group, const sgpWeightVector &weights);
protected:
protected:  
  sgpWeightVector m_objectiveWeights;
};

class sgpMatchFailedTracer: public scInterface {
public:
   virtual void execute(ulong64 stepNo, ulong64 groupNo, ulong64 matchNo, 
     const sgpGaGeneration &input, uint first, uint second, int matchRes) = 0;  
};

class sgpTournamentFailedTracer: public scInterface {
public:
   virtual void execute(const sgpGaGeneration &input, uint itemIndex) = 0;  
};

class sgpGaOperatorEliteBasic: public sgpGaOperatorElite {
public:
  virtual void execute(sgpGaGeneration &input, sgpGaGeneration &output, uint limit);
};

class sgpGaOperatorEliteByObjective: public sgpGaOperatorElite {
public:
  sgpGaOperatorEliteByObjective(uint primaryObjectiveIndex = 0): sgpGaOperatorElite(), m_primaryObjectiveIndex(primaryObjectiveIndex) {}
  void setPrimaryObjectiveIndex(uint value);
  virtual void execute(sgpGaGeneration &input, sgpGaGeneration &output, uint limit);
protected:
  uint m_primaryObjectiveIndex;  
};

class sgpGaOperatorEliteByWeights: public sgpGaOperatorElite {
public:
  sgpGaOperatorEliteByWeights();
  virtual ~sgpGaOperatorEliteByWeights() {}
  void setObjectiveWeights(const sgpWeightVector &value);
  virtual void execute(sgpGaGeneration &input, sgpGaGeneration &output, uint limit);
protected:
  sgpWeightVector m_objectiveWeights;  
};

// monitored operator
class sgpOperatorXOverReporting: public sgpGaOperatorXOver {
public:
  virtual void getCounters(scDataNode &output) = 0;
};

class sgpGaOperatorXOverBasic: public sgpOperatorXOverReporting {
public:
  sgpGaOperatorXOverBasic();
  virtual ~sgpGaOperatorXOverBasic();
  virtual void execute(sgpGaGeneration &newGeneration);
  void setProbability(double aValue);
  virtual void setMetaInfo(const sgpGaGenomeMetaList &list);
  virtual void getCounters(scDataNode &output) {}
protected:
  virtual bool crossGenomes(sgpGaGeneration &newGeneration, uint first, uint second);
  void crossVars(sgpGaGenome &firstGenome, sgpGaGenome &secondGenome, uint genIndex, uint endPoint);
protected:
  double m_probability;  
  uint m_genomeSize;
};


// implements species - if difference between genomes is too big, cross will not be performed
class sgpGaOperatorXOverSpecies: public sgpGaOperatorXOverBasic {
public:
  sgpGaOperatorXOverSpecies() {m_matchThreshold = SGP_GA_DEF_SPECIES_THRESHOLD;};
  virtual ~sgpGaOperatorXOverSpecies() {};
  // 0..1
  void setMatchThreshold(double aValue);
  void setCompareTool(sgpGaGenomeCompareTool *tool);
  // run
  virtual void init();
protected:
  virtual bool crossGenomes(sgpGaGeneration &newGeneration, uint first, uint second);
  virtual double calcGenomeDiff(const sgpGaGenome &genome1, const sgpGaGenome &genome2);
  double calcRangedDiff(const scDataNode &var1, const scDataNode &var2, const scDataNode &minValue, const scDataNode &maxValue);
  virtual void setMetaInfo(const sgpGaGenomeMetaList &list);
  virtual void checkCompareTool();
protected:
  double m_matchThreshold; 
  sgpGaGenomeCompareToolGuard m_compareTool;
};

// ----------------------------------------------------------------------------
// Global function definitions
// ----------------------------------------------------------------------------
// select index of item basing on it's probability
uint selectProbItem(const scDataNode &list);
void selectProbItem(const scDataNode &list, double probSum, std::vector<uint> &output, uint limit);

#endif // _GAOPERATORBASIC_H__
