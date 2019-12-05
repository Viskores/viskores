//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtkm_m_filter_CellSetConnectivity_h
#define vtkm_m_filter_CellSetConnectivity_h

#include <vtkm/filter/FilterDataSet.h>

namespace vtkm
{
namespace filter
{
/// \brief Finds groups of cells that are connected together through their topology.
///
/// Finds and labels groups of cells that are connected together through their topology.
/// Two cells are considered connected if they share an edge. CellSetConnectivity identifies some
/// number of components and assigns each component a unique integer.
/// The result of the filter is a cell field of type vtkm::Id with the default name of 'component'.
/// Each entry in the cell field will be a number that identifies to which component the cell belongs.
class CellSetConnectivity : public vtkm::filter::FilterDataSet<CellSetConnectivity>
{
public:
  using SupportedTypes = vtkm::TypeListScalarAll;
  VTKM_CONT CellSetConnectivity();

  void SetOutputFieldName(const std::string& name) { this->OutputFieldName = name; }

  VTKM_CONT
  const std::string& GetOutputFieldName() const { return this->OutputFieldName; }


  template <typename Policy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          vtkm::filter::PolicyBase<Policy> policy);

  template <typename Policy>
  VTKM_CONT bool MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                    const vtkm::cont::Field& field,
                                    const vtkm::filter::PolicyBase<Policy>&)
  {
    result.AddField(field);
    return true;
  }

private:
  std::string OutputFieldName;
};
}
}

#include <vtkm/filter/CellSetConnectivity.hxx>

#endif //vtkm_m_filter_CellSetConnectivity_h
