//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtkm_io_VTKVisItFileReader_h
#define vtkm_io_VTKVisItFileReader_h

#include <string>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/io/vtkm_io_export.h>

namespace vtkm
{
namespace io
{

/// Reader for ".visit" files, a simple file format for partioned data sets.
/// The file format consists of the keyword "!NBLOCKS <N>", where N is the number of
/// partitions, followed by a list of the N files. For example:
///
/// ```
/// # This is a comment
/// !NBLOCKS 2
/// file1.vtk
/// file2.vtk
/// ```
///
/// Note: .visit files support time varying partitioned data, but it is not supported
/// in this reader.
///

class VTKM_IO_EXPORT VTKVisItFileReader
{
public:
  VTKM_CONT VTKVisItFileReader(const char* fileName);
  VTKM_CONT VTKVisItFileReader(const std::string& fileName);

  VTKM_CONT vtkm::cont::PartitionedDataSet ReadPartitionedDataSet();

private:
  std::string FileName;
};

}
} //vtkm::io

#endif //vtkm_io_VTKVisItFileReader_h
