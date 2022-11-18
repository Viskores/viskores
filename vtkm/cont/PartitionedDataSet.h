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
#include <vtkm/cont/internal/FieldCollection.h>

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

  /// Create a new PartitionedDataSet with a DataSet vector @a partitions.
  VTKM_CONT
  explicit PartitionedDataSet(const std::vector<vtkm::cont::DataSet>& partitions);
  /// Create a new PartitionedDataSet with the capacity set to be @a size.
  VTKM_CONT
  explicit PartitionedDataSet(vtkm::Id size);

  VTKM_CONT
  PartitionedDataSet() = default;

  /// Get the field @a field_name from partition @a partition_index.
  VTKM_CONT
  vtkm::cont::Field GetFieldFromPartition(const std::string& field_name, int partition_index) const;

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

  ///@{
  /// Methods to Add and Get fields on a PartitionedDataSet
  VTKM_CONT
  vtkm::IdComponent GetNumberOfFields() const { return this->Fields.GetNumberOfFields(); }

  //Fields on partitions.
  VTKM_CONT void AddField(const Field& field) { this->Fields.AddField(field); }

  template <typename T, typename Storage>
  VTKM_CONT void AddGlobalField(const std::string& fieldName,
                                const vtkm::cont::ArrayHandle<T, Storage>& field)
  {
    this->AddField(vtkm::cont::Field(fieldName, vtkm::cont::Field::Association::Global, field));
  }

  template <typename T>
  VTKM_CONT void AddGlobalField(const std::string& fieldName, const std::vector<T>& field)
  {
    this->AddField(
      make_Field(fieldName, vtkm::cont::Field::Association::Global, field, vtkm::CopyFlag::On));
  }

  template <typename T>
  VTKM_CONT void AddGlobalField(const std::string& fieldName, const T* field, const vtkm::Id& n)
  {
    this->AddField(
      make_Field(fieldName, vtkm::cont::Field::Association::Global, field, n, vtkm::CopyFlag::On));
  }

  template <typename T, typename Storage>
  VTKM_CONT void AddPartitionsField(const std::string& fieldName,
                                    const vtkm::cont::ArrayHandle<T, Storage>& field)
  {
    this->AddField(vtkm::cont::Field(fieldName, vtkm::cont::Field::Association::Partitions, field));
  }

  template <typename T>
  VTKM_CONT void AddPartitionsField(const std::string& fieldName, const std::vector<T>& field)
  {
    this->AddField(
      make_Field(fieldName, vtkm::cont::Field::Association::Partitions, field, vtkm::CopyFlag::On));
  }

  template <typename T>
  VTKM_CONT void AddPartitionsField(const std::string& fieldName, const T* field, const vtkm::Id& n)
  {
    this->AddField(make_Field(
      fieldName, vtkm::cont::Field::Association::Partitions, field, n, vtkm::CopyFlag::On));
  }

  VTKM_CONT
  const vtkm::cont::Field& GetField(vtkm::Id index) const { return this->Fields.GetField(index); }

  VTKM_CONT
  vtkm::cont::Field& GetField(vtkm::Id index) { return this->Fields.GetField(index); }

  VTKM_CONT
  const vtkm::cont::Field& GetField(
    const std::string& name,
    vtkm::cont::Field::Association assoc = vtkm::cont::Field::Association::Any) const
  {
    return this->Fields.GetField(name, assoc);
  }

  VTKM_CONT
  vtkm::cont::Field& GetField(
    const std::string& name,
    vtkm::cont::Field::Association assoc = vtkm::cont::Field::Association::Any)
  {
    return this->Fields.GetField(name, assoc);
  }

  VTKM_CONT
  const vtkm::cont::Field& GetGlobalField(const std::string& name) const
  {
    return this->GetField(name, vtkm::cont::Field::Association::Global);
  }

  VTKM_CONT
  const vtkm::cont::Field& GetPartitionsField(const std::string& name) const
  {
    return this->GetField(name, vtkm::cont::Field::Association::Partitions);
  }

  VTKM_CONT
  vtkm::cont::Field& GetGlobalField(const std::string& name)
  {
    return this->GetField(name, vtkm::cont::Field::Association::Global);
  }

  VTKM_CONT
  vtkm::cont::Field& GetPartitionsField(const std::string& name)
  {
    return this->GetField(name, vtkm::cont::Field::Association::Partitions);
  }

  VTKM_CONT
  bool HasField(const std::string& name,
                vtkm::cont::Field::Association assoc = vtkm::cont::Field::Association::Any) const
  {
    return this->Fields.HasField(name, assoc);
  }

  VTKM_CONT
  bool HasGlobalField(const std::string& name) const
  {
    return (this->Fields.GetFieldIndex(name, vtkm::cont::Field::Association::Global) != -1);
  }

  VTKM_CONT
  bool HasPartitionsField(const std::string& name) const
  {
    return (this->Fields.GetFieldIndex(name, vtkm::cont::Field::Association::Partitions) != -1);
  }
  ///@}

  /// Copies the partitions from the source. The fields on the PartitionedDataSet are not copied.
  VTKM_CONT
  void CopyPartitions(const vtkm::cont::PartitionedDataSet& source);

  VTKM_CONT
  void PrintSummary(std::ostream& stream) const;

  ///@{
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
  ///@}

private:
  std::vector<vtkm::cont::DataSet> Partitions;

  vtkm::cont::internal::FieldCollection Fields{ vtkm::cont::Field::Association::Partitions,
                                                vtkm::cont::Field::Association::Global };
};
}
} // namespace vtkm::cont

#endif
