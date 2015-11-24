//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_io_reader_VTKDataSetReaderBase_h
#define vtk_m_io_reader_VTKDataSetReaderBase_h

#include <vtkm/io/internal/Endian.h>
#include <vtkm/io/internal/VTKDataSetCells.h>
#include <vtkm/io/internal/VTKDataSetStructures.h>
#include <vtkm/io/internal/VTKDataSetTypes.h>

#include <vtkm/Types.h>
#include <vtkm/VecTraits.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/io/ErrorIO.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/smart_ptr/scoped_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/type_traits/is_same.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>


namespace vtkm {
namespace io {
namespace reader {

namespace internal {

struct VTKDataSetFile
{
  std::string FileName;
  vtkm::Id2 Version;
  std::string Title;
  bool IsBinary;
  vtkm::io::internal::DataSetStructure Structure;
  std::ifstream Stream;
};

inline void PrintVTKDataFileSummary(const VTKDataSetFile &df, std::ostream &out)
{
  out << "\tFile: " << df.FileName << std::endl;
  out << "\tVersion: " << df.Version[0] << "." << df.Version[0] << std::endl;
  out << "\tTitle: " << df.Title << std::endl;
  out << "\tFormat: " << (df.IsBinary ? "BINARY" : "ASCII") << std::endl;
  out << "\tDataSet type: " << vtkm::io::internal::DataSetStructureString(df.Structure)
      << std::endl;
}


inline void parseAssert(bool condition)
{
  if (!condition)
  {
    throw vtkm::io::ErrorIO("Parse Error");
  }
}


template <typename T> struct StreamIOType { typedef T Type; };
template <> struct StreamIOType<vtkm::Int8> { typedef vtkm::Int16 Type; };
template <> struct StreamIOType<vtkm::UInt8> { typedef vtkm::UInt16 Type; };


// Since Fields and DataSets store data in the default DynamicArrayHandle, convert
// the data to the closest type supported by default. The following will
// need to be updated if DynamicArrayHandle or TypeListTagCommon changes.
template <typename T> struct ClosestCommonType { typedef T Type; };
template <> struct ClosestCommonType<vtkm::Int8> { typedef vtkm::Int32 Type; };
template <> struct ClosestCommonType<vtkm::UInt8> { typedef vtkm::Int32 Type; };
template <> struct ClosestCommonType<vtkm::Int16> { typedef vtkm::Int32 Type; };
template <> struct ClosestCommonType<vtkm::UInt16> { typedef vtkm::Int32 Type; };
template <> struct ClosestCommonType<vtkm::UInt32> { typedef vtkm::Int64 Type; };
template <> struct ClosestCommonType<vtkm::UInt64> { typedef vtkm::Int64 Type; };

template <typename T> struct ClosestFloat { typedef T Type; };
template <> struct ClosestFloat<vtkm::Int8> { typedef vtkm::Float32 Type; };
template <> struct ClosestFloat<vtkm::UInt8> { typedef vtkm::Float32 Type; };
template <> struct ClosestFloat<vtkm::Int16> { typedef vtkm::Float32 Type; };
template <> struct ClosestFloat<vtkm::UInt16> { typedef vtkm::Float32 Type; };
template <> struct ClosestFloat<vtkm::Int32> { typedef vtkm::Float64 Type; };
template <> struct ClosestFloat<vtkm::UInt32> { typedef vtkm::Float64 Type; };
template <> struct ClosestFloat<vtkm::Int64> { typedef vtkm::Float64 Type; };
template <> struct ClosestFloat<vtkm::UInt64> { typedef vtkm::Float64 Type; };

template <typename T>
vtkm::cont::DynamicArrayHandle CreateDynamicArrayHandle(const std::vector<T> &vec)
{
  switch (vtkm::VecTraits<T>::NUM_COMPONENTS)
  {
  case 1:
    {
    typedef typename ClosestCommonType<T>::Type CommonType;
    if (!boost::is_same<T, CommonType>::value)
    {
      std::cerr << "Type " << vtkm::io::internal::DataTypeName<T>::Name()
                << " is currently unsupported. Converting to "
                << vtkm::io::internal::DataTypeName<CommonType>::Name() << "."
                << std::endl;
    }

    vtkm::cont::ArrayHandle<CommonType> output;
    output.Allocate(static_cast<vtkm::Id>(vec.size()));
    for (vtkm::Id i = 0; i < output.GetNumberOfValues(); ++i)
    {
      output.GetPortalControl().Set(i,
        static_cast<CommonType>(vec[static_cast<std::size_t>(i)]));
    }

    return vtkm::cont::DynamicArrayHandle(output);
    }
  case 2:
  case 3:
    {
    typedef typename vtkm::VecTraits<T>::ComponentType InComponentType;
    typedef typename ClosestFloat<InComponentType>::Type OutComponentType;
    typedef vtkm::Vec<OutComponentType, 3> CommonType;
    if (!boost::is_same<T, CommonType>::value)
    {
      std::cerr << "Type " << vtkm::io::internal::DataTypeName<InComponentType>::Name()
                << "[" << vtkm::VecTraits<T>::NUM_COMPONENTS << "] "
                << "is currently unsupported. Converting to "
                << vtkm::io::internal::DataTypeName<OutComponentType>::Name() << "[3]."
                << std::endl;
    }

    vtkm::cont::ArrayHandle<CommonType> output;
    output.Allocate(static_cast<vtkm::Id>(vec.size()));
    for (vtkm::Id i = 0; i < output.GetNumberOfValues(); ++i)
    {
      CommonType outval = CommonType();
      for (vtkm::IdComponent j = 0; j < vtkm::VecTraits<T>::NUM_COMPONENTS; ++j)
      {
        outval[j] = static_cast<OutComponentType>(
            vtkm::VecTraits<T>::GetComponent(vec[static_cast<std::size_t>(i)], j));
      }
      output.GetPortalControl().Set(i, outval);
    }

    return vtkm::cont::DynamicArrayHandle(output);
    }
  default:
    {
    std::cerr << "Only 1, 2, or 3 components supported. Skipping." << std::endl;
    return vtkm::cont::DynamicArrayHandle(vtkm::cont::ArrayHandle<vtkm::Float32>());
    }
  }
}

} // namespace internal


class VTKDataSetReaderBase
{
public:
  explicit VTKDataSetReaderBase(const char *fileName)
    : DataFile(new internal::VTKDataSetFile), DataSet(), Loaded(false)
  {
    this->DataFile->FileName = fileName;
  }

