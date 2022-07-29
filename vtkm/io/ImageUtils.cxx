//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ErrorExecution.h>

#include <vtkm/io/FileUtils.h>
#include <vtkm/io/ImageReaderBase.h>
#include <vtkm/io/ImageReaderPNG.h>
#include <vtkm/io/ImageReaderPNM.h>
#include <vtkm/io/ImageUtils.h>
#include <vtkm/io/ImageWriterBase.h>
#include <vtkm/io/ImageWriterPNG.h>
#include <vtkm/io/ImageWriterPNM.h>

#include <vtkm/cont/ErrorBadValue.h>

#include <memory>

namespace vtkm
{
namespace io
{

void WriteImageFile(const vtkm::cont::DataSet& dataSet,
                    const std::string& fullPath,
                    const std::string& fieldName)
{
  std::unique_ptr<vtkm::io::ImageWriterBase> writer;
  if (EndsWith(fullPath, ".ppm"))
  {
    writer = std::unique_ptr<vtkm::io::ImageWriterPNM>(new ImageWriterPNM(fullPath));
  }
  else
  {
    writer = std::unique_ptr<vtkm::io::ImageWriterPNG>(new ImageWriterPNG(fullPath));
  }
  writer->WriteDataSet(dataSet, fieldName);
  VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Wrote image data at: " << fullPath);
}

vtkm::cont::DataSet ReadImageFile(const std::string& fullPath, const std::string& fieldName)
{
  std::ifstream check(fullPath.c_str());
  if (!check.good())
  {
    throw vtkm::cont::ErrorBadValue("File does not exist: " + fullPath);
  }

  std::unique_ptr<vtkm::io::ImageReaderBase> reader;
  if (EndsWith(fullPath, ".png"))
  {
    reader = std::unique_ptr<vtkm::io::ImageReaderPNG>(new ImageReaderPNG(fullPath));
  }
  else if (EndsWith(fullPath, ".ppm") || EndsWith(fullPath, ".pnm"))
  {
    reader = std::unique_ptr<vtkm::io::ImageReaderPNM>(new ImageReaderPNM(fullPath));
  }
  else
  {
    throw vtkm::cont::ErrorBadValue("Unsupported file type: " + fullPath);
  }
  reader->SetPointFieldName(fieldName);
  return reader->ReadDataSet();
}

} // namespace vtkm::io
} // namespace vtkm:
