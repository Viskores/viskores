//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/io/VTKDataSetReaderBase.h>

#include <vtkm/VecTraits.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleOffsetsToNumComponents.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/UnknownArrayHandle.h>

#include <algorithm>
#include <string>
#include <vector>

namespace
{

inline void PrintVTKDataFileSummary(const vtkm::io::internal::VTKDataSetFile& df, std::ostream& out)
{
  out << "\tFile: " << df.FileName << std::endl;
  out << "\tVersion: " << df.Version[0] << "." << df.Version[0] << std::endl;
  out << "\tTitle: " << df.Title << std::endl;
  out << "\tFormat: " << (df.IsBinary ? "BINARY" : "ASCII") << std::endl;
  out << "\tDataSet type: " << vtkm::io::internal::DataSetStructureString(df.Structure)
      << std::endl;
}

// Since Fields and DataSets store data in the default UnknownArrayHandle, convert
// the data to the closest type supported by default. The following will
// need to be updated if UnknownArrayHandle or TypeListCommon changes.
template <typename T>
struct ClosestCommonType
{
  using Type = T;
};
template <>
struct ClosestCommonType<vtkm::Int8>
{
  using Type = vtkm::Int32;
};
template <>
struct ClosestCommonType<vtkm::UInt8>
{
  using Type = vtkm::Int32;
};
template <>
struct ClosestCommonType<vtkm::Int16>
{
  using Type = vtkm::Int32;
};
template <>
struct ClosestCommonType<vtkm::UInt16>
{
  using Type = vtkm::Int32;
};
template <>
struct ClosestCommonType<vtkm::UInt32>
{
  using Type = vtkm::Int64;
};
template <>
struct ClosestCommonType<vtkm::UInt64>
{
  using Type = vtkm::Int64;
};

template <typename T>
struct ClosestFloat
{
  using Type = T;
};
template <>
struct ClosestFloat<vtkm::Int8>
{
  using Type = vtkm::Float32;
};
template <>
struct ClosestFloat<vtkm::UInt8>
{
  using Type = vtkm::Float32;
};
template <>
struct ClosestFloat<vtkm::Int16>
{
  using Type = vtkm::Float32;
};
template <>
struct ClosestFloat<vtkm::UInt16>
{
  using Type = vtkm::Float32;
};
template <>
struct ClosestFloat<vtkm::Int32>
{
  using Type = vtkm::Float64;
};
template <>
struct ClosestFloat<vtkm::UInt32>
{
  using Type = vtkm::Float64;
};
template <>
struct ClosestFloat<vtkm::Int64>
{
  using Type = vtkm::Float64;
};
template <>
struct ClosestFloat<vtkm::UInt64>
{
  using Type = vtkm::Float64;
};

template <typename T>
vtkm::cont::UnknownArrayHandle CreateUnknownArrayHandle(const std::vector<T>& vec)
{
  switch (vtkm::VecTraits<T>::NUM_COMPONENTS)
  {
    case 1:
    {
      using CommonType = typename ClosestCommonType<T>::Type;
      constexpr bool not_same = !std::is_same<T, CommonType>::value;
      if (not_same)
      {
        VTKM_LOG_S(vtkm::cont::LogLevel::Info,
                   "Type " << vtkm::io::internal::DataTypeName<T>::Name()
                           << " is currently unsupported. Converting to "
                           << vtkm::io::internal::DataTypeName<CommonType>::Name() << ".");
      }

      vtkm::cont::ArrayHandle<CommonType> output;
      output.Allocate(static_cast<vtkm::Id>(vec.size()));
      auto portal = output.WritePortal();
      for (vtkm::Id i = 0; i < output.GetNumberOfValues(); ++i)
      {
        portal.Set(i, static_cast<CommonType>(vec[static_cast<std::size_t>(i)]));
      }

      return vtkm::cont::UnknownArrayHandle(output);
    }
    case 2:
    case 3:
    case 9:
    {
      constexpr auto numComps = vtkm::VecTraits<T>::NUM_COMPONENTS;

      using InComponentType = typename vtkm::VecTraits<T>::ComponentType;
      using OutComponentType = typename ClosestFloat<InComponentType>::Type;
      using CommonType = vtkm::Vec<OutComponentType, numComps>;
      constexpr bool not_same = !std::is_same<T, CommonType>::value;
      if (not_same)
      {
        VTKM_LOG_S(vtkm::cont::LogLevel::Info,
                   "Type " << vtkm::io::internal::DataTypeName<InComponentType>::Name() << "["
                           << vtkm::VecTraits<T>::GetNumberOfComponents(T()) << "] "
                           << "is currently unsupported. Converting to "
                           << vtkm::io::internal::DataTypeName<OutComponentType>::Name() << "["
                           << numComps << "].");
      }

      vtkm::cont::ArrayHandle<CommonType> output;
      output.Allocate(static_cast<vtkm::Id>(vec.size()));
      auto portal = output.WritePortal();
      for (vtkm::Id i = 0; i < output.GetNumberOfValues(); ++i)
      {
        CommonType outval = CommonType();
        for (vtkm::IdComponent j = 0; j < numComps; ++j)
        {
          outval[j] = static_cast<OutComponentType>(
            vtkm::VecTraits<T>::GetComponent(vec[static_cast<std::size_t>(i)], j));
        }
        portal.Set(i, outval);
      }

      return vtkm::cont::UnknownArrayHandle(output);
    }
    default:
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Warn, "Only 1, 2, 3, or 9 components supported. Skipping.");
      return vtkm::cont::UnknownArrayHandle(vtkm::cont::ArrayHandle<vtkm::Float32>());
    }
  }
}

} // anonymous namespace

namespace vtkm
{
namespace io
{

VTKDataSetReaderBase::VTKDataSetReaderBase(const char* fileName)
  : DataFile(new internal::VTKDataSetFile)
  , DataSet()
  , Loaded(false)
{
  this->DataFile->FileName = fileName;
}

VTKDataSetReaderBase::VTKDataSetReaderBase(const std::string& fileName)
  : DataFile(new internal::VTKDataSetFile)
  , DataSet()
  , Loaded(false)
{
  this->DataFile->FileName = fileName;
}

VTKDataSetReaderBase::~VTKDataSetReaderBase() {}

const vtkm::cont::DataSet& VTKDataSetReaderBase::ReadDataSet()
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
    catch (std::ifstream::failure& e)
    {
      std::string message("IO Error: ");
      throw vtkm::io::ErrorIO(message + e.what());
    }
  }

  return this->DataSet;
}

void VTKDataSetReaderBase::PrintSummary(std::ostream& out) const
{
  out << "VTKDataSetReader" << std::endl;
  PrintVTKDataFileSummary(*this->DataFile.get(), out);
  this->DataSet.PrintSummary(out);
}

void VTKDataSetReaderBase::ReadPoints()
{
  std::string dataType;
  std::size_t numPoints;
  this->DataFile->Stream >> numPoints >> dataType >> std::ws;

  vtkm::cont::UnknownArrayHandle points =
    this->DoReadArrayVariant(vtkm::cont::Field::Association::Points, dataType, numPoints, 3);

  this->DataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", points));
}

