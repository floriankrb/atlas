/*
 * (C) Copyright 1996-2016 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include <limits>
#include <cassert>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <memory>

#include "atlas/atlas.h"
#include "atlas/functionspace/NodeColumns.h"
#include "atlas/grid/grids.h"
#include "atlas/internals/AtlasTool.h"
#include "atlas/mesh/actions/BuildDualMesh.h"
#include "atlas/mesh/actions/BuildEdges.h"
#include "atlas/mesh/actions/BuildHalo.h"
#include "atlas/mesh/actions/BuildParallelFields.h"
#include "atlas/mesh/actions/BuildPeriodicBoundaries.h"
#include "atlas/mesh/actions/BuildStatistics.h"
#include "atlas/mesh/actions/BuildXYZField.h"
#include "atlas/mesh/generators/MeshGenerator.h"
#include "atlas/mesh/Mesh.h"
#include "atlas/mesh/Nodes.h"
#include "atlas/parallel/mpi/mpi.h"
#include "atlas/util/Config.h"
#include "atlas/util/io/Gmsh.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/filesystem/PathName.h"
#include "eckit/geometry/Point3.h"
#include "eckit/mpi/ParallelContextBehavior.h"
#include "eckit/option/CmdArgs.h"
#include "eckit/option/Separator.h"
#include "eckit/option/SimpleOption.h"
#include "eckit/parser/Tokenizer.h"
#include "eckit/runtime/Context.h"
#include "eckit/runtime/Tool.h"


//------------------------------------------------------------------------------

using namespace eckit;
using namespace atlas;
using namespace atlas::mesh::actions;
using namespace atlas::grid;
using namespace atlas::functionspace;
using namespace atlas::mesh;
using namespace eckit::option;

//------------------------------------------------------------------------------

void usage(const std::string& tool)
{
  Log::info() << "usage: " << tool << " (--grid.name=name|--grid.json=path) [OPTION]... OUTPUT [--help]" << std::endl;
}

class Meshgen2Gmsh : public AtlasTool {

  virtual void execute(const Args& args);
  virtual int numberOfPositionalArguments() { return 1; }
  virtual int minimimumPositionalArguments() { return 0; }
  virtual std::string briefDescription() {
    return "Mesh generator for Structured compatible meshes";
  }
  virtual std::string usage() {
    return name() + " (--grid.name=name|--grid.json=path) [OPTION]... OUTPUT [--help]";
  }

public:

  Meshgen2Gmsh(int argc,char **argv): AtlasTool(argc,argv)
  {
    add_option( new SimpleOption<std::string>("grid.name","Grid unique identifier\n"
      +indent()+"     Example values: N80, F40, O24, L32") );
    add_option( new SimpleOption<PathName>("grid.json","Grid described by json file") );
    add_option( new SimpleOption<double>("angle","Maximum element-edge slant deviation from meridian in degrees. \n"
      +indent()+"     Value range between 0 and 30\n"
      +indent()+"         0: Mostly triangular, with only perfect quads\n"
      +indent()+"        30: Mostly skewed quads with only triags when skewness becomes too large\n"
      +indent()+"        -1: Only triangles") );

    add_option( new SimpleOption<bool>("3d","Output mesh as sphere, and generate mesh connecting East and West in case serial") );
    add_option( new SimpleOption<bool>("include_pole","Include pole point") );
    add_option( new SimpleOption<bool>("patch_pole","Patch poles with elements.") );
    add_option( new SimpleOption<bool>("ghost","Output ghost elements") );
    add_option( new Separator("Advanced") );
    add_option( new SimpleOption<long>("halo","Halo size") );
    add_option( new SimpleOption<bool>("edges","Build edge datastructure") );
    add_option( new SimpleOption<bool>("brick","Build brick dual mesh") );
    add_option( new SimpleOption<bool>("stats","Write statistics file") );
    add_option( new SimpleOption<bool>("info","Write Info") );

    std::string help_str =
        "NAME\n"
        "       atlas-meshgen - Mesh generator for Structured compatible meshes\n"
        "\n"
        "SYNOPSIS\n"
        "       atlas-meshgen (--grid.name=name|--grid.json=path) [OPTION]... OUTPUT [--help] \n"
        "\n"
        "DESCRIPTION\n"
        "\n"
//          +options_str.str()+
        "\n"
        "AUTHOR\n"
        "       Written by Willem Deconinck.\n"
        "\n"
        "ECMWF                        November 2014"
        ;

//    bool help=false;
//    args.get("help",help);
//    if( help )
//    {
//      if( eckit::mpi::rank() == 0 )
//        Log::info() << help_str << std::endl;
//      do_run = false;
//      return;
//    }

//    atlas_init(argc,argv);
  }

private:

  std::string key;
  long halo;
  bool edges;
  bool brick;
  bool stats;
  bool info;
  int surfdim;
  bool with_pole;
  bool stitch_pole;
  bool ghost;
  std::string identifier;
  std::vector<long> reg_nlon_nlat;
  std::vector<long> fgg_nlon_nlat;
  std::vector<long> rgg_nlon;
  PathName path_in;
  PathName path_out;

  eckit::LocalConfiguration meshgenerator_config;

};

//------------------------------------------------------------------------------

void Meshgen2Gmsh::execute(const Args& args)
{
  key = "";
  args.get("grid.name",key);

  edges = false;
  args.get("edges",edges);
  stats = false;
  args.get("stats",stats);
  info = false;
  args.get("info",info);
  halo       = 0;
  args.get("halo",halo);
  bool dim_3d=false;
  args.get("3d",dim_3d);
  surfdim    = dim_3d ? 3 : 2;
  brick = false;
  args.get("brick",brick);
  ghost = false;
  args.get("ghost",ghost);

  std::string path_in_str = "";
  if( args.get("grid.json",path_in_str) ) path_in = path_in_str;

  if( args.count() )
    path_out = args(0);
  else
    path_out = "mesh.msh";

  if( path_in_str.empty() && key.empty() ) {
    Log::warning() << "missing argument --grid.name or --grid.json" << std::endl;
    Log::warning() << "Usage: " << usage() << std::endl;
    return;
  }


  if( edges )
    halo = std::max(halo,1l);

  meshgenerator_config = args.get();
  if( eckit::mpi::size() > 1 )
    meshgenerator_config.set("3d",false);


  grid::load();

  SharedPtr<global::Structured> grid;
  if( key.size() )
  {
    try{ grid.reset( global::Structured::create(key) ); }
    catch( eckit::BadParameter& e ){}
  }
  else if( path_in.path().size() )
  {
    Log::info() << "Creating grid from file " << path_in << std::endl;
    try{ grid.reset( global::Structured::create( atlas::util::Config(path_in) ) ); }
    catch( eckit::BadParameter& e ){}
  }
  else
  {
    Log::error() << "No grid specified." << std::endl;
  }

  if( !grid ) return;
  SharedPtr<mesh::generators::MeshGenerator> meshgenerator (
      mesh::generators::MeshGenerator::create("Structured",meshgenerator_config) );


  SharedPtr<mesh::Mesh> mesh;
  try {
  mesh.reset( meshgenerator->generate(*grid) );
  }
  catch ( eckit::BadParameter& e)
  {
    Log::error() << e.what() << std::endl;
    Log::error() << e.callStack() << std::endl;
    throw e;
  }
  SharedPtr<functionspace::NodeColumns> nodes_fs( new functionspace::NodeColumns(*mesh,Halo(halo)) );
  // nodes_fs->checksum(mesh->nodes().lonlat());
  // Log::info() << "  checksum lonlat : " << nodes_fs->checksum(mesh->nodes().lonlat()) << std::endl;

  if( edges )
  {
    build_edges(*mesh);
    build_pole_edges(*mesh);
    build_edges_parallel_fields(*mesh);
    if( brick )
      build_brick_dual_mesh(*mesh);
    else
      build_median_dual_mesh(*mesh);
  }

  if( stats )
    build_statistics(*mesh);

  atlas::util::io::Gmsh gmsh;
  gmsh.options.set("info",info);
  gmsh.options.set("ghost",ghost);
  if( surfdim == 3 )
  {
    mesh::actions::BuildXYZField("xyz")(*mesh);
    gmsh.options.set("nodes",std::string("xyz"));
  }
  Log::info() << "Writing mesh to gmsh file \"" << path_out << "\" generated from grid \"" << grid->shortName() << "\"" << std::endl;
  gmsh.write( *mesh, path_out );
  atlas_finalize();
}

//------------------------------------------------------------------------------

int main( int argc, char **argv )
{
  Meshgen2Gmsh tool(argc,argv);
  return tool.start();
}
