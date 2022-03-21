//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_NewFilter_h
#define vtk_m_filter_NewFilter_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/PartitionedDataSet.h>

#include <vtkm/filter/FieldSelection.h>
#include <vtkm/filter/vtkm_filter_core_export.h>

namespace vtkm
{
namespace filter
{
/// \brief base class for all filters.
///
/// This is the base class for all filters. To add a new filter, one can subclass this (or any of
/// the existing subclasses e.g. FilterField, FilterParticleAdvection, etc.) and implement relevant
/// methods.
///
/// \section FilterUsage Usage
///
/// To execute a filter, one typically calls the `auto result = filter.Execute(input)`. Typical
/// usage is as follows:
///
/// \code{cpp}
///
/// // create the concrete subclass (e.g. Contour).
/// vtkm::filter::contour::Contour contour;
///
/// // select fields to map to the output, if different from default which is to map all input
/// // fields.
/// contour.SetFieldToPass({"var1", "var2"});
///
/// // execute the filter on vtkm::cont::DataSet.
/// vtkm::cont::DataSet dsInput = ...
/// auto outputDS = contour.Execute(dsInput);
///
/// // or, execute on a vtkm::cont::PartitionedDataSet
/// vtkm::cont::PartitionedDataSet mbInput = ...
/// auto outputMB = contour.Execute(mbInput);
/// \endcode
///
/// `Execute` methods take in the input DataSet or PartitionedDataSet to process and return the
/// result. The type of the result is same as the input type, thus `Execute(DataSet&)` returns
/// a DataSet while `Execute(PartitionedDataSet&)` returns a PartitionedDataSet.
///
/// `Execute` simply calls the pure virtual function `DoExecute(DataSet&)` which is the main
/// extension point of the Filter interface. Filter developer needs to override
/// `DoExecute(DataSet)` to implement the business logic of filtering operations on a single
/// DataSet.
///
/// The default implementation of `Execute(PartitionedDataSet&)` is merely provided for
/// convenience. Internally, it calls `DoExecutePartitions(PartitionedDataSet)` to iterate DataSets
/// of a PartitionedDataSet and pass each individual DataSets to `DoExecute(DataSet&)`,
/// possibly in a multi-threaded setting. Developer of `DoExecute(DataSet&)` needs to indicate
/// the thread-safeness of `DoExecute(DataSet&)` by overriding the `CanThread()` virtual method
/// which by default returns `true`.
///
/// In the case that filtering on a PartitionedDataSet can not be simply implemented as a
/// for-each loop on the component DataSets, filter implementor needs to override the
/// `DoExecutePartitions(PartitionedDataSet&)`. See the implementation of
/// `FilterParticleAdvection::Execute(PartitionedDataSet&)` for an example.
///
/// \section FilterSubclassing Subclassing
///
/// In many uses cases, one subclasses one of the immediate subclasses of this class such as
/// FilterField, FilterParticleAdvection, etc. Those may impose additional constraints on the
/// methods to implement in the subclasses. Here, we describes the things to consider when directly
/// subclassing vtkm::filter::NewFilter.
///
/// \subsection FilterExecution Execute
///
/// A concrete subclass of Filter must provide `DoExecute` implementation that provides the meat
/// for the filter i.e. the implementation for the filter's data processing logic. There are two
/// signatures available; which one to implement depends on the nature of the filter.
///
/// Let's consider simple filters that do not need to do anything special to handle
/// PartitionedDataSet e.g. clip, contour, etc. These are the filters where executing the filter
/// on a PartitionedDataSet simply means executing the filter on one partition at a time and
/// packing the output for each iteration info the result PartitionedDataSet. For such filters,
/// one must implement the following signature.
///
/// \code{cpp}
///
/// vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input);
///
/// \endcode
///
/// The role of this method is to execute on the input dataset and generate the result and return
/// it.  If there are any errors, the subclass must throw an exception
/// (e.g. `vtkm::cont::ErrorFilterExecution`).
///
/// In this simple case, the NewFilter superclass handles iterating over multiple partitions in the
/// input PartitionedDataSet and calling `DoExecute(DataSet&)` iteratively.
///
/// The aforementioned approach is also suitable for filters that need special handling for
/// PartitionedDataSets that requires certain cross DataSet operations (usually scatter/gather
/// and reduction on DataSets) before and/or after the per DataSet operation. This can be done by
/// overriding `DoExecutePartitions(PartitionedDataSet&)` while calling to the base class
/// `DoExecutePartitions(PartitionedDataSet&) as helper function for iteration on DataSets.
///
/// \code{cpp}
/// vtkm::cont::PartitionedDataSet FooFilter::DoExecutePartitions(
///   const vtkm::cont::PartitionedDataSet& input)
/// {
///   // Do pre execute stuff, e.g. scattering to each DataSet
///   auto output = this->NewFilter::DoExecutePartitions(input);
///   // Do post execute stuff, e.g gather/reduce from DataSets
///   return output;
/// }
/// \endcode
///
/// For more complex filters, like streamlines, particle tracking, where the processing of
/// PartitionedDataSets cannot be modelled as mapping and reduction operation on DataSet, one
/// needs fully implement `DoExecutePartitions(PartitionedDataSet&)`. Now the subclass is given
/// full control over the execution, including any mapping of fields to output (described in next
/// sub-section).
///
/// \subsection Creating results and mapping fields
///
/// For subclasses that map input fields into output fields, the implementation of its
/// `DoExecute(DataSet&)` should create the `DataSet` to be returned with a call to
/// `NewFilter::CreateResult` or a similar method in a subclass (such as
/// `NewFilterField::CreateResultField`).
///
/// \code{cpp}
/// VTKM_CONT DataSet SomeFilter::DoExecute(const vtkm::cont::DataSet& input)
/// {
///   vtkm::cont::UnknownCellSet outCellSet;
///   outCellSet = ... // Generation of the new CellSet
///
///   // Mapper is a callable object (function object, lambda, etc.) that takes an input Field
///   // and maps it to an output Field and then add the output Field to the output DataSet
///   auto mapper = [](auto& outputDs, const auto& inputField) {
///      auto outputField = ... // Business logic for mapping input field to output field
///      output.AddField(outputField);
///   };
///   // This passes coordinate systems directly from input to output. If the points of
///   // the cell set change at all, they will have to be mapped by hand.
///   return this->CreateResult(input, outCellSet, input.GetCoordinateSystems(), mapper);
/// }
/// \endcode
///
/// In addition to creating a new `DataSet` filled with the proper cell structure and coordinate
/// systems, `CreateResult` iterates through each `FieldToPass` in the input DataSet and calls the
/// FieldMapper to map the input Field to output Field. For simple filters that just pass on input
/// fields to the output DataSet without any computation, an overload of
/// `CreateResult(const vtkm::cont::DataSet& input)` is also
/// provided as a convenience that uses the default mapper which trivially adds input Field to
/// output DataSet (via a shallow copy).
///
/// \subsection FilterThreadSafety CanThread
///
/// By default, the implementation of `DoExecute(DataSet&)` should model a *pure function*, i.e. it
/// does not have any mutable shared state. This makes it thread-safe by default and allows
/// the default implementation of `DoExecutePartitions(PartitionedDataSet&)` to be simply a parallel
/// for-each, thus facilitates multi-threaded execution without any lock.
///
/// Many legacy (VTKm 1.x) filter implementations needed to store states between the mesh generation
/// phase and field mapping phase of filter execution, for example, parameters for field
/// interpolation. The shared mutable states were mostly stored as mutable data members of the
/// filter class (either in terms of ArrayHandle or some kind of Worket). The new filter interface,
/// by combining the two phases into a single call to `DoExecute(DataSet&)`, we have eliminated most
/// of the cases that require such shared mutable states. New implementations of filters that
/// require passing information between these two phases can now use local variables within the
/// `DoExecute(DataSet&)`. For example:
///
/// \code{cpp}
/// struct SharedState; // shared states between mesh generation and field mapping.
/// VTKM_CONT DataSet ThreadSafeFilter::DoExecute(const vtkm::cont::DataSet& input)
/// {
///   // Mutable states that was a data member of the filter is now a local variable.
///   // Each invocation of Execute(DataSet) in the multi-threaded execution of
///   // Execute(PartitionedDataSet&) will have a copy of `states` on each thread's stack
///   // thus making it thread-safe.
///   SharedStates states;
///
///   vtkm::cont::DataSet output;
///   output = ... // Generation of the new DataSet and store interpolation parameters in `states`
///
///   // Lambda capture of `states`, effectively passing the shared states to the Mapper.
///   auto mapper = [&states](auto& outputDs, const auto& inputField) {
///      auto outputField = ... // Use `states` for mapping input field to output field
///      output.AddField(outputField);
///   };
///   this->MapFieldsOntoOutput(input, output, mapper);
///
///   return output;
/// }
/// \endcode
///
/// In the rare cases that filter implementation can not be made thread-safe, the implementation
/// needs to override the `CanThread()` virtual method to return `false`. The default
/// `Execute(PartitionedDataSet&)` implementation will fallback to a serial for loop execution.
///
/// \subsection FilterThreadScheduling DoExecute
/// The default multi-threaded execution of `Execute(PartitionedDataSet&)` uses a simple FIFO queue
/// of DataSet and pool of *worker* threads. Implementation of Filter subclass can override the
/// `DoExecutePartitions(PartitionedDataSet)` virtual method to provide implementation specific
/// scheduling policy. The default number of *worker* threads in the pool are determined by the
/// `DetermineNumberOfThreads()` virtual method using several backend dependent heuristic.
/// Implementations of Filter subclass can also override
/// `DetermineNumberOfThreads()` to provide implementation specific heuristic.
///
class VTKM_FILTER_CORE_EXPORT NewFilter
{
public:
  VTKM_CONT
  virtual ~NewFilter();

