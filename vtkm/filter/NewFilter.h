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
/// This is the base class for all filters. To add a new filter, one can
/// subclass this (or any of the existing subclasses e.g. FilterField,
/// FilterParticleAdvection, etc.) and implement relevant methods.
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
/// // or, execute on a vtkm::cont::PartitionedDataSet
/// vtkm::cont::PartitionedDataSet mbInput = ...
/// auto outputMB = filter.Execute(mbInput);
/// \endcode
///
/// `Execute` methods take in the input DataSet or PartitionedDataSet to
/// process and return the result. The type of the result is same as the input
/// type, thus `Execute(DataSet&)` returns a DataSet while
/// `Execute(PartitionedDataSet&)` returns a PartitionedDataSet.
///
/// The pure virtual function `Execute(DataSet&)` is the main extension point of the
/// Filter interface. Filter developer needs to override `Execute(DataSet)` to implement
/// the business logic of filtering operations on a single DataSet.
///
/// The default implementation of `Execute(PartitionedDataSet&)` is merely provided for
/// convenience. Internally, it iterates DataSets of a PartitionedDataSet and pass
/// each individual DataSets to `Execute(DataSet&)`, possibly in a multi-threaded setting.
/// Developer of `Execute(DataSet&)` needs to indicate the thread-safeness of `Execute(DataSet&)`
/// by overriding the `CanThread()` virtual method which by default returns `true`.
///
/// In the case that filtering on a PartitionedDataSet can not be simply implemented as a
/// for-each loop on the component DataSets, filter implementor needs to override the
/// `Execute(PartitionedDataSet&)`. See the implementation of
/// `FilterParticleAdvection::Execute(PartitionedDataSet&)` for an example.
///
/// \section FilterSubclassing Subclassing
///
/// Typically, one subclasses one of the immediate subclasses of this class such as
/// FilterField, FilterParticleAdvection, etc. Those may impose
/// additional constraints on the methods to implement in the subclasses.
/// Here, we describes the things to consider when directly subclassing
/// vtkm::filter::Filter.
///
/// \subsection FilterPreExecutePostExecute PreExecute and PostExecute
///
/// Subclasses may provide implementations for either or both of the following protected
/// methods.
///
/// \code{cpp}
///
/// void PreExecute(const vtkm::cont::PartitionedDataSet& input);
///
/// void PostExecute(const vtkm::cont::PartitionedDataSet& input,
///           vtkm::cont::PartitionedDataSet& output);
///
/// \endcode
///
/// As the name suggests, these are called and the before the beginning and after the end of
/// iterative `Filter::Execute(DataSet&)` calls. Most filters that don't need to handle
/// PartitionedDataSet specially, e.g. clip, cut, iso-contour, need not worry
/// about these methods or provide any implementation. If, however, your filter
/// needs to do some initialization e.g. allocation buffers to accumulate
/// results, or finalization e.g. reduce results across all partitions, then
/// these methods provide convenient hooks for the same.
///
/// \subsection FilterExecution Execute
///
/// A concrete subclass of Filter must provide `Execute` implementation that provides the meat
/// for the filter i.e. the implementation for the filter's data processing logic. There are
/// two signatures available; which one to implement depends on the nature of the filter.
///
/// Let's consider simple filters that do not need to do anything special to
/// handle PartitionedDataSet e.g. clip, contour, etc. These are the filters
/// where executing the filter on a PartitionedDataSet simply means executing
/// the filter on one partition at a time and packing the output for each
/// iteration info the result PartitionedDataSet. For such filters, one must
/// implement the following signature.
///
/// \code{cpp}
///
/// vtkm::cont::DataSet Execution(const vtkm::cont::DataSet& input);
///
/// \endcode
///
/// The role of this method is to execute on the input dataset and generate the
/// result and return it.  If there are any errors, the subclass must throw an
/// exception (e.g. `vtkm::cont::ErrorFilterExecution`).
///
/// In this simple case, the Filter superclass handles iterating over multiple
/// partitions in the input PartitionedDataSet and calling
/// `Execute(DataSet&)` iteratively.
///
/// The aforementioned approach is also suitable for filters that need special
/// handling for PartitionedDataSets which can be modelled as PreExecute and
/// PostExecute steps (e.g. `vtkm::filter::Histogram`).
///
/// For more complex filters, like streamlines, particle tracking, where the
/// processing of PartitionedDataSets cannot be modelled as a reduction of the
/// results, one can implement the following signature.
///
/// \code{cpp}
/// vtkm::cont::PartitionedDataSet Execute(
///         const vtkm::cont::PartitionedDataSet& input);
/// \endcode
///
/// The responsibility of this method is the same, except now the subclass is
/// given full control over the execution, including any mapping of fields to
/// output (described in next sub-section).
///
/// \subsection FilterMappingFields MapFieldsOntoOutput
///
/// For subclasses that map input fields into output fields, the implementation of its
/// `Execute(DataSet&)` should call `Filter::MapFieldsOntoOutput` with a properly defined
/// `Mapper`, before returning the output DataSet. For example:
///
/// \code{cpp}
/// VTKM_CONT DataSet SomeFilter::Execute(const vtkm::cont::DataSet& input)
/// {
///   vtkm::cont::DataSet output;
///   output = ... // Generation of the new DataSet
///
///   // Mapper is a callable object (function object, lambda, etc.) that takes an input Field
///   // and maps it to an output Field and then add the output Field to the output DataSet
///   auto mapper = [](auto& outputDs, const auto& inputField) {
///      auto outputField = ... // Business logic for mapping input field to output field
///      output.AddField(outputField);
///   };
///   MapFieldsOntoOutput(input, output, mapper);
///
///   return output;
/// }
/// \endcode
///
/// `MapFieldsOntoOutput` iterates through each `FieldToPass` in the input DataSet and calls the
/// Mapper to map the input Field to output Field. For simple filters that just pass on input
/// fields to the output DataSet without any computation, an overload of
/// `MapFieldsOntoOutput(const vtkm::cont::DataSet& input, vtkm::cont::DataSet& output)` is also
/// provided as a convenience that uses the default mapper which trivially add input Field to
/// output DaaSet (via a shallow copy).
///
/// \subsection FilterThreadSafety CanThread
///
/// By default, the implementation of `Execute(DataSet&)` should model a *pure function*, i.e. it
/// does not have any mutable shared state. This makes it thread-safe by default and allows
/// the default implementation of `Execute(PartitionedDataSet&)` to be simply a parallel for-each,
/// thus facilitates multi-threaded execution without any lock.
///
/// Many legacy (VTKm 1.x) filter implementations needed to store states between the mesh generation
/// phase and field mapping phase of filter execution, for example, parameters for field
/// interpolation. The shared mutable states were mostly stored as mutable data members of the
/// filter class (either in terms of ArrayHandle or some kind of Worket). The new filter interface,
/// by combining the two phases into a single call to `Execute(DataSet&)`, we have eliminated most
/// of the cases that require such shared mutable states. New implementations of filters that
/// require passing information between these two phases can now use local variables within the
/// `Execute(DataSet&)`. For example:
///
/// \code{cpp}
/// struct SharedState; // shared states between mesh generation and field mapping.
/// VTKM_CONT DataSet ThreadSafeFilter::Execute(const vtkm::cont::DataSet& input)
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
///   MapFieldsOntoOutput(input, output, mapper);
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
/// `DoExecute(PartitionedDataSet)` virtual method to provide implementation specific scheduling
/// policy. The default number of *worker* threads in the pool are determined by the
/// `DetermineNumberOfThreads()` virtual method using several backend dependent heuristic.
/// Implementations of Filter subclass can also override
/// `DetermineNumberOfThreads()` to provide implementation specific heuristic.
///
/// \subsection FilterNameLookup Overriding Overloaded Functions
/// Since we have two overloads of `Execute`, we need to work with C++'s rule for name lookup for
/// inherited, overloaded functions when overriding them. In most uses cases, we intend to only
/// override the `Execute(DataSet&)` overload in an implementation of a NewFilter subclass, such as
///
/// \code{cpp}
/// class FooFilter : public NewFilter
/// {
///   ...
///   vtkm::cont::DataSet Execute(const vtkm::cont::DataSet& input) override;
///   ...
/// }
/// \endcode
///
/// However, the compiler will stop the name lookup process once it sees the
/// `FooFilter::Execute(DataSet)`. When a user calls `FooFilter::Execute(PartitionedDataSet&)`,
/// the compiler will not find the overload from the base class `NewFilter`, resulting in failed
/// overload resolution. The solution to such a problem is to use a using-declaration in the
/// subclass definition to bring the `NewFilter::Execute(PartitionedDataSet&)` into scope for
/// name lookup. For example:
///
/// \code{cpp}
/// class FooFilter : public NewFilter
/// {
///   ...
///   using vtkm::filter::NewFilter::Execute; // bring overloads of Execute into name lookup
///   vtkm::cont::DataSet Execute(const vtkm::cont::DataSet& input) override;
///   ...
/// }
/// \endcode
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

  /// \brief Specify which subset of types a filter supports.
  ///
  /// A filter is able to state what subset of types it supports.
  using SupportedTypes = VTKM_DEFAULT_TYPE_LIST;

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
  /// Select the coordinate system index to make active to use when processing the input
  /// DataSet. This is used primarily by the Filter to select the coordinate system
  /// to use as a field when \c UseCoordinateSystemAsField is true.
  VTKM_CONT
  void SetActiveCoordinateSystem(vtkm::Id index) { this->CoordinateSystemIndex = index; }

  VTKM_CONT
  vtkm::Id GetActiveCoordinateSystemIndex() const { return this->CoordinateSystemIndex; }
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
  vtkm::Id CoordinateSystemIndex = 0;

  template <typename Mapper>
  VTKM_CONT void MapFieldsOntoOutput(const vtkm::cont::DataSet& input,
                                     vtkm::cont::DataSet& output,
                                     Mapper&& mapper)
  {
    for (vtkm::IdComponent cc = 0; cc < input.GetNumberOfFields(); ++cc)
    {
      auto field = input.GetField(cc);
      if (this->GetFieldsToPass().IsFieldSelected(field))
      {
        mapper(output, field);
      }
    }
  }

  VTKM_CONT void MapFieldsOntoOutput(const vtkm::cont::DataSet& input, vtkm::cont::DataSet& output)
  {
    MapFieldsOntoOutput(input, output, defaultMapper);
  }

private:
  VTKM_CONT
  virtual vtkm::Id DetermineNumberOfThreads(const vtkm::cont::PartitionedDataSet& input);

  //@{
  /// when operating on vtkm::cont::PartitionedDataSet, we
  /// want to do processing across ranks as well. Just adding pre/post handles
  /// for the same does the trick.
  VTKM_CONT virtual void PreExecute(const vtkm::cont::PartitionedDataSet&) {}

  VTKM_CONT virtual void PostExecute(const vtkm::cont::PartitionedDataSet&,
                                     vtkm::cont::PartitionedDataSet&)
  {
  }
  //@}

  VTKM_CONT virtual vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inData) = 0;
  VTKM_CONT virtual vtkm::cont::PartitionedDataSet DoExecute(
    const vtkm::cont::PartitionedDataSet& inData);

  static void defaultMapper(vtkm::cont::DataSet& output, const vtkm::cont::Field& field)
  {
    output.AddField(field);
  };

  vtkm::filter::FieldSelection FieldsToPass = vtkm::filter::FieldSelection::MODE_ALL;
  bool RunFilterWithMultipleThreads = false;
};
}
} // namespace vtkm::filter

#endif