  virtual ~VTKDataSetReaderBase()
  { }

  const vtkm::cont::DataSet& ReadDataSet()
  {
    if (!this->Loaded)
    {
      try
      {
        this->OpenFile();
        this->ReadHeader();
        this->Read();
        this->CloseFile();
        this->Loaded = true;
      }
      catch (std::ifstream::failure e)
      {
        std::string message("IO Error: ");
        throw vtkm::io::ErrorIO(message + e.what());
      }
    }

    return this->DataSet;
  }

  const vtkm::cont::DataSet& GetDataSet() const
  {
    return this->DataSet;
  }

  virtual void PrintSummary(std::ostream &out) const
  {
    out << "VTKDataSetReader" << std::endl;
    PrintVTKDataFileSummary(*this->DataFile.get(), out);
    this->DataSet.PrintSummary(out);
  }

protected:
  void ReadPoints()
  {
    std::string tag, dataType;
    std::size_t numPoints;
    this->DataFile->Stream >> tag >> numPoints >> dataType >> std::ws;
    internal::parseAssert(tag == "POINTS");

    vtkm::cont::DynamicArrayHandle points;
    this->DoReadDynamicArray(dataType, numPoints, 3, points);

    this->DataSet.AddCoordinateSystem(
        vtkm::cont::CoordinateSystem("coordinates", 1, points));
  }

