#ifndef DIY_IO_UTILS_HPP
#define DIY_IO_UTILS_HPP

#if defined(_WIN32)
#include <direct.h>
#include <io.h>
#else
#include <unistd.h>     // mkstemp() on Mac
#include <dirent.h>
#endif

#include <cstdio>       // remove()
#include <cstdlib>      // mkstemp() on Linux
#include <sys/stat.h>

namespace diy
{
namespace io
{
namespace utils
{
  /**
   * returns true if the filename exists and refers to a directory.
   */
  inline bool is_directory(const std::string& filename)
  {
#if defined(_WIN32)
    DWORD attr = GetFileAttributes(filename.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES) && ((attr & FILE_ATTRIBUTE_DIRECTORY) != 0);
#else
    struct stat s;
    return (stat(filename.c_str(), &s) == 0 && S_ISDIR(s.st_mode));
#endif
  }

  /**
   * creates a new directory. returns true on success.
   */
  inline bool make_directory(const std::string& filename)
  {
#if defined(_WIN32)
    return _mkdir(filename.c_str()) == 0;
#else
    return mkdir(filename.c_str(), 0755) == 0;
#endif
  }

  /**
   * truncates a file to the given length, if the file exists and can be opened
   * for writing.
   */
  inline void truncate(const std::string& filename, size_t length)
  {
#if defined(_WIN32)
    int fd = -1;
    _sopen_s(&fd, filename.c_str(), _O_WRONLY | _O_BINARY, _SH_DENYNO, _S_IWRITE);
    if (fd != -1)
    {
      _chsize_s(fd, 0);
      _close(fd);
    }
#else
    ::truncate(filename.c_str(), length);
#endif
  }

  inline int mkstemp(std::string& filename)
  {
    const size_t slen = filename.size();
    char *s_template = new char[filename.size() + 1];
    std::copy_n(filename.c_str(), slen+1, s_template);

    int handle = -1;
#if defined(_WIN32)
    if (_mktemp_s(s_template, slen+1) == 0) // <- returns 0 on success.
      _sopen_s(&handle, s_template, _O_WRONLY | _O_CREAT | _O_BINARY, _SH_DENYNO, _S_IWRITE);
#elif defined(__MACH__)
    // TODO: figure out how to open with O_SYNC
    handle = ::mkstemp(s_template);
#else
    handle = mkostemp(s_template, O_WRONLY | O_SYNC);
#endif
    if (handle != -1)
      filename = s_template;
    return handle;
  }

  inline void close(int fd)
  {
#if defined(_WIN32)
    _close(fd);
#else
    fsync(fd);
    close(fd);
#endif
  }

  inline void sync(int fd)
  {
#if !defined(_WIN32)
    fsync(fd);
#endif
  }

  inline bool remove(const std::string& filename)
  {
    return ::remove(filename.c_str()) == 0;
  }
}
}
}

#endif
