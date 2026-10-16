#pragma once
// Compat: error.hpp incluye <FSTREAM.H> (iostream clasico de MSVC) bajo clang.
// Lo redirigimos al <fstream> real y sacamos ostream al namespace global.
#include <fstream>
using std::ostream;
using std::flush;
using std::endl;