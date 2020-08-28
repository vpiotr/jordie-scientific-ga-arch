/////////////////////////////////////////////////////////////////////////////
// Name:        GaGenerationUInt.cpp
// Project:     sgpLib
// Purpose:     Entity container for GA algorithms - uint items.
// Author:      Piotr Likus
// Modified by:
// Created:     14/07/2013
/////////////////////////////////////////////////////////////////////////////

#include "sc\utils.h"

#include "sgp\GaGenerationUInt.h"
#include "sgp\EntityForGaUInt.h"

// ----------------------------------------------------------------------------
// sgpGaGenerationUInt
// ----------------------------------------------------------------------------
sgpEntityBase *sgpGaGenerationUInt::cloneItem(int index) const {
  return new sgpEntityForGaUInt(
    *(
        checked_cast<sgpEntityForGaUInt *>(
          &(const_cast<sgpGaGenerationUInt *>(this)->m_items[index])
        )
     )
  );
}  

sgpEntityBase *sgpGaGenerationUInt::newItem() const {
  return new sgpEntityForGaUInt();
}  

sgpEntityBase *sgpGaGenerationUInt::newItem(const sgpEntityBase &src) const {
  return new sgpEntityForGaUInt(
    *(
        checked_cast<sgpEntityForGaUInt *>(
          &const_cast<sgpEntityBase &>(src)
        )
     )
  );
}  

class StructureWriterForGaGenUIntCode: public base::StructureWriterIntf {
public:
  StructureWriterForGaGenUIntCode(const sgpGaGenerationUInt &source, const scDataNode &beforeValues): 
      m_source(source), m_beforeValues(beforeValues) 
      {}

  virtual void visit(base::StructureOutputIntf &output) {
    beginWrite(output);
    writeNodeCode(output, m_beforeValues);
    writeGenCode(output);
    endWrite(output);
  }

protected:
  virtual void beginWrite(base::StructureOutputIntf &output) {
    output.beginWrite();
    output.beginArray();
  }

  virtual void endWrite(base::StructureOutputIntf &output) {
    output.endArray();
    output.endWrite();
  }

  virtual void writeGenCode(base::StructureOutputIntf &output) {

    const sgpEntityForGaUInt *entity;
    uint value = 0;
    uint genomeLen;

    for(uint i=0, epos=m_source.size(); i != epos; ++i)
    {
      entity = &static_cast<const sgpEntityForGaUInt &>(m_source[i]);
      genomeLen = entity->getGenomeSize(0);
      if (genomeLen > 0)
      {
        output.beginArrayOf(value, genomeLen);
       
        for(uint j=0, eposj = genomeLen; j != eposj; ++j)
        {
          entity->getGenomeItem(0, j, value);
          output.writeValue(value);
        }

        output.endArray();
      }
    }
  }

  virtual void writeNodeCode(base::StructureOutputIntf &output, const scDataNode &node) 
  {
    uint value = 0;
    uint genomeLen;
    const scDataNode *nodePtr;
    scDataNode helper;

    for(uint i=0, epos=node.size(); i != epos; ++i)
    {
      nodePtr = node.getNodePtrR(i, helper);
      genomeLen = nodePtr->size();
      if (genomeLen > 0)
      {
        output.beginArrayOf(value, genomeLen);
       
        for(uint j=0, eposj = genomeLen; j != eposj; ++j)
        {
          value = nodePtr->getUInt(j);
          output.writeValue(value);
        }

        output.endArray();
      }
    }
  }

private:
  const sgpGaGenerationUInt &m_source;
  const scDataNode &m_beforeValues;
};

class StructureWriterForGaGenUIntFit: public base::StructureWriterIntf {
public:
  StructureWriterForGaGenUIntFit(const sgpGaGenerationUInt &source, const scDataNode &beforeValues): 
    m_source(source), m_beforeValues(beforeValues) {}

  virtual void visit(base::StructureOutputIntf &output) 
  {
    if (m_source.empty())
    {
      writeEmptyStruct(output);
      return;
    }

    beginWrite(output);
    writeNodeFit(output);
    writeGenFit(output);
    endWrite(output);
  }

  virtual void writeEmptyStruct(base::StructureOutputIntf &output) 
  {
    beginWrite(output);
    endWrite(output);
  }

  virtual void beginWrite(base::StructureOutputIntf &output) 
  {
    output.beginWrite();
    output.beginArray();
  }

  virtual void endWrite(base::StructureOutputIntf &output) 
  {
    output.endArray();
    output.endWrite();
  }

  virtual void writeNodeFit(base::StructureOutputIntf &output) 
  {
    double value = 0.0;
    uint fitnessLen;
    const scDataNode *nodePtr;
    scDataNode helper;

    for(uint i=0, epos=m_beforeValues.size(); i != epos; ++i)
    {
      nodePtr = m_beforeValues.getNodePtrR(i, helper);
      fitnessLen = nodePtr->size();

      if (fitnessLen > 0)
      {
        output.beginArrayOf(value, fitnessLen);
       
        for(uint j=0, eposj = fitnessLen; j != eposj; ++j)
        {
          value = nodePtr->getDouble(j);
          output.writeValue(value);
        }

        output.endArray();
      }
    }
   
  }

  virtual void writeGenFit(base::StructureOutputIntf &output) 
  {
    const sgpEntityForGaUInt *entity;
    double value = 0.0;
    uint fitnessLen;

    for(uint i=0, epos=m_source.size(); i != epos; ++i)
    {
      entity = &static_cast<const sgpEntityForGaUInt &>(m_source[i]);
      fitnessLen = entity->getFitnessSize();
      if (fitnessLen > 0)
      {
        output.beginArrayOf(value, fitnessLen);
       
        for(uint j=0, eposj = fitnessLen; j != eposj; ++j)
        {
          value = entity->getFitness(j);
          output.writeValue(value);
        }

        output.endArray();
      }
    }
    
  }
private:
  const sgpGaGenerationUInt &m_source;
  const scDataNode &m_beforeValues;
};

base::StructureWriterIntf *sgpGaGenerationUInt::newWriterForCode(const scDataNode &extraValues) const
{
  return new StructureWriterForGaGenUIntCode(*this, extraValues);
}

base::StructureWriterIntf *sgpGaGenerationUInt::newWriterForFitness(const scDataNode &extraValues) const
{
  return new StructureWriterForGaGenUIntFit(*this, extraValues);
}