  VTKM_CONT
  virtual bool CanThread() const;

  VTKM_CONT
  bool GetRunMultiThreadedFilter() const
  {
    return this->CanThread() && this->RunFilterWithMultipleThreads;
  }

  VTKM_CONT
  void SetRunMultiThreadedFilter(bool val)
  {
    if (this->CanThread())
      this->RunFilterWithMultipleThreads = val;
    else
    {
      std::string msg =
        "Multi threaded filter not supported for " + std::string(typeid(*this).name());
      VTKM_LOG_S(vtkm::cont::LogLevel::Info, msg);
    }
  }

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
                       vtkm::filter::FieldSelection::Mode mode)
  {
    this->FieldsToPass = fieldsToPass;
    this->FieldsToPass.SetMode(mode);
  }

  VTKM_CONT
  void SetFieldsToPass(
    const std::string& fieldname,
    vtkm::cont::Field::Association association,
    vtkm::filter::FieldSelection::Mode mode = vtkm::filter::FieldSelection::Mode::Select)
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
  //@}

  //@{
  /// Executes the filter on the input PartitionedDataSet and produces a result PartitionedDataSet.
  ///
  /// On success, this the dataset produced. On error, vtkm::cont::ErrorExecution will be thrown.
  VTKM_CONT vtkm::cont::PartitionedDataSet Execute(const vtkm::cont::PartitionedDataSet& input);
  //@}

