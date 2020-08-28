/////////////////////////////////////////////////////////////////////////////
// Name:        GaEvolver.h
// Project:     scLib
// Purpose:     Class that finds a better set of values using GA algorithm.
// Author:      Piotr Likus
// Modified by:
// Created:     21/02/2009
/////////////////////////////////////////////////////////////////////////////


#ifndef _GAEVOLVER_H__
#define _GAEVOLVER_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file GaEvolver.h
///
/// GA evolution using genomes encoded in scDataNode arrays.
///
/// Definitions:
/// - variable: single value that represents user algorithm parameter for fitness function
/// - genome: set of variables representing single entity
/// - generation: set of genomes
/// - fitness: overall score assigned to genome
/// 
/// Genome can contain:
/// - a constant value (read-only) specified in meta as "gen_type" = "const"
///     used for genome identification
/// - ranged integer (meta contains "min", "max" values & "gen_type" = ranged)
/// - ranged double (meta contains "min", "max" values & "gen_type" = ranged)
/// - alphastring (value contains a fixed number of chars from a given set of chars, 
///    meta contains "gen_type" = "alpha_string" & "alpha_set" = set of characters)
///
/// To perform operator on genome total virtual genome length is required.
/// Calculated as number of points where mutation can occur.
/// - ranged value: x1
/// - alpha_string: length of string
/// - constant: 0
/// 
/// Mutation, cross-over can occur inside a alpha_string.
/// Later: 
///   Mutation and cross-over probability is equal to:
///     p = base_prob * error_dynamic (annealing)   
/// 
/// World evolution:
/// - generate template
/// - execute GA to build a full generation
/// - (x1) run this generation to have fitness result
/// - run GA on this generation
/// - go to (x1)
///
/// Function evolution:
/// - init:
////  a) set initial genome set + fitness, fill work vars
///   b) set initial genome set + generate population from it
///    - generate random params near given template (minimal dynamic)
///    - take template(s), expand to full population, perfom mutation on each with base prob
///    - evaluate genomes
/// - (x1) run GA with genomes + fitness
///   - run selection, mutation, xover to prepare new generation
///     - new generation size is calculated so it fills empty space
///   - part of old generation (worst) is terminated (total - EliteLimit)
///   - output: 
///     - list of old genomes survived
///     - list of new genomes
///   - run fitness calc
///   - if <true> returned (and fitness function exists):
///     - if next steps required 
///       - prepare new generation - mix survived old + best from new generation 
///       - go to (x1)

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
//std
#include <set>
//boost
#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_list.hpp>
//sc
#include "sc/dtypes.h"
#include "sc/events/Events.h"
#include "base/bit.h"

#include "sgp\FitnessValue.h"
#include "sgp\EntityBase.h"
#include "sgp\GaGeneration.h"
#include "sgp\FitnessDefs.h"
#include "sgp\FitnessFunction.h"

#define COUNT_EVAL

// ----------------------------------------------------------------------------
// Simple type definitions
// ----------------------------------------------------------------------------
//typedef scDataNode sgpGaGenome;     ///<-- set of variables
typedef scDataNode sgpGaGenomeList; ///<-- list of genomes
typedef scDataNode sgpGaFitnessList; ///<-- list of genomes
typedef std::set<uint> sgpObjectiveIndexSet;
typedef std::vector<int> scVectorOfInt;

enum sgpGaGenomeType {
  gagtConst = 1,
  gagtRanged = 2,
  gagtAlphaString = 4,
  gagtValue = 8
};

/// Meta information for params that can be optimized during evolution
struct sgpGaGenomeMetaInfo {
  sgpGaGenomeType genType;
  scDataNode minValue;
  scDataNode maxValue;
  uint genSize;
  uint userType;
};


typedef sgpEntityBase *sgpEntityBasePtr;

typedef boost::ptr_vector<sgpEntityBase> sgpGaGenomeWorkList;

