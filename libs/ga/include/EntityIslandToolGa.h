/////////////////////////////////////////////////////////////////////////////
// Name:        EntityIslandToolGa.h
// Project:     sgpLib 
// Purpose:     Tool for island support - GA-specific.
// Author:      Piotr Likus
// Modified by:
// Created:     23/07/2011
/////////////////////////////////////////////////////////////////////////////

#ifndef _SGPENTISLTOOLGA_H__
#define _SGPENTISLTOOLGA_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/** \file EntityIslandToolGa.h
\brief Tool for island support - GA-specific.

Long description
*/

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sgp\EntityIslandTool.h"

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
class sgpEntityIslandToolGa: public sgpEntityIslandToolIntf {
public:  
  sgpEntityIslandToolGa():m_islandLimit(0), m_islandIdPos(-1), m_bitSize(-1) {}
  virtual ~sgpEntityIslandToolGa() {} 
  void setIslandLimit(uint value);
  void setIslandIdPos(int value);
  void setBitSize(uint value);
  virtual void prepareIslandMap(const sgpGaGeneration &newGeneration, scDataNode &output);
  virtual bool getIslandId(const sgpEntityBase &entity, uint &output);
  virtual bool setIslandId(sgpEntityBase &entity, uint value);
protected:
  bool intGetIslandId(const sgpEntityBase &entity, scDataNode &workNode, uint &output);
private:
  uint m_islandLimit;
  int m_islandIdPos;
  uint m_bitSize;
};

#endif // _SGPENTISLTOOLGA_H__
