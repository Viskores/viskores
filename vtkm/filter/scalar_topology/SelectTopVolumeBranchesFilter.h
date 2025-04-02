//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
// Copyright (c) 2018, The Regents of the University of California, through
// Lawrence Berkeley National Laboratory (subject to receipt of any required approvals
// from the U.S. Dept. of Energy).  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright notice, this
//     list of conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley National
//     Laboratory, U.S. Dept. of Energy nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
//=============================================================================

#ifndef vtk_m_filter_scalar_topology_SelectTopVolumeBranchesFilter_h
#define vtk_m_filter_scalar_topology_SelectTopVolumeBranchesFilter_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/scalar_topology/vtkm_filter_scalar_topology_export.h>

namespace vtkm
{
namespace filter
{
namespace scalar_topology
{

/// \brief Compute branch decompostion from distributed contour tree
class VTKM_FILTER_SCALAR_TOPOLOGY_EXPORT SelectTopVolumeBranchesFilter : public vtkm::filter::Filter
{
public:
  VTKM_CONT SelectTopVolumeBranchesFilter() = default;

  VTKM_CONT void SetSavedBranches(const vtkm::Id& numBranches)
  {
    this->NumSavedBranches = numBranches;
  }

  VTKM_CONT void SetPresimplifyThreshold(const vtkm::Id& presimpThres)
  {
    this->PresimplifyThreshold = presimpThres;
  }

  VTKM_CONT vtkm::Id GetSavedBranches() { return this->NumSavedBranches; }
  VTKM_CONT vtkm::Id GetPresimplifyThreshold() { return this->PresimplifyThreshold; }
  VTKM_CONT vtkm::cont::LogLevel GetTimingsLogLevel() { return this->TimingsLogLevel; }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet&) override;
  VTKM_CONT vtkm::cont::PartitionedDataSet DoExecutePartitions(
    const vtkm::cont::PartitionedDataSet& inData) override;

  vtkm::Id NumSavedBranches = 1;
  vtkm::Id PresimplifyThreshold = 0;
  vtkm::cont::ArrayHandle<vtkm::Id> BranchVolume;
  vtkm::cont::ArrayHandle<vtkm::Id> BranchSaddleEpsilon;
  vtkm::cont::ArrayHandle<vtkm::Id> SortedBranchByVolume;
  vtkm::cont::UnknownArrayHandle BranchSaddleIsoValue;

  // the parent branch for top volume branches
  // we only care about the parent branch for top volume branches at the moment
  // as a result, the index stored in this array follows the descending order of branch volumes
  vtkm::cont::ArrayHandle<vtkm::Id> TopVolBranchParent;

  /// Log level to be used for outputting timing information. Default is vtkm::cont::LogLevel::Perf
  vtkm::cont::LogLevel TimingsLogLevel = vtkm::cont::LogLevel::Perf;
};

} // namespace scalar_topology
} // namespace worklet
} // namespace vtkm

#endif
