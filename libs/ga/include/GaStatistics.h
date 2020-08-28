/////////////////////////////////////////////////////////////////////////////
// Name:        GaStatistics.h
// Project:     sgpLib
// Purpose:     Statistics definitions (timers, counters)
// Author:      Piotr Likus
// Modified by:
// Created:     22/08/2010
/////////////////////////////////////////////////////////////////////////////

#ifndef _GASTATS_H__
#define _GASTATS_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file GaStatistics.h
///
/// File description

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sc/dtypes.h"
#include "sgp/ExperimentStats.h"

// ----------------------------------------------------------------------------
// Simple type definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Forward class definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------
const scString TIMER_SHAPE_DETECT = "gx-shape-detect";
const scString TIMER_ISLAND_OPT = "gx-island-opt";

//Gx experiment
const scString COUNTER_EVAL = "gx-ceval";

const scString TIMER_EXPERIMENT_ALL = "gx-exp-all";
const scString TIMER_EXPERIMENT_STEP = "gx-exp-step";
const scString TIMER_EVAL = "gx-eval";
const scString TIMER_EVAL_STEP = "gx-eval-step";

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------


#endif // _GASTATS_H__