  void ReadCells(vtkm::cont::ArrayHandle<vtkm::Id> &connectivity,
                 vtkm::cont::ArrayHandle<vtkm::IdComponent> &numIndices)
  {
    vtkm::Id numCells, numInts;
    this->DataFile->Stream >> numCells >> numInts >> std::ws;

    connectivity.Allocate(numInts - numCells);
    numIndices.Allocate(numCells);

    std::vector<vtkm::Int32> buffer(static_cast<std::size_t>(numInts));
    this->ReadArray(buffer);

    vtkm::Int32 *buffp = &buffer[0];
    vtkm::cont::ArrayHandle<vtkm::Id>::PortalControl connectivityPortal =
      connectivity.GetPortalControl();
    vtkm::cont::ArrayHandle<vtkm::IdComponent>::PortalControl numIndicesPortal =
      numIndices.GetPortalControl();
    for (vtkm::Id i = 0, connInd = 0; i < numCells; ++i)
    {
      vtkm::IdComponent numInds = static_cast<vtkm::IdComponent>(*buffp++);
      numIndicesPortal.Set(i, numInds);
      for (vtkm::IdComponent j = 0; j < numInds; ++j, ++connInd)
      {
        connectivityPortal.Set(connInd, static_cast<vtkm::Id>(*buffp++));
      }
    }
  }

  void ReadShapes(vtkm::cont::ArrayHandle<vtkm::UInt8> &shapes)
  {
    std::string tag;
    vtkm::Id numCells;
    this->DataFile->Stream >> tag >> numCells >> std::ws;
    internal::parseAssert(tag == "CELL_TYPES");

    shapes.Allocate(numCells);
    std::vector<vtkm::Int32> buffer(static_cast<std::size_t>(numCells));
    this->ReadArray(buffer);

    vtkm::Int32 *buffp = &buffer[0];
    vtkm::cont::ArrayHandle<vtkm::UInt8>::PortalControl shapesPortal =
      shapes.GetPortalControl();
    for (vtkm::Id i = 0; i < numCells; ++i)
    {
      shapesPortal.Set(i, static_cast<vtkm::UInt8>(*buffp++));
    }
  }

  void ReadAttributes()
  {
    if (this->DataFile->Stream.eof())
    {
      return;
    }

    vtkm::cont::Field::AssociationEnum association = vtkm::cont::Field::ASSOC_ANY;
    std::size_t size;

    std::string tag;
    this->DataFile->Stream >> tag;
    while (!this->DataFile->Stream.eof())
    {
      if (tag == "POINT_DATA")
      {
        association = vtkm::cont::Field::ASSOC_POINTS;
      }
      else if (tag == "CELL_DATA")
      {
        association = vtkm::cont::Field::ASSOC_CELL_SET;
      }
      else
      {
        internal::parseAssert(false);
      }

      this->DataFile->Stream >> size;
      while (!this->DataFile->Stream.eof())
      {
        std::string name;
        vtkm::cont::ArrayHandle<vtkm::Float32> empty;
        vtkm::cont::DynamicArrayHandle data(empty);

        this->DataFile->Stream >> tag;
        if (tag == "SCALARS")
        {
          this->ReadScalars(size, name, data);
        }
        else if (tag == "COLOR_SCALARS")
        {
          this->ReadColorScalars(size, name);
        }
        else if (tag == "LOOKUP_TABLE")
        {
          this->ReadLookupTable(name);
        }
        else if (tag == "VECTORS" || tag == "NORMALS")
        {
          this->ReadVectors(size, name, data);
        }
        else if (tag == "TEXTURE_COORDINATES")
        {
          this->ReadTextureCoordinates(size, name, data);
        }
        else if (tag == "TENSORS")
        {
          this->ReadTensors(size, name, data);
        }
        else if (tag == "FIELD")
        {
          this->ReadFields(name);
        }
        else
        {
          break;
        }

        if (data.GetNumberOfValues() > 0)
        {
          name = tag + ":" + name;
          switch (association)
          {
          case vtkm::cont::Field::ASSOC_POINTS:
            this->DataSet.AddField(vtkm::cont::Field(name, 0, association, data));
            break;
          case vtkm::cont::Field::ASSOC_CELL_SET:
            data.CastAndCall(PermuteCellData(this->CellsPermutation, data));
            this->DataSet.AddField(
                vtkm::cont::Field(name, 0, association, "cells", data));
            break;
          default:
            break;
          }
        }
      }
    }
  }

  void SetCellsPermutation(const vtkm::cont::ArrayHandle<vtkm::Id> &permutation)
  {
    this->CellsPermutation = permutation;
  }

  void TransferDataFile(VTKDataSetReaderBase &reader)
  {
    reader.DataFile.swap(this->DataFile);
    this->DataFile.reset(NULL);
  }

