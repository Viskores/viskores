//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_MultiBlock_h
#define vtk_m_cont_MultiBlock_h
#include <limits>
#include <vtkm/StaticAssert.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Field.h>

namespace vtkm
{
namespace cont
{

class VTKM_CONT_EXPORT MultiBlock
{
public:
  /// create a new MultiBlock containng a single DataSet "ds"
  VTKM_CONT
  MultiBlock(const vtkm::cont::DataSet& ds);
  /// create a new MultiBlock with the existing one "src"
  VTKM_CONT
  MultiBlock(const vtkm::cont::MultiBlock& src);
  /// create a new MultiBlock with a DataSet vector "mblocks"
  VTKM_CONT
  explicit MultiBlock(const std::vector<vtkm::cont::DataSet>& mblocks);
  /// create a new MultiBlock with the capacity set to be "size"
  VTKM_CONT
  explicit MultiBlock(vtkm::Id size);

  VTKM_CONT
  MultiBlock();

  VTKM_CONT
  MultiBlock& operator=(const vtkm::cont::MultiBlock& src);

  VTKM_CONT
  ~MultiBlock();
  /// get the field "field_name" from block "block_index"
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
  void AddBlock(const vtkm::cont::DataSet& ds);
  /// add DataSet "ds" to position "index" of the contained DataSet vector
  VTKM_CONT
  void InsertBlock(vtkm::Id index, const vtkm::cont::DataSet& ds);
  /// replace the "index" positioned element of the contained DataSet vector with "ds"
  VTKM_CONT
  void ReplaceBlock(vtkm::Id index, const vtkm::cont::DataSet& ds);
  /// append the DataSet vector "mblocks"  to the end of the contained one
  VTKM_CONT
  void AddBlocks(const std::vector<vtkm::cont::DataSet>& mblocks);

  VTKM_CONT
  void PrintSummary(std::ostream& stream) const;

  //@{
  /// API to support range-based for loops on blocks.
  std::vector<DataSet>::iterator begin() noexcept { return this->Blocks.begin(); }
  std::vector<DataSet>::iterator end() noexcept { return this->Blocks.end(); }
  std::vector<DataSet>::const_iterator begin() const noexcept { return this->Blocks.begin(); }
  std::vector<DataSet>::const_iterator end() const noexcept { return this->Blocks.end(); }
  std::vector<DataSet>::const_iterator cbegin() const noexcept { return this->Blocks.begin(); }
  std::vector<DataSet>::const_iterator cend() const noexcept { return this->Blocks.end(); }
  //@}
private:
  std::vector<vtkm::cont::DataSet> Blocks;
};
}
} // namespace vtkm::cont

#endif
