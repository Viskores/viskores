//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_NewFilterField_h
#define vtk_m_filter_NewFilterField_h

#include <vtkm/filter/NewFilter.h>

namespace vtkm
{
namespace filter
{

class NewFilterField : public vtkm::filter::NewFilter
{
public:
  VTKM_CONT
  void SetOutputFieldName(const std::string& name) { this->OutputFieldName = name; }

  VTKM_CONT
  const std::string& GetOutputFieldName() const { return this->OutputFieldName; }

  //@{
  /// Choose the field to operate on. Note, if
  /// `this->UseCoordinateSystemAsField` is true, then the active field is not used.
  VTKM_CONT
  void SetActiveField(
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::ANY)
  {
    this->SetActiveField(0, name, association);
  }

  void SetActiveField(
    vtkm::IdComponent index,
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::ANY)
  {
    auto index_st = static_cast<std::size_t>(index);
    ResizeIfNeeded(index_st);
    this->ActiveFieldNames[index_st] = name;
    this->ActiveFieldAssociation[index_st] = association;
  }

  VTKM_CONT const std::string& GetActiveFieldName(vtkm::IdComponent index = 0) const
  {
    VTKM_ASSERT((index >= 0) &&
                (index < static_cast<vtkm::IdComponent>(this->ActiveFieldNames.size())));
    return this->ActiveFieldNames[index];
  }

  VTKM_CONT vtkm::cont::Field::Association GetActiveFieldAssociation(
    vtkm::IdComponent index = 0) const
  {
    return this->ActiveFieldAssociation[index];
  }
  //@}

  //@{
  /// To simply use the active coordinate system as the field to operate on, set
  /// UseCoordinateSystemAsField to true.
  VTKM_CONT
  void SetUseCoordinateSystemAsField(bool val) { SetUseCoordinateSystemAsField(0, val); }

  VTKM_CONT
  void SetUseCoordinateSystemAsField(vtkm::IdComponent index, bool val)
  {
    auto index_st = static_cast<std::size_t>(index);
    ResizeIfNeeded(index_st);
    this->UseCoordinateSystemAsField[index] = val;
  }

  VTKM_CONT
  bool GetUseCoordinateSystemAsField(vtkm::IdComponent index = 0) const
  {
    VTKM_ASSERT((index >= 0) &&
                (index < static_cast<vtkm::IdComponent>(this->ActiveFieldNames.size())));
    return this->UseCoordinateSystemAsField[index];
  }
  //@}

protected:
  VTKM_CONT
  const vtkm::cont::Field& GetFieldFromDataSet(const vtkm::cont::DataSet& input) const
  {
    return GetFieldFromDataSet(0, input);
  }

  VTKM_CONT
  const vtkm::cont::Field& GetFieldFromDataSet(vtkm::IdComponent index,
                                               const vtkm::cont::DataSet& input) const
  {
    if (this->UseCoordinateSystemAsField[index])
    {
      return input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());
    }
    else
    {
      return input.GetField(this->GetActiveFieldName(index),
                            this->GetActiveFieldAssociation(index));
    }
  }

private:
  void ResizeIfNeeded(size_t index_st)
  {
    if (ActiveFieldNames.size() <= index_st)
    {
      auto oldSize = ActiveFieldNames.size();
      ActiveFieldNames.resize(index_st + 1);
      ActiveFieldAssociation.resize(index_st + 1);
      UseCoordinateSystemAsField.resize(index_st + 1);
      for (std::size_t i = oldSize; i <= index_st; ++i)
      {
        ActiveFieldAssociation[i] = cont::Field::Association::ANY;
        UseCoordinateSystemAsField[i] = false;
      }
    }
  }

  std::string OutputFieldName;

  std::vector<std::string> ActiveFieldNames;
  std::vector<vtkm::cont::Field::Association> ActiveFieldAssociation;
  std::vector<bool> UseCoordinateSystemAsField;
};
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_NewFilterField_h
