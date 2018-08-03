Replace std::random_shuffle with std::shuffle

std::random_shuffle is deprecated in C++14 because it's using std::rand
which uses a non uniform distribution and the underlying algorithm is
unspecified. Using std::shuffle can provide a reliable result in a 64 bit version.
