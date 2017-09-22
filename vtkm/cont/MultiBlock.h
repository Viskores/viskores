//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
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
  /// creat a new MultiBlcok containng a single DataSet "ds"
  VTKM_CONT
  MultiBlock(const vtkm::cont::DataSet& ds);
  /// creat a new MultiBlcok with the exisiting one "src"
  VTKM_CONT
  MultiBlock(const vtkm::cont::MultiBlock& src);
  /// creat a new MultiBlcok with a DataSet vector "mblocks"
  VTKM_CONT
  MultiBlock(const std::vector<vtkm::cont::DataSet>& mblocks);
  /// creat a new MultiBlcok with the capacity set to be "size"
  VTKM_CONT
  MultiBlock(vtkm::Id size);

  VTKM_CONT
  MultiBlock();

  VTKM_CONT
  MultiBlock& operator=(const vtkm::cont::MultiBlock& src);

  VTKM_CONT
  ~MultiBlock();
  /// get the field "field_name" from blcok "block_index"
  VTKM_CONT
  vtkm::cont::Field GetField(const std::string& field_name, const int& block_index);

  VTKM_CONT
  vtkm::Id GetNumberOfBlocks() const;

  VTKM_CONT
  const vtkm::cont::DataSet& GetBlock(vtkm::Id blockId) const;

  VTKM_CONT
  const std::vector<vtkm::cont::DataSet>& GetBlocks() const;
  /// add DataSet "ds" to the end of the contained DataSet vector
  VTKM_CONT
  void AddBlock(vtkm::cont::DataSet& ds);
  /// add DataSet "ds" to position "index" of the contained DataSet vector
  VTKM_CONT
  void InsertBlock(vtkm::Id index, vtkm::cont::DataSet& ds);
  /// replace the "index" positioned element of the contained DataSet vector with "ds"
  VTKM_CONT
  void ReplaceBlock(vtkm::Id index, vtkm::cont::DataSet& ds);
  /// append the DataSet vector "mblocks"  to the end of the contained one
  VTKM_CONT
  void AddBlocks(std::vector<vtkm::cont::DataSet>& mblocks);
  /// get the unified bounds of the same indexed coordinate system within all contained DataSet
  VTKM_CONT
  vtkm::Bounds GetBounds(vtkm::Id coordinate_system_index = 0) const;

  template <typename TypeList>
  VTKM_CONT vtkm::Bounds GetBounds(vtkm::Id coordinate_system_index, TypeList) const;

  template <typename TypeList, typename StorageList>
  VTKM_CONT vtkm::Bounds GetBounds(vtkm::Id coordinate_system_index, TypeList, StorageList) const;
  /// get the bounds of a coordinate system within a given DataSet
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
  /// get the unified range of the same feild within all contained DataSet
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
};
}
} // namespace vtkm::cont

#endif
