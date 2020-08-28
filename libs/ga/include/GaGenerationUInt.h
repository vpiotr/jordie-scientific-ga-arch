/////////////////////////////////////////////////////////////////////////////
// Name:        GaGenerationUInt.h
// Project:     sgpLib
// Purpose:     Entity container for GA algorithms - uint items.
// Author:      Piotr Likus
// Modified by:
// Created:     14/07/2013
/////////////////////////////////////////////////////////////////////////////

#ifndef _SGPGAGENERUINT_H__
#define _SGPGAGENERUINT_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/** \file GaGenerationUInt.h
\brief Entity container for GA algorithms - uint items.

*/

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sgp\EntityBase.h"
#include "sgp\GaGeneration.h"

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

class sgpGaGenerationUInt: public sgpGaGeneration {
  typedef sgpGaGeneration inherited;
public:
  sgpGaGenerationUInt(): inherited() {}
  virtual ~sgpGaGenerationUInt() {}

  virtual sgpEntityBase *cloneItem(int index) const;
  virtual sgpEntityBase *newItem() const;
  virtual sgpEntityBase *newItem(const sgpEntityBase &src) const;

  virtual sgpGaGeneration *newEmpty() const {
  	return new sgpGaGenerationUInt();
  }
  virtual base::StructureWriterIntf *newWriterForCode(const scDataNode &extraValues) const;
  virtual base::StructureWriterIntf *newWriterForFitness(const scDataNode &extraValues) const;
protected:
};
  

#endif // _SGPGAGENERUINT_H__
