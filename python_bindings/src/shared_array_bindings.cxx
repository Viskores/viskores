//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include "shared_data_helpers.h"

#include <nanobind/stl/string.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleGroupVecVariable.h>
#include <viskores/cont/ArrayHandleSOA.h>

namespace viskores::python::bindings
{

namespace
{

std::string ArrayHandleRepr(const std::string& name, viskores::Id numberOfValues)
{
  std::ostringstream stream;
  stream << "viskores.cont." << name << "(values=" << numberOfValues << ")";
  return stream.str();
}

std::string ArrayHandleRepr(const std::string& name,
                            viskores::Id numberOfValues,
                            viskores::IdComponent numberOfComponents)
{
  std::ostringstream stream;
  stream << "viskores.cont." << name << "(values=" << numberOfValues
         << ", components=" << numberOfComponents << ")";
  return stream.str();
}

template <typename ArrayType>
void BindArrayHandleNumPyMethods(nb::class_<ArrayType>& cls)
{
  cls.def("GetNumberOfValues", &ArrayType::GetNumberOfValues)
    .def(
      "AsNumPy",
      [](const ArrayType& self, bool copy)
      { return UnknownArrayToNumPyArray(viskores::cont::UnknownArrayHandle{ self }, copy); },
      nb::arg("copy") = true);
}

template <typename ArrayType>
void RegisterArrayHandleSOAClass(nb::module_& m,
                                 const std::function<void(const char*)>& erase_existing_name,
                                 const std::string& name)
{
  erase_existing_name(name.c_str());
  nb::class_<ArrayType> cls(m, name.c_str(), doc::ClassDoc(name.c_str()));
  cls.def(nb::init<>())
    .def("__repr__",
         [name](const ArrayType& self)
         { return ArrayHandleRepr(name, self.GetNumberOfValues()); });
  BindArrayHandleNumPyMethods(cls);
}

template <typename ComponentType>
void RegisterArrayHandleRecombineVecClass(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name,
  const std::string& name)
{
  using ArrayType = viskores::cont::ArrayHandleRecombineVec<ComponentType>;

  erase_existing_name(name.c_str());
  nb::class_<ArrayType> cls(m, name.c_str(), doc::ClassDoc(name.c_str()));
  cls.def("__repr__",
          [name](const ArrayType& self)
          {
            return ArrayHandleRepr(
              name, self.GetNumberOfValues(), self.GetNumberOfComponentsFlat());
          })
    .def("GetNumberOfComponentsFlat", &ArrayType::GetNumberOfComponentsFlat);
  BindArrayHandleNumPyMethods(cls);
}

template <typename ComponentType>
viskores::cont::ArrayHandle<ComponentType> ParseGroupVecVariableComponents(nb::handle object)
{
  viskores::cont::UnknownArrayHandle unknown = PythonObjectToUnknownArray(object);
  if (unknown.GetNumberOfComponentsFlat() != 1)
  {
    throw std::runtime_error("ArrayHandleGroupVecVariable components must be a 1D scalar array.");
  }
  if (!unknown.IsBaseComponentType<ComponentType>())
  {
    throw std::runtime_error(
      "ArrayHandleGroupVecVariable components do not match the requested component type.");
  }

  viskores::cont::ArrayHandle<ComponentType> components;
  viskores::cont::ArrayCopy(unknown, components);
  return components;
}

void ValidateGroupVecVariableOffsets(
  const viskores::cont::ArrayHandle<viskores::Id>& offsets,
  viskores::Id numberOfComponents)
{
  const viskores::Id numberOfOffsets = offsets.GetNumberOfValues();
  if (numberOfOffsets < 1)
  {
    throw std::runtime_error(
      "ArrayHandleGroupVecVariable offsets must contain at least one value.");
  }

  auto offsetsPortal = offsets.ReadPortal();
  viskores::Id previous = offsetsPortal.Get(0);
  if (previous != 0)
  {
    throw std::runtime_error("ArrayHandleGroupVecVariable offsets must start with 0.");
  }

  for (viskores::Id index = 1; index < numberOfOffsets; ++index)
  {
    const viskores::Id current = offsetsPortal.Get(index);
    if (current < previous)
    {
      throw std::runtime_error("ArrayHandleGroupVecVariable offsets must be nondecreasing.");
    }
    if (current > numberOfComponents)
    {
      throw std::runtime_error(
        "ArrayHandleGroupVecVariable offsets cannot exceed the number of components.");
    }
    previous = current;
  }
}

viskores::cont::ArrayHandle<viskores::Id> ParseGroupVecVariableOffsets(nb::handle object,
                                                                       viskores::Id components)
{
  viskores::cont::ArrayHandle<viskores::Id> offsets;
  viskores::cont::UnknownArrayHandle unknown;
  if (TryPythonObjectToRegisteredArray(object, unknown) &&
      unknown.GetNumberOfComponentsFlat() == 1 &&
      unknown.IsBaseComponentType<viskores::Id>())
  {
    viskores::cont::ArrayCopy(unknown, offsets);
  }
  else
  {
    std::vector<viskores::Id> offsetValues = ParseIdSequence(object);
    offsets = viskores::cont::make_ArrayHandle(offsetValues, viskores::CopyFlag::On);
  }

  ValidateGroupVecVariableOffsets(offsets, components);
  return offsets;
}

template <typename ComponentType>
void RegisterArrayHandleGroupVecVariableClass(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name,
  const std::string& name)
{
  using ComponentsArrayType = viskores::cont::ArrayHandle<ComponentType>;
  using OffsetsArrayType = viskores::cont::ArrayHandle<viskores::Id>;
  using ArrayType =
    viskores::cont::ArrayHandleGroupVecVariable<ComponentsArrayType, OffsetsArrayType>;

  erase_existing_name(name.c_str());
  nb::class_<ArrayType> cls(m, name.c_str(), doc::ClassDoc(name.c_str()));
  cls.def("__init__",
          [](ArrayType* self, nb::object componentsObject, nb::object offsetsObject)
          {
            ComponentsArrayType components =
              ParseGroupVecVariableComponents<ComponentType>(componentsObject);
            OffsetsArrayType offsets =
              ParseGroupVecVariableOffsets(offsetsObject, components.GetNumberOfValues());
            new (self) ArrayType(components, offsets);
          },
          nb::arg("components"),
          nb::arg("offsets"))
    .def("__repr__",
         [name](const ArrayType& self)
         {
           return ArrayHandleRepr(
             name, self.GetNumberOfValues(), self.GetNumberOfComponentsFlat());
         })
    .def("__len__", [](const ArrayType& self) { return self.GetNumberOfValues(); })
    .def("__getitem__",
         [](const ArrayType& self, viskores::Id index)
         {
           return GroupVecVariableValueToNumPyArray(
             viskores::cont::UnknownArrayHandle{ self }, index);
         },
         nb::arg("index"))
    .def("GetNumberOfValues", &ArrayType::GetNumberOfValues)
    .def("GetNumberOfComponentsFlat", &ArrayType::GetNumberOfComponentsFlat)
    .def("GetComponentsArray",
         [](const ArrayType& self)
         { return viskores::cont::UnknownArrayHandle{ self.GetComponentsArray() }; })
    .def("GetOffsetsArray",
         [](const ArrayType& self)
         { return viskores::cont::UnknownArrayHandle{ self.GetOffsetsArray() }; })
    .def(
      "AsList",
      [](const ArrayType& self)
      { return GroupVecVariableToPythonList(viskores::cont::UnknownArrayHandle{ self }); })
    .def(
      "AsNumPy",
      [](const ArrayType& self, bool copy)
      { return GroupVecVariableToPythonList(viskores::cont::UnknownArrayHandle{ self }, copy); },
      nb::arg("copy") = true);
}

template <typename ComponentType, viskores::IdComponent NumberOfComponents>
void RegisterArrayHandleSOAVecClassForWidth(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name,
  const std::string& suffix)
{
  RegisterArrayHandleSOAClass<
    viskores::cont::ArrayHandleSOA<viskores::Vec<ComponentType, NumberOfComponents>>>(
    m,
    erase_existing_name,
    "ArrayHandleSOAVec" + std::to_string(NumberOfComponents) + suffix);
}

template <typename ComponentType>
struct RegisterSOAVecComponentCountFunctor
{
  nb::module_& Module;
  const std::function<void(const char*)>& EraseExistingName;
  const std::string& Suffix;

