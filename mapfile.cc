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

#include "mapfile.hh"

// Std Library
#include <unordered_map>
#include <string>
#include <sstream>

// Other C++ Libraries
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <gsl.h>

// C Libraries
#include <mapserver/mapserver.h>
#include <glob.h>

using namespace gsl;

#ifndef msProjLibInitFromEnv
void msProjLibInitFromEnv()
{
  const char *val;

  if( (val=getenv( "PROJ_LIB" )) != NULL ) {
    msSetPROJ_LIB(val, NULL);
  }
}
#endif

// Not in Boost < 1.48, have 1.46
#ifdef OLD_BOOST
namespace boost {
  namespace filesystem {
    boost::filesystem::path canonical(
				      const boost::filesystem::path& p,
				      const boost::filesystem::path& base = boost::filesystem::current_path())
    {
      boost::filesystem::path abs_p = boost::filesystem::absolute(p,base);
      boost::filesystem::path result;
      for(boost::filesystem::path::iterator it=abs_p.begin();
	  it!=abs_p.end();
	  ++it)
	{
	  if(*it == "..")
	    {
	      // /a/b/.. is not necessarily /a if b is a symbolic link
	      if(boost::filesystem::is_symlink(result) )
                result /= *it;
	      // /a/b/../.. is not /a/b/.. under most circumstances
	      // We can end up with ..s in our result because of symbolic links
	      else if(result.filename() == "..")
                result /= *it;
	      // Otherwise it should be safe to resolve the parent
	      else
                result = result.parent_path();
	    }
	  else if(*it == ".")
	    {
	      // Ignore
	    }
	  else
	    {
	      // Just cat other path entries
	      result /= *it;
	    }
	}
      return result;
    }
  }
}
#endif

Mapfile::Mapfile(const boost::filesystem::path &mapfile) : needs_manual_review(false), m_debug(false)
{

  // Read and parse Mapfile
  map = msLoadMap((char*)mapfile.c_str(), NULL);
  if(!map) {
    msWriteError(stderr);
    throw std::runtime_error("Failed to open mapfile.");
  }

  msApplyDefaultSubstitutions(map);

  add_file(mapfile, "mapfile");
}

Mapfile::~Mapfile()
{
  if(map != NULL)
    msFreeMap(map);	
}


void Mapfile::add_file(const boost::filesystem::path &p, const std::string &type = "file")
{
  // Workaround MapServer bug of not keeping full path for FONTSET
  // In the default mapfile even thought it does for everything else.
  if (std::getenv("MS_DEFAULT_MAPFILE") != NULL) {
    auto default_path = boost::filesystem::path(std::getenv("MS_DEFAULT_MAPFILE")).parent_path() / p;
    if (boost::filesystem::exists(default_path)) {
      auto canonical_path = boost::filesystem::canonical(default_path);

      if(m_debug)
	printf("\tFound %s: %s\n", type.c_str(), canonical_path.c_str());

      required_files[canonical_path.string()]++;
      return;
    }
  }

  if (boost::filesystem::exists(p)) {
    auto canonical_path = boost::filesystem::canonical(p);

    if(m_debug)
      printf("\tFound %s: %s\n", type.c_str(), canonical_path.c_str());
    required_files[canonical_path.string()]++;
  } else { 
    auto mappath = boost::filesystem::path(map->mappath);
    if(p.is_relative())
      mappath /= p;
    else
      mappath = p;

    add_warning(type + std::string(": ") + mappath.string() + std::string(" not found!"));
  } 
}

void Mapfile::add_font(const std::string &font)
{
  if(m_debug)
    printf("\tReference font %s\n", font.c_str());

  const char* font_file = msLookupHashTable( &(map->fontset.fonts) , font.c_str() );
  if (font_file != NULL) {
    add_file(boost::filesystem::path(font_file), "font");
  } else {
    add_warning(std::string("FONT ") + font + std::string(" not found in fontset"));
  }
}

void Mapfile::add_symbol(const std::string &symbol_name)
{
  if(m_debug)
    printf("\tReference symbol %s\n", symbol_name.c_str());

  bool found = false;
  for (int i = 0; i < map->symbolset.numsymbols; i++) {
    auto s = map->symbolset.symbol[i];
    if (s != NULL && s->name && std::string(s->name) == symbol_name) {
      found = true;
      if(s->type == MS_SYMBOL_TRUETYPE && s->font) {
	add_font(s->font);
      }
      if(s->type == MS_SYMBOL_PIXMAP && s->full_pixmap_path) {
	add_file(boost::filesystem::path(s->full_pixmap_path), "image");
      }
    }
  }
  if (!found) {
    add_warning(std::string("Symbol ") + symbol_name + std::string(" NOT FOUND"));
  }
}

void Mapfile::add_warning(const std::string &warning)
{
  needs_manual_review = true;
  warnings.push_back(warning);
}