void VTKDataSetReaderBase::ReadCells(vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
                                     vtkm::cont::ArrayHandle<vtkm::IdComponent>& numIndices)
{
  if (this->DataFile->Version[0] < 5)
  {
    vtkm::Id numCells, numInts;
    this->DataFile->Stream >> numCells >> numInts >> std::ws;

    connectivity.Allocate(numInts - numCells);
    numIndices.Allocate(numCells);

    std::vector<vtkm::Int32> buffer(static_cast<std::size_t>(numInts));
    this->ReadArray(buffer);

    vtkm::Int32* buffp = buffer.data();
    auto connectivityPortal = connectivity.WritePortal();
    auto numIndicesPortal = numIndices.WritePortal();
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
  else
  {
    vtkm::Id offsetsSize, connSize;
    this->DataFile->Stream >> offsetsSize >> connSize >> std::ws;

    std::string tag, dataType;
    this->DataFile->Stream >> tag >> dataType >> std::ws;
    internal::parseAssert(tag == "OFFSETS");
    auto offsets =
      this->DoReadArrayVariant(vtkm::cont::Field::Association::Any, dataType, offsetsSize, 1);
    offsets.CastAndCallForTypes<vtkm::List<vtkm::Int64, vtkm::Int32>,
                                vtkm::List<vtkm::cont::StorageTagBasic>>(
      [&](const auto& offsetsAH) {
        // Convert on host. There will be several other passes of this array on the host anyway.
        numIndices.Allocate(offsetsSize - 1);
        auto offsetPortal = offsetsAH.ReadPortal();
        auto numIndicesPortal = numIndices.WritePortal();
        for (vtkm::Id cellIndex = 0; cellIndex < offsetsSize - 1; ++cellIndex)
        {
          numIndicesPortal.Set(cellIndex,
                               static_cast<vtkm::IdComponent>(offsetPortal.Get(cellIndex + 1) -
                                                              offsetPortal.Get(cellIndex)));
        }
      });

    this->DataFile->Stream >> tag >> dataType >> std::ws;
    internal::parseAssert(tag == "CONNECTIVITY");
    auto conn =
      this->DoReadArrayVariant(vtkm::cont::Field::Association::Any, dataType, connSize, 1);
    vtkm::cont::ArrayCopyShallowIfPossible(conn, connectivity);
  }
}

void VTKDataSetReaderBase::ReadShapes(vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes)
{
  std::string tag;
  vtkm::Id numCells;
  this->DataFile->Stream >> tag >> numCells >> std::ws;
  internal::parseAssert(tag == "CELL_TYPES");

  shapes.Allocate(numCells);
  std::vector<vtkm::Int32> buffer(static_cast<std::size_t>(numCells));
  this->ReadArray(buffer);

  vtkm::Int32* buffp = buffer.data();
  auto shapesPortal = shapes.WritePortal();
  for (vtkm::Id i = 0; i < numCells; ++i)
  {
    shapesPortal.Set(i, static_cast<vtkm::UInt8>(*buffp++));
  }
}

void VTKDataSetReaderBase::ReadAttributes()
{
  if (this->DataFile->Stream.eof())
  {
    return;
  }

  vtkm::cont::Field::Association association = vtkm::cont::Field::Association::Any;
  std::size_t size;

  std::string tag;
  this->DataFile->Stream >> tag;
  while (!this->DataFile->Stream.eof())
  {
    if (tag == "POINT_DATA")
    {
      association = vtkm::cont::Field::Association::Points;
    }
    else if (tag == "CELL_DATA")
    {
      association = vtkm::cont::Field::Association::Cells;
    }
    else if (tag == "FIELD") // can see field in this position also
    {
      this->ReadGlobalFields(nullptr);
      this->DataFile->Stream >> tag;
      continue;
    }
    else
    {
      internal::parseAssert(false);
    }

    this->DataFile->Stream >> size;
    while (!this->DataFile->Stream.eof())
    {
      this->DataFile->Stream >> tag;
      if (tag == "SCALARS")
      {
        this->ReadScalars(association, size);
      }
      else if (tag == "COLOR_SCALARS")
      {
        this->ReadColorScalars(association, size);
      }
      else if (tag == "LOOKUP_TABLE")
      {
        this->ReadLookupTable();
      }
      else if (tag == "VECTORS" || tag == "NORMALS")
      {
        this->ReadVectors(association, size);
      }
      else if (tag == "TEXTURE_COORDINATES")
      {
        this->ReadTextureCoordinates(association, size);
      }
      else if (tag == "TENSORS")
      {
        this->ReadTensors(association, size);
      }
      else if (tag == "FIELD")
      {
        this->ReadFields(association, size);
      }
      else if (tag == "GLOBAL_IDS" || tag == "PEDIGREE_IDS")
      {
        this->ReadGlobalOrPedigreeIds(association, size);
      }
      else
      {
        break;
      }
    }
  }
}

void VTKDataSetReaderBase::CloseFile()
{
  this->DataFile->Stream.close();
}

void VTKDataSetReaderBase::OpenFile()
{
  this->DataFile->Stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  try
  {
    this->DataFile->Stream.open(this->DataFile->FileName.c_str(),
                                std::ios_base::in | std::ios_base::binary);
  }
  catch (std::ifstream::failure&)
  {
    std::string message("could not open file \"" + this->DataFile->FileName + "\"");
    throw vtkm::io::ErrorIO(message);
  }
}

void VTKDataSetReaderBase::ReadHeader()
{
  char vstring[] = "# vtk DataFile Version";
  const std::size_t vlen = sizeof(vstring);

  // Read version line
  char vbuf[vlen];
  this->DataFile->Stream.read(vbuf, vlen - 1);
  vbuf[vlen - 1] = '\0';
  if (std::string(vbuf) != std::string(vstring))
  {
    throw vtkm::io::ErrorIO("Incorrect file format.");
  }

  char dot;
  this->DataFile->Stream >> this->DataFile->Version[0] >> dot >> this->DataFile->Version[1];
  // skip rest of the line
  std::string skip;
  std::getline(this->DataFile->Stream, skip);

  if ((this->DataFile->Version[0] > 4) ||
      (this->DataFile->Version[0] == 4 && this->DataFile->Version[1] > 2))
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
               "Reader may not correctly read >v4.2 files. Reading version "
                 << this->DataFile->Version[0] << "." << this->DataFile->Version[1] << ".\n");
  }

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
    throw vtkm::io::ErrorIO("Unsupported DataSet type.");
  }
}


