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
#ifdef !vtk_m_cont_MultiBlock_h
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
  MultiBlock(const vtkm::cont::DataSet &ds)
  {
    this->blocks.push_back(ds);
  }

  MultiBlock(const vtkm::cont::MultiBlock &src)
  {
    this->blocks = src.GetBlocks();
  }

  MultiBlock(const std::vector<vtkm::cont::DataSet> &mblocks)
  {
    this->blocks = mblocks;
  }
  
  MultiBlock()
  {
  }

  MultiBlock &operator=(const vtkm::cont::MultiBlock &src);

  ~MultiBlock(){}

  vtkm::cont::Field GetField(const std::string &field_name, 
                             const int &domain_index);
  vtkm::Id GetNumberOfBlocks() const;
  const vtkm::cont::DataSet &GetBlock(vtkm::Id blockId) const;
  const std::vector<vtkm::cont::DataSet> &GetBlocks() const; 

  void AddBlock(vtkm::cont::DataSet &ds);
  void AddBlocks(std::vector<vtkm::cont::DataSet> &mblocks);

  vtkm::Bounds GetBounds(vtkm::Id coordinate_system_index = 0) const;

  vtkm::Bounds GetBlockBounds(const std::size_t &domain_index,
                               vtkm::Id coordinate_system_index = 0) const;

  vtkm::cont::ArrayHandle<vtkm::Range> GetGlobalRange(const std::string &field_name) const;
  vtkm::cont::ArrayHandle<vtkm::Range> GetGlobalRange(const int &index) const;

  void PrintSummary(std::ostream &stream) const;
private:
    std::vector<vtkm::cont::DataSet> blocks;
    std::vector<vtkm::Id>            block_ids;
};



}
} // namespace vtkm::cont

#endif 
