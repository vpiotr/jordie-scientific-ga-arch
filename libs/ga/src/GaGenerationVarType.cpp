/////////////////////////////////////////////////////////////////////////////
// Name:        GaGenerationVarType.cpp
// Project:     sgpLib
// Purpose:     Entity container for GA algorithms - variant type items.
// Author:      Piotr Likus
// Modified by:
// Created:     13/07/2013
/////////////////////////////////////////////////////////////////////////////

#include "sc\utils.h"

#include "sgp\GaGenerationVarType.h"
#include "sgp\EntityForGaVarType.h"

// ----------------------------------------------------------------------------
// sgpGaGenerationVarType
// ----------------------------------------------------------------------------
sgpEntityBase *sgpGaGenerationVarType::cloneItem(int index) const {
  return new sgpEntityForGaVarType(
    *(
        checked_cast<sgpEntityForGaVarType *>(
          &(const_cast<sgpGaGenerationVarType *>(this)->m_items[index])
        )
     )
  );
}  

sgpEntityBase *sgpGaGenerationVarType::newItem() const {
  return new sgpEntityForGaVarType();
}  

sgpEntityBase *sgpGaGenerationVarType::newItem(const sgpEntityBase &src) const {
  return new sgpEntityForGaVarType(
    *(
        checked_cast<sgpEntityForGaVarType *>(
          &const_cast<sgpEntityBase &>(src)
        )
     )
  );
}  

