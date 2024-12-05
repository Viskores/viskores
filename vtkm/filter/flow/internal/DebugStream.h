#pragma once


class DebugStreamType
{
public:
  DebugStreamType(int rank)
    : Stream("Debug." + std::to_string(rank) + ".txt")
  {
  }

  template <typename T>
  DebugStreamType& operator<<(const T& value)
  {
    std::lock_guard<std::mutex> lock(this->Mutex);
    this->Stream << value;
    return *this;
  }

  DebugStreamType& operator<<(std::ostream& (*manip)(std::ostream&))
  {
    std::lock_guard<std::mutex> lock(this->Mutex);
    this->Stream << manip;
    return *this;
  }

private:
  std::ofstream Stream;
  std::mutex Mutex;
};
