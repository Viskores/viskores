//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include "pyviskores_common.h"
#include "pyviskores_bindings.h"

#include <nanobind/stl/string.h>

namespace viskores::python::bindings
{

#if VISKORES_PYTHON_ENABLE_INTEROP
NanobindBufferStateHolder::NanobindBufferStateHolder()
  : HandleStorage(0)
  , State(std::make_shared<viskores::interop::BufferState>(this->HandleStorage))
{
}

NanobindBufferStateHolder::NanobindBufferStateHolder(unsigned long handle)
  : HandleStorage(static_cast<GLuint>(handle))
  , State(std::make_shared<viskores::interop::BufferState>(this->HandleStorage))
{
}

NanobindBufferStateHolder::NanobindBufferStateHolder(unsigned long handle, unsigned long bufferType)
  : HandleStorage(static_cast<GLuint>(handle))
  , State(std::make_shared<viskores::interop::BufferState>(
      this->HandleStorage, static_cast<GLenum>(bufferType)))
{
}

unsigned long NanobindBufferStateHolder::GetHandle()
{
  this->HandleStorage = *this->State->GetHandle();
  return static_cast<unsigned long>(this->HandleStorage);
}

void NanobindBufferStateHolder::SetHandle(unsigned long handle)
{
  this->HandleStorage = static_cast<GLuint>(handle);
}

bool NanobindBufferStateHolder::HasType() const { return this->State->HasType(); }

unsigned long NanobindBufferStateHolder::GetType() const
{
  return static_cast<unsigned long>(this->State->GetType());
}

void NanobindBufferStateHolder::SetType(unsigned long bufferType)
{
  this->State->SetType(static_cast<GLenum>(bufferType));
}

long long NanobindBufferStateHolder::GetSize() const { return this->State->GetSize(); }

long long NanobindBufferStateHolder::GetCapacity() const { return this->State->GetCapacity(); }

std::string NanobindBufferStateHolder::Repr() const
{
  std::ostringstream stream;
  stream << "BufferState(handle=" << this->HandleStorage;
  if (this->State->HasType())
  {
    stream << ", type=" << this->State->GetType();
  }
  stream << ", size=" << this->State->GetSize();
  stream << ", capacity=" << this->State->GetCapacity() << ")";
  return stream.str();
}
#endif

#if VISKORES_PYTHON_ENABLE_INTEROP || VISKORES_PYTHON_ENABLE_TESTING_UTILS
void RegisterNanobindHelperFunctions(nb::module_& m,
                                     const std::function<void(const char*)>& erase_existing_name)
{
#if VISKORES_PYTHON_ENABLE_TESTING_UTILS
  erase_existing_name("MakeGhostCellDataSet");
  m.attr("MakeGhostCellDataSet") = nb::cpp_function(
    [](const std::string& datasetType,
       nb::object dimensions,
       long long ghostLayers,
       const char* ghostName,
       bool addMidGhost) {
      const viskores::Id3 dims = ParseDimensions(dimensions);
      return WrapDataSet(MakeGhostCellDataSetImpl(datasetType,
                                                dims[0],
                                                dims[1],
                                                dims[2],
                                                static_cast<int>(ghostLayers),
                                                ghostName,
                                                addMidGhost));
    },
    nb::arg("dataset_type"),
    nb::arg("dimensions"),
    nb::arg("ghost_layers"),
    nb::arg("ghost_name") = "default",
    nb::arg("add_mid_ghost") = false);
#endif

#if VISKORES_PYTHON_ENABLE_INTEROP
  erase_existing_name("TransferToOpenGL");
  m.attr("TransferToOpenGL") = nb::cpp_function(
    [](nb::object sourceObject,
       nb::object stateObject,
       nb::object fieldNameObject,
       nb::object associationObject,
       int coordinateSystemIndex) {
      std::shared_ptr<viskores::interop::BufferState> state;
      NanobindBufferStateHolder* holder = nullptr;
      if (nb::try_cast(stateObject, holder))
      {
        state = holder->State;
      }
      else
      {
        throw std::runtime_error("state must be a viskores.interop.BufferState.");
      }

      std::shared_ptr<viskores::cont::DataSet> dataSet;
      if (nb::try_cast(sourceObject, dataSet))
      {
      }

      if (dataSet)
      {
        if (!fieldNameObject.is_none())
        {
          if (!nb::isinstance<nb::str>(fieldNameObject))
          {
            throw std::runtime_error("field_name must be a string.");
          }
          std::string fieldName = nb::cast<std::string>(fieldNameObject);
          const auto association =
            ParseAssociation(associationObject, viskores::cont::Field::Association::Any);
          auto field = dataSet->GetField(fieldName, association);
          TransferUnknownArrayToOpenGL(field.GetData(), *state);
        }
        else
        {
          auto coordinateSystem = dataSet->GetCoordinateSystem(coordinateSystemIndex);
          TransferUnknownArrayToOpenGL(coordinateSystem.GetData(), *state);
        }
      }
      else
      {
        auto array = NumPyArrayToUnknownArray(sourceObject);
        TransferUnknownArrayToOpenGL(array, *state);
      }
    },
    nb::arg("source"),
    nb::arg("state"),
    nb::arg("field_name") = nb::none(),
    nb::arg("association") = nb::none(),
    nb::arg("coordinate_system_index") = 0);
#endif
}
#else
void RegisterNanobindHelperFunctions(nb::module_&, const std::function<void(const char*)>&) {}
#endif

#if VISKORES_PYTHON_ENABLE_INTEROP
void RegisterNanobindInteropClasses(nb::module_& m,
                                    const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("BufferState");
  nb::class_<NanobindBufferStateHolder>(m, "BufferState")
    .def("__init__",
         [](NanobindBufferStateHolder* self, nb::object handleObject, nb::object bufferTypeObject) {
           const unsigned long handle = handleObject.is_none()
                                          ? 0UL
                                          : nb::cast<unsigned long>(handleObject);
           if (bufferTypeObject.is_none())
           {
             new (self) NanobindBufferStateHolder(handle);
             return;
           }
           const unsigned long bufferType = nb::cast<unsigned long>(bufferTypeObject);
           new (self) NanobindBufferStateHolder(handle, bufferType);
         },
         nb::arg("handle") = nb::none(),
         nb::arg("buffer_type") = nb::none())
    .def("GetHandle", &NanobindBufferStateHolder::GetHandle)
    .def("SetHandle", &NanobindBufferStateHolder::SetHandle, nb::arg("handle"))
    .def("HasType", &NanobindBufferStateHolder::HasType)
    .def("GetType", &NanobindBufferStateHolder::GetType)
    .def("SetType", &NanobindBufferStateHolder::SetType, nb::arg("buffer_type"))
    .def("GetSize", &NanobindBufferStateHolder::GetSize)
    .def("GetCapacity", &NanobindBufferStateHolder::GetCapacity)
    .def("__repr__", &NanobindBufferStateHolder::Repr);
}
#else
void RegisterNanobindInteropClasses(nb::module_&, const std::function<void(const char*)>&) {}
#endif

} // namespace viskores::python::bindings