void VTKDataSetReaderBase::AddField(const std::string& name,
                                    vtkm::cont::Field::Association association,
                                    vtkm::cont::UnknownArrayHandle& data)
{
  if (data.GetNumberOfValues() > 0)
  {
    switch (association)
    {
      case vtkm::cont::Field::Association::Points:
      case vtkm::cont::Field::Association::WholeMesh:
        this->DataSet.AddField(vtkm::cont::Field(name, association, data));
        break;
      case vtkm::cont::Field::Association::Cells:
        this->DataSet.AddField(vtkm::cont::Field(name, association, data));
        break;
      default:
        VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
                   "Not recording field '" << name << "' because it has an unknown association");
        break;
    }
  }
}

void VTKDataSetReaderBase::ReadScalars(vtkm::cont::Field::Association association,
                                       std::size_t numElements)
{
  std::string dataName, dataType, lookupTableName;
  vtkm::IdComponent numComponents = 1;
  this->DataFile->Stream >> dataName >> dataType;
  std::string tag;
  this->DataFile->Stream >> tag;
  if (tag != "LOOKUP_TABLE")
  {
    try
    {
      numComponents = std::stoi(tag);
    }
    catch (std::invalid_argument&)
    {
      internal::parseAssert(false);
    }
    this->DataFile->Stream >> tag;
  }

  internal::parseAssert(tag == "LOOKUP_TABLE");
  this->DataFile->Stream >> lookupTableName >> std::ws;

  vtkm::cont::UnknownArrayHandle data =
    this->DoReadArrayVariant(association, dataType, numElements, numComponents);
  this->AddField(dataName, association, data);
}

void VTKDataSetReaderBase::ReadColorScalars(vtkm::cont::Field::Association association,
                                            std::size_t numElements)
{
  VTKM_LOG_S(vtkm::cont::LogLevel::Warn, "Support for COLOR_SCALARS is not implemented. Skipping.");

  std::string dataName;
  vtkm::IdComponent numComponents;
  this->DataFile->Stream >> dataName >> numComponents >> std::ws;
  std::string dataType = this->DataFile->IsBinary ? "unsigned_char" : "float";
  vtkm::cont::UnknownArrayHandle data =
    this->DoReadArrayVariant(association, dataType, numElements, numComponents);
  this->AddField(dataName, association, data);
}

void VTKDataSetReaderBase::ReadLookupTable()
{
  VTKM_LOG_S(vtkm::cont::LogLevel::Warn, "Support for LOOKUP_TABLE is not implemented. Skipping.");

  std::string dataName;
  std::size_t numEntries;
  this->DataFile->Stream >> dataName >> numEntries >> std::ws;
  this->SkipArray(numEntries, vtkm::Vec<vtkm::io::internal::ColorChannel8, 4>());
}