// ----------------------------------------------------------------------------
// Forward class definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------
const scString SGP_GA_OPERATOR_MUTATE = "mut";   ///<- mutation operator
const scString SGP_GA_OPERATOR_SELECT = "sel";   ///<- selection operator
const scString SGP_GA_OPERATOR_XOVER  = "xover"; ///<- cross-over operator
const scString SGP_GA_OPERATOR_MONITOR= "monit"; ///<- reporting operator
const scString SGP_GA_OPERATOR_ELITE  = "elite"; ///<- select best entities
const scString SGP_GA_OPERATOR_INIT  = "init";   ///<- fill first generation
const scString SGP_GA_OPERATOR_EVAL  = "eval";   ///<- evaluate population
const scString SGP_GA_OPERATOR_ALLOC  = "alloc"; ///<- allocate population

const uint SGP_GA_DEF_POPSIZE = 20;
const uint SGP_GA_WIDX_GENOME = 0;
const uint SGP_GA_WIDX_FITNESS = 1;

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------

/// array of meta information, one element per each variable in genome
/// each contains "gen_type" - enumerate value and optionally "min" and "max" 
/// can be used also to describe dynamic evolution parameters
typedef std::vector<sgpGaGenomeMetaInfo> sgpGaGenomeMetaList; 

//typedef scDataNode sgpGaGeneration;

class sgpGaGenomeMetaBuilder {
public:
  sgpGaGenomeMetaBuilder() {}
  virtual ~sgpGaGenomeMetaBuilder() {}
  void clear() {m_meta.clear();}
  void getMeta(sgpGaGenomeMetaList &output) {output = m_meta;}  
  void setMeta(const sgpGaGenomeMetaList &input) {m_meta = input;}
  void addMetaRangedInt(int minValue, int maxValue);
  void addMetaRangedUInt(uint minValue, uint maxValue);
  void addMetaRangedDouble(double minValue, double maxValue);
  void addMetaAlphaString(const scString &charSet, uint maxLen);
  void addMetaConst(const scDataNode &value);
  void addMetaValue(const scDataNode &value);
  void setLastUserType(uint userType);
  void setLastSize(uint aSize);
  uint getMetaSize() { return m_meta.size(); }
protected:
  sgpGaGenomeMetaList m_meta;  
};

/// Extended fitness function - use as base if you need additional functions defined here
class sgpFitnessFunctionEx: public sgpFitnessFunction {
public:
  /// Executed before whole generation is evaluated
  virtual void initProcess(sgpGaGeneration &newGeneration) {
    setStopStatus(0);
  }

  /// Executed after whole generation was evaluated. Returns <false> if process should be aborted.
  virtual bool postProcess(sgpGaGeneration &newGeneration) {return true;}

  /// Returns list of dynamic parameters - parameters that can be optimized during evolution
  virtual bool describeDynamicParams(sgpGaGenomeMetaList &output, scDataNode &nameList, const scDataNode &filter = scDataNode()) {return false;}
  /// Updates dynamic parameters - parameters that can be optimized during evolution
  virtual void useDynamicParams(const sgpGaGenomeMetaList &values, const scDataNode &nameList) {}
};

/// class for filtering fitness on population level
class sgpFitnessFilter {
public:
  sgpFitnessFilter() {}
  virtual ~sgpFitnessFilter() {}

  /// Executed before whole generation is evaluated
  virtual void initProcess(sgpGaGeneration &newGeneration) = 0;
  /// Executed after whole generation was evaluated
  virtual bool postProcess(sgpGaGeneration &newGeneration) = 0;
};

class sgpGaGenomeCompareTool: public scObject {
public:
  sgpGaGenomeCompareTool() {}
  virtual ~sgpGaGenomeCompareTool() {};
  virtual void setMaxLength(uint value) { m_maxLength = value; }
  virtual void setMetaInfo(const sgpGaGenomeMetaList &list) {m_meta = list;};
  virtual double calcGenomeDiff(const sgpGaGeneration &newGeneration, uint first, uint second) = 0;
  virtual double calcGenomeDiff(const sgpGaGeneration &newGeneration, uint first, uint second, uint genNo) = 0;
protected:
  uint m_maxLength;  
  sgpGaGenomeMetaList m_meta;  
};

