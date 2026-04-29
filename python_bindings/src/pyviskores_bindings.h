//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_python_bindings_pyviskores_bindings_h
#define viskores_python_bindings_pyviskores_bindings_h

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>

#include <functional>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace nb = nanobind;

namespace viskores::python::bindings
{

void SetPythonError(const std::exception& error);

#define VISKORES_PYTHON_SCALAR_TYPE_LIST(Macro)   \
  Macro(viskores::Float32, "Float32", "f_32")     \
  Macro(viskores::Float64, "Float64", "f_64")     \
  Macro(viskores::Int8, "Int8", "i_8")            \
  Macro(viskores::UInt8, "UInt8", "ui_8")         \
  Macro(viskores::Int16, "Int16", "i_16")         \
  Macro(viskores::UInt16, "UInt16", "ui_16")      \
  Macro(viskores::Int32, "Int32", "i_32")         \
  Macro(viskores::UInt32, "UInt32", "ui_32")      \
  Macro(viskores::Int64, "Int64", "i_64")         \
  Macro(viskores::UInt64, "UInt64", "ui_64")

#define VISKORES_PYTHON_FLOAT_TYPE_LIST(Macro) \
  Macro(viskores::Float32, "Float32", "f_32")  \
  Macro(viskores::Float64, "Float64", "f_64")

template <typename ComponentType>
struct ScalarBindingName;

#define VISKORES_PYTHON_DECLARE_SCALAR_BINDING_NAME(Type, PublicSuffix, SOASuffix) \
  template <>                                                                      \
  struct ScalarBindingName<Type>                                                   \
  {                                                                                \
    static constexpr const char* Public = PublicSuffix;                            \
    static constexpr const char* SOA = SOASuffix;                                  \
  };

VISKORES_PYTHON_SCALAR_TYPE_LIST(VISKORES_PYTHON_DECLARE_SCALAR_BINDING_NAME)

#undef VISKORES_PYTHON_DECLARE_SCALAR_BINDING_NAME

template <typename Functor>
bool TryRegisteredScalarTypes(const Functor& functor)
{
#define VISKORES_PYTHON_TRY_SCALAR_TYPE(Type, PublicSuffix, SOASuffix) \
  if (functor.template operator()<Type>())                              \
  {                                                                     \
    return true;                                                        \
  }
  VISKORES_PYTHON_SCALAR_TYPE_LIST(VISKORES_PYTHON_TRY_SCALAR_TYPE)
#undef VISKORES_PYTHON_TRY_SCALAR_TYPE
  return false;
}

template <typename Functor>
bool TryRegisteredFloatTypes(const Functor& functor)
{
#define VISKORES_PYTHON_TRY_FLOAT_TYPE(Type, PublicSuffix, SOASuffix) \
  if (functor.template operator()<Type>())                             \
  {                                                                    \
    return true;                                                       \
  }
  VISKORES_PYTHON_FLOAT_TYPE_LIST(VISKORES_PYTHON_TRY_FLOAT_TYPE)
#undef VISKORES_PYTHON_TRY_FLOAT_TYPE
  return false;
}

template <typename Functor>
void ForEachRegisteredScalarType(const Functor& functor)
{
#define VISKORES_PYTHON_CALL_SCALAR_TYPE(Type, PublicSuffix, SOASuffix) \
  functor.template operator()<Type>();
  VISKORES_PYTHON_SCALAR_TYPE_LIST(VISKORES_PYTHON_CALL_SCALAR_TYPE)
#undef VISKORES_PYTHON_CALL_SCALAR_TYPE
}

// Fixed-width Vec interop is limited to the compiled widths below. Revisit this
// once the Python API needs arbitrary fixed component counts on input/zero-copy paths.
template <typename Functor>
bool TryCompiledVecComponentCounts(const Functor& functor)
{
  return functor.template operator()<2>() || functor.template operator()<3>() ||
    functor.template operator()<4>();
}

template <typename Functor>
bool TryCompiledVecComponentCount(viskores::IdComponent numberOfComponents,
                                  const Functor& functor)
{
  switch (numberOfComponents)
  {
    case 2:
      return functor.template operator()<2>();
    case 3:
      return functor.template operator()<3>();
    case 4:
      return functor.template operator()<4>();
    default:
      return false;
  }
}

template <typename Functor>
void ForEachCompiledVecComponentCount(const Functor& functor)
{
  functor.template operator()<2>();
  functor.template operator()<3>();
  functor.template operator()<4>();
}

nb::object UnknownArrayToNumPyArray(const viskores::cont::UnknownArrayHandle& array,
                                    bool copy = true);
nb::object GroupVecVariableToPythonList(const viskores::cont::UnknownArrayHandle& array,
                                        bool copy = true);
nb::object GroupVecVariableValueToNumPyArray(const viskores::cont::UnknownArrayHandle& array,
                                             viskores::Id index,
                                             bool copy = true);
nb::object FieldToNumPyArray(const viskores::cont::Field& field);
viskores::cont::UnknownArrayHandle NumPyArrayToUnknownArray(nb::handle object, bool copy = true);
std::vector<viskores::Float64> ParseIsoValues(nb::handle object);
nb::object IdArrayToNumPy(const viskores::cont::ArrayHandle<viskores::Id>& array);
nb::object VectorToNumPy(const std::vector<viskores::FloatDefault>& values);
viskores::cont::ArrayHandle<viskores::Id3> ParseId3Array(nb::handle object);
nb::object CreateId3ArrayObject(const viskores::cont::ArrayHandle<viskores::Id3>& array);
#if VISKORES_PYTHON_ENABLE_FILTER_SCALAR_TOPOLOGY
nb::object EdgePairArrayToNumPy(
  const viskores::worklet::contourtree_augmented::EdgePairArray& saddlePeak);
viskores::Id3 ComputeNumberOfBlocksPerAxis(viskores::Id3 globalSize, viskores::Id numberOfBlocks);
std::tuple<viskores::Id3, viskores::Id3, viskores::Id3>
ComputeBlockExtents(viskores::Id3 globalSize, viskores::Id3 blocksPerDim, viskores::Id blockNo);
viskores::cont::DataSet CreateSubDataSet(const viskores::cont::DataSet& ds,
                                         viskores::Id3 blockOrigin,
                                         viskores::Id3 blockSize,
                                         const std::string& fieldName);
#endif

viskores::cont::Field::Association ParseAssociation(
  nb::handle object,
  viskores::cont::Field::Association defaultValue = viskores::cont::Field::Association::Any);
viskores::Id3 ParseDimensions(nb::handle object);
std::pair<viskores::Id3, viskores::IdComponent> ParseDimensionsAndRank(nb::handle object);
viskores::Vec3f ParseVec3(nb::handle object, const viskores::Vec3f& defaultValue);
viskores::RangeId3 ParseRangeId3(nb::handle object);
viskores::Range ParseRange(nb::handle object,
                           const viskores::Range& defaultValue = viskores::Range());
viskores::Bounds ParseBounds(nb::handle object);
viskores::Vec3f_32 ParseColorTableColor(nb::handle object);
viskores::rendering::Color ParseColor(
  nb::handle object,
  const viskores::rendering::Color& defaultValue = viskores::rendering::Color(0, 0, 0, 1));
#if VISKORES_PYTHON_ENABLE_FILTER_CONTOUR || VISKORES_PYTHON_ENABLE_FILTER_ENTITY_EXTRACTION
bool ParseImplicitFunction(nb::handle object, viskores::ImplicitFunctionGeneral& function);
#endif

viskores::cont::DataSet* RequireDataSet(nb::handle object);
viskores::cont::PartitionedDataSet* RequirePartitionedDataSet(nb::handle object);
std::shared_ptr<viskores::cont::ColorTable> RequireColorTable(nb::handle object);
nb::object WrapDataSet(const viskores::cont::DataSet& dataSet);
nb::object WrapPartitionedDataSet(const viskores::cont::PartitionedDataSet& dataSet);

template <typename VecType>
nb::tuple Vec3ToTuple(const VecType& value)
{
  return nb::make_tuple(value[0], value[1], value[2]);
}

inline nb::tuple BoundsToTuple(const viskores::Bounds& bounds)
{
  return nb::make_tuple(
    bounds.X.Min, bounds.X.Max, bounds.Y.Min, bounds.Y.Max, bounds.Z.Min, bounds.Z.Max);
}

template <typename TargetType, typename ClassType, typename Setter, typename Getter>
ClassType& BindVec3Property(ClassType& cls,
                            const char* setName,
                            const char* getName,
                            Setter setter,
                            Getter getter,
                            const char* argName = "value")
{
  cls
    .def(
      setName,
      [setter, getter](TargetType& self, nb::object valueObject)
      { (self.*setter)(ParseVec3(valueObject, (self.*getter)())); },
      nb::arg(argName))
    .def(getName, [getter](const TargetType& self) { return Vec3ToTuple((self.*getter)()); });
  return cls;
}

template <typename TargetType>
nb::class_<TargetType> BindClassWithDefaultConstructor(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name,
  const char* name)
{
  erase_existing_name(name);
  nb::class_<TargetType> cls(m, name, doc::ClassDoc(name));
  cls.def(nb::init<>());
  return cls;
}

template <typename TargetType, typename BaseType>
nb::class_<TargetType, BaseType> BindClassWithDefaultConstructor(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name,
  const char* name)
{
  erase_existing_name(name);
  nb::class_<TargetType, BaseType> cls(m, name, doc::ClassDoc(name));
  cls.def(nb::init<>());
  return cls;
}

template <typename TargetType, typename ClassType, typename Setter, typename Getter>
ClassType& BindId3Property(ClassType& cls,
                           const char* setName,
                           const char* getName,
                           Setter setter,
                           Getter getter,
                           const char* argName = "value")
{
  cls
    .def(
      setName,
      [setter](TargetType& self, nb::object valueObject)
      { (self.*setter)(ParseDimensions(valueObject)); },
      nb::arg(argName))
    .def(getName, [getter](const TargetType& self) { return Vec3ToTuple((self.*getter)()); });
  return cls;
}

template <typename TargetType, typename ValueType, typename PythonValueType, typename ClassType, typename Setter>
ClassType& BindCastedSetter(ClassType& cls,
                            const char* setName,
                            Setter setter,
                            const char* argName = "value")
{
  cls.def(
    setName,
    [setter](TargetType& self, PythonValueType value)
    {
      ValueType converted = static_cast<ValueType>(value);
      (self.*setter)(converted);
    },
    nb::arg(argName));
  return cls;
}

template <typename TargetType,
          typename ValueType,
          typename PythonValueType,
          typename ClassType,
          typename Setter,
          typename Getter>
ClassType& BindCastedProperty(ClassType& cls,
                              const char* setName,
                              const char* getName,
                              Setter setter,
                              Getter getter,
                              const char* argName = "value")
{
  BindCastedSetter<TargetType, ValueType, PythonValueType>(cls, setName, setter, argName);
  cls.def(getName, [getter](const TargetType& self) { return (self.*getter)(); });
  return cls;
}

template <typename FilterType>
nb::object ExecuteFilterOnPythonDataObject(FilterType& filter, nb::handle dataObject)
{
  viskores::cont::DataSet* dataSet = nullptr;
  if (nb::try_cast(dataObject, dataSet))
  {
    return WrapDataSet(filter.Execute(*dataSet));
  }

  viskores::cont::PartitionedDataSet* partitionedDataSet = nullptr;
  if (nb::try_cast(dataObject, partitionedDataSet))
  {
    return WrapPartitionedDataSet(filter.Execute(*partitionedDataSet));
  }

  throw std::runtime_error("Expected a viskores.DataSet or viskores.PartitionedDataSet instance.");
}

template <typename FilterType>
nb::object ExecuteFilterToPython(FilterType& filter, nb::handle dataObject)
{
  return ExecuteFilterOnPythonDataObject<FilterType>(filter, dataObject);
}

template <typename FilterType>
void SetSelectedFieldsToPass(FilterType& filter, nb::object fieldsObject)
{
  auto& selection = filter.GetFieldsToPass();
  selection.ClearFields();
  using FieldSelectionType = std::remove_reference_t<decltype(selection)>;
  selection.SetMode(FieldSelectionType::Mode::Select);

  if (nb::isinstance<nb::str>(fieldsObject))
  {
    selection.AddField(nb::cast<std::string>(fieldsObject));
    return;
  }

  if (!nb::isinstance<nb::sequence>(fieldsObject) || nb::isinstance<nb::str>(fieldsObject))
  {
    throw std::runtime_error("fields must be a string or sequence");
  }

  nb::sequence sequence = nb::borrow<nb::sequence>(fieldsObject);
  const size_t size = static_cast<size_t>(nb::len(sequence));
  for (size_t index = 0; index < size; ++index)
  {
    nb::handle item = sequence[index];
    if (!nb::isinstance<nb::str>(item))
    {
      throw std::runtime_error("fields must contain only strings.");
    }
    selection.AddField(nb::cast<std::string>(item));
  }
}

template <typename FilterType, typename ClassType>
ClassType& BindFilterActiveFieldMethods(ClassType& cls)
{
  cls
    .def(
      "SetActiveField",
      [](FilterType& self, const char* name, nb::object associationObject)
      {
        self.SetActiveField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("GetActiveFieldName", [](const FilterType& self) { return self.GetActiveFieldName(); });
  return cls;
}

template <typename FilterType, typename ClassType>
ClassType& BindFilterActiveFieldNameMethods(ClassType& cls)
{
  cls
    .def(
      "SetActiveField",
      [](FilterType& self, const char* name) { self.SetActiveField(name); },
      nb::arg("name"))
    .def("GetActiveFieldName", [](const FilterType& self) { return self.GetActiveFieldName(); });
  return cls;
}

template <typename FilterType, typename ClassType>
ClassType& BindFilterActiveFieldAssociationMethod(ClassType& cls)
{
  cls.def("GetActiveFieldAssociation",
          [](const FilterType& self) { return self.GetActiveFieldAssociation(); });
  return cls;
}

template <typename FilterType, typename ClassType>
ClassType& BindFilterOutputFieldMethods(ClassType& cls)
{
  cls.def("SetOutputFieldName", &FilterType::SetOutputFieldName, nb::arg("name"))
    .def("GetOutputFieldName", &FilterType::GetOutputFieldName);
  return cls;
}

template <typename ClassType, typename Setter, typename Getter>
ClassType& BindFilterUseCoordinateSystemAsFieldMethods(ClassType& cls,
                                                       const char* setName,
                                                       Setter setter,
                                                       const char* getName,
                                                       Getter getter)
{
  cls.def(setName, setter, nb::arg("enabled")).def(getName, getter);
  return cls;
}

template <typename ClassType, typename Setter, typename Getter>
ClassType& BindFilterCoordinateSystemIndexMethods(ClassType& cls,
                                                  const char* setName,
                                                  Setter setter,
                                                  const char* getName,
                                                  Getter getter)
{
  cls.def(setName, setter, nb::arg("index")).def(getName, getter);
  return cls;
}

template <typename FilterType, typename ClassType>
ClassType& BindFilterCoordinateSystemFieldMethods(ClassType& cls)
{
  BindFilterUseCoordinateSystemAsFieldMethods(
    cls,
    "SetUseCoordinateSystemAsField",
    [](FilterType& self, bool enabled) { self.SetUseCoordinateSystemAsField(enabled); },
    "GetUseCoordinateSystemAsField",
    [](const FilterType& self) { return self.GetUseCoordinateSystemAsField(); });
  BindFilterCoordinateSystemIndexMethods(
    cls,
    "SetActiveCoordinateSystem",
    [](FilterType& self, viskores::Id index) { self.SetActiveCoordinateSystem(index); },
    "GetActiveCoordinateSystemIndex",
    [](const FilterType& self) { return self.GetActiveCoordinateSystemIndex(); });
  return cls;
}

template <typename FilterType, typename ClassType>
ClassType& BindFilterFieldsToPassMethod(ClassType& cls)
{
  cls.def(
    "SetFieldsToPass",
    [](FilterType& self, nb::object fieldsObject)
    { SetSelectedFieldsToPass(self, fieldsObject); },
    nb::arg("fields"));
  return cls;
}

template <typename FilterType, typename ClassType>
ClassType& BindFilterPrimaryFieldMethods(ClassType& cls)
{
  cls
    .def(
      "SetPrimaryField",
      [](FilterType& self, const char* name, nb::object associationObject)
      {
        self.SetPrimaryField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("GetPrimaryFieldName",
         [](const FilterType& self) { return self.GetPrimaryFieldName(); })
    .def("GetPrimaryFieldAssociation",
         [](const FilterType& self) { return self.GetPrimaryFieldAssociation(); });
  return cls;
}

template <typename FilterType, typename ClassType>
ClassType& BindFilterSecondaryFieldMethods(ClassType& cls)
{
  cls
    .def(
      "SetSecondaryField",
      [](FilterType& self, const char* name, nb::object associationObject)
      {
        self.SetSecondaryField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("GetSecondaryFieldName",
         [](const FilterType& self) { return self.GetSecondaryFieldName(); })
    .def("GetSecondaryFieldAssociation",
         [](const FilterType& self) { return self.GetSecondaryFieldAssociation(); });
  return cls;
}

template <typename FilterType, typename ClassType>
ClassType& BindFilterExecuteMethod(ClassType& cls)
{
  cls.def("Execute", &ExecuteFilterToPython<FilterType>, nb::arg("data"), doc::ExecuteFilter);
  return cls;
}

template <typename FilterType, typename ClassType>
ClassType& BindActiveFieldRepr(ClassType& cls, const char* qualifiedName)
{
  cls.def("__repr__",
          [qualifiedName](const FilterType& self)
          {
            std::ostringstream stream;
            stream << qualifiedName << "(active_field=\"" << self.GetActiveFieldName() << "\")";
            return stream.str();
          });
  return cls;
}

template <typename FilterType, typename ClassType>
ClassType& BindFieldFilterMethods(ClassType& cls)
{
  BindFilterActiveFieldMethods<FilterType>(cls);
  BindFilterOutputFieldMethods<FilterType>(cls);
  BindFilterCoordinateSystemFieldMethods<FilterType>(cls);
  BindFilterExecuteMethod<FilterType>(cls);
  return cls;
}

void RegisterNanobindSharedDataClasses(nb::module_& m,
                                       const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindTestingClasses(nb::module_& m,
                                    const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindSourceClasses(nb::module_& m,
                                   const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindIOClasses(nb::module_& m,
                               const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindGeneratedClasses(nb::module_& m,
                                      const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindFieldConversionClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindVectorAnalysisClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindContourClasses(nb::module_& m,
                                    const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindFieldTransformClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindEntityExtractionClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindAdditionalFilterClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindDensityEstimateClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindGeometryRefinementClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindImageProcessingClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindResamplingClasses(nb::module_& m,
                                       const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindScalarTopologyClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindHelperFunctions(nb::module_& m,
                                     const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindInteropClasses(nb::module_& m,
                                    const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindColorTableClass(nb::module_& m,
                                     const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindCameraClass(nb::module_& m,
                                 const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindRenderingClasses(nb::module_& m,
                                      const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindImplicitFunctionClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindCompatibilityFunctions(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name);
void RegisterNanobindModuleConstants(nb::module_& m);
void RegisterNanobindModule(nb::module_& m);

#if VISKORES_PYTHON_ENABLE_RENDERING
viskores::rendering::Camera* RequireCamera(nb::handle object);
viskores::rendering::Mapper* RequireMapper(nb::handle object);
#endif

#if VISKORES_PYTHON_ENABLE_INTEROP
struct NanobindBufferStateHolder
{
  GLuint HandleStorage = 0;
  std::shared_ptr<viskores::interop::BufferState> State;

  NanobindBufferStateHolder();
  explicit NanobindBufferStateHolder(unsigned long handle);
  NanobindBufferStateHolder(unsigned long handle, unsigned long bufferType);
  unsigned long GetHandle();
  void SetHandle(unsigned long handle);
  bool HasType() const;
  unsigned long GetType() const;
  void SetType(unsigned long bufferType);
  long long GetSize() const;
  long long GetCapacity() const;
  std::string Repr() const;
};

void TransferUnknownArrayToOpenGL(const viskores::cont::UnknownArrayHandle& array,
                                  viskores::interop::BufferState& state);
#endif

} // namespace viskores::python::bindings

#endif