void VTKDataSetReaderBase::ReadTextureCoordinates(vtkm::cont::Field::Association association,
                                                  std::size_t numElements)
{
  std::string dataName;
  vtkm::IdComponent numComponents;
  std::string dataType;
  this->DataFile->Stream >> dataName >> numComponents >> dataType >> std::ws;

  vtkm::cont::UnknownArrayHandle data =
    this->DoReadArrayVariant(association, dataType, numElements, numComponents);
  this->AddField(dataName, association, data);
}

void VTKDataSetReaderBase::ReadVectors(vtkm::cont::Field::Association association,
                                       std::size_t numElements)
{
  std::string dataName;
  std::string dataType;
  this->DataFile->Stream >> dataName >> dataType >> std::ws;

  vtkm::cont::UnknownArrayHandle data =
    this->DoReadArrayVariant(association, dataType, numElements, 3);
  this->AddField(dataName, association, data);
}

void VTKDataSetReaderBase::ReadTensors(vtkm::cont::Field::Association association,
                                       std::size_t numElements)
{
  std::string dataName;
  std::string dataType;
  this->DataFile->Stream >> dataName >> dataType >> std::ws;

  vtkm::cont::UnknownArrayHandle data =
    this->DoReadArrayVariant(association, dataType, numElements, 9);
  this->AddField(dataName, association, data);
}

void VTKDataSetReaderBase::ReadFields(vtkm::cont::Field::Association association,
                                      std::size_t expectedNumElements)
{
  std::string dataName;
  vtkm::Id numArrays;
  this->DataFile->Stream >> dataName >> numArrays >> std::ws;
  for (vtkm::Id i = 0; i < numArrays; ++i)
  {
    std::size_t numTuples;
    vtkm::IdComponent numComponents;
    std::string arrayName, dataType;
    this->DataFile->Stream >> arrayName >> numComponents >> numTuples >> dataType >> std::ws;
    if (numTuples == expectedNumElements)
    {
      vtkm::cont::UnknownArrayHandle data =
        this->DoReadArrayVariant(association, dataType, numTuples, numComponents);
      this->AddField(arrayName, association, data);
    }
    else
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
                 "Field " << arrayName
                          << "'s size does not match expected number of elements. Skipping");
    }
  }
}

void VTKDataSetReaderBase::ReadGlobalFields(std::vector<vtkm::Float32>* visitBounds)
{
  std::string dataName;
  vtkm::Id numArrays;
  this->DataFile->Stream >> dataName >> numArrays >> std::ws;
  for (vtkm::Id i = 0; i < numArrays; ++i)
  {
    std::size_t numTuples;
    vtkm::IdComponent numComponents;
    std::string arrayName, dataType;
    this->DataFile->Stream >> arrayName >> numComponents >> numTuples >> dataType >> std::ws;
    if (arrayName == "avtOriginalBounds" && visitBounds)
    {
      visitBounds->resize(6);
      internal::parseAssert(numComponents == 1 && numTuples == 6);
      // parse the bounds and fill the bounds vector
      this->ReadArray(*visitBounds);
    }
    else
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Info,
                 "Support for global field " << arrayName << " not implemented. Skipping.");
      this->DoSkipArrayVariant(dataType, numTuples, numComponents);
    }
  }
}

void VTKDataSetReaderBase::ReadGlobalOrPedigreeIds(vtkm::cont::Field::Association association,
                                                   std::size_t numElements)
{
  std::string dataName;
  std::string dataType;
  this->DataFile->Stream >> dataName >> dataType >> std::ws;
  internal::parseAssert(dataType == "vtkIdType");

  std::vector<vtkm::Int32> buffer(numElements); // vtk writes vtkIdType as int
  this->ReadArray(buffer);
  vtkm::cont::UnknownArrayHandle data(vtkm::cont::make_ArrayHandleMove(std::move(buffer)));
  this->AddField(dataName, association, data);

  this->SkipArrayMetaData(1);
}

class VTKDataSetReaderBase::SkipArrayVariant
{
public:
  SkipArrayVariant(VTKDataSetReaderBase* reader, std::size_t numElements)
    : Reader(reader)
    , NumElements(numElements)
  {
  }

  template <typename T>
  void operator()(T) const
  {
    this->Reader->SkipArray(this->NumElements, T());
  }

