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

#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/Field.h>
#include <vtkm/filter/CreateResult.h>

// #define DEBUG_PRINT

namespace vtkm
{
namespace filter
{

namespace debug
{
#ifdef DEBUG_PRINT
//----------------------------------------------------------------------------
template <typename T, typename S = vtkm::cont::DeviceAdapterId>
void MeshQualityDebug(const vtkm::cont::ArrayHandle<T, S>& outputArray, const char* name)
{
  typedef vtkm::cont::internal::Storage<T, S> StorageType;
  typedef typename StorageType::PortalConstType PortalConstType;
  PortalConstType readPortal = outputArray.GetPortalConstControl();
  vtkm::Id numElements = readPortal.GetNumberOfValues();
  std::cout << name << "= " << numElements << " [";
  for (vtkm::Id i = 0; i < numElements; i++)
    std::cout << (int)readPortal.Get(i) << " ";
  std::cout << "]\n";
}
#else
template <typename T, typename S>
void MeshQualityDebug(const vtkm::cont::ArrayHandle<T, S>& vtkmNotUsed(outputArray),
                      const char* vtkmNotUsed(name))
{
}
#endif
} // namespace debug


inline VTKM_CONT MeshQuality::MeshQuality(CellMetric metric)
  : vtkm::filter::FilterCell<MeshQuality>()
{
  this->SetUseCoordinateSystemAsField(true);
  this->MyMetric = metric;
  if (this->MyMetric < CellMetric::AREA || this->MyMetric >= CellMetric::NUMBER_OF_CELL_METRICS)
  {
    VTKM_ASSERT(true);
  }
  this->OutputName = MetricNames[(int)this->MyMetric];
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

  //Invoke the MeshQuality worklet
  vtkm::cont::ArrayHandle<T> outArray;
  vtkm::worklet::MeshQuality<CellMetric> qualityWorklet;
  qualityWorklet.SetMetric(this->MyMetric);
  this->Invoke(qualityWorklet, vtkm::filter::ApplyPolicyCellSet(cellSet, policy), points, outArray);

  vtkm::cont::DataSet result;
  result.CopyStructure(input); //clone of the input dataset

  //Append the metric values of all cells into the output
  //dataset as a new field
  result.AddField(vtkm::cont::make_FieldCell(this->OutputName, outArray));

  return result;
}

} // namespace filter
} // namespace vtkm
