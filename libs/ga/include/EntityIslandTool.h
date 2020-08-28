/////////////////////////////////////////////////////////////////////////////
// Name:        EntityIslandTool.h
// Project:     sgpLib
// Purpose:     Island support for evolving entities.
// Author:      Piotr Likus
// Modified by:
// Created:     23/07/2011
/////////////////////////////////////////////////////////////////////////////

#ifndef _SGPENTITYISL_H__
#define _SGPENTITYISL_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/** \file EntityIsland.h
\brief Short file description

Long description
*/

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sc\dtypes.h"
#include "sgp\GaEvolver.h"

// ----------------------------------------------------------------------------
// Simple type definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Forward class definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------
class sgpEntityIslandToolIntf: public scInterface {
public:  
  sgpEntityIslandToolIntf() {}
  virtual ~sgpEntityIslandToolIntf() {}
  virtual void prepareIslandMap(const sgpGaGeneration &newGeneration, scDataNode &output) = 0;
  virtual bool getIslandId(const sgpEntityBase &entity, uint &output) = 0;
  virtual bool setIslandId(sgpEntityBase &entity, uint value) = 0;
};

#endif // _SGPENTITYISL_H__
