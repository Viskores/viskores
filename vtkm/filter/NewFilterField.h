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

class VTKM_FILTER_CORE_EXPORT NewFilterField : public vtkm::filter::NewFilter
{
public:
  NewFilterField() { this->SetActiveCoordinateSystem(0); }

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
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::Any)
  {
    this->SetActiveField(0, name, association);
  }

  void SetActiveField(
    vtkm::IdComponent index,
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::Any)
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
  /// Select the coordinate system coord_idx to make active to use when processing the input
  /// DataSet. This is used primarily by the Filter to select the coordinate system
  /// to use as a field when \c UseCoordinateSystemAsField is true.
  VTKM_CONT
  void SetActiveCoordinateSystem(vtkm::Id coord_idx)
  {
    this->SetActiveCoordinateSystem(0, coord_idx);
  }

  VTKM_CONT
  void SetActiveCoordinateSystem(vtkm::IdComponent index, vtkm::Id coord_idx)
  {
    auto index_st = static_cast<std::size_t>(index);
    ResizeIfNeeded(index_st);
    this->ActiveCoordinateSystemIndices[index_st] = coord_idx;
  }

  VTKM_CONT
  vtkm::Id GetActiveCoordinateSystemIndex() const
  {
    return this->GetActiveCoordinateSystemIndex(0);
  }

  VTKM_CONT
  vtkm::Id GetActiveCoordinateSystemIndex(vtkm::IdComponent index) const
  {
    auto index_st = static_cast<std::size_t>(index);
    return this->ActiveCoordinateSystemIndices[index_st];
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
    this->ResizeIfNeeded(index_st);
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
    return this->GetFieldFromDataSet(0, input);
  }

  VTKM_CONT
  const vtkm::cont::Field& GetFieldFromDataSet(vtkm::IdComponent index,
                                               const vtkm::cont::DataSet& input) const
  {
    if (this->UseCoordinateSystemAsField[index])
    {
      return input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex(index));
    }
    else
    {
      return input.GetField(this->GetActiveFieldName(index),
                            this->GetActiveFieldAssociation(index));
    }
  }

  template <typename Functor, typename... Args>
  VTKM_CONT void CastAndCallScalarField(const vtkm::cont::UnknownArrayHandle& fieldArray,
                                        Functor&& functor,
                                        Args&&... args) const
  {
    fieldArray
      .CastAndCallForTypesWithFloatFallback<vtkm::TypeListFieldScalar, VTKM_DEFAULT_STORAGE_LIST>(
        std::forward<Functor>(functor), std::forward<Args>(args)...);
  }

  template <typename Functor, typename... Args>
  VTKM_CONT void CastAndCallScalarField(const vtkm::cont::Field& field,
                                        Functor&& functor,
                                        Args&&... args) const
  {
    this->CastAndCallScalarField(
      field.GetData(), std::forward<Functor>(functor), std::forward<Args>(args)...);
  }


private:
  template <vtkm::IdComponent VecSize>
  struct ScalarToVec
  {
    template <typename T>
    using type = vtkm::Vec<T, VecSize>;
  };

