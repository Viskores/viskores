//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/StaticAssert.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/PartitionedDataSet.h>

namespace vtkm
{
namespace cont
{

VTKM_CONT
PartitionedDataSet::PartitionedDataSet(const vtkm::cont::DataSet& ds)
{
  this->Partitions.insert(this->Partitions.end(), ds);
}

VTKM_CONT
PartitionedDataSet::PartitionedDataSet(const vtkm::cont::PartitionedDataSet& src)
{
  this->Partitions = src.GetPartitions();
}

VTKM_CONT
PartitionedDataSet::PartitionedDataSet(const std::vector<vtkm::cont::DataSet>& partitions)
{
  this->Partitions = partitions;
}

VTKM_CONT
PartitionedDataSet::PartitionedDataSet(vtkm::Id size)
{
  this->Partitions.reserve(static_cast<std::size_t>(size));
}

VTKM_CONT
PartitionedDataSet::PartitionedDataSet()
{
}

VTKM_CONT
PartitionedDataSet::~PartitionedDataSet()
{
}

VTKM_CONT
PartitionedDataSet& PartitionedDataSet::operator=(const vtkm::cont::PartitionedDataSet& src)
{
  this->Partitions = src.GetPartitions();
  return *this;
}

VTKM_CONT
vtkm::cont::Field PartitionedDataSet::GetField(const std::string& field_name, int partition_index)
{
  assert(partition_index >= 0);
  assert(static_cast<std::size_t>(partition_index) < this->Partitions.size());
  return this->Partitions[static_cast<std::size_t>(partition_index)].GetField(field_name);
}

VTKM_CONT
vtkm::Id PartitionedDataSet::GetNumberOfPartitions() const
{
  return static_cast<vtkm::Id>(this->Partitions.size());
}

VTKM_CONT
const vtkm::cont::DataSet& PartitionedDataSet::GetPartition(vtkm::Id blockId) const
{
  return this->Partitions[static_cast<std::size_t>(blockId)];
}

VTKM_CONT
const std::vector<vtkm::cont::DataSet>& PartitionedDataSet::GetPartitions() const
{
  return this->Partitions;
}

VTKM_CONT
void PartitionedDataSet::AppendPartition(const vtkm::cont::DataSet& ds)
{
  this->Partitions.insert(this->Partitions.end(), ds);
}

VTKM_CONT
void PartitionedDataSet::AppendPartitions(const std::vector<vtkm::cont::DataSet>& partitions)
{
  this->Partitions.insert(this->Partitions.end(), partitions.begin(), partitions.end());
}

VTKM_CONT
void PartitionedDataSet::InsertPartition(vtkm::Id index, const vtkm::cont::DataSet& ds)
{
  if (index <= static_cast<vtkm::Id>(this->Partitions.size()))
  {
    this->Partitions.insert(this->Partitions.begin() + index, ds);
  }
  else
  {
    std::string msg = "invalid insert position\n ";
    throw ErrorBadValue(msg);
  }
}

VTKM_CONT
void PartitionedDataSet::ReplacePartition(vtkm::Id index, const vtkm::cont::DataSet& ds)
{
  if (index < static_cast<vtkm::Id>(this->Partitions.size()))
    this->Partitions.at(static_cast<std::size_t>(index)) = ds;
  else
  {
    std::string msg = "invalid replace position\n ";
    throw ErrorBadValue(msg);
  }
}

VTKM_CONT
void PartitionedDataSet::PrintSummary(std::ostream& stream) const
{
  stream << "PartitionedDataSet [" << this->Partitions.size() << " partitions]:\n";

  for (size_t part = 0; part < this->Partitions.size(); ++part)
  {
    stream << "Partition " << part << ":\n";
    this->Partitions[part].PrintSummary(stream);
  }
}
}
} // namespace vtkm::cont
