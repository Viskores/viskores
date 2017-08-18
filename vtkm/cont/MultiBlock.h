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
#include <limits>
#include <vtkm/StaticAssert.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/Field.h>

namespace vtkm
{
namespace cont
{

class VTKM_CONT_EXPORT MultiBlock
{
public:
  VTKM_CONT
  MultiBlock(const vtkm::cont::DataSet& ds);

  VTKM_CONT
  MultiBlock(const vtkm::cont::MultiBlock& src);

  VTKM_CONT
  MultiBlock(const std::vector<vtkm::cont::DataSet>& mblocks);

  VTKM_CONT
  MultiBlock(vtkm::Id size);

  VTKM_CONT
  MultiBlock();

  VTKM_CONT
  MultiBlock& operator=(const vtkm::cont::MultiBlock& src);

  VTKM_CONT
  ~MultiBlock();

  VTKM_CONT
  vtkm::cont::Field GetField(const std::string& field_name, const int& domain_index);

  VTKM_CONT
  vtkm::Id GetNumberOfBlocks() const;

  VTKM_CONT
  vtkm::Id GetCapacity();

  VTKM_CONT
  void SetCapacity(vtkm::Id size);

  VTKM_CONT
  const vtkm::cont::DataSet& GetBlock(vtkm::Id blockId) const;

  VTKM_CONT
  const std::vector<vtkm::cont::DataSet>& GetBlocks() const;

  VTKM_CONT
  void AddBlock(vtkm::cont::DataSet& ds);

  VTKM_CONT
  void InsertBlock(vtkm::Id index, vtkm::cont::DataSet& ds);

  VTKM_CONT
  void OverWriteBlock(vtkm::Id index, vtkm::cont::DataSet& ds);

  VTKM_CONT
  void AddBlocks(std::vector<vtkm::cont::DataSet>& mblocks);

  VTKM_CONT
  vtkm::Bounds GetBounds(vtkm::Id coordinate_system_index = 0) const;

  template <typename TypeList>
  VTKM_CONT vtkm::Bounds GetBounds(vtkm::Id coordinate_system_index, TypeList) const;

  template <typename TypeList, typename StorageList>
  VTKM_CONT vtkm::Bounds GetBounds(vtkm::Id coordinate_system_index, TypeList, StorageList) const;

  VTKM_CONT
  vtkm::Bounds GetBlockBounds(const std::size_t& block_index,
                              vtkm::Id coordinate_system_index = 0) const;

  template <typename TypeList>
  VTKM_CONT vtkm::Bounds GetBlockBounds(const std::size_t& block_index,
                                        vtkm::Id coordinate_system_index,
                                        TypeList) const;

  template <typename TypeList, typename StorageList>
  VTKM_CONT vtkm::Bounds GetBlockBounds(const std::size_t& block_index,
                                        vtkm::Id coordinate_system_index,
                                        TypeList,
                                        StorageList) const;

  VTKM_CONT
  vtkm::cont::ArrayHandle<vtkm::Range> GetGlobalRange(const std::string& field_name) const;

  template <typename TypeList>
  VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> GetGlobalRange(const std::string& field_name,
                                                                TypeList) const;

  template <typename TypeList, typename StorageList>
  VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> GetGlobalRange(const std::string& field_name,
                                                                TypeList,
                                                                StorageList) const;

  VTKM_CONT
  vtkm::cont::ArrayHandle<vtkm::Range> GetGlobalRange(const int& index) const;

  template <typename TypeList>
  VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> GetGlobalRange(const int& index, TypeList) const;

  template <typename TypeList, typename StorageList>
  VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> GetGlobalRange(const int& index,
                                                                TypeList,
                                                                StorageList) const;

  VTKM_CONT
  void PrintSummary(std::ostream& stream) const;

private:
  std::vector<vtkm::cont::DataSet> blocks;
  vtkm::Id capacity;
  bool capacitytag;
  std::vector<vtkm::Id> block_ids;
};
}
} // namespace vtkm::cont

#endif
