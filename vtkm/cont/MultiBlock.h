//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_MultiBlock_h
#define vtk_m_cont_MultiBlock_h

#include <vtkm/StaticAssert.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

namespace vtkm {
namespace cont {

class MultiBlock
{
public:
  VTKM_CONT
  MultiBlock(const vtkm::cont::DataSet &ds)
  {
    this->blocks.push_back(ds);
  }

  VTKM_CONT
  MultiBlock(const vtkm::cont::MultiBlock &src)
  {
    this->blocks = src.GetBlocks();
  }

  VTKM_CONT
  MultiBlock()
  {
  }

  VTKM_CONT
  MultiBlock(const std::vector<vtkm::cont::DataSet> &mblocks)
  {
    this->blocks = mblocks;
  }

  VTKM_CONT
  MultiBlock &operator=(const vtkm::cont::MultiBlock &src)
  {
    this->blocks = src.GetBlocks();
    return *this;
  }

  ~MultiBlock()
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfBlocks() const
  { 
    return static_cast<vtkm::Id>(this->blocks.size());
  }
 
  VTKM_CONT
  const vtkm::cont::DataSet &GetBlock(vtkm::Id blockId) const
  {
    return this->blocks[(std::size_t) blockId];
  }
  
  const std::vector<vtkm::cont::DataSet> &GetBlocks() const
  {
    return this->blocks;
  }
 
  VTKM_CONT
  void AddBlock(vtkm::cont::DataSet &ds)
  {
    this->blocks.push_back(ds);
  }

  VTKM_CONT
  void AddBlocks(std::vector<vtkm::cont::DataSet> &mblocks)
  {    
    for(std::size_t i = 0; i < mblocks.size(); i++)
    {
      AddBlock(mblocks[i]);
    }
  }

private:
    std::vector<vtkm::cont::DataSet> blocks;
};



}
} // namespace vtkm::cont

#endif 