  // FIXME: Is this actually materialize? Are there different kinds of Invoker?
  /// Specify the vtkm::cont::Invoker to be used to execute worklets by
  /// this filter instance. Overriding the default allows callers to control
  /// which device adapters a filter uses.
  void SetInvoker(vtkm::cont::Invoker inv) { this->Invoke = inv; }

protected:
  vtkm::cont::Invoker Invoke;

  /// \brief Create the output data set for `DoExecute`.
  ///
  /// This form of `CreateResult` will create an output data set with the same cell
  /// structure and coordinate system as the input and pass all fields (as requested
  /// by the `Filter` state).
  ///
  /// \param[in] inDataSet The input data set being modified (usually the one passed into
  /// `DoExecute`). The returned `DataSet` is filled with the cell set, coordinate system, and
  /// fields of `inDataSet` (as selected by the `FieldsToPass` state of the filter).
  ///
  VTKM_CONT vtkm::cont::DataSet CreateResult(const vtkm::cont::DataSet& inDataSet) const;

  /// \brief Create the output data set for `DoExecute`.
  ///
  /// This form of `CreateResult` will create an output data set with the given `CellSet`. You must
  /// also provide a field mapper function, which is a function that takes the output `DataSet`
  /// being created and a `Field` from the input and then applies any necessary transformations to
  /// the field array and adds it to the `DataSet`.
  ///
  /// This form of `CreateResult` returns a `DataSet` with _no_ coordinate systems. The calling
  /// program must add any necessary `CoordinateSystem`s.
  ///
  /// \param[in] inDataSet The input data set being modified (usually the one passed
  ///     into `DoExecute`). The returned `DataSet` is filled with fields of `inDataSet`
  ///     (as selected by the `FieldsToPass` state of the filter).
  /// \param[in] resultCellSet The `CellSet` of the output will be set to this.
  /// \param[in] fieldMapper A function or functor that takes a `DataSet` as its first
  ///     argument and a `Field` as its second argument. The `DataSet` is the data being
  ///     created and will eventually be returned by `CreateResult`. The `Field` comes from
  ///     `inDataSet`. The function should map the `Field` to match `resultCellSet` and then
  ///     add the resulting field to the `DataSet`. If the mapping is not possible, then
  ///     the function should do nothing.
  ///
  template <typename FieldMapper>
  VTKM_CONT vtkm::cont::DataSet CreateResult(const vtkm::cont::DataSet& inDataSet,
                                             const vtkm::cont::UnknownCellSet& resultCellSet,
                                             FieldMapper&& fieldMapper) const
  {
    vtkm::cont::DataSet outDataSet;
    outDataSet.SetCellSet(resultCellSet);
    this->MapFieldsOntoOutput(inDataSet, outDataSet, fieldMapper);
    return outDataSet;
  }