protected:
  template <vtkm::IdComponent VecSize, typename Functor, typename... Args>
  VTKM_CONT void CastAndCallVecField(const vtkm::cont::UnknownArrayHandle& fieldArray,
                                     Functor&& functor,
                                     Args&&... args) const
  {
    using VecList =
      vtkm::ListTransform<vtkm::TypeListFieldScalar, ScalarToVec<VecSize>::template type>;
    fieldArray.CastAndCallForTypesWithFloatFallback<VecList, VTKM_DEFAULT_STORAGE_LIST>(
      std::forward<Functor>(functor), std::forward<Args>(args)...);
  }

  template <vtkm::IdComponent VecSize, typename Functor, typename... Args>
  VTKM_CONT void CastAndCallVecField(const vtkm::cont::Field& field,
                                     Functor&& functor,
                                     Args&&... args) const
  {
    this->CastAndCallVecField<VecSize>(
      field.GetData(), std::forward<Functor>(functor), std::forward<Args>(args)...);
  }

  /// \brief Create the output data set for `DoExecute`
  ///
  /// This form of `CreateResult` will create an output data set with the same cell
  /// structure and coordinate system as the input and pass all fields (as requested
  /// by the `Filter` state). Additionally, it will add the provided field to the
  /// result.
  ///
  /// \param[in] inDataSet The input data set being modified (usually the one passed
  ///     into `DoExecute`). The returned `DataSet` is filled with fields of `inDataSet`
  ///     (as selected by the `FieldsToPass` state of the filter).
  /// \param[in] resultField A `Field` that is added to the returned `DataSet`.
  ///
  VTKM_CONT vtkm::cont::DataSet CreateResultField(const vtkm::cont::DataSet& inDataSet,
                                                  const vtkm::cont::Field& resultField) const;

  /// \brief Create the output data set for `DoExecute`
  ///
  /// This form of `CreateResult` will create an output data set with the same cell
  /// structure and coordinate system as the input and pass all fields (as requested
  /// by the `Filter` state). Additionally, it will add a field matching the provided
  /// specifications to the result.
  ///
  /// \param[in] inDataSet The input data set being modified (usually the one passed
  ///     into `DoExecute`). The returned `DataSet` is filled with fields of `inDataSet`
  ///     (as selected by the `FieldsToPass` state of the filter).
  /// \param[in] resultFieldName The name of the field added to the returned `DataSet`.
  /// \param[in] resultFieldAssociation The association of the field (e.g. point or cell)
  ///     added to the returned `DataSet`.
  /// \param[in] resultFieldArray An array containing the data for the field added to the
  ///     returned `DataSet`.
  ///
  VTKM_CONT vtkm::cont::DataSet CreateResultField(
    const vtkm::cont::DataSet& inDataSet,
    const std::string& resultFieldName,
    vtkm::cont::Field::Association resultFieldAssociation,
    const vtkm::cont::UnknownArrayHandle& resultFieldArray) const
  {
    return this->CreateResultField(
      inDataSet, vtkm::cont::Field{ resultFieldName, resultFieldAssociation, resultFieldArray });
  }

  /// \brief Create the output data set for `DoExecute`
  ///
  /// This form of `CreateResult` will create an output data set with the same cell
  /// structure and coordinate system as the input and pass all fields (as requested
  /// by the `Filter` state). Additionally, it will add a point field matching the
  /// provided specifications to the result.
  ///
  /// \param[in] inDataSet The input data set being modified (usually the one passed
  ///     into `DoExecute`). The returned `DataSet` is filled with fields of `inDataSet`
  ///     (as selected by the `FieldsToPass` state of the filter).
  /// \param[in] resultFieldName The name of the field added to the returned `DataSet`.
  /// \param[in] resultFieldArray An array containing the data for the field added to the
  ///     returned `DataSet`.
  ///
  VTKM_CONT vtkm::cont::DataSet CreateResultFieldPoint(
    const vtkm::cont::DataSet& inDataSet,
    const std::string& resultFieldName,
    const vtkm::cont::UnknownArrayHandle& resultFieldArray) const
  {
    return this->CreateResultField(inDataSet,
                                   vtkm::cont::Field{ resultFieldName,
                                                      vtkm::cont::Field::Association::Points,
                                                      resultFieldArray });
  }

  /// \brief Create the output data set for `DoExecute`
  ///
  /// This form of `CreateResult` will create an output data set with the same cell
  /// structure and coordinate system as the input and pass all fields (as requested
  /// by the `Filter` state). Additionally, it will add a cell field matching the
  /// provided specifications to the result.
  ///
  /// \param[in] inDataSet The input data set being modified (usually the one passed
  ///     into `DoExecute`). The returned `DataSet` is filled with fields of `inDataSet`
  ///     (as selected by the `FieldsToPass` state of the filter).
  /// \param[in] resultFieldName The name of the field added to the returned `DataSet`.
  /// \param[in] resultFieldArray An array containing the data for the field added to the
  ///     returned `DataSet`.
  ///
  VTKM_CONT vtkm::cont::DataSet CreateResultFieldCell(
    const vtkm::cont::DataSet& inDataSet,
    const std::string& resultFieldName,
    const vtkm::cont::UnknownArrayHandle& resultFieldArray) const
  {
    return this->CreateResultField(inDataSet,
                                   vtkm::cont::Field{ resultFieldName,
                                                      vtkm::cont::Field::Association::Cells,
                                                      resultFieldArray });
  }

private:
  void ResizeIfNeeded(size_t index_st);

  std::string OutputFieldName;

  std::vector<std::string> ActiveFieldNames;
  std::vector<vtkm::cont::Field::Association> ActiveFieldAssociation;
  std::vector<bool> UseCoordinateSystemAsField;
  std::vector<vtkm::Id> ActiveCoordinateSystemIndices;
};
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_NewFilterField_h