  template <typename T>
  void operator()(vtkm::IdComponent numComponents, T) const
  {
    this->Reader->SkipArray(this->NumElements * static_cast<std::size_t>(numComponents), T());
  }

protected:
  VTKDataSetReaderBase* Reader;
  std::size_t NumElements;
};

class VTKDataSetReaderBase::ReadArrayVariant : public SkipArrayVariant
{
public:
  ReadArrayVariant(VTKDataSetReaderBase* reader,
                   vtkm::cont::Field::Association association,
                   std::size_t numElements,
                   vtkm::cont::UnknownArrayHandle& data)
    : SkipArrayVariant(reader, numElements)
    , Association(association)
    , Data(&data)
  {
  }

  template <typename T>
  void operator()(T) const
  {
    std::vector<T> buffer(this->NumElements);
    this->Reader->ReadArray(buffer);
    if ((this->Association != vtkm::cont::Field::Association::Cells) ||
        (this->Reader->GetCellsPermutation().GetNumberOfValues() < 1))
    {
      *this->Data = CreateUnknownArrayHandle(buffer);
    }
    else
    {
      // If we are reading data associated with a cell set, we need to (sometimes) permute the
      // data due to differences between VTK and VTK-m cell shapes.
      auto permutation = this->Reader->GetCellsPermutation().ReadPortal();
      vtkm::Id outSize = permutation.GetNumberOfValues();
      std::vector<T> permutedBuffer(static_cast<std::size_t>(outSize));
      for (vtkm::Id outIndex = 0; outIndex < outSize; outIndex++)
      {
        std::size_t inIndex = static_cast<std::size_t>(permutation.Get(outIndex));
        permutedBuffer[static_cast<std::size_t>(outIndex)] = buffer[inIndex];
      }
      *this->Data = CreateUnknownArrayHandle(permutedBuffer);
    }
  }

  template <typename T>
  void operator()(vtkm::IdComponent numComponents, T) const
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
               "Support for " << numComponents << " components not implemented. Skipping.");
    SkipArrayVariant::operator()(numComponents, T());
  }

private:
  vtkm::cont::Field::Association Association;
  vtkm::cont::UnknownArrayHandle* Data;
};

void VTKDataSetReaderBase::DoSkipArrayVariant(std::string dataType,
                                              std::size_t numElements,
                                              vtkm::IdComponent numComponents)
{
  // string requires some special handling
  if (dataType == "string" || dataType == "utf8_string")
  {
    const vtkm::Id stringCount = numComponents * static_cast<vtkm::Id>(numElements);
    this->SkipStringArray(stringCount);
  }
  else
  {
    vtkm::io::internal::DataType typeId = vtkm::io::internal::DataTypeId(dataType);
    vtkm::io::internal::SelectTypeAndCall(
      typeId, numComponents, SkipArrayVariant(this, numElements));
  }
}

vtkm::cont::UnknownArrayHandle VTKDataSetReaderBase::DoReadArrayVariant(
  vtkm::cont::Field::Association association,
  std::string dataType,
  std::size_t numElements,
  vtkm::IdComponent numComponents)
{
  // Create empty data to start so that the return can check if data were actually read
  vtkm::cont::ArrayHandle<vtkm::Float32> empty;
  vtkm::cont::UnknownArrayHandle data(empty);

  // string requires some special handling
  if (dataType == "string" || dataType == "utf8_string")
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
               "Support for data type 'string' and 'utf8_string' is not implemented. Skipping.");
    const vtkm::Id stringCount = numComponents * static_cast<vtkm::Id>(numElements);
    this->SkipStringArray(stringCount);
  }
  else
  {
    vtkm::io::internal::DataType typeId = vtkm::io::internal::DataTypeId(dataType);
    vtkm::io::internal::SelectTypeAndCall(
      typeId, numComponents, ReadArrayVariant(this, association, numElements, data));
  }

  return data;
}

void VTKDataSetReaderBase::ReadArray(std::vector<vtkm::io::internal::DummyBitType>& buffer)
{
  VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
             "Support for data type 'bit' is not implemented. Skipping.");
  this->SkipArray(buffer.size(), vtkm::io::internal::DummyBitType());
  buffer.clear();
}