  /// \brief Create the output data set for `DoExecute`.
  ///
  /// This form of `CreateResult` will create an output data set with the given `CellSet`
  /// and set of `CoordinateSystem`s. You must also provide a field mapper function, which
  /// is a function that takes the output `DataSet` being created and a `Field` from the
  /// input and then applies any necessary transformations to the field array and adds it
  /// to the `DataSet`.
  ///
  /// \param[in] inDataSet The input data set being modified (usually the one passed
  ///     into `DoExecute`). The returned `DataSet` is filled with fields of `inDataSet`
  ///     (as selected by the `FieldsToPass` state of the filter).
  /// \param[in] resultCellSet The `CellSet` of the output will be set to this.
  /// \param[in] resultCoordSystems These `CoordinateSystem`s will be added to the output.
  /// \param[in] fieldMapper A function or functor that takes a `DataSet` as its first
  ///     argument and a `Field` as its second argument. The `DataSet` is the data being
  ///     created and will eventually be returned by `CreateResult`. The `Field` comes from
  ///     `inDataSet`. The function should map the `Field` to match `resultCellSet` and then
  ///     add the resulting field to the `DataSet`. If the mapping is not possible, then
  ///     the function should do nothing.
  ///
  template <typename FieldMapper>
  VTKM_CONT vtkm::cont::DataSet CreateResult(
    const vtkm::cont::DataSet& inDataSet,
    const vtkm::cont::UnknownCellSet& resultCellSet,
    const std::vector<vtkm::cont::CoordinateSystem>& resultCoordSystems,
    FieldMapper&& fieldMapper) const
  {
    vtkm::cont::DataSet outDataSet = this->CreateResult(inDataSet, resultCellSet, fieldMapper);
    for (auto&& cs : resultCoordSystems)
    {
      outDataSet.AddCoordinateSystem(cs);
    }
    return outDataSet;
  }

  /// \brief Create the output data set for `DoExecute`.
  ///
  /// This form of `CreateResult` will create an output data set with the given `CellSet`
  /// and `CoordinateSystem`. You must also provide a field mapper function, which is a
  /// function that takes the output `DataSet` being created and a `Field` from the input
  /// and then applies any necessary transformations to the field array and adds it to
  /// the `DataSet`.
  ///
  /// \param[in] inDataSet The input data set being modified (usually the one passed
  ///     into `DoExecute`). The returned `DataSet` is filled with fields of `inDataSet`
  ///     (as selected by the `FieldsToPass` state of the filter).
  /// \param[in] resultCellSet The `CellSet` of the output will be set to this.
  /// \param[in] resultCoordSystem This `CoordinateSystem` will be added to the output.
  /// \param[in] fieldMapper A function or functor that takes a `DataSet` as its first
  ///     argument and a `Field` as its second argument. The `DataSet` is the data being
  ///     created and will eventually be returned by `CreateResult`. The `Field` comes from
  ///     `inDataSet`. The function should map the `Field` to match `resultCellSet` and then
  ///     add the resulting field to the `DataSet`. If the mapping is not possible, then
  ///     the function should do nothing.
  ///
  template <typename FieldMapper>
  VTKM_CONT vtkm::cont::DataSet CreateResult(const vtkm::cont::DataSet& inDataSet,
                                             const vtkm::cont::UnknownCellSet& resultCellSet,
                                             const vtkm::cont::CoordinateSystem& resultCoordSystem,
                                             FieldMapper&& fieldMapper) const
  {
    return this->CreateResult(inDataSet,
                              resultCellSet,
                              std::vector<vtkm::cont::CoordinateSystem>{ resultCoordSystem },
                              fieldMapper);
  }

  VTKM_CONT virtual vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inData) = 0;
  VTKM_CONT virtual vtkm::cont::PartitionedDataSet DoExecutePartitions(
    const vtkm::cont::PartitionedDataSet& inData);

private:
  VTKM_CONT
  virtual vtkm::Id DetermineNumberOfThreads(const vtkm::cont::PartitionedDataSet& input);

  template <typename FieldMapper>
  VTKM_CONT void MapFieldsOntoOutput(const vtkm::cont::DataSet& input,
                                     vtkm::cont::DataSet& output,
                                     FieldMapper&& fieldMapper) const
  {
    for (vtkm::IdComponent cc = 0; cc < input.GetNumberOfFields(); ++cc)
    {
      auto field = input.GetField(cc);
      if (this->GetFieldsToPass().IsFieldSelected(field))
      {
        fieldMapper(output, field);
      }
    }
  }

  vtkm::filter::FieldSelection FieldsToPass = vtkm::filter::FieldSelection::Mode::All;
  bool RunFilterWithMultipleThreads = false;
};
}
} // namespace vtkm::filter

#endif
