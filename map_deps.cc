/* Program to find all the dependencies in a MapServer mapfile
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016-2017 James Klassen
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

// Std Library
#include <iostream>

// Other C++ Libraries
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <gsl.h>

#include "mapfile.hh"

int main(int argc, char *argv[])
{
  if(!argv[1]) {
    std::cerr << "Usage: " << argv[0] << " <path to mapfile>" << std::endl;
    exit(1);
  }

  auto mapfile = boost::filesystem::path(argv[1]);
  if (boost::filesystem::exists(mapfile)) {

    auto ms = MapServer();  // Initialize and cleanup MapServer library
    std::cout << "Found Mapserver:" << std::endl << ms.version() << std::endl;

    std::cout << "Trying mapfile " << mapfile << std::endl;

    try {	
      Mapfile m(mapfile);
      m.process_mapfile();
      m.print();
    } catch (const std::runtime_error &e) {
      std::cout << "Error processing mapfile: " << mapfile << ":" << e.what() << std::endl;
    }
  } else {
    std::cerr << "Mapfile: " << mapfile << " doesn't exist!" << std::endl;
  }
  return 0;
}