void VTKDataSetReaderBase::SkipArray(std::size_t numElements,
                                     vtkm::io::internal::DummyBitType,
                                     vtkm::IdComponent numComponents)
{
  if (this->DataFile->IsBinary)
  {
    numElements = (numElements + 7) / 8;
    this->DataFile->Stream.seekg(static_cast<std::streamoff>(numElements), std::ios_base::cur);
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
  this->SkipArrayMetaData(numComponents);
}

void VTKDataSetReaderBase::SkipStringArray(std::size_t numStrings)
{
  if (this->DataFile->IsBinary)
  {
    for (std::size_t i = 0; i < numStrings; ++i)
    {
      auto firstByte = this->DataFile->Stream.peek();
      auto type = firstByte >> 6;
      switch (type)
      {
        case 3: // length stored in 1 byte
        {
          auto length = this->DataFile->Stream.get();
          length &= 0x3F;
          this->DataFile->Stream.seekg(static_cast<std::streamoff>(length), std::ios_base::cur);
          break;
        }
        case 2: // length stored in 2 bytes
        {
          vtkm::UInt16 length = 0;
          auto bytes = reinterpret_cast<char*>(&length);
          this->DataFile->Stream.read(bytes, 2);
          std::swap(bytes[0], bytes[1]);
          length &= 0x3FFF;
          this->DataFile->Stream.seekg(static_cast<std::streamoff>(length), std::ios_base::cur);
          break;
        }
        case 1: // length stored in 4 bytes
        {
          vtkm::UInt32 length = 0;
          auto bytes = reinterpret_cast<char*>(&length);
          this->DataFile->Stream.read(bytes, 4);
          std::reverse(bytes, bytes + 4);
          length &= 0x3FFFFFFF;
          this->DataFile->Stream.seekg(static_cast<std::streamoff>(length), std::ios_base::cur);
          break;
        }
        default: // length stored in 8 bytes
        {
          vtkm::UInt64 length = 0;
          auto bytes = reinterpret_cast<char*>(&length);
          this->DataFile->Stream.read(bytes, 8);
          std::reverse(bytes, bytes + 8);
          this->DataFile->Stream.seekg(static_cast<std::streamoff>(length), std::ios_base::cur);
          break;
        }
      }
    }
  }
  else
  {
    for (std::size_t i = 0; i < numStrings; ++i)
    {
      // ASCII mode stores one string per line
      this->DataFile->Stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
  }
}

void VTKDataSetReaderBase::SkipArrayMetaData(vtkm::IdComponent numComponents)
{
  if (!this->DataFile->Stream.good())
  {
    return;
  }

  auto begining = this->DataFile->Stream.tellg();

  std::string tag;
  this->DataFile->Stream >> tag;
  if (tag != "METADATA")
  {
    this->DataFile->Stream.seekg(begining);
    return;
  }

  VTKM_LOG_S(vtkm::cont::LogLevel::Warn, "METADATA is not supported. Attempting to Skip.");

  this->DataFile->Stream >> tag >> std::ws;
  if (tag == "COMPONENT_NAMES")
  {
    std::string name;
    for (vtkm::IdComponent i = 0; i < numComponents; ++i)
    {
      this->DataFile->Stream >> name >> std::ws;
    }
  }
  else if (tag == "INFORMATION")
  {
    int numKeys = 0;
    this->DataFile->Stream >> numKeys >> std::ws;

    // Skipping INFORMATION is tricky. The reader needs to be aware of the types of the
    // information, which is not provided in the file.
    // Here we will just skip until an empty line is found.
    // However, if there are no keys, then there is nothing to read (and the stream tends
    // to skip over empty lines.
    if (numKeys > 0)
    {
      std::string line;
      do
      {
        std::getline(this->DataFile->Stream, line);
      } while (this->DataFile->Stream.good() && !line.empty());

      // Eat any remaining whitespace after the INFORMATION to be ready to read the next token
      this->DataFile->Stream >> std::ws;
    }
  }
  else
  {
    internal::parseAssert(false);
  }
}
}
} // namespace vtkm::io
