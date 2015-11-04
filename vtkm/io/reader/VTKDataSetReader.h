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
#ifndef vtk_m_io_reader_VTKDataSetReader_h
#define vtk_m_io_reader_VTKDataSetReader_h

#include <vtkm/io/reader/VTKDataSetReaderBase.h>
#include <vtkm/io/reader/VTKStructuredPointsReader.h>
#include <vtkm/io/reader/VTKStructuredGridReader.h>
#include <vtkm/io/reader/VTKPolyDataReader.h>
#include <vtkm/io/reader/VTKUnstructuredGridReader.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/smart_ptr/scoped_ptr.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm {
namespace io {
namespace reader {

class VTKDataSetReader : public VTKDataSetReaderBase
{
public:
  VTKDataSetReader(const char *fileName)
    : VTKDataSetReaderBase(fileName)
  { }

  virtual void PrintSummary(std::ostream &out) const
  {
    if (this->Reader)
    {
      this->Reader->PrintSummary(out);
    }
    else
    {
      VTKDataSetReaderBase::PrintSummary(out);
    }
  }

private:
  virtual void CloseFile()
  {
    if (this->Reader)
    {
      this->Reader->CloseFile();
    }
    else
    {
      VTKDataSetReaderBase::CloseFile();
    }
  }

  virtual void Read()
  {
    switch (this->DataFile->Structure)
    {
    case internal::DATASET_STRUCTURED_POINTS:
      this->Reader.reset(new VTKStructuredPointsReader(""));
      break;
    case internal::DATASET_STRUCTURED_GRID:
      this->Reader.reset(new VTKStructuredGridReader(""));
      break;
    case internal::DATASET_POLYDATA:
      this->Reader.reset(new VTKPolyDataReader(""));
      break;
    case internal::DATASET_UNSTRUCTURED_GRID:
      this->Reader.reset(new VTKUnstructuredGridReader(""));
      break;
    default:
      throw vtkm::io::ErrorIO("Unsupported DataSet type.");
    }

    this->TransferDataFile(*this->Reader.get());
    this->Reader->Read();
    this->DataSet = this->Reader->GetDataSet();
  }

  boost::scoped_ptr<VTKDataSetReaderBase> Reader;
};

}
}
} // vtkm::io::reader

#endif // vtk_m_io_reader_VTKReader_h
