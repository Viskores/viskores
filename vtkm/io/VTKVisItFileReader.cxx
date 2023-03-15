//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <algorithm>
#include <fstream>
#include <vtkm/cont/Logging.h>
#include <vtkm/io/ErrorIO.h>
#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/io/VTKVisItFileReader.h>

namespace vtkm
{
namespace io
{

VTKVisItFileReader::VTKVisItFileReader(const char* fileName)
  : FileName(fileName)
{
}

VTKVisItFileReader::VTKVisItFileReader(const std::string& fileName)
  : FileName(fileName)
{
}

vtkm::cont::PartitionedDataSet VTKVisItFileReader::ReadPartitionedDataSet()
{
  //Get the base path of the input file.
  std::string baseDirPath = ".";

  //Get the base dir name
  auto pos = this->FileName.rfind("/");
  if (pos != std::string::npos)
    baseDirPath = this->FileName.substr(0, pos);

  //Open up the file of filenames.
  std::ifstream stream(this->FileName);
  if (stream.fail())
    throw vtkm::io::ErrorIO("Failed to open file: " + this->FileName);

  int numBlocks = -1;
  std::string line;
  std::vector<std::string> fileNames;
  while (stream.good())
  {
    std::getline(stream, line);
    if (line.size() == 0 || line[0] == '#')
      continue;
    else if (line.rfind("!NBLOCKS", 0) != std::string::npos)
    {
      //!NBLOCKS is already set!!
      if (numBlocks > 0)
        throw vtkm::io::ErrorIO("Invalid file: " + this->FileName +
                                ". `!NBLOCKS` specified more than once.");

      numBlocks = std::atoi(line.substr(8, line.size()).c_str());
      if (numBlocks <= 0)
        throw vtkm::io::ErrorIO("Invalid file: " + this->FileName +
                                ". Number of blocks (!NBLOCKS) must be > 0.");
    }
    else if (numBlocks > 0)
    {
      char char_to_remove = ' ';
      line.erase(std::remove(line.begin(), line.end(), char_to_remove), line.end());
      if (line.find(".vtk") != std::string::npos)
      {
        fileNames.push_back(baseDirPath + "/" + line);
      }
      else
      {
        VTKM_LOG_S(vtkm::cont::LogLevel::Info,
                   "Skipping: " << line << ". It does not appear to be a legacy VTK file.");
        continue;
      }
    }
    else
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Info,
                 "Skipping line that occurs before `!NBLOCKS`: " << line);
      continue;
    }
  }

  if (numBlocks < 0)
  {
    throw vtkm::io::ErrorIO("`!NBLOCKS` line not provided in VisIt file: " + this->FileName);
  }

  if (static_cast<std::size_t>(numBlocks) != fileNames.size())
    throw vtkm::io::ErrorIO("Wrong number of partitions in VisIt file: " + this->FileName);

  vtkm::cont::PartitionedDataSet pds;

  //Read all the files.
  for (auto&& fn : fileNames)
  {
    vtkm::io::VTKDataSetReader reader(fn);
    pds.AppendPartition(reader.ReadDataSet());
  }

  return pds;
}

}
} //vtkm::io
