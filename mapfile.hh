/*
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

#ifndef _MAPFILE_HH_
#define _MAPFILE_HH_

#include <string>
#include <unordered_map>

#include <boost/filesystem.hpp>

#include <mapserver/mapserver.h>

class Mapfile {
  bool m_debug;

  bool needs_manual_review;
  std::vector<std::string> warnings;
  std::unordered_map<std::string, int> required_files;

  mapObj *map;

  void add_file(const boost::filesystem::path &p, const std::string &type);
  void add_font(const std::string &font);
  void add_symbol(const std::string &symbol_name);
  void add_warning(const std::string &warning);

public:
  Mapfile(const boost::filesystem::path &mapfile);
  ~Mapfile();

  int process_mapfile();

  void print();
  void debug(bool b);

};


class MapServer {
public:
  MapServer();
  ~MapServer();

  void proj_lib(const std::string &path);
  std::string version();
};

#endif
