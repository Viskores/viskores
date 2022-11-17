//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/io/VTKDataSetWriter.h>

#include <vtkm/CellShape.h>

#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Field.h>

#include <vtkm/io/ErrorIO.h>

#include <vtkm/io/internal/Endian.h>
#include <vtkm/io/internal/VTKDataSetTypes.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{

struct CallForBaseTypeFunctor
{
  template <typename T, typename Functor, typename... Args>
  void operator()(T t,
                  bool& success,
                  Functor functor,
                  const vtkm::cont::UnknownArrayHandle& array,
                  Args&&... args)
  {
    if (!array.IsBaseComponentType<T>())
    {
      return;
    }

    success = true;

    functor(t, array, std::forward<Args>(args)...);
  }
};

template <typename Functor, typename... Args>
void CallForBaseType(Functor&& functor, const vtkm::cont::UnknownArrayHandle& array, Args&&... args)
{
  bool success = true;
  vtkm::ListForEach(CallForBaseTypeFunctor{},
                    vtkm::TypeListScalarAll{},
                    success,
                    std::forward<Functor>(functor),
                    array,
                    std::forward<Args>(args)...);
  if (!success)
  {
    std::ostringstream out;
    out << "Unrecognized base type in array to be written out.\nArray: ";
    array.PrintSummary(out);

    throw vtkm::cont::ErrorBadValue(out.str());
  }
}

template <typename T>
using ArrayHandleRectilinearCoordinates =
  vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<T>,
                                          vtkm::cont::ArrayHandle<T>,
                                          vtkm::cont::ArrayHandle<T>>;

struct OutputArrayDataFunctor
{
  template <typename T>
  VTKM_CONT void operator()(T,
                            const vtkm::cont::UnknownArrayHandle& array,
                            std::ostream& out,
                            vtkm::io::FileType fileType) const
  {
    auto componentArray = array.ExtractArrayFromComponents<T>();
    auto portal = componentArray.ReadPortal();
    switch (fileType)
    {
      case vtkm::io::FileType::ASCII:
        this->OutputAsciiArray(portal, out);
        break;
      case vtkm::io::FileType::BINARY:
        this->OutputBinaryArray(portal, out);
        break;
    }
  }

  template <typename PortalType>
  void OutputAsciiArray(const PortalType& portal, std::ostream& out) const
  {
    using T = typename PortalType::ValueType::ComponentType;

    vtkm::Id numValues = portal.GetNumberOfValues();
    for (vtkm::Id valueIndex = 0; valueIndex < numValues; ++valueIndex)
    {
      auto value = portal.Get(valueIndex);
      for (vtkm::IdComponent cIndex = 0; cIndex < value.GetNumberOfComponents(); ++cIndex)
      {
        out << ((cIndex == 0) ? "" : " ");
        if (std::numeric_limits<T>::is_integer && sizeof(T) == 1)
        {
          out << static_cast<int>(value[cIndex]);
        }
        else
        {
          out << value[cIndex];
        }
      }
      out << "\n";
    }
  }

  template <typename PortalType>
  void OutputBinaryArray(const PortalType& portal, std::ostream& out) const
  {
    using T = typename PortalType::ValueType::ComponentType;

    vtkm::Id numValues = portal.GetNumberOfValues();
    std::vector<T> tuple;
    for (vtkm::Id valueIndex = 0; valueIndex < numValues; ++valueIndex)
    {
      auto value = portal.Get(valueIndex);
      tuple.resize(static_cast<std::size_t>(value.GetNumberOfComponents()));
      for (vtkm::IdComponent cIndex = 0; cIndex < value.GetNumberOfComponents(); ++cIndex)
      {
        tuple[static_cast<std::size_t>(cIndex)] = value[cIndex];
      }
      if (vtkm::io::internal::IsLittleEndian())
      {
        vtkm::io::internal::FlipEndianness(tuple);
      }
      out.write(reinterpret_cast<const char*>(tuple.data()),
                static_cast<std::streamsize>(tuple.size() * sizeof(T)));
    }
  }
};

void OutputArrayData(const vtkm::cont::UnknownArrayHandle& array,
                     std::ostream& out,
                     vtkm::io::FileType fileType)
{
  CallForBaseType(OutputArrayDataFunctor{}, array, out, fileType);
}

struct GetFieldTypeNameFunctor
{
  template <typename Type>
  void operator()(Type, const vtkm::cont::UnknownArrayHandle& array, std::string& name) const
  {
    if (array.IsBaseComponentType<Type>())
    {
      name = vtkm::io::internal::DataTypeName<Type>::Name();
    }
  }
};

std::string GetFieldTypeName(const vtkm::cont::UnknownArrayHandle& array)
{
  std::string name;
  CallForBaseType(GetFieldTypeNameFunctor{}, array, name);
  return name;
}

template <vtkm::IdComponent DIM>
void WriteDimensions(std::ostream& out, const vtkm::cont::CellSetStructured<DIM>& cellSet)
{
  auto pointDimensions = cellSet.GetPointDimensions();
  using VTraits = vtkm::VecTraits<decltype(pointDimensions)>;

  out << "DIMENSIONS ";
  out << VTraits::GetComponent(pointDimensions, 0) << " ";
  out << (DIM > 1 ? VTraits::GetComponent(pointDimensions, 1) : 1) << " ";
  out << (DIM > 2 ? VTraits::GetComponent(pointDimensions, 2) : 1) << "\n";
}

void WritePoints(std::ostream& out, const vtkm::cont::DataSet& dataSet, vtkm::io::FileType fileType)
{
  ///\todo: support other coordinate systems
  int cindex = 0;
  auto cdata = dataSet.GetCoordinateSystem(cindex).GetData();

  std::string typeName = GetFieldTypeName(cdata);

  vtkm::Id npoints = cdata.GetNumberOfValues();
  out << "POINTS " << npoints << " " << typeName << " " << '\n';

  OutputArrayData(cdata, out, fileType);
}

template <class CellSetType>
void WriteExplicitCellsAscii(std::ostream& out, const CellSetType& cellSet)
{
  vtkm::Id nCells = cellSet.GetNumberOfCells();

  vtkm::Id conn_length = 0;
  for (vtkm::Id i = 0; i < nCells; ++i)
  {
    conn_length += 1 + cellSet.GetNumberOfPointsInCell(i);
  }

  out << "CELLS " << nCells << " " << conn_length << '\n';

  for (vtkm::Id i = 0; i < nCells; ++i)
  {
    vtkm::cont::ArrayHandle<vtkm::Id> ids;
    vtkm::Id nids = cellSet.GetNumberOfPointsInCell(i);
    cellSet.GetIndices(i, ids);
    out << nids;
    auto IdPortal = ids.ReadPortal();
    for (int j = 0; j < nids; ++j)
    {
      out << " " << IdPortal.Get(j);
    }
    out << '\n';
  }

  out << "CELL_TYPES " << nCells << '\n';
  for (vtkm::Id i = 0; i < nCells; ++i)
  {
    vtkm::Id shape = cellSet.GetCellShape(i);
    out << shape << '\n';
  }
}

template <class CellSetType>
void WriteExplicitCellsBinary(std::ostream& out, const CellSetType& cellSet)
{
  vtkm::Id nCells = cellSet.GetNumberOfCells();

  vtkm::Id conn_length = 0;
  for (vtkm::Id i = 0; i < nCells; ++i)
  {
    conn_length += 1 + cellSet.GetNumberOfPointsInCell(i);
  }

  out << "CELLS " << nCells << " " << conn_length << '\n';

  std::vector<vtkm::Int32> buffer;
  buffer.reserve(static_cast<std::size_t>(conn_length));
  for (vtkm::Id i = 0; i < nCells; ++i)
  {
    vtkm::cont::ArrayHandle<vtkm::Id> ids;
    vtkm::Id nids = cellSet.GetNumberOfPointsInCell(i);
    cellSet.GetIndices(i, ids);
    buffer.push_back(static_cast<vtkm::Int32>(nids));
    auto IdPortal = ids.ReadPortal();
    for (int j = 0; j < nids; ++j)
    {
      buffer.push_back(static_cast<vtkm::Int32>(IdPortal.Get(j)));
    }
  }
  if (vtkm::io::internal::IsLittleEndian())
  {
    vtkm::io::internal::FlipEndianness(buffer);
  }
  VTKM_ASSERT(static_cast<vtkm::Id>(buffer.size()) == conn_length);
  out.write(reinterpret_cast<const char*>(buffer.data()),
            static_cast<std::streamsize>(buffer.size() * sizeof(vtkm::Int32)));

  out << "CELL_TYPES " << nCells << '\n';
  buffer.resize(0);
  buffer.reserve(static_cast<std::size_t>(nCells));
  for (vtkm::Id i = 0; i < nCells; ++i)
  {
    buffer.push_back(static_cast<vtkm::Int32>(cellSet.GetCellShape(i)));
  }
  if (vtkm::io::internal::IsLittleEndian())
  {
    vtkm::io::internal::FlipEndianness(buffer);
  }
  VTKM_ASSERT(static_cast<vtkm::Id>(buffer.size()) == nCells);
  out.write(reinterpret_cast<const char*>(buffer.data()),
            static_cast<std::streamsize>(buffer.size() * sizeof(vtkm::Int32)));
}

template <class CellSetType>
void WriteExplicitCells(std::ostream& out, const CellSetType& cellSet, vtkm::io::FileType fileType)
{
  switch (fileType)
  {
    case vtkm::io::FileType::ASCII:
      WriteExplicitCellsAscii(out, cellSet);
      break;
    case vtkm::io::FileType::BINARY:
      WriteExplicitCellsBinary(out, cellSet);
      break;
  }
}

void WritePointFields(std::ostream& out,
                      const vtkm::cont::DataSet& dataSet,
                      vtkm::io::FileType fileType)
{
  bool wrote_header = false;
  for (vtkm::Id f = 0; f < dataSet.GetNumberOfFields(); f++)
  {
    const vtkm::cont::Field field = dataSet.GetField(f);

    if (field.GetAssociation() != vtkm::cont::Field::Association::Points)
    {
      continue;
    }

    if (field.GetName() == dataSet.GetCoordinateSystemName())
    {
      // Do not write out the first coordinate system as a field.
      continue;
    }

    vtkm::Id npoints = field.GetNumberOfValues();
    int ncomps = field.GetData().GetNumberOfComponentsFlat();

    if (!wrote_header)
    {
      out << "POINT_DATA " << npoints << '\n';
      wrote_header = true;
    }

    std::string typeName = GetFieldTypeName(field.GetData());
    std::string name = field.GetName();
    for (auto& c : name)
    {
      if (std::isspace(c))
      {
        c = '_';
      }
    }
    out << "SCALARS " << name << " " << typeName << " " << ncomps << '\n';
    out << "LOOKUP_TABLE default" << '\n';

    OutputArrayData(field.GetData(), out, fileType);
  }
}

void WriteCellFields(std::ostream& out,
                     const vtkm::cont::DataSet& dataSet,
                     vtkm::io::FileType fileType)
{
  bool wrote_header = false;
  for (vtkm::Id f = 0; f < dataSet.GetNumberOfFields(); f++)
  {
    const vtkm::cont::Field field = dataSet.GetField(f);
    if (!field.IsCellField())
    {
      continue;
    }


    vtkm::Id ncells = field.GetNumberOfValues();
    int ncomps = field.GetData().GetNumberOfComponentsFlat();

    if (!wrote_header)
    {
      out << "CELL_DATA " << ncells << '\n';
      wrote_header = true;
    }

    std::string typeName = GetFieldTypeName(field.GetData());

    std::string name = field.GetName();
    for (auto& c : name)
    {
      if (std::isspace(c))
      {
        c = '_';
      }
    }

    out << "SCALARS " << name << " " << typeName << " " << ncomps << '\n';
    out << "LOOKUP_TABLE default" << '\n';

    OutputArrayData(field.GetData(), out, fileType);
  }
}

template <class CellSetType>
void WriteDataSetAsUnstructured(std::ostream& out,
                                const vtkm::cont::DataSet& dataSet,
                                const CellSetType& cellSet,
                                vtkm::io::FileType fileType)
{
  out << "DATASET UNSTRUCTURED_GRID" << '\n';
  WritePoints(out, dataSet, fileType);
  WriteExplicitCells(out, cellSet, fileType);
}

template <vtkm::IdComponent DIM>
void WriteDataSetAsStructuredPoints(std::ostream& out,
                                    const vtkm::cont::ArrayHandleUniformPointCoordinates& points,
                                    const vtkm::cont::CellSetStructured<DIM>& cellSet)
{
  out << "DATASET STRUCTURED_POINTS\n";

  WriteDimensions(out, cellSet);

  auto portal = points.ReadPortal();
  auto origin = portal.GetOrigin();
  auto spacing = portal.GetSpacing();
  out << "ORIGIN " << origin[0] << " " << origin[1] << " " << origin[2] << "\n";
  out << "SPACING " << spacing[0] << " " << spacing[1] << " " << spacing[2] << "\n";
}

template <typename T, vtkm::IdComponent DIM>
void WriteDataSetAsRectilinearGrid(std::ostream& out,
                                   const ArrayHandleRectilinearCoordinates<T>& points,
                                   const vtkm::cont::CellSetStructured<DIM>& cellSet,
                                   vtkm::io::FileType fileType)
{
  out << "DATASET RECTILINEAR_GRID\n";

  WriteDimensions(out, cellSet);

  std::string typeName = vtkm::io::internal::DataTypeName<T>::Name();
  vtkm::cont::ArrayHandle<T> dimArray;

  dimArray = points.GetFirstArray();
  out << "X_COORDINATES " << dimArray.GetNumberOfValues() << " " << typeName << "\n";
  OutputArrayData(dimArray, out, fileType);

  dimArray = points.GetSecondArray();
  out << "Y_COORDINATES " << dimArray.GetNumberOfValues() << " " << typeName << "\n";
  OutputArrayData(dimArray, out, fileType);

  dimArray = points.GetThirdArray();
  out << "Z_COORDINATES " << dimArray.GetNumberOfValues() << " " << typeName << "\n";
  OutputArrayData(dimArray, out, fileType);
}

template <vtkm::IdComponent DIM>
void WriteDataSetAsStructuredGrid(std::ostream& out,
                                  const vtkm::cont::DataSet& dataSet,
                                  const vtkm::cont::CellSetStructured<DIM>& cellSet,
                                  vtkm::io::FileType fileType)
{
  out << "DATASET STRUCTURED_GRID" << '\n';

  WriteDimensions(out, cellSet);

  WritePoints(out, dataSet, fileType);
}

template <vtkm::IdComponent DIM>
void WriteDataSetAsStructured(std::ostream& out,
                              const vtkm::cont::DataSet& dataSet,
                              const vtkm::cont::CellSetStructured<DIM>& cellSet,
                              vtkm::io::FileType fileType)
{
  ///\todo: support rectilinear

  // Type of structured grid (uniform, rectilinear, curvilinear) is determined by coordinate system
  auto coordSystem = dataSet.GetCoordinateSystem().GetData();
  if (coordSystem.IsType<vtkm::cont::ArrayHandleUniformPointCoordinates>())
  {
    // uniform is written as "structured points"
    WriteDataSetAsStructuredPoints(
      out, coordSystem.AsArrayHandle<vtkm::cont::ArrayHandleUniformPointCoordinates>(), cellSet);
  }
  else if (coordSystem.IsType<ArrayHandleRectilinearCoordinates<vtkm::Float32>>())
  {
    WriteDataSetAsRectilinearGrid(
      out,
      coordSystem.AsArrayHandle<ArrayHandleRectilinearCoordinates<vtkm::Float32>>(),
      cellSet,
      fileType);
  }
  else if (coordSystem.IsType<ArrayHandleRectilinearCoordinates<vtkm::Float64>>())
  {
    WriteDataSetAsRectilinearGrid(
      out,
      coordSystem.AsArrayHandle<ArrayHandleRectilinearCoordinates<vtkm::Float64>>(),
      cellSet,
      fileType);
  }
  else
  {
    // Curvilinear is written as "structured grid"
    WriteDataSetAsStructuredGrid(out, dataSet, cellSet, fileType);
  }
}

void Write(std::ostream& out, const vtkm::cont::DataSet& dataSet, vtkm::io::FileType fileType)
{
  // The Paraview parser cannot handle scientific notation:
  out << std::fixed;
  // This causes a big problem for the dataset writer.
  // Fixed point and floating point are fundamentally different.
  // This is a workaround, but until Paraview supports parsing floats,
  // this is as good as we can do.
#ifdef VTKM_USE_DOUBLE_PRECISION
  out << std::setprecision(18);
#else
  out << std::setprecision(10);
#endif
  out << "# vtk DataFile Version 3.0" << '\n';
  out << "vtk output" << '\n';
  switch (fileType)
  {
    case vtkm::io::FileType::ASCII:
      out << "ASCII" << '\n';
      break;
    case vtkm::io::FileType::BINARY:
      out << "BINARY" << '\n';
      break;
  }

  vtkm::cont::UnknownCellSet cellSet = dataSet.GetCellSet();
  if (cellSet.IsType<vtkm::cont::CellSetExplicit<>>())
  {
    WriteDataSetAsUnstructured(
      out, dataSet, cellSet.AsCellSet<vtkm::cont::CellSetExplicit<>>(), fileType);
  }
  else if (cellSet.IsType<vtkm::cont::CellSetStructured<1>>())
  {
    WriteDataSetAsStructured(
      out, dataSet, cellSet.AsCellSet<vtkm::cont::CellSetStructured<1>>(), fileType);
  }
  else if (cellSet.IsType<vtkm::cont::CellSetStructured<2>>())
  {
    WriteDataSetAsStructured(
      out, dataSet, cellSet.AsCellSet<vtkm::cont::CellSetStructured<2>>(), fileType);
  }
  else if (cellSet.IsType<vtkm::cont::CellSetStructured<3>>())
  {
    WriteDataSetAsStructured(
      out, dataSet, cellSet.AsCellSet<vtkm::cont::CellSetStructured<3>>(), fileType);
  }
  else if (cellSet.IsType<vtkm::cont::CellSetSingleType<>>())
  {
    // these function just like explicit cell sets
    WriteDataSetAsUnstructured(
      out, dataSet, cellSet.AsCellSet<vtkm::cont::CellSetSingleType<>>(), fileType);
  }
  else if (cellSet.IsType<vtkm::cont::CellSetExtrude>())
  {
    WriteDataSetAsUnstructured(
      out, dataSet, cellSet.AsCellSet<vtkm::cont::CellSetExtrude>(), fileType);
  }
  else
  {
    throw vtkm::cont::ErrorBadType("Could not determine type to write out.");
  }

  WritePointFields(out, dataSet, fileType);
  WriteCellFields(out, dataSet, fileType);
}

} // anonymous namespace

namespace vtkm
{
namespace io
{

VTKDataSetWriter::VTKDataSetWriter(const char* fileName)
  : FileName(fileName)
{
}

VTKDataSetWriter::VTKDataSetWriter(const std::string& fileName)
  : FileName(fileName)
{
}

void VTKDataSetWriter::WriteDataSet(const vtkm::cont::DataSet& dataSet) const
{
  if (dataSet.GetNumberOfCoordinateSystems() < 1)
  {
    throw vtkm::cont::ErrorBadValue(
      "DataSet has no coordinate system, which is not supported by VTK file format.");
  }
  try
  {
    std::ofstream fileStream(this->FileName.c_str(), std::fstream::trunc | std::fstream::binary);
    Write(fileStream, dataSet, this->GetFileType());
    fileStream.close();
  }
  catch (std::ofstream::failure& error)
  {
    throw vtkm::io::ErrorIO(error.what());
  }
}

vtkm::io::FileType VTKDataSetWriter::GetFileType() const
{
  return this->FileType;
}

void VTKDataSetWriter::SetFileType(vtkm::io::FileType type)
{
  this->FileType = type;
}

}
} // namespace vtkm::io
