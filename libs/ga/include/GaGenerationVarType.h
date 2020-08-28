/////////////////////////////////////////////////////////////////////////////
// Name:        GaGenerationVarType.h
// Project:     sgpLib
// Purpose:     Entity container for GA algorithms - variant type items.
// Author:      Piotr Likus
// Modified by:
// Created:     14/07/2013
/////////////////////////////////////////////////////////////////////////////

#ifndef _SGPGAGENERVARTP_H__
#define _SGPGAGENERVARTP_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/** \file GaGenerationVarType.h
\brief Entity container for GA algorithms - variant type items.

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

class sgpGaGenerationVarType: public sgpGaGeneration {
  typedef sgpGaGeneration inherited;
public:
  sgpGaGenerationVarType(): inherited() {}
  virtual ~sgpGaGenerationVarType() {}

  virtual sgpEntityBase *cloneItem(int index) const;
  virtual sgpEntityBase *newItem() const;
  virtual sgpEntityBase *newItem(const sgpEntityBase &src) const;

  virtual sgpGaGeneration *newEmpty() const {
  	return new sgpGaGenerationVarType();
  }
  
protected:
};
  

#endif // _SGPGAGENERVARTP_H__
