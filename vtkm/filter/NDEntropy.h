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
#ifndef vtk_m_filter_NDEntropy_h
#define vtk_m_filter_NDEntropy_h

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
class NDEntropy : public vtkm::filter::FilterDataSet<NDEntropy>
{
public:
  VTKM_CONT
  NDEntropy();

  VTKM_CONT
  void AddFieldAndBin(const std::string& fieldName, vtkm::Id numOfBins);

  template <typename Policy, typename Device>
  VTKM_CONT vtkm::filter::Result DoExecute(const vtkm::cont::DataSet& inData,
                                           vtkm::filter::PolicyBase<Policy> policy,
                                           Device);

private:
  std::vector<vtkm::Id> NumOfBins;
  std::vector<std::string> FieldNames;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/NDEntropy.hxx>

#endif //vtk_m_filter_NDEntropy_h