  template <viskores::IdComponent NumberOfComponents>
  void operator()() const
  {
    RegisterArrayHandleSOAVecClassForWidth<ComponentType, NumberOfComponents>(
      this->Module, this->EraseExistingName, this->Suffix);
  }
};

template <typename ComponentType>
void RegisterArrayHandleSOAVecClassesForScalar(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name)
{
  const std::string suffix = ScalarBindingName<ComponentType>::SOA;
  ForEachCompiledVecComponentCount(
    RegisterSOAVecComponentCountFunctor<ComponentType>{ m, erase_existing_name, suffix });
}

template <typename ComponentType>
void RegisterVariableComponentArrayHandleClassesForScalar(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name)
{
  const std::string suffix = ScalarBindingName<ComponentType>::Public;
  RegisterArrayHandleRecombineVecClass<ComponentType>(
    m, erase_existing_name, "ArrayHandleRecombineVec" + suffix);
  RegisterArrayHandleGroupVecVariableClass<ComponentType>(
    m, erase_existing_name, "ArrayHandleGroupVecVariable" + suffix);
}


struct RegisterSOAVecClassesFunctor
{
  nb::module_& Module;
  const std::function<void(const char*)>& EraseExistingName;

  template <typename ComponentType>
  void operator()() const
  {
    RegisterArrayHandleSOAVecClassesForScalar<ComponentType>(
      this->Module, this->EraseExistingName);
  }
};

struct RegisterVariableComponentArrayClassesFunctor
{
  nb::module_& Module;
  const std::function<void(const char*)>& EraseExistingName;

