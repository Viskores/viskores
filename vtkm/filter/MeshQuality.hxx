//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//=========================================================================
#ifndef vtk_m_filter_MeshQuality_hxx
#define vtk_m_filter_MeshQuality_hxx

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/Field.h>
#include <vtkm/filter/CellMeasures.h>
#include <vtkm/filter/CreateResult.h>

// #define DEBUG_PRINT

namespace vtkm
{
namespace filter
{


inline VTKM_CONT MeshQuality::MeshQuality(CellMetric metric)
  : vtkm::filter::FilterField<MeshQuality>()
{
  this->SetUseCoordinateSystemAsField(true);
  this->MyMetric = metric;
  if (this->MyMetric < CellMetric::AREA || this->MyMetric >= CellMetric::NUMBER_OF_CELL_METRICS)
  {
    VTKM_ASSERT(true);
  }
  this->SetOutputFieldName(MetricNames[(int)this->MyMetric]);
}

template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet MeshQuality::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& points,
  const vtkm::filter::FieldMetadata& fieldMeta,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  VTKM_ASSERT(fieldMeta.IsPointField());

  //TODO: Should other cellset types be supported?
  vtkm::cont::CellSetExplicit<> cellSet;
  input.GetCellSet().CopyTo(cellSet);

  vtkm::worklet::MeshQuality<CellMetric> qualityWorklet;

  if (this->MyMetric == vtkm::filter::CellMetric::RELATIVE_SIZE_SQUARED ||
      this->MyMetric == vtkm::filter::CellMetric::SHAPE_AND_SIZE)
  {
    vtkm::FloatDefault averageArea = 1.;
    vtkm::worklet::MeshQuality<CellMetric> subWorklet;
    vtkm::cont::ArrayHandle<T> array;
    subWorklet.SetMetric(vtkm::filter::CellMetric::AREA);
    this->Invoke(subWorklet, vtkm::filter::ApplyPolicyCellSet(cellSet, policy), points, array);
    T zero = 0.0;
    vtkm::FloatDefault totalArea = (vtkm::FloatDefault)vtkm::cont::Algorithm::Reduce(array, zero);

    vtkm::FloatDefault averageVolume = 1.;
    subWorklet.SetMetric(vtkm::filter::CellMetric::VOLUME);
    this->Invoke(subWorklet, vtkm::filter::ApplyPolicyCellSet(cellSet, policy), points, array);
    vtkm::FloatDefault totalVolume = (vtkm::FloatDefault)vtkm::cont::Algorithm::Reduce(array, zero);

    vtkm::Id numVals = array.GetNumberOfValues();
    if (numVals > 0)
    {
      averageArea = totalArea / static_cast<vtkm::FloatDefault>(numVals);
      averageVolume = totalVolume / static_cast<vtkm::FloatDefault>(numVals);
    }
    qualityWorklet.SetAverageArea(averageArea);
    qualityWorklet.SetAverageVolume(averageVolume);
  }

  //Invoke the MeshQuality worklet
  vtkm::cont::ArrayHandle<T> outArray;
  qualityWorklet.SetMetric(this->MyMetric);
  this->Invoke(qualityWorklet, vtkm::filter::ApplyPolicyCellSet(cellSet, policy), points, outArray);

  vtkm::cont::DataSet result;
  result.CopyStructure(input); //clone of the input dataset

  //Append the metric values of all cells into the output
  //dataset as a new field
  result.AddField(vtkm::cont::make_FieldCell(this->GetOutputFieldName(), outArray));

  return result;
}

} // namespace filter
} // namespace vtkm
#endif
