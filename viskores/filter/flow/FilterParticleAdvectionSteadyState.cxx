//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/filter/flow/FilterParticleAdvectionSteadyState.h>

#include <viskores/filter/flow/internal/BoundsMap.h>
#include <viskores/filter/flow/internal/DataSetIntegratorSteadyState.h>
#include <viskores/filter/flow/internal/ParticleAdvector.h>

namespace viskores
{
namespace filter
{
namespace flow
{

template <typename Derived>
VISKORES_CONT typename FilterParticleAdvectionSteadyState<Derived>::FieldType
FilterParticleAdvectionSteadyState<Derived>::GetField(const viskores::cont::DataSet& data) const
{
  const Derived* inst = static_cast<const Derived*>(this);
  return inst->GetField(data);
}

template <typename Derived>
VISKORES_CONT typename FilterParticleAdvectionSteadyState<Derived>::TerminationType
FilterParticleAdvectionSteadyState<Derived>::GetTermination(
  const viskores::cont::DataSet& data) const
{
  const Derived* inst = static_cast<const Derived*>(this);
  return inst->GetTermination(data);
}

template <typename Derived>
VISKORES_CONT typename FilterParticleAdvectionSteadyState<Derived>::AnalysisType
FilterParticleAdvectionSteadyState<Derived>::GetAnalysis(const viskores::cont::DataSet& data) const
{
  const Derived* inst = static_cast<const Derived*>(this);
  return inst->GetAnalysis(data);
}

#if 0
template <typename Derived>
VISKORES_CONT void FilterParticleAdvectionSteadyState<Derived>::BuildBoundsLocator(
  const viskores::cont::PartitionedDataSet& input)
{
  // Build hexahedrons for each partition.
  std::vector<viskores::Bounds> partitionBounds;
  for (const auto& ds : input.GetPartitions())
    partitionBounds.push_back(ds.GetBounds());
  int numLocalBlocks = input.GetNumberOfPartitions();

  std::vector<viskores::Bounds> globalPartitionsBounds;
  //#ifdef VISKORES_ENABLE_MPI
  const auto comm = viskores::cont::EnvironmentTracker::GetCommunicator();
  int rank = comm.rank(), numRanks = comm.size();

  std::vector<int> blocksPerRank(0, static_cast<size_t>(numRanks));
  comm.all_gather(numLocalBlocks, blocksPerRank.data(), 1, MPI_INT);
  //comm.all_gather(numLocalBlocks, blocksPerRank.data(), 1, MPI_INT);
  viskores::Id totalBlocks =
    std::accumulate(blocksPerRank.begin(), blocksPerRank.end(), viskores::Id(0));
  std::vector<std::size_t> offsets(numRanks, 0);
  for (std::size_t i = 1; i < numRanks; i++)
    offsets[i] = offsets[i - 1] + blocksPerRank[i - 1];

  //Put local bounds into global vector.
  globalPartitionsBounds.resize(totalBlocks);
  for (viskores::Id i = 0; i < numLocalBlocks; i++)
    globalPartitionsBounds[offsets[rank] + i] = partitionBounds[static_cast<std::size_t>(i)];

  //comm.all_gather(
  //    globalPartitionsBounds.data(), globalPartitionsBounds.size(), viskores::BoundsType());

  //#endif
  this->BoundsLocator = viskores::cont::CellLocatorTwoLevel(input);
}
#endif

template <typename Derived>
VISKORES_CONT viskores::cont::PartitionedDataSet
FilterParticleAdvectionSteadyState<Derived>::DoExecutePartitions(
  const viskores::cont::PartitionedDataSet& input)
{
  //using ParticleType    = FilterParticleAdvectionSteadyState<Derived>::ParticleType;
  //using FieldType       = FilterParticleAdvectionSteadyState<Derived>::FieldType;
  //using TerminationType = FilterParticleAdvectionSteadyState<Derived>::TerminationType;
  //using AnalysisType    = FilterParticleAdvectionSteadyState<Derived>::AnalysisType;
  using DSIType = viskores::filter::flow::internal::
    DataSetIntegratorSteadyState<ParticleType, FieldType, TerminationType, AnalysisType>;

  this->ValidateOptions();
  this->BuildBoundsLocator(input);
  if (this->BlockIdsSet)
    this->BoundsMap = viskores::filter::flow::internal::BoundsMap(input, this->BlockIds);
  else
    this->BoundsMap = viskores::filter::flow::internal::BoundsMap(input);

  std::vector<DSIType> dsi;
  for (viskores::Id i = 0; i < input.GetNumberOfPartitions(); i++)
  {
    viskores::Id blockId = this->BoundsMap.GetLocalBlockId(i);
    auto dataset = input.GetPartition(i);

    // Build the field for the current dataset
    FieldType field = this->GetField(dataset);
    // Build the termination for the current dataset
    TerminationType termination = this->GetTermination(dataset);
    // Build the analysis for the current dataset
    AnalysisType analysis = this->GetAnalysis(dataset);

    dsi.emplace_back(blockId, field, dataset, this->SolverType, termination, analysis);
  }

  viskores::filter::flow::internal::ParticleAdvector<DSIType> pav(
    this->BoundsMap, dsi, this->UseThreadedAlgorithm);

  viskores::cont::ArrayHandle<ParticleType> particles;
  this->Seeds.AsArrayHandle(particles);
  return pav.Execute(particles, this->StepSize);
}

}
}
} // namespace viskores::filter::flow

#include <viskores/filter/flow/ParticleAdvection.h>
#include <viskores/filter/flow/Streamline.h>
#include <viskores/filter/flow/WarpXStreamline.h>

namespace viskores
{
namespace filter
{
namespace flow
{

template class FilterParticleAdvectionSteadyState<viskores::filter::flow::ParticleAdvection>;
template class FilterParticleAdvectionSteadyState<viskores::filter::flow::Streamline>;
template class FilterParticleAdvectionSteadyState<viskores::filter::flow::WarpXStreamline>;

} // namespace flow
} // namespace filter
} // namespace viskores
