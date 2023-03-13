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
  baseDirPath = "../vtk-m/data/data/uniform";

  //Get the base dir name
  auto pos = this->FileName.rfind("/");
  if (pos != std::string::npos)
    baseDirPath = baseDirPath.substr(0, pos);

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
    else if (line.find("!NBLOCKS") != std::string::npos)
    {
      if (numBlocks > 0)
        throw vtkm::io::ErrorIO("Invalid file: " + this->FileName +
                                ". Number of blocks already specified");
      numBlocks = std::atoi(line.substr(8, line.size()).c_str());

      if (numBlocks <= 0)
        throw vtkm::io::ErrorIO("Invalid file: " + this->FileName +
                                ". Number of blocks must be > 0");
    }
    else if (numBlocks > 0)
    {
      char char_to_remove = ' ';
      line.erase(std::remove(line.begin(), line.end(), char_to_remove), line.end());
      if (line.find(".vtk") != std::string::npos)
        fileNames.push_back(baseDirPath + "/" + line);
      else
        std::cerr << "Skipping: " << line << std::endl;
    }
    else
    {
      std::cerr << "Skipping line: " << line << std::endl;
    }
  }

  vtkm::cont::PartitionedDataSet pds;

  //Read all the files.
  for (const auto fn : fileNames)
  {
    std::ifstream s(fn);
    if (s.fail())
      throw vtkm::io::ErrorIO("Failed to open file: " + fn);

    vtkm::io::VTKDataSetReader reader(fn);
    pds.AppendPartition(reader.ReadDataSet());
  }

  return pds;
}

}
} //vtkm::io