  template <typename ComponentType>
  void operator()() const
  {
    RegisterVariableComponentArrayHandleClassesForScalar<ComponentType>(
      this->Module, this->EraseExistingName);
  }
};

template <typename ComponentType>
bool TryExtractArrayFromComponents(const viskores::cont::UnknownArrayHandle& array,
                                   nb::object& output)
{
  if (!array.IsBaseComponentType<ComponentType>())
  {
    return false;
  }

  output = nb::cast(array.ExtractArrayFromComponents<ComponentType>());
  return true;
}

struct TryExtractArrayFromComponentsFunctor
{
  const viskores::cont::UnknownArrayHandle& Array;
  nb::object& Output;

  template <typename ComponentType>
  bool operator()() const
  {
    return TryExtractArrayFromComponents<ComponentType>(this->Array, this->Output);
  }
};

nb::object ExtractArrayFromComponentsObject(const viskores::cont::UnknownArrayHandle& array)
{
  nb::object output;
  if (TryRegisteredScalarTypes(TryExtractArrayFromComponentsFunctor{ array, output }))
  {
    return output;
  }
  throw std::runtime_error("Unsupported field component type for component extraction.");
}

template <typename ComponentType, viskores::IdComponent NumberOfComponents>
bool TryArrayCopyToSOA(const viskores::cont::UnknownArrayHandle& source, nb::handle destination)
{
  using ArrayType =
    viskores::cont::ArrayHandleSOA<viskores::Vec<ComponentType, NumberOfComponents>>;
  ArrayType* typedDestination = nullptr;
  if (!nb::try_cast(destination, typedDestination))
  {
    return false;
  }

  viskores::cont::ArrayCopy(source, *typedDestination);
  return true;
}

template <typename ComponentType>
struct TryArrayCopyToSOAComponentCountFunctor
{
  const viskores::cont::UnknownArrayHandle& Source;
  nb::handle Destination;

  template <viskores::IdComponent NumberOfComponents>
  bool operator()() const
  {
    return TryArrayCopyToSOA<ComponentType, NumberOfComponents>(
      this->Source, this->Destination);
  }
};

template <typename ComponentType>
bool TryArrayCopyToSOAComponent(const viskores::cont::UnknownArrayHandle& source,
                                nb::handle destination)
{
  return TryCompiledVecComponentCounts(
    TryArrayCopyToSOAComponentCountFunctor<ComponentType>{ source, destination });
}

struct TryArrayCopyToSOAComponentFunctor
{
  const viskores::cont::UnknownArrayHandle& Source;
  nb::handle Destination;

  template <typename ComponentType>
  bool operator()() const
  {
    return TryArrayCopyToSOAComponent<ComponentType>(this->Source, this->Destination);
  }
};

template <typename ComponentType, viskores::IdComponent NumberOfComponents>
bool TryPythonObjectToSOAUnknownArray(nb::handle object, viskores::cont::UnknownArrayHandle& array)
{
  using ArrayType =
    viskores::cont::ArrayHandleSOA<viskores::Vec<ComponentType, NumberOfComponents>>;
  ArrayType* typedArray = nullptr;
  if (!nb::try_cast(object, typedArray))
  {
    return false;
  }

  array = viskores::cont::UnknownArrayHandle(*typedArray);
  return true;
}

template <typename ComponentType>
struct TryPythonObjectToSOAUnknownArrayComponentCountFunctor
{
  nb::handle Object;
  viskores::cont::UnknownArrayHandle& Array;