typedef std::auto_ptr<sgpGaGenomeCompareTool> sgpGaGenomeCompareToolGuard;

// parameters that can be passed during evolution between operators
// example of usage: storage for island params 
class sgpGaExperimentParams {
public:
  sgpGaExperimentParams() {}
  virtual ~sgpGaExperimentParams() {}
  virtual bool isDefined(uint groupId, uint paramId) const = 0;
  virtual uint getGroupCount() const = 0;
  virtual bool getInt(uint groupId, uint paramId, int &value) const = 0;
  virtual bool getDouble(uint groupId, uint paramId, double &value) const = 0;
  virtual void getParamList(scDataNode &values) const = 0;
  virtual void getParamValuesForGroup(uint groupId, scDataNode &values) const = 0;
  virtual bool getParamValueForGroup(uint groupId, uint paramId, scDataNode &value) const = 0;
};

class sgpGaExperimentParamsStored: public sgpGaExperimentParams {
public:
  // create
  sgpGaExperimentParamsStored();
  virtual ~sgpGaExperimentParamsStored();
  // construct
  void clear();
  void defineParam(const scString &name, uint paramId, const scDataNode &minValue, const scDataNode &maxValue, bool isStatic = false);
  
  virtual uint getGroupCount() const;
  virtual void getParamList(scDataNode &values) const;
  virtual void setParamList(const scDataNode &values);
  virtual void setParamListCheck(const scDataNode &values);
  
  virtual void getDynamicParamList(scDataNode &values) const;
  virtual void setDynamicParamList(const scDataNode &values);  
  virtual void setParamListByIndex(const scDataNode &values);
  
  virtual void getParamMeta(scDataNode &meta) const;
  virtual void getParamMetaForGroup(uint groupId, scDataNode &meta) const;
  virtual void getParamValuesForGroup(uint groupId, scDataNode &values) const;
  virtual bool getParamValueForGroup(uint groupId, uint paramId, scDataNode &value) const;

  // run
  virtual bool isDefined(uint groupId, uint paramId) const;
  virtual bool getInt(uint groupId, uint paramId, int &value) const;
  virtual bool getDouble(uint groupId, uint paramId, double &value) const;
  void setInt(uint groupId, uint paramId, int value);
  void setDouble(uint groupId, uint paramId, double value);
  bool getParamIndex(uint groupId, uint paramId, uint &value) const;
protected:
  void setParamListValues(const scDataNode &values);
  uint calcCompactParamId(uint groupId, uint paramId) const;
  bool getParamName(uint groupId, uint paramId, scString &value) const;
protected:
  typedef std::map<uint, uint> ParamIndexMap;
  typedef std::map<uint, scString> ParamId2NameMap;
  mutable ParamIndexMap m_paramIndexMap;
  ParamId2NameMap m_paramId2NameMap;
  //scDataNode m_paramMap;  
  scDataNode m_params;
  scDataNode m_meta;
  scDataNode m_staticParams;
};

class sgpGaOperator: public scObject {
public:
  sgpGaOperator() {};
  virtual ~sgpGaOperator() {};
  virtual void setMetaInfo(const sgpGaGenomeMetaList &list) {m_meta = list;};
  virtual bool describeDynamicParams(sgpGaGenomeMetaList &output, scDataNode &nameList, const scDataNode &filter = scDataNode()) {return false;}
  virtual void useDynamicParams(const sgpGaGenomeMetaList &values, const scDataNode &nameList) {}
  virtual void init() {};
protected:
  sgpGaGenomeMetaList &getMetaInfo() { return m_meta; }
protected:
  sgpGaGenomeMetaList m_meta;  
};

/// Class which creates an operator on request
class sgpGaOperatorFactory {
public:
   sgpGaOperatorFactory() {}
   virtual ~sgpGaOperatorFactory() {}

