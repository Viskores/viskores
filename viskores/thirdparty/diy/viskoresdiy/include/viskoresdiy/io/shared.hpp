#ifndef VISKORESDIY_IO_SHARED_HPP
#define VISKORESDIY_IO_SHARED_HPP

#include <sstream>
#include <fstream>
#include "../mpi.hpp"

namespace diy
{
namespace io
{

class SharedOutFile: public std::ostringstream
{
    public:
                SharedOutFile(std::string filename, diy::mpi::communicator world, int root = 0):
                    filename_(filename),
                    world_(world),
                    root_(root)                     {}

                ~SharedOutFile()                    { close(); }

        void    close()
        {
            auto str = this->str();
            std::vector<char> contents(str.begin(), str.end());
            if (world_.rank() == root_)
            {
                std::vector<std::vector<char>> all_contents;
                diy::mpi::gather(world_, contents, all_contents, root_);

                // write the file serially
                std::ofstream fout(filename_);
                for (auto& cntnts : all_contents)
                    fout.write(cntnts.data(), cntnts.size());
            } else
                diy::mpi::gather(world_, contents, root_);
        }

    private:
        std::string             filename_;
        diy::mpi::communicator  world_;
        int                     root_;
};

}
}

#endif