  template <viskores::IdComponent NumberOfComponents>
  bool operator()() const
  {
    return TryPythonObjectToSOAUnknownArray<ComponentType, NumberOfComponents>(
      this->Object, this->Array);
  }
};

template <typename ComponentType>
bool TryPythonObjectToSOAUnknownArrayComponent(nb::handle object,
                                               viskores::cont::UnknownArrayHandle& array)
{
  return TryCompiledVecComponentCounts(
    TryPythonObjectToSOAUnknownArrayComponentCountFunctor<ComponentType>{ object, array });
}

template <typename ComponentType>
bool TryPythonObjectToRecombineVecUnknownArray(nb::handle object,
                                               viskores::cont::UnknownArrayHandle& array)
{
  using ArrayType = viskores::cont::ArrayHandleRecombineVec<ComponentType>;
  ArrayType* typedArray = nullptr;
  if (!nb::try_cast(object, typedArray))
  {
    return false;
  }

  array = viskores::cont::UnknownArrayHandle(*typedArray);
  return true;
}

template <typename ComponentType>
bool TryPythonObjectToGroupVecVariableUnknownArray(nb::handle object,
                                                   viskores::cont::UnknownArrayHandle& array)
{
  using ArrayType = viskores::cont::ArrayHandleGroupVecVariable<
    viskores::cont::ArrayHandle<ComponentType>,
    viskores::cont::ArrayHandle<viskores::Id>>;
  ArrayType* typedArray = nullptr;
  if (!nb::try_cast(object, typedArray))
  {
    return false;
  }

  array = viskores::cont::UnknownArrayHandle(*typedArray);
  return true;
}

template <typename ComponentType>
bool TryPythonObjectToArrayForComponent(nb::handle object,
                                        viskores::cont::UnknownArrayHandle& array)
{
  return TryPythonObjectToSOAUnknownArrayComponent<ComponentType>(object, array) ||
    TryPythonObjectToRecombineVecUnknownArray<ComponentType>(object, array) ||
    TryPythonObjectToGroupVecVariableUnknownArray<ComponentType>(object, array);
}

struct TryPythonObjectToArrayForComponentFunctor
{
  nb::handle Object;
  viskores::cont::UnknownArrayHandle& Array;