  virtual void CloseFile()
  {
    this->DataFile->Stream.close();
  }

private:
  void OpenFile()
  {
    this->DataFile->Stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    this->DataFile->Stream.open(this->DataFile->FileName.c_str(),
                                std::ios_base::in | std::ios_base::binary);
  }

  void ReadHeader()
  {
    char vstring[] = "# vtk DataFile Version";
    const int vlen = sizeof(vstring);

    // Read version line
    char vbuf[vlen];
    this->DataFile->Stream.read(vbuf, vlen - 1);
    vbuf[vlen - 1] = '\0';
    if (std::string(vbuf) != "# vtk DataFile Version")
    {
      throw vtkm::io::ErrorIO("Incorrect file format.");
    }

    char dot;
    this->DataFile->Stream >> this->DataFile->Version[0] >> dot
                           >> this->DataFile->Version[1] >> std::ws;

    // Read title line
    std::getline(this->DataFile->Stream, this->DataFile->Title);

    // Read format line
    this->DataFile->IsBinary = false;
    std::string format;
    this->DataFile->Stream >> format >> std::ws;
    if (format == "BINARY")
    {
      this->DataFile->IsBinary = true;
    }
    else if (format != "ASCII")
    {
      throw vtkm::io::ErrorIO("Unsupported Format.");
    }

    // Read structure line
    std::string tag, structStr;
    this->DataFile->Stream >> tag >> structStr >> std::ws;
    internal::parseAssert(tag == "DATASET");

    this->DataFile->Structure = vtkm::io::internal::DataSetStructureId(structStr);
    if (this->DataFile->Structure == vtkm::io::internal::DATASET_UNKNOWN)
    {
      throw  vtkm::io::ErrorIO("Unsupported DataSet type.");
    }
  }

  virtual void Read() = 0;

  void ReadScalars(std::size_t numElements, std::string &dataName,
                   vtkm::cont::DynamicArrayHandle &data)
  {
    std::string dataType, lookupTableName;
    vtkm::IdComponent numComponents = 1;
    this->DataFile->Stream >> dataName >> dataType;

    std::string tag;
    this->DataFile->Stream >> tag;
    if (tag != "LOOKUP_TABLE")
    {
      try
      {
        numComponents = boost::lexical_cast<vtkm::IdComponent>(tag);
      }
      catch (boost::bad_lexical_cast &)
      {
        internal::parseAssert(false);
      }
      this->DataFile->Stream >> tag;
    }

    internal::parseAssert(tag == "LOOKUP_TABLE");
    this->DataFile->Stream >> lookupTableName >> std::ws;

    this->DoReadDynamicArray(dataType, numElements, numComponents, data);
  }

  void ReadColorScalars(std::size_t numElements, std::string &dataName)
  {
    std::cerr << "Support for COLOR_SCALARS is not implemented. Skipping."
              << std::endl;

    std::size_t numValues;
    this->DataFile->Stream >> dataName >> numValues >> std::ws;
    this->SkipArray(numElements * numValues, vtkm::io::internal::ColorChannel8());
  }

  void ReadLookupTable(std::string &dataName)
  {
    std::cerr << "Support for LOOKUP_TABLE is not implemented. Skipping."
              << std::endl;

    std::size_t numEntries;
    this->DataFile->Stream >> dataName >> numEntries >> std::ws;
    this->SkipArray(numEntries, vtkm::Vec<vtkm::io::internal::ColorChannel8, 4>());
  }

  void ReadTextureCoordinates(std::size_t numElements, std::string &dataName,
                              vtkm::cont::DynamicArrayHandle &data)
  {
    vtkm::IdComponent numComponents;
    std::string dataType;
    this->DataFile->Stream >> dataName >> numComponents >> dataType >> std::ws;

    this->DoReadDynamicArray(dataType, numElements, numComponents, data);
  }

  void ReadVectors(std::size_t numElements, std::string &dataName,
                   vtkm::cont::DynamicArrayHandle &data)
  {
    std::string dataType;
    this->DataFile->Stream >> dataName >> dataType >> std::ws;

    this->DoReadDynamicArray(dataType, numElements, 3, data);
  }

  void ReadTensors(std::size_t numElements, std::string &dataName,
                   vtkm::cont::DynamicArrayHandle &data)
  {
    std::string dataType;
    this->DataFile->Stream >> dataName >> dataType >> std::ws;

    this->DoReadDynamicArray(dataType, numElements, 9, data);
  }

