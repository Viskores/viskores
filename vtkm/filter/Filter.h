//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_Filter_h
#define vtk_m_filter_Filter_h

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/MultiBlock.h>

#include <vtkm/filter/FieldSelection.h>
#include <vtkm/filter/FilterTraits.h>
#include <vtkm/filter/PolicyBase.h>

namespace vtkm
{
namespace filter
{
/// \brief base class for all filters.
///
/// This is the base class for all filters. To add a new filter, one can
/// subclass this (or any of the existing subclasses e.g. FilterField,
/// FilterDataSet, FilterDataSetWithField, etc. and implement relevant methods.
///
/// \section FilterUsage Usage
///
/// To execute a filter, one typically calls the `auto result =
/// filter.Execute(input)`. Typical usage is as follows:
///
/// \code{cpp}
///
/// // create the concrete subclass (e.g. MarchingCubes).
/// vtkm::filter::MarchingCubes marchingCubes;
///
/// // select fieds to map to the output, if different from default which is to
/// // map all input fields.
/// marchingCubes.SetFieldToPass({"var1", "var2"});
///
/// // execute the filter on vtkm::cont::DataSet.
/// vtkm::cont::DataSet dsInput = ...
/// auto outputDS = filter.Execute(dsInput);
///
/// // or, execute on a vtkm::cont::MultiBlock
/// vtkm::cont::MultiBlock mbInput = ...
/// auto outputMB = filter.Execute(mbInput);
/// \endcode
///
/// `Execute` methods take in the input dataset or multiblock to process and
/// return the result. The type of the result is same as the input type, thus
/// `Execute(DataSet&)` returns a DataSet while `Execute(MultiBlock&)` returns a
/// MultiBlock.
///
/// The implementation for `Execute(DataSet&)` is merely provided for
/// convenience. Internally, it creates MultiBlock with a single block for the
/// input and then forwards the call to `Execute(MultiBlock&)`. The method
/// returns the first block, if any, from the MultiBlock returned by the
/// forwarded call. If the MultiBlock returned has more than 1 block, then
/// `vtkm::cont::ErrorFilterExecution` will be thrown.
///
/// \section FilterSubclassing Subclassing
///
/// Typically, one subclasses one of the immediate subclasses of this class such as
/// FilterField, FilterDataSet, FilterDataSetWithField, etc. Those may impose
/// additional constraints on the methods to implement in the subclasses.
/// Here, we describes the things to consider when directly subclassing
/// vtkm::filter::Filter.
///
/// \subsection FilterPreExecutePostExecute PreExecute and PostExecute
///
/// Subclasses may provide implementations for either or both of the following
/// methods.
///
/// \code{cpp}
///
/// template <typename DerivedPolicy>
/// void PreExecute(const vtkm::cont::MultiBlock& input,
///           const vtkm::filter::PolicyBase<DerivedPolicy>& policy);
///
/// template <typename DerivedPolicy>
/// void PostExecute(const vtkm::cont::MultiBlock& input, vtkm::cont::MultiBlock& output
///           const vtkm::filter::PolicyBase<DerivedPolicy>& policy);
///
/// \endcode
///
/// As the name suggests, these are called and the beginning and before the end
/// of an `Filter::Execute` call. Most filters that don't need to handle
/// multiblock datasets specially, e.g. clip, cut, iso-contour, need not worry
/// about these methods or provide any implementation. If, however, your filter
/// needs do to some initialization e.g. allocation buffers to accumulate
/// results, or finalization e.g. reduce results across all blocks, then these
/// methods provide convenient hooks for the same.
///
/// \subsection FilterPrepareForExecution PrepareForExecution
///
/// A concrete subclass of Filter must provide `PrepareForExecution`
/// implementation that provides the meat for the filter i.e. the implementation
/// for the filter's data processing logic. There are two signatures
/// available; which one to implement depends on the nature of the filter.
///
/// Let's consider simple filters that do not need to do anything special to
/// handle multiblock datasets e.g. clip, contour, etc. These are the filters
/// where executing the filter on a MultiBlock simply means executing the filter
/// on one block at a time and packing the output for each iteration info the
/// result MultiBlock. For such filters, one must implement the following
/// signature.
///
/// \code{cpp}
///
/// template <typename DerivedPolicy>
/// vtkm::cont::DataSet PrepareForExecution(
///         const vtkm::cont::DataSet& input,
///         const vtkm::filter::PolicyBase<DerivedPolicy>& policy);
///
/// \endcode
///
/// The role of this method is to execute on the input dataset and generate the
/// result and return it.  If there are any errors, the subclass must throw an
/// exception (e.g. `vtkm::cont::ErrorFilterExecution`).
///
/// In this case, the Filter superclass handles iterating over multiple blocks
/// in the input MultiBlock and calling `PrepareForExecution` iteratively.
///
/// The aforementioned approach is also suitable for filters that need special
/// handling for multiblock datasets which can be modelled as PreExecute and
/// PostExecute steps (e.g. `vtkm::filter::Histogram`).
///
/// For more complex filters, like streamlines, particle tracking, where the
/// processing of multiblock datasets cannot be modelled as a reduction of the
/// results, one can implement the following signature.
///
/// \code{cpp}
/// template <typename DerivedPolicy>
/// vtkm::cont::MultiBlock PrepareForExecution(
///         const vtkm::cont::MultiBlock& input,
///         const vtkm::filter::PolicyBase<DerivedPolicy>& policy);
/// \endcode
///
/// The responsibility of this method is the same, except now the subclass is
/// given full control over the execution, including any mapping of fields to
/// output (described in next sub-section).
///
/// \subsection FilterMapFieldOntoOutput MapFieldOntoOutput
///
/// Subclasses may provide `MapFieldOntoOutput` method with the following
/// signature:
///
/// \code{cpp}
///
/// template <typename DerivedPolicy>
/// VTKM_CONT bool MapFieldOntoOutput(vtkm::cont::DataSet& result,
///                                   const vtkm::cont::Field& field,
///                                   const vtkm::filter::PolicyBase<DerivedPolicy>& policy);
///
/// \endcode
///
/// When present, this method will be called after each block execution to map
/// an input field from the corresponding input block to the output block.
///
template <typename Derived>
class Filter
{
public:
  VTKM_CONT
  Filter();

