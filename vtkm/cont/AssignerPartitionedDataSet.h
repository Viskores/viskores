//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_AssignerPartitionedDataSet_h
#define vtk_m_cont_AssignerPartitionedDataSet_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/Types.h>
#include <vtkm/internal/ExportMacros.h>
#include <vtkm/thirdparty/diy/Configure.h>

#include <vector>

#include <vtkm/thirdparty/diy/diy.h>

#ifdef VTKM_MSVC
#pragma warning(push)
// disable C4275: non-dll interface base class warnings
#pragma warning(disable : 4275)
#endif

namespace vtkm
{
namespace cont
{

class PartitionedDataSet;

/// \brief Assigner for PartitionedDataSet partitions.
///
/// `AssignerPartitionedDataSet` is a `vtkmdiy::StaticAssigner` implementation
/// that uses `PartitionedDataSet`'s partition distribution to build
/// global-id/rank associations needed for several `diy` operations.
/// It uses a contiguous assignment strategy to map partitions to global ids,
/// i.e. partitions on rank 0 come first, then rank 1, etc. Any rank may have 0
/// partitions.
///
/// AssignerPartitionedDataSet uses collectives in the constructor hence it is
/// essential it gets created on all ranks irrespective of whether the rank has
/// any partitions.
///
class VTKM_CONT_EXPORT AssignerPartitionedDataSet : public vtkmdiy::StaticAssigner
{
public:
  /// Initialize the assigner using a partitioned dataset.
  /// This may initialize collective operations to populate the assigner with
  /// information about partitions on all ranks.
  VTKM_CONT
  AssignerPartitionedDataSet(const vtkm::cont::PartitionedDataSet& pds);

  VTKM_CONT
  AssignerPartitionedDataSet(vtkm::Id num_partitions);

  VTKM_CONT
  virtual ~AssignerPartitionedDataSet();

  ///@{
  /// vtkmdiy::Assigner API implementation.
  VTKM_CONT
  void local_gids(int rank, std::vector<int>& gids) const override;

  VTKM_CONT
  int rank(int gid) const override;
  //@}
private:
  std::vector<vtkm::Id> IScanPartitionCounts;
};
}
}

#ifdef VTKM_MSVC
#pragma warning(pop)
#endif

#endif
