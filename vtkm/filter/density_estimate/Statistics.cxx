//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/density_estimate/Statistics.h>
#include <vtkm/worklet/DescriptiveStatistics.h>

namespace vtkm
{
namespace filter
{
namespace density_estimate
{

VTKM_CONT vtkm::cont::DataSet Statistics::DoExecute(const vtkm::cont::DataSet& inData)
{
  vtkm::worklet::DescriptiveStatistics worklet;
  vtkm::cont::DataSet output;

  auto resolveType = [&](const auto& concrete) {
    auto result = worklet.Run(concrete);

    for (size_t i = 0; i < RequiredStatsList.size(); i++)
    {
      vtkm::cont::ArrayHandle<vtkm::FloatDefault> stat;
      stat.Allocate(1);
      Stats statEnum = RequiredStatsList[i];

      switch (statEnum)
      {
        case Stats::N:
        {
          stat.WritePortal().Set(0, static_cast<vtkm::FloatDefault>(result.N()));
          break;
        }
        case Stats::Min:
        {
          stat.WritePortal().Set(0, static_cast<vtkm::FloatDefault>(result.Min()));
          break;
        }
        case Stats::Max:
        {
          stat.WritePortal().Set(0, static_cast<vtkm::FloatDefault>(result.Max()));
          break;
        }
        case Stats::Sum:
        {
          stat.WritePortal().Set(0, static_cast<vtkm::FloatDefault>(result.Sum()));
          break;
        }
        case Stats::Mean:
        {
          stat.WritePortal().Set(0, static_cast<vtkm::FloatDefault>(result.Mean()));
          break;
        }
        case Stats::SampleStdDev:
        {
          stat.WritePortal().Set(0, static_cast<vtkm::FloatDefault>(result.SampleStddev()));
          break;
        }
        case Stats::PopulationStdDev:
        {
          stat.WritePortal().Set(0, static_cast<vtkm::FloatDefault>(result.PopulationStddev()));
          break;
        }
        case Stats::SampleVariance:
        {
          stat.WritePortal().Set(0, static_cast<vtkm::FloatDefault>(result.SampleVariance()));
          break;
        }
        case Stats::PopulationVariance:
        {
          stat.WritePortal().Set(0, static_cast<vtkm::FloatDefault>(result.PopulationVariance()));
          break;
        }
        case Stats::Skewness:
        {
          stat.WritePortal().Set(0, static_cast<vtkm::FloatDefault>(result.Skewness()));
          break;
        }
        case Stats::Kurtosis:
        {
          stat.WritePortal().Set(0, static_cast<vtkm::FloatDefault>(result.Kurtosis()));
          break;
        }
        default:
        {
          throw vtkm::cont::ErrorFilterExecution(
            "Unsupported statistics variable in statistics filter.");
        }
      }

      output.AddField({ this->StatsName[static_cast<int>(statEnum)],
                        vtkm::cont::Field::Association::WholeDataSet,
                        stat });
    }
  };
  const auto& fieldArray = this->GetFieldFromDataSet(inData).GetData();
  fieldArray
    .CastAndCallForTypesWithFloatFallback<vtkm::TypeListFieldScalar, VTKM_DEFAULT_STORAGE_LIST>(
      resolveType);

  return output;
}

} // namespace density_estimate
} // namespace filter
} // namespace vtkm