  template <typename ComponentType>
  bool operator()() const
  {
    return TryPythonObjectToArrayForComponent<ComponentType>(this->Object, this->Array);
  }
};

} // namespace

bool TryPythonObjectToRegisteredArray(nb::handle object, viskores::cont::UnknownArrayHandle& array)
{
  viskores::cont::UnknownArrayHandle* unknownArray = nullptr;
  if (nb::try_cast(object, unknownArray))
  {
    array = *unknownArray;
    return true;
  }

  if (TryRegisteredScalarTypes(TryPythonObjectToArrayForComponentFunctor{ object, array }))
  {
    return true;
  }

  return false;
}

viskores::cont::UnknownArrayHandle PythonObjectToUnknownArray(nb::handle object)
{
  viskores::cont::UnknownArrayHandle array;
  if (TryPythonObjectToRegisteredArray(object, array))
  {
    return array;
  }
  return NumPyArrayToUnknownArray(object);
}

void ArrayCopyToPythonDestination(nb::handle sourceObject, nb::handle destination)
{
  viskores::cont::UnknownArrayHandle source;
  if (!TryPythonObjectToRegisteredArray(sourceObject, source))
  {
    source = NumPyArrayToUnknownArray(sourceObject);
  }

  if (TryRegisteredScalarTypes(TryArrayCopyToSOAComponentFunctor{ source, destination }))
  {
    return;
  }

  throw std::runtime_error("ArrayCopy destination must be a supported ArrayHandleSOA class.");
}

void RegisterNanobindArrayClasses(nb::module_& m,
                                  const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("UnknownArrayHandle");
  nb::class_<viskores::cont::UnknownArrayHandle>(
    m, "UnknownArrayHandle", doc::ClassDoc("UnknownArrayHandle"))
    .def(nb::init<>())
    .def("__repr__",
         [](const viskores::cont::UnknownArrayHandle& self)
         {
           std::ostringstream stream;
           stream << "viskores.cont.UnknownArrayHandle(type=\"" << self.GetArrayTypeName()
                  << "\", values=" << self.GetNumberOfValues()
                  << ", components=" << self.GetNumberOfComponentsFlat() << ")";
           return stream.str();
         })
    .def("IsValid", &viskores::cont::UnknownArrayHandle::IsValid)
    .def("GetArrayTypeName", &viskores::cont::UnknownArrayHandle::GetArrayTypeName)
    .def("GetNumberOfValues", &viskores::cont::UnknownArrayHandle::GetNumberOfValues)
    .def("GetNumberOfComponentsFlat",
         &viskores::cont::UnknownArrayHandle::GetNumberOfComponentsFlat)
    .def(
      "AsNumPy",
      [](const viskores::cont::UnknownArrayHandle& self, bool copy)
      { return UnknownArrayToNumPyArray(self, copy); },
      nb::arg("copy") = true)
    .def("IsStorageTypeSOA",
         [](const viskores::cont::UnknownArrayHandle& self)
         { return self.IsStorageType<viskores::cont::StorageTagSOA>(); })
    .def("ExtractArrayFromComponents",
         [](const viskores::cont::UnknownArrayHandle& self)
         { return ExtractArrayFromComponentsObject(self); });

  ForEachRegisteredScalarType(RegisterSOAVecClassesFunctor{ m, erase_existing_name });

  erase_existing_name("ArrayHandleSOAVec3f");
  m.attr("ArrayHandleSOAVec3f") = std::is_same<viskores::FloatDefault, viskores::Float32>::value
    ? m.attr("ArrayHandleSOAVec3f_32")
    : m.attr("ArrayHandleSOAVec3f_64");

  ForEachRegisteredScalarType(
    RegisterVariableComponentArrayClassesFunctor{ m, erase_existing_name });

  erase_existing_name("ArrayHandleRecombineVecFloatDefault");
  m.attr("ArrayHandleRecombineVecFloatDefault") =
    std::is_same<viskores::FloatDefault, viskores::Float32>::value
    ? m.attr("ArrayHandleRecombineVecFloat32")
    : m.attr("ArrayHandleRecombineVecFloat64");

  erase_existing_name("ArrayHandleGroupVecVariableFloatDefault");
  m.attr("ArrayHandleGroupVecVariableFloatDefault") =
    std::is_same<viskores::FloatDefault, viskores::Float32>::value
    ? m.attr("ArrayHandleGroupVecVariableFloat32")
    : m.attr("ArrayHandleGroupVecVariableFloat64");

  erase_existing_name("ArrayHandleGroupVecVariableId");
  m.attr("ArrayHandleGroupVecVariableId") = std::is_same<viskores::Id, viskores::Int32>::value
    ? m.attr("ArrayHandleGroupVecVariableInt32")
    : m.attr("ArrayHandleGroupVecVariableInt64");

  erase_existing_name("array_from_numpy");
  m.attr("array_from_numpy") = nb::cpp_function([](nb::object values, bool copy)
                                                { return NumPyArrayToUnknownArray(values, copy); },
                                                nb::arg("values"),
                                                nb::arg("copy") = true,
                                                doc::ArrayFromNumPy);

  erase_existing_name("asnumpy");
  m.attr("asnumpy") = nb::cpp_function(
    [](nb::handle object, bool copy) -> nb::object
    {
      viskores::cont::UnknownArrayHandle array;
      if (TryPythonObjectToRegisteredArray(object, array))
      {
        return UnknownArrayToNumPyArray(array, copy);
      }

      viskores::cont::Field* field = nullptr;
      if (nb::try_cast(object, field))
      {
        return UnknownArrayToNumPyArray(field->GetData(), copy);
      }

      return nb::module_::import_("numpy").attr("asarray")(object);
    },
    nb::arg("object"),
    nb::arg("copy") = true,
    doc::AsNumPy);

  erase_existing_name("ArrayCopy");
  m.attr("ArrayCopy") = nb::cpp_function([](nb::handle source, nb::handle destination)
                                         { ArrayCopyToPythonDestination(source, destination); },
                                         nb::arg("source"),
                                         nb::arg("destination"),
                                         doc::ArrayCopy);
}

} // namespace viskores::python::bindings
