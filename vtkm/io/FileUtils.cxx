//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/io/FileUtils.h>

#include <vtkm/cont/ErrorBadValue.h>

#include <algorithm>
// TODO (nadavi): Once we get c++17 installed uncomment this
// #include <filesystem>
#include <errno.h>
#include <sys/stat.h>

#ifdef _MSC_VER
#include <direct.h>
#endif


namespace vtkm
{
namespace io
{

bool EndsWith(const std::string& value, const std::string& ending)
{
  if (ending.size() > value.size())
  {
    return false;
  }
  return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

std::string Filename(const std::string& filePath)
{
  // TODO (nadavi): Once we get c++17 installed uncomment this
  // std::filesystem::path path(filePath);
  // return path.filename();

#ifdef _MSC_VER
  auto lastSlashPos = filePath.rfind(GetWindowsPathSeperator(filePath));
#else
  auto lastSlashPos = filePath.rfind('/');
#endif
  if (lastSlashPos != std::string::npos)
  {
    return filePath.substr(lastSlashPos + 1);
  }
  return filePath;
}

std::string ParentPath(const std::string& filePath)
{
  // TODO (nadavi): Once we get c++17 installed uncomment this
  // std::filesystem::path path(filePath);
  // return path.parent_path();

#ifdef _MSC_VER
  auto lastSlashPos = filePath.rfind(GetWindowsPathSeperator(filePath));
#else
  auto lastSlashPos = filePath.rfind('/');
#endif
  if (lastSlashPos != std::string::npos)
  {
    return filePath.substr(0, lastSlashPos);
  }
  return "";
}

bool CreateDirectoriesFromFilePath(const std::string& filePath)
{
  // TODO (nadavi): Once we get c++17 installed uncomment this
  // auto dir = ParentPath(filePath);
  // if (dir.empty())
  // {
  //   return false;
  // }
  // return std::filesystem::create_directories(dir);

  auto dir = ParentPath(filePath);
  if (dir.empty())
  {
    return false;
  }

#ifdef _MSC_VER
  auto ret = _mkdir(dir.c_str());
#else
  mode_t mode = 0755;
  auto ret = mkdir(dir.c_str(), mode);
#endif

  if (ret == 0)
  {
    return true;
  }

  switch (errno)
  {
    case ENOENT:
    {
      if (!CreateDirectoriesFromFilePath(dir))
      {
        return false;
      }
      return CreateDirectoriesFromFilePath(filePath);
    }
    case EEXIST:
      return false;
    default:
      return false;
  }
}

std::string MergePaths(const std::string& filePathPrefix, const std::string& filePathSuffix)
{
  // TODO (nadavi): Once we get c++17 installed uncomment this
  // NOTE: This function may not directly mimic the c++ filesystem '/' behavior
  //       the edge case tests will probably need to be updated when switching.
  // std::filesystem::path prefix(filePathPrefix);
  // std::filesystem::path suffix(filePathSuffix);
  // std::filesystem::path fullPath = prefix / suffix;
  // return fullPath.string();

  auto prefix = filePathPrefix;
  auto suffix = filePathSuffix;
  char prefixPathSeperator = '/';
  char suffixPathSeperator = '/';

  if (prefix.empty() && suffix.empty())
  {
    return "";
  }
  else if (prefix.empty())
  {
    return suffix;
  }
  else if (suffix.empty())
  {
    return prefix;
  }

#ifdef _MSC_VER
  prefixPathSeperator = GetWindowsPathSeperator(prefix);
  suffixPathSeperator = suffix[0] == '/' || suffix[0] == '\\' ? suffix[0] : prefixPathSeperator;
#endif

  if (prefix[prefix.length() - 1] == prefixPathSeperator)
  {
    prefix = prefix.substr(0, prefix.length() - 1);
  }
  if (suffix[0] == suffixPathSeperator)
  {
    suffix = suffix.substr(1, suffix.length());
  }
  return prefix + prefixPathSeperator + suffix;
}

std::string PrefixStringToFilename(const std::string& filePath, const std::string& prefix)
{
  auto parentPath = ParentPath(filePath);
  auto filename = Filename(filePath);
  filename = prefix + filename;
  return MergePaths(parentPath, filename);
}

char GetWindowsPathSeperator(const std::string& filePath)
{
  auto slashType = filePath.rfind('/');
  if (slashType == std::string::npos)
  {
    return '\\';
  }
  return '/';
}

} // namespace vtkm::io
} // namespace vtkm
