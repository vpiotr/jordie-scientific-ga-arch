/////////////////////////////////////////////////////////////////////////////
// Name:        EntityIslandToolGa.cpp
// Project:     sgpLib 
// Purpose:     Tool for island support - GA-specific.
// Author:      Piotr Likus
// Modified by:
// Created:     23/07/2011
/////////////////////////////////////////////////////////////////////////////

#include "base\bitstr.h"

//#include "sc\utils.h"

#include "sgp\EntityIslandToolGa.h"

using namespace dtp;

void sgpEntityIslandToolGa::setIslandLimit(uint value)
{
  m_islandLimit = value;
}

void sgpEntityIslandToolGa::setIslandIdPos(int value)
{
  m_islandIdPos = value;
}

void sgpEntityIslandToolGa::setBitSize(uint value)
{
  m_bitSize = value;
}

void sgpEntityIslandToolGa::prepareIslandMap(const sgpGaGeneration &newGeneration, scDataNode &output)
{
  uint islandId, lastIslandId;
  const sgpEntityBase *workInfo;
  scString islandName;
  scDataNode genome;
  scDataNode islandNode;
  std::vector<scString> islandNames;
  uint islancCnt = 0;

  lastIslandId = UINT_MAX;

  assert(m_islandLimit > 0);
  for(uint i = newGeneration.beginPos(), epos = newGeneration.endPos(); i != epos; i++)
  {
    workInfo = newGeneration.atPtr(i);
    if (!intGetIslandId(*workInfo, islandNode, islandId))
      islandId = 0;
    
    if (islandId != lastIslandId)
    {
      if (islancCnt <= islandId)
      {
        islandNames.resize(islandId+1);
        islancCnt = islandNames.size();
      }

      islandName = islandNames[islandId];
      if (islandName.empty()) {
        islandName = toString(islandId);
        islandNames[islandId] = islandName;
      }

      if (!output.hasChild(islandName)) {
        output.addChild(islandName, new scDataNode(ict_array, vt_uint));
      }

      lastIslandId = islandId;
    }

    output[islandName].addItemAsUInt(i);  
  }    
}

bool sgpEntityIslandToolGa::getIslandId(const sgpEntityBase &entity, uint &output)
{
  scDataNode value;
  return intGetIslandId(entity, value, output);
}

bool sgpEntityIslandToolGa::intGetIslandId(const sgpEntityBase &entity, scDataNode &workNode, uint &output)
{
  assert(m_islandIdPos >= 0);
  uint islandId;
  entity.getGenomeItem(0, m_islandIdPos, workNode);

  if (workNode.getValueType() == vt_string)
    islandId = bitStringToInt<uint>(workNode.getAsString());
  else
    islandId = grayToBin(workNode.getAsUInt(), m_bitSize);
  
  islandId = islandId % m_islandLimit;
  output = islandId;
  return true;
}

bool sgpEntityIslandToolGa::setIslandId(sgpEntityBase &entity, uint value)
{
  uint islandId = value % m_islandLimit;
  scDataNode workNode;
  
  entity.getGenomeItem(0, m_islandIdPos, workNode);

  if (workNode.getValueType() == vt_string) {  
    scString strValue;
    intToBitString(islandId, workNode.getAsString().length(), strValue);
    workNode = scDataNode(strValue);
  } else {
    workNode.setAsUInt(binToGray(value, m_bitSize));
  }

  entity.setGenomeItem(0, m_islandIdPos, workNode);

  return true;
}