   virtual sgpGaOperator *newOperator(const scString &operatorType) = 0;
};

typedef boost::ptr_map<scString, sgpGaOperator> sgpGaOperatorMap; 
typedef boost::ptr_list<sgpGaOperator> sgpGaOperatorList; 
typedef std::list<sgpGaOperator *> sgpGaOperatorListExt; 
typedef std::auto_ptr<sgpFitnessFunction> sgpFitnessFunctionGuard;
typedef std::auto_ptr<sgpGaGeneration> sgpGaGenerationGuard;

class sgpGaOperatorMutate: public sgpGaOperator {
public:
  virtual void execute(sgpGaGeneration &newGeneration) = 0;
};

class sgpGaOperatorEvaluate: public sgpGaOperator {
public:
  virtual bool execute(uint stepNo, bool isNewGen, sgpGaGeneration &generation) = 0;
};

class sgpGaOperatorSelect: public sgpGaOperator {
public:
  virtual void execute(sgpGaGeneration &input, sgpGaGeneration &output, uint limit) = 0;
};

class sgpGaOperatorElite: public sgpGaOperatorSelect {
public:
  static uint calcEliteLimit(uint eliteMode, double eliteRatio, uint popSize);
};

class sgpGaOperatorXOver: public sgpGaOperator {
public:
  virtual void execute(sgpGaGeneration &newGeneration) = 0;
protected:  
  virtual void executeWithProb(double aProb, sgpGaGeneration &newGeneration);
  virtual bool canExecute();
  virtual bool crossGenomes(sgpGaGeneration &newGeneration, uint first, uint second) = 0;
};

class sgpGaOperatorAlloc: public sgpGaOperator {
public:
  //virtual sgpGaGeneration *execute() { return new sgpGaGeneration; }
  virtual sgpGaGeneration *execute() = 0;
};

class sgpGaOperatorEvalMonitorIntf: public sgpGaOperator {
public: 
  virtual void handleBeforeEvaluate(uint stepNo, const sgpGaGeneration &input) {}
  virtual void handleBeforeEvaluatePostProc(uint stepNo, const sgpGaGeneration &input) {}
  virtual void handleAfterEvaluate(uint stepNo, const sgpGaGeneration &input) {}
};

class sgpGaOperatorMonitor: public sgpGaOperatorEvalMonitorIntf {
  typedef sgpGaOperatorEvalMonitorIntf inherited;
public: 
  sgpGaOperatorMonitor(): inherited() {}
  virtual ~sgpGaOperatorMonitor() {}
  void execute(uint stepNo, const sgpGaGeneration &input);
  void addBeforeStep(sgpGaOperatorMonitor *gaOperator);
  void addAfterStep(sgpGaOperatorMonitor *gaOperator);
  void addBeforeStepExt(sgpGaOperatorMonitor *gaOperator);
  void addAfterStepExt(sgpGaOperatorMonitor *gaOperator);
protected:
  virtual void performBeforeExecute(uint stepNo, const sgpGaGeneration &input);
  virtual void intExecute(uint stepNo, const sgpGaGeneration &input) = 0;
  virtual void performAfterExecute(uint stepNo, const sgpGaGeneration &input);  
protected:  
  sgpGaOperatorList m_beforeSteps;
  sgpGaOperatorList m_afterSteps;
  sgpGaOperatorListExt m_beforeStepsExt;
  sgpGaOperatorListExt m_afterStepsExt;
};

class sgpGaOperatorInitBase: public sgpGaOperator {
public:
  sgpGaOperatorInitBase(): sgpGaOperator() {}
  virtual ~sgpGaOperatorInitBase() {}
  virtual void execute(uint limit, sgpGaGeneration &newGeneration) = 0;
  virtual void addListener(uint eventCode, scListener *listener) {}
  virtual void removeListener(uint eventCode, scListener *listener) {}
};

