//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_filter_NDHistogram_h
#define vtk_m_filter_NDHistogram_h

#include <vtkm/filter/FilterDataSet.h>

namespace vtkm
{
namespace filter
{
/// \brief Clean a mesh to an unstructured grid
///
/// This filter takes a data set and essentially copies it into a new data set.
/// The newly constructed data set will have the same cells as the input and
/// the topology will be stored in a \c CellSetExplicit<>. The filter will also
/// optionally remove all unused points.
///
/// Note that the result of \c CleanGrid is not necessarily smaller than the
/// input. For example, "cleaning" a data set with a \c CellSetStructured
/// topology will actually result in a much larger data set.
///
/// \todo Add a feature to merge points that are coincident or within a
/// tolerance.
///
class NDHistogram : public vtkm::filter::FilterDataSet<NDHistogram>
{
public:
  VTKM_CONT
  NDHistogram();

  VTKM_CONT
  void AddFieldAndBin(const std::string& fieldName, vtkm::Id numOfBins);

  template <typename Policy, typename Device>
  VTKM_CONT vtkm::filter::Result DoExecute(const vtkm::cont::DataSet& inData,
                                           vtkm::filter::PolicyBase<Policy> policy,
                                           Device);

  // This index is the field position in FieldNames
  // (or the input _fieldName string vector of SetFields() Function)
  VTKM_CONT
  vtkm::Float64 GetBinDelta(size_t fieldIdx);

  // This index is the field position in FieldNames
  // (or the input _fieldName string vector of SetFields() Function)
  VTKM_CONT
  vtkm::Range GetDataRange(size_t fieldIdx);

private:
  std::vector<vtkm::Id> NumOfBins;
  std::vector<std::string> FieldNames;
  std::vector<vtkm::Float64> BinDeltas;
  std::vector<vtkm::Range> DataRanges; //Min Max of the field
};
}
} // namespace vtkm::filter

#include <vtkm/filter/NDHistogram.hxx>

#endif //vtk_m_filter_NDHistogram_h