  VTKM_CONT
  ~Filter();

  //@{
  /// \brief Specify which fields get passed from input to output.
  ///
  /// After a filter successfully executes and returns a new data set, fields are mapped from
  /// input to output. Depending on what operation the filter does, this could be a simple shallow
  /// copy of an array, or it could be a computed operation. You can control which fields are
  /// passed (and equivalently which are not) with this parameter.
  ///
  /// By default, all fields are passed during execution.
  ///
  VTKM_CONT
  void SetFieldsToPass(const vtkm::filter::FieldSelection& fieldsToPass)
  {
    this->FieldsToPass = fieldsToPass;
  }

  VTKM_CONT
  void SetFieldsToPass(const vtkm::filter::FieldSelection& fieldsToPass,
                       vtkm::filter::FieldSelection::ModeEnum mode)
  {
    this->FieldsToPass = fieldsToPass;
    this->FieldsToPass.SetMode(mode);
  }

  VTKM_CONT
  void SetFieldsToPass(
    const std::string& fieldname,
    vtkm::cont::Field::Association association,
    vtkm::filter::FieldSelection::ModeEnum mode = vtkm::filter::FieldSelection::MODE_SELECT)
  {
    this->SetFieldsToPass({ fieldname, association }, mode);
  }

  VTKM_CONT
  const vtkm::filter::FieldSelection& GetFieldsToPass() const { return this->FieldsToPass; }
  VTKM_CONT
  vtkm::filter::FieldSelection& GetFieldsToPass() { return this->FieldsToPass; }
  //@}

  //@{
  /// Executes the filter on the input and produces a result dataset.
  ///
  /// On success, this the dataset produced. On error, vtkm::cont::ErrorExecution will be thrown.
  VTKM_CONT vtkm::cont::DataSet Execute(const vtkm::cont::DataSet& input);

  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet Execute(const vtkm::cont::DataSet& input,
                                        const vtkm::filter::PolicyBase<DerivedPolicy>& policy);
  //@}

  //@{
  /// Executes the filter on the input MultiBlock and produces a result MultiBlock.
  ///
  /// On success, this the dataset produced. On error, vtkm::cont::ErrorExecution will be thrown.
  VTKM_CONT vtkm::cont::MultiBlock Execute(const vtkm::cont::MultiBlock& input);

  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::MultiBlock Execute(const vtkm::cont::MultiBlock& input,
                                           const vtkm::filter::PolicyBase<DerivedPolicy>& policy);
  //@}

  /// Map fields from input dataset to output.
  /// This is not intended for external use. Subclasses of Filter, however, may
  /// use this method to map fields.
  template <typename DerivedPolicy>
  VTKM_CONT void MapFieldsToPass(const vtkm::cont::DataSet& input,
                                 vtkm::cont::DataSet& output,
                                 const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

private:
  vtkm::filter::FieldSelection FieldsToPass;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/Filter.hxx>
#endif
