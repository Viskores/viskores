//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_AmrArrays_h
#define vtk_m_filter_AmrArrays_h

#include <vtkm/filter/FilterDataSet.h>

namespace vtkm
{
namespace filter
{

class AmrArrays : public vtkm::filter::FilterDataSet<AmrArrays>
{
public:
  using SupportedTypes = vtkm::List<vtkm::UInt8>;

  VTKM_CONT
  AmrArrays();

  template <typename DerivedPolicy>
  vtkm::cont::PartitionedDataSet PrepareForExecution(
    const vtkm::cont::PartitionedDataSet& input,
    const vtkm::filter::PolicyBase<DerivedPolicy>&);

  template <typename DerivedPolicy>
  VTKM_CONT bool MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                    const vtkm::cont::Field& field,
                                    const vtkm::filter::PolicyBase<DerivedPolicy>&)
  {
    result.AddField(field);
    return true;
  }

private:
  /// the list of ids contains all amrIds of the level above/below that have an overlap
  VTKM_CONT
  void GenerateParentChildInformation();

  /// the corresponding template function based on the dimension of this dataset
  VTKM_CONT
  template <vtkm::IdComponent Dim>
  void ComputeGenerateParentChildInformation();

  /// generate the vtkGhostType array based on the overlap analogously to vtk
  /// blanked cells: 8 normal cells: 0
  VTKM_CONT
  void GenerateGhostType();

  /// the corresponding template function based on the dimension of this dataset
  VTKM_CONT
  template <vtkm::IdComponent Dim>
  void ComputeGenerateGhostType();

  /// Add helper arrays like in ParaView
  VTKM_CONT
  void GenerateIndexArrays();

  /// the input partitioned dataset
  vtkm::cont::PartitionedDataSet AmrDataSet;

  /// per level
  /// contains the index where the PartitionIds start for each level
  std::vector<std::vector<vtkm::Id>> PartitionIds;

  /// per partitionId
  /// contains all PartitonIds of the level above that have an overlap
  std::vector<std::vector<vtkm::Id>> ParentsIdsVector;

  /// per partitionId
  /// contains all PartitonIds of the level below that have an overlap
  std::vector<std::vector<vtkm::Id>> ChildrenIdsVector;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/AmrArrays.hxx>

#endif //vtk_m_filter_AmrArrays_h