class sgpGaOperatorInit: public sgpGaOperatorInitBase {
public:
  sgpGaOperatorInit(): sgpGaOperatorInitBase() {}
  virtual ~sgpGaOperatorInit() {}
  static void buildRandomGenome(const sgpGaGenomeMetaList &metaList, scDataNode &output);
  static void getRandomRanged(const scDataNode &minValue, const scDataNode &maxValue, scDataNode &output);
  static void getRandomRanged(const scDataNode &startValue, 
    const scDataNode &minValue, const scDataNode &maxValue, scDataNode &output);
};

class sgpGaOperatorInitEntity: public sgpGaOperator {
public:
  sgpGaOperatorInitEntity(): sgpGaOperator() {}
  virtual ~sgpGaOperatorInitEntity() {}
  virtual void buildRandomEntity(sgpEntityBase &output) = 0;
};

// calculate distance between entities in population
class sgpDistanceFunction {
public:
  // returns value in absolute cell points
  virtual void calcDistanceAbs(uint first, uint second, double &distance) = 0;
  // returns relative value (0..1). maximum =1 for maximum distance
  virtual void calcDistanceRel(uint first, uint second, double &distance) = 0;
};

class sgpGaEvolver: public scObject {
public:
  sgpGaEvolver();
  virtual ~sgpGaEvolver();
  ulong64 run(ulong64 limit);
  void runStep();
  void stop();
  void reset();
  void checkPrepared();
public:  
// properties  
  void setGeneration(const sgpGaGenomeList &genomeList, const sgpGaFitnessList &fitnessList);
  const sgpGaGeneration *getGeneration() const;
  virtual bool canBuildGeneration();
  virtual void buildGeneration();
  virtual void buildGeneration(bool evaluate);
  void buildGeneration(const sgpGaGenomeList &templateList, bool evaluate = true);
  void setGenomeMeta(const sgpGaGenomeMetaList &metaList);
  void setOperator(const scString &operatorType, sgpGaOperator *gaOperator, bool owned);
  void setFitnessFunction(sgpFitnessFunction *function);
  sgpFitnessFunction *getFitnessFunction() const;
  void getOldGenomes(sgpGaGenomeList &output) const;
  void getNewGenomes(sgpGaGenomeList &output) const;
  void getAllGenomes(sgpGaGenomeList &output) const; 
  void getGeneration(sgpGaGeneration &output) const; 
  void getBestGenome(sgpGaGenome &output) const;   
  void getBestGenome(scDataNode &output) const;   
  void getBestGenomeByObjective(uint objIndex, bool searchForMin, scDataNode &output) const;
  uint getBestGenomeIndexByWeights(const sgpWeightVector &weights) const;
  void getTopGenomes(int limit, sgpGaGenomeList &output) const;
  static void getTopGenomes(const sgpGaGeneration &input, int limit, 
    sgpGaGenomeList &output);
  static void getTopGenomes(const sgpGaGeneration &input, int limit, 
    sgpGaGenomeList &output, sgpEntityIndexList &indices);
  static void getTopGenomesByObjective(const sgpGaGeneration &input, int limit, 
    uint objectiveIndex, sgpGaGenomeList &output, sgpEntityIndexList &indices);
  static void getTopGenomesByObjective(const sgpGaGeneration &input, int limit, 
    uint objectiveIndex, sgpEntityIndexList &indices);
  static void getTopGenomesByWeights(const sgpGaGeneration &input, int limit, 
    const sgpWeightVector &weights, sgpGaGenomeList *output, sgpEntityIndexList &indices, 
    const sgpEntityIndexSet *requiredItems);
  static void getTopGenomesByWeights(const sgpGaGeneration &input, int limit, 
    const sgpWeightVector &weights, sgpGaGenomeList &output, sgpEntityIndexList &indices);
  static void getTopGenomesByWeights(const sgpGaGeneration &input, int limit, 
    const sgpWeightVector &weights, sgpEntityIndexList &indices);
  void setEliteLimit(uint a_limit);
  void setPopulationSize(uint a_size);
  uint getPopulationSize() const;
  sgpGaOperator *getOperator(const scString &operatorType) const;
  void setFitness(uint genomeIndex, double value);
  virtual bool describeDynamicParams(sgpGaGenomeMetaList &output, scDataNode &nameList, const scDataNode &filter = scDataNode());
  virtual void useDynamicParams(const sgpGaGenomeMetaList &values, const scDataNode &nameList);
  virtual void getUsedTimers(scDataNode &list) {}
  virtual void getUsedCounters(scDataNode &list) {}
protected:
  virtual void buildGenerationWithInitOperator(bool evaluate);
  virtual void initOperators();
  virtual bool runOperators(uint stepNo, bool runEval);
  virtual void runMutate();
  virtual void runSelection(uint stepNo);
  virtual void runElite();
  virtual void runXOver();
  virtual void runMonitorExecute(uint stepNo);
  virtual void runMonitorBeforeEvaluate(uint stepNo, const sgpGaGeneration *target);
  virtual void runMonitorBeforeEvaluatePostProc(uint stepNo, const sgpGaGeneration *target);
  virtual void runMonitorAfterEvaluate(uint stepNo, const sgpGaGeneration *target);
  virtual bool runEvaluate(uint stepNo, bool useCurrGener = false);
  virtual void runInit();
  virtual void runInitOperator();
  void buildRandomGenome(sgpGaGenome &output);
  void checkState();
  virtual void useNewGeneration();
  void terminateOutdated();
  virtual bool isPrepared();
  virtual void prepare();
  virtual sgpGaGeneration *newGeneration();
  void runDefaultInit();
  virtual void processInitGeneration(bool evaluate);
  virtual bool intDescribeDynamicParams(sgpGaGenomeMetaList &output, scDataNode &nameList, const scDataNode &filter = scDataNode()) {return false;}
  virtual void intUseDynamicParams(const sgpGaGenomeMetaList &values, const scDataNode &nameList) {}
  void performDummySelect();
  bool isEliteActive();
  virtual uint getNewGenerationSize();  
  virtual uint getNewGenerationSpaceLeft();
  virtual void checkGenerationSize();
  virtual void newGenerationSizeIncreased(uint growCnt);
  virtual void signalStepChanged(uint stepNo);
  sgpGaOperator *getOperatorFromReg(const scString &operatorType) const;
  sgpGaOperator *getOperatorOwned(const scString &operatorType) const;
protected:
// configuration
  uint m_eliteLimit;
  uint m_populationSize;
  sgpGaGenomeMetaList m_genomeMeta;
  sgpGaOperatorMap m_operatorMap;
  scObjectRegistry m_operatorReg;
  sgpFitnessFunctionGuard m_fitnessFunction;  
// state  
  sgpGaGenerationGuard m_generation;
  sgpGaGenerationGuard m_newGeneration;
};

typedef std::auto_ptr<sgpGaEvolver> sgpGaEvolverGuard;

namespace sgp {
  /// Decode UInt encoded as bitstring
  uint decodeBitStrUInt(const scString &value);
  /// Encode uint as bitstring 
  scString encodeBitStrUInt(uint value, uint len);

  /// Decode double encoded as bitstring
  double decodeBitStrDouble(const scString &value);
  /// Encode double as bitstring
  void encodeBitStrDouble(double value, uint len, scString &output);

  /// Decode double encoded as uint with bit count = bitSize (with gray code)
  inline double decodeUIntDouble(uint value, uint bitSize) {
    const uint maxValue = (1 << bitSize) - 1;
    return static_cast<double>(grayToBin(value, bitSize)) / static_cast<double>(maxValue);
  }

  /// Decode double encoded as uint with bit count = bitSize (with gray code)
  template <int bitSize>
  double decodeUIntDouble(uint value) {
    const uint maxValue = (1 << bitSize) - 1;
    return static_cast<double>(grayToBin(value, bitSize)) / static_cast<double>(maxValue);
  }
};

#endif // _GAEVOLVER_H__