int Mapfile::process_mapfile()
{
  auto cwd = boost::filesystem::current_path();
  auto _ = finally([&cwd]{ boost::filesystem::current_path(cwd); });

  // Pull information out of mapfile...

  // MAPPATH
  if(map->mappath) {
    if(m_debug)
      printf("\tFound mappath: %s\n", map->mappath);
    boost::filesystem::current_path(map->mappath);
  }

  // SHAPEPATH
  if(map->shapepath) {
    if(m_debug)
      printf("\tFound shapepath: %s\n", map->shapepath);
  }

  // Check output formats, DRIVER="TEMPLATE"
  for(int i = 0; i < map->numoutputformats; i++) {
    auto of = map->outputformatlist[i];
    auto of_driver = std::string(of->driver);
    if(of_driver == std::string("TEMPLATE")) {
      for(int j=0; j < of->numformatoptions; j++) {
	auto fo = std::string(of->formatoptions[j]);
	if (boost::algorithm::starts_with(fo, "FILE=")) {
	  std::ostringstream s;
	  s << "OUTPUTFORMAT: " << of->name << " " << of->driver << " " << fo;
	  add_warning(s.str());
	}
      }
    }
  }

  // FONTSET
  if(map->fontset.filename) {
    if(m_debug)
      printf("\tFound FONTSET %s with %d fonts.\n",
	     map->fontset.filename, map->fontset.numfonts);
    add_file(map->fontset.filename, "fontset");
  }

  // SYMBOLSET
  if(map->symbolset.filename) {
    if(m_debug)
      printf("\tFound SYMBOLSET %s with %d symbols\n",
	     map->symbolset.filename, map->symbolset.numsymbols);
    add_file(map->symbolset.filename, "symbolset");
  }

  // WEB
  {
    if(map->web.header)
      add_file(map->web.header, "web.header");
    if(map->web.footer)
      add_file(map->web.footer, "web.footer");
    if(map->web.empty)
      add_file(map->web.empty, "web.empty");
    if(map->web.error)
      add_file(map->web.error, "web.error");

    if(map->web._template)
      add_file(map->web._template, "web.template");
    if(map->web.mintemplate)
      add_file(map->web.mintemplate, "web.mintemplate");
    if(map->web.maxtemplate)
      add_file(map->web.maxtemplate, "web.maxtemplate");
  }

  // scalebar->label, legend->label

  // For each layer
  for(int l = 0; l < map->numlayers; l++) {
    layerObj *layer = GET_LAYER(map, l);
    //msLayerOpen(layer);

    if(layer->name) {
      if(m_debug)
	printf("\tLayer %s\n", layer->name);

      for (int c = 0; c < layer->numclasses; c++) {
	if (layer->_class[c]) {
	  auto klass = layer->_class[c];
	  if (layer->_class[c]->_template)
	    add_file(klass->_template, "layer.class.template");
	
	  for (int s = 0; s < klass->numstyles; s++) {
	    auto style = klass->styles[s];
	    if (style->symbolname)
	      add_symbol(style->symbolname);
	  }

	  for (int l = 0; l < klass->numlabels; l++) {
	    auto label = klass->labels[l];
	    if (label->font)
	      add_font(label->font);
	  }
	
	}
      }

      switch (layer->connectiontype) {
      case MS_INLINE: // 0, no dependencies
	break;
      case MS_SHAPEFILE: // 1, look for glob mappath/shapepath/data.*
	// TILEINDEX, DATA
	if (layer->tileindex) {
	  add_warning("TILEINDEX needs manual review");
	  auto basepath = boost::filesystem::path("");
	  if (map->shapepath) 
	    basepath /= boost::filesystem::path(map->shapepath);
	  basepath /= boost::filesystem::path(layer->tileindex);
	  basepath.replace_extension(".*");

	  glob_t g;
	  if (0 == glob(basepath.string().c_str(), 0, NULL, &g)) {
	    for(int ig = 0; ig < g.gl_pathc; ig++) {
	      add_file( boost::filesystem::path(g.gl_pathv[ig]), "layer.tileindex" );
	    }
	  }
	  /*
	  {
	    rectObj searchrect = { 0, 0, 100, 100 };
	    int is_query = 0;
	    int tilelayerindex;
	    int tileitemindex;
	    int tilesrsindex;
	    layerObj *tlp;
	    auto err = msDrawRasterTileLayer(map, layer, searchrect, is_query, &tilelayerindex, &tileitemindex, &tilesrsindex, &tlp);

	    msDrawRasterCleanupTileLayer(tlp, tilelayerindex);

	    } */
	} else if (layer->data) {
	  auto basepath = boost::filesystem::path("");
	  if (map->shapepath) 
	    basepath /= boost::filesystem::path(map->shapepath);
	  basepath /= boost::filesystem::path(layer->data);
	  basepath.replace_extension(".*");

	  glob_t g;
	  if (0 == glob(basepath.string().c_str(), 0, NULL, &g)) {
	    for(int ig = 0; ig < g.gl_pathc; ig++) {
	      add_file( boost::filesystem::path(g.gl_pathv[ig]), "layer.data" );
	    }
	  }
	}	
	break;
      case MS_TILED_SHAPEFILE: // 2
	add_warning("CONNECTIONTYPE TILED_SHAPEFILE requires manual review");
	break;
      case MS_OGR: // 4
	// CONNECTION, DATA
	add_warning("CONNECTIONTYPE OGR requires manual review");
	break;
      case MS_POSTGIS: // 6
	// CONNECTION, DATA
	add_warning("CONNECTIONTYPE POSTGIS requires manual review");
	break;
      case MS_WMS: // 7 
	// CONNECTION
	add_warning("CONNECTIONTYPE WMS requires manual review");
	break;
      case MS_ORACLESPATIAL: // 8
	// CONNECTION, DATA
	add_warning("CONNECTIONTYPE ORACLESPATIAL requires manual review");
	break;
      case MS_WFS: // 9
	// CONNECTION
	add_warning("CONNECTIONTYPE WFS requires manual review");
	break;
      case MS_GRATICULE: // 10
	add_warning("CONNECTIONTYPE GRATICULE requires manual review");
	break;
      case MS_MYSQL: // 11
	// CONNECTION
	add_warning("CONNECTIONTYPE MYSQL requires manual review");
	break;
      case MS_RASTER: // 12 
	add_warning("CONNECTIONTYPE RASTER requires manual review");
	break;
      case MS_PLUGIN: // 13
	add_warning("CONNECTIONTYPE PLUGIN requires manual review");
	break;
      case MS_UNION: // 14
	add_warning("CONNECTIONTYPE UNION requires manual review");
	break;
      case MS_UVRASTER: // 15
	add_warning("CONNECTIONTYPE UVRASTER requires manual review");
	break;
      case MS_CONTOUR: // 16
	add_warning("CONNECTIONTYPE CONTOUR requires manual review");
	break;
      default:
	std::ostringstream s;
	s << "CONNECTIONTYPE unknown (" << layer->connectiontype << "), requires manual review.";
	add_warning(s.str());
      }

      // This group needs to check file or URL
      if (layer->footer)
	add_file(layer->footer, "layer.footer");	
      if (layer->header)
	add_file(layer->header, "layer.header");
      if (layer->_template)
	if (strncmp("dummy", layer->_template, 6) != 0)
	  add_file(layer->_template, "layer.template");

      if (layer->styleitem) {
	auto si = std::string(layer->styleitem);
	if (boost::algorithm::starts_with(si, std::string("javascript://"))) {
	  add_warning(std::string("Found javascript STYLEITEM ") + std::string(layer->styleitem));
	}
      }

      if (layer->tileindex) {
	// Need to read tileindex!
	add_warning(std::string("Found TILEINDEX ") + std::string(layer->tileindex) + std::string(" needs manual review")); 
      }
      if (layer->plugin_library) {
	add_warning(std::string("Found Plugin Library ") + std::string(layer->plugin_library) + std::string(" needs manual review"));
      }

    }
  }

  boost::filesystem::current_path(cwd);
  return MS_SUCCESS;
}


