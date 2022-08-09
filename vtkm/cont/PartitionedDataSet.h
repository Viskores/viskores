//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_PartitionedDataSet_h
#define vtk_m_cont_PartitionedDataSet_h
#include <limits>
#include <vtkm/StaticAssert.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Field.h>

namespace vtkm
{
namespace cont
{

class VTKM_CONT_EXPORT PartitionedDataSet
{
  using StorageVec = std::vector<vtkm::cont::DataSet>;

public:
  using iterator = typename StorageVec::iterator;
  using const_iterator = typename StorageVec::const_iterator;
  using value_type = typename StorageVec::value_type;
  using reference = typename StorageVec::reference;
  using const_reference = typename StorageVec::const_reference;

  /// Create a new PartitionedDataSet containng a single DataSet @a ds.
  VTKM_CONT
  PartitionedDataSet(const vtkm::cont::DataSet& ds);
  /// Create a new PartitionedDataSet with the existing one @a src.
  VTKM_CONT
  PartitionedDataSet(const vtkm::cont::PartitionedDataSet& src);
  /// Create a new PartitionedDataSet with a DataSet vector @a partitions.
  VTKM_CONT
  explicit PartitionedDataSet(const std::vector<vtkm::cont::DataSet>& partitions);
  /// Create a new PartitionedDataSet with the capacity set to be @a size.
  VTKM_CONT
  explicit PartitionedDataSet(vtkm::Id size);

  VTKM_CONT
  PartitionedDataSet();

  VTKM_CONT
  PartitionedDataSet& operator=(const vtkm::cont::PartitionedDataSet& src);

  VTKM_CONT
  ~PartitionedDataSet();
  /// Get the field @a field_name from partition @a partition_index.
  VTKM_CONT
  vtkm::cont::Field GetField(const std::string& field_name, int partition_index) const;

  /// Get number of DataSet objects stored in this PartitionedDataSet.
  VTKM_CONT
  vtkm::Id GetNumberOfPartitions() const;

  /// Get number of partations across all MPI ranks.
  /// @warning This method requires global communication (MPI_Allreduce) if MPI is enabled.
  VTKM_CONT
  vtkm::Id GetGlobalNumberOfPartitions() const;

  /// Get the DataSet @a partId.
  VTKM_CONT
  const vtkm::cont::DataSet& GetPartition(vtkm::Id partId) const;

  /// Get an STL vector of all DataSet objects stored in PartitionedDataSet.
  VTKM_CONT
  const std::vector<vtkm::cont::DataSet>& GetPartitions() const;

  /// Add DataSet @a ds to the end of the contained DataSet vector.
  VTKM_CONT
  void AppendPartition(const vtkm::cont::DataSet& ds);

  /// Add DataSet @a ds to position @a index of the contained DataSet vector.
  VTKM_CONT
  void InsertPartition(vtkm::Id index, const vtkm::cont::DataSet& ds);

  /// Replace the @a index positioned element of the contained DataSet vector
  /// with @a ds.
  VTKM_CONT
  void ReplacePartition(vtkm::Id index, const vtkm::cont::DataSet& ds);

  /// Append the DataSet vector @a partitions to the end of the contained one.
  VTKM_CONT
  void AppendPartitions(const std::vector<vtkm::cont::DataSet>& partitions);

  VTKM_CONT
  void PrintSummary(std::ostream& stream) const;

  //@{
  /// API to support range-based for loops on partitions.
  VTKM_CONT
  iterator begin() noexcept { return this->Partitions.begin(); }
  VTKM_CONT
  iterator end() noexcept { return this->Partitions.end(); }
  VTKM_CONT
  const_iterator begin() const noexcept { return this->Partitions.begin(); }
  VTKM_CONT
  const_iterator end() const noexcept { return this->Partitions.end(); }
  VTKM_CONT
  const_iterator cbegin() const noexcept { return this->Partitions.cbegin(); }
  VTKM_CONT
  const_iterator cend() const noexcept { return this->Partitions.cend(); }
  //@}
private:
  std::vector<vtkm::cont::DataSet> Partitions;
};
}
} // namespace vtkm::cont

#endif