  void ReadFields(std::string &dataName)
  {
    std::cerr << "Support for FIELD is not implemented. Skipping."
              << std::endl;

    vtkm::Id numArrays;
    this->DataFile->Stream >> dataName >> numArrays >> std::ws;
    for (vtkm::Id i = 0; i < numArrays; ++i)
    {
      std::size_t numTuples;
      vtkm::IdComponent numComponents;
      std::string arrayName, dataType;
      this->DataFile->Stream >> arrayName >> numComponents >> numTuples
                             >> dataType >> std::ws;

      this->DoSkipDynamicArray(dataType, numTuples, numComponents);
    }
  }

  class SkipDynamicArray
  {
  public:
    SkipDynamicArray(VTKDataSetReaderBase *reader,
                     std::size_t numElements)
      : Reader(reader), NumElements(numElements)
    { }

    template <typename T>
    void operator()(T) const
    {
      this->Reader->SkipArray(this->NumElements, T());
    }

    template <typename T>
    void operator()(vtkm::IdComponent numComponents, T) const
    {
      this->Reader->SkipArray(
          this->NumElements * static_cast<std::size_t>(numComponents), T());
    }

  protected:
    VTKDataSetReaderBase *Reader;
    std::size_t NumElements;
  };

  class ReadDynamicArray : public SkipDynamicArray
  {
  public:
    ReadDynamicArray(VTKDataSetReaderBase *reader,
                     std::size_t numElements,
                     vtkm::cont::DynamicArrayHandle &data)
      : SkipDynamicArray(reader, numElements), Data(&data)
    { }

    template <typename T>
    void operator()(T) const
    {
      std::vector<T> buffer(this->NumElements);
      this->Reader->ReadArray(buffer);

      *this->Data = internal::CreateDynamicArrayHandle(buffer);
    }

    template <typename T>
    void operator()(vtkm::IdComponent numComponents, T) const
    {
      std::cerr << "Support for " << numComponents
                << " components not implemented. Skipping." << std::endl;
      SkipDynamicArray::operator()(numComponents, T());
    }

  private:
    vtkm::cont::DynamicArrayHandle *Data;
  };

  void DoSkipDynamicArray(std::string dataType, std::size_t numElements,
                          vtkm::IdComponent numComponents)
  {
    vtkm::io::internal::DataType typeId = vtkm::io::internal::DataTypeId(dataType);
    vtkm::io::internal::SelectTypeAndCall(typeId, numComponents,
                                          SkipDynamicArray(this, numElements));
  }

  void DoReadDynamicArray(std::string dataType, std::size_t numElements,
                          vtkm::IdComponent numComponents,
                          vtkm::cont::DynamicArrayHandle &data)
  {
    vtkm::io::internal::DataType typeId = vtkm::io::internal::DataTypeId(dataType);
    vtkm::io::internal::SelectTypeAndCall(typeId, numComponents,
                                          ReadDynamicArray(this, numElements, data));
  }

  template <typename T>
  void ReadArray(std::vector<T> &buffer)
  {
    std::size_t numElements = buffer.size();
    if (this->DataFile->IsBinary)
    {
      this->DataFile->Stream.read(reinterpret_cast<char*>(&buffer[0]),
          static_cast<std::streamsize>(numElements * sizeof(T)));
      if(vtkm::io::internal::IsLittleEndian())
      {
        vtkm::io::internal::FlipEndianness(buffer);
      }
    }
    else
    {
      typedef typename vtkm::VecTraits<T>::ComponentType ComponentType;
      const vtkm::IdComponent numComponents = vtkm::VecTraits<T>::NUM_COMPONENTS;

      for (std::size_t i = 0; i < numElements; ++i)
      {
        for (vtkm::IdComponent j = 0; j < numComponents; ++j)
        {
          typename internal::StreamIOType<ComponentType>::Type val;
          this->DataFile->Stream >> val;
          vtkm::VecTraits<T>::SetComponent(buffer[i], j,
                                           static_cast<ComponentType>(val));
        }
      }
    }
    this->DataFile->Stream >> std::ws;
  }

  template <vtkm::IdComponent NumComponents>
  void ReadArray(std::vector<vtkm::Vec<vtkm::io::internal::DummyBitType,
                                       NumComponents> > &buffer)
  {
    std::cerr << "Support for data type 'bit' is not implemented. Skipping."
              << std::endl;
    this->SkipArray(buffer.size(), vtkm::Vec<vtkm::io::internal::DummyBitType,
                    NumComponents>());
    buffer.clear();
  }

  void ReadArray(std::vector<vtkm::io::internal::DummyBitType> &buffer)
  {
    std::cerr << "Support for data type 'bit' is not implemented. Skipping."
              << std::endl;
    this->SkipArray(buffer.size(), vtkm::io::internal::DummyBitType());
    buffer.clear();
  }

  template <typename T>
  void SkipArray(std::size_t numElements, T)
  {
    if (this->DataFile->IsBinary)
    {
      this->DataFile->Stream.seekg(
          static_cast<std::streamoff>(numElements * sizeof(T)),
          std::ios_base::cur);
    }
    else
    {
      typedef typename vtkm::VecTraits<T>::ComponentType ComponentType;
      const vtkm::IdComponent numComponents = vtkm::VecTraits<T>::NUM_COMPONENTS;

      for (std::size_t i = 0; i < numElements; ++i)
      {
        for (vtkm::IdComponent j = 0; j < numComponents; ++j)
        {
          typename internal::StreamIOType<ComponentType>::Type val;
          this->DataFile->Stream >> val;
        }
      }
    }
    this->DataFile->Stream >> std::ws;
  }

  template <vtkm::IdComponent NumComponents>
  void SkipArray(std::size_t numElements,
                 vtkm::Vec<vtkm::io::internal::DummyBitType, NumComponents>)
  {
    this->SkipArray(numElements * static_cast<std::size_t>(NumComponents),
                    vtkm::io::internal::DummyBitType());
  }

  void SkipArray(std::size_t numElements, vtkm::io::internal::DummyBitType)
  {
    if (this->DataFile->IsBinary)
    {
      numElements = (numElements + 7) / 8;
      this->DataFile->Stream.seekg(static_cast<std::streamoff>(numElements),
                                   std::ios_base::cur);
    }
    else
    {
      for (std::size_t i = 0; i < numElements; ++i)
      {
        vtkm::UInt16 val;
        this->DataFile->Stream >> val;
      }
    }
    this->DataFile->Stream >> std::ws;
  }

  class PermuteCellData
  {
  public:
    PermuteCellData(const vtkm::cont::ArrayHandle<vtkm::Id> &permutation,
                    vtkm::cont::DynamicArrayHandle &data)
      : Permutation(permutation), Data(&data)
    { }

    template <typename T>
    void operator()(const vtkm::cont::ArrayHandle<T> &handle) const
    {
      vtkm::cont::ArrayHandle<T> out;
      out.Allocate(this->Permutation.GetNumberOfValues());

      vtkm::cont::ArrayHandle<vtkm::Id>::PortalConstControl permutationPortal =
        this->Permutation.GetPortalConstControl();
      typename vtkm::cont::ArrayHandle<T>::PortalConstControl inPortal =
        handle.GetPortalConstControl();
      typename vtkm::cont::ArrayHandle<T>::PortalControl outPortal =
        out.GetPortalControl();
      for (vtkm::Id i = 0; i < out.GetNumberOfValues(); ++i)
      {
        outPortal.Set(i, inPortal.Get(permutationPortal.Get(i)));
      }
      *this->Data = vtkm::cont::DynamicArrayHandle(out);
    }

  private:
    const vtkm::cont::ArrayHandle<vtkm::Id> Permutation;
    vtkm::cont::DynamicArrayHandle *Data;
  };

protected:
  boost::scoped_ptr<internal::VTKDataSetFile> DataFile;
  vtkm::cont::DataSet DataSet;

private:
  bool Loaded;
  vtkm::cont::ArrayHandle<vtkm::Id> CellsPermutation;

  friend class VTKDataSetReader;
};

}
}
} // vtkm::io::reader

VTKM_BASIC_TYPE_VECTOR(vtkm::io::internal::ColorChannel8)
VTKM_BASIC_TYPE_VECTOR(vtkm::io::internal::DummyBitType)

#endif // vtk_m_io_reader_VTKDataSetReaderBase_h