void Mapfile::print()
{
  printf("Required Files:\n");
  printf("\tRefs:\tFilename:\n");
  for(auto it = required_files.cbegin(); it != required_files.cend(); ++it) {
    printf("\t%d\t%s\n", it->second, (it->first).c_str());
  }

  if(needs_manual_review)
    printf("\tWARNING: Requires Manual Review\n");
  for (auto i = warnings.begin(); i != warnings.end(); ++i) {
    printf("\tWARNING: %s\n", i->c_str());
  }
}


void Mapfile::debug(bool b)
{
  m_debug = b;
}


MapServer::MapServer()
{
  if( msSetup() != MS_SUCCESS ) {
    char nl[2] = "\n";
    throw std::runtime_error(msGetErrorString(nl));
  }

  /* Use PROJ_LIB env vars if set */
  msProjLibInitFromEnv();
  //msSetPROJ_LIB("/srv/www/conf/proj", NULL);

  /* Use MS_ERRORFILE and MS_DEBUGLEVEL env vars if set */
  if ( msDebugInitFromEnv() != MS_SUCCESS ) {
    char nl[] = "\n";
    std::string err(msGetErrorString(nl));
    msCleanup();
    throw std::runtime_error(err);
  }
}


MapServer::~MapServer() {
  msCleanup();
}


void MapServer::proj_lib(const std::string &path)
{
  if (!path.empty()) {
    msSetPROJ_LIB(path.c_str(), NULL);
  }
}


std::string MapServer::version()
{
	return std::string(msGetVersion());
}
