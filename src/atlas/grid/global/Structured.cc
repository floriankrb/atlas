/*
 * (C) Copyright 1996-2016 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include <typeinfo>
#include <string>
#include "eckit/memory/Builder.h"
#include "eckit/memory/Factory.h"
#include "atlas/grid/global/Structured.h"

using eckit::Factory;
using eckit::MD5;
using eckit::BadParameter;

namespace atlas {
namespace grid {
namespace global {

//------------------------------------------------------------------------------

// register_BuilderT1(Grid, ReducedGrid, ReducedGrid::grid_type_str());

Structured* Structured::create(const eckit::Parametrisation& p) {

  Structured* grid = dynamic_cast<Structured*>(Grid::create(p));
  if (!grid) throw BadParameter("Grid is not a reduced grid", Here());
  return grid;

}

Structured* Structured::create(const std::string& uid)
{
  Structured* grid = dynamic_cast<Structured*>( Grid::create(uid) );
  if( !grid )
    throw BadParameter("Grid "+uid+" is not a reduced grid",Here());
  return grid;
}

//ReducedGrid* ReducedGrid::create(const eckit::Properties& g)
//{
//  ReducedGrid* grid = dynamic_cast<ReducedGrid*>( Grid::create(g) );
//  if( !grid )
//    throw BadParameter("Grid is not a reduced grid",Here());
//  return grid;
//}

std::string Structured::className() { return "atlas.ReducedGrid"; }

Structured::Structured(const Domain& d) : Grid(d), N_(0)
{
}

Structured::Structured(const eckit::Parametrisation& params) : N_(0)
{
  setup(params);

  if( ! params.get("grid_type",grid_type_) ) throw BadParameter("grid_type missing in Params",Here());
  if( ! params.get("shortName",shortName_) ) throw BadParameter("shortName missing in Params",Here());
  //if( ! params.has("hash") ) throw BadParameter("hash missing in Params",Here());
}

void Structured::setup(const eckit::Parametrisation& params)
{
  eckit::ValueList list;

  std::vector<long> npts_per_lat;
  std::vector<double> latitudes;

  if( ! params.get("npts_per_lat",npts_per_lat) ) throw BadParameter("npts_per_lat missing in Params",Here());
  if( ! params.get("latitudes",latitudes) ) throw BadParameter("latitudes missing in Params",Here());

  params.get("N",N_);

  setup(latitudes.size(),latitudes.data(),npts_per_lat.data());
}

Structured::Structured(size_t nlat, const double lats[], const long nlons[], const Domain& d)
  : Grid(d)
{
  setup(nlat,lats,nlons);
}

void Structured::setup( const size_t nlat, const double lats[], const long nlons[], const double lonmin[], const double lonmax[] )
{
  ASSERT(nlat > 1);  // can't have a grid with just one latitude

  nlons_.assign(nlons,nlons+nlat);

  lat_.assign(lats,lats+nlat);

  lonmin_.assign(lonmin,lonmin+nlat);
  lonmax_.assign(lonmax,lonmax+nlat);

  lon_inc_.resize(nlat);
  
  npts_ = 0;
  nlonmax_ = 0;
  double lon_min(1000), lon_max(-1000);

  for(size_t jlat = 0; jlat < nlat; ++jlat)
  {
    //ASSERT( nlon(jlat) > 1 ); // can't have grid with just one longitude
    nlonmax_ = std::max(nlon(jlat),nlonmax_);

    lon_min = std::min(lon_min,lonmin_[jlat]);
    lon_max = std::max(lon_max,lonmax_[jlat]);
    lon_inc_[jlat] = (lonmax_[jlat] - lonmin_[jlat])/(nlons_[jlat]-1);

    npts_ += nlons_[jlat];
  }

  bounding_box_ = BoundBox(lat_[0]/*north*/, lat_[nlat-1]/*south*/, lon_max/*east*/, lon_min/*west*/ );
}


void Structured::setup( const size_t nlat, const double lats[], const long nlons[] )
{
  std::vector<double> lonmin(nlat,0.);
  std::vector<double> lonmax(nlat);
  for(size_t jlat = 0; jlat < nlat; ++jlat)
  {
    if( nlons[jlat] )
      lonmax[jlat] = 360.-360./static_cast<double>(nlons[jlat]);
    else
      lonmax[jlat] = 0.;
  }
  setup(nlat,lats,nlons,lonmin.data(),lonmax.data());
}

void Structured::setup_lat_hemisphere(const size_t N, const double lat[], const long lon[])
{
  std::vector<long> nlons(2*N);
  std::copy( lon, lon+N, nlons.begin() );
  std::reverse_copy( lon, lon+N, nlons.begin()+N );
  std::vector<double> lats(2*N);
  std::copy( lat, lat+N, lats.begin() );
  std::reverse_copy( lat, lat+N, lats.begin()+N );
  for(size_t j = N; j < 2*N; ++j)
    lats[j] *= -1.;
  setup(2*N,lats.data(),nlons.data());
}

size_t Structured::N() const
{
  return N_;
}

BoundBox Structured::boundingBox() const
{
  return bounding_box_;
}

size_t Structured::npts() const { return npts_; }

void Structured::lonlat( std::vector<Point>& pts ) const
{
  pts.resize(npts());
  int c(0);
  for(size_t jlat = 0; jlat < nlat(); ++jlat)
  {
    double y = lat(jlat);
    for(size_t jlon = 0; jlon < nlon(jlat); ++jlon)
    {
      pts[c++].assign(lon(jlat,jlon),y);
    }
  }
}

std::string Structured::gridType() const
{
  return grid_type_;
}

// eckit::Properties ReducedGrid::spec() const
// {
//   eckit::Properties grid_spec;
//
//   grid_spec.set("grid_type",gridType());
//
//   grid_spec.set("nlat",nlat());
//
//   grid_spec.set("latitudes",eckit::makeVectorValue(latitudes()));
//   grid_spec.set("npts_per_lat",eckit::makeVectorValue(npts_per_lat()));
//
//   BoundBox bbox = boundingBox();
//   grid_spec.set("bbox_s", bbox.min().lat());
//   grid_spec.set("bbox_w", bbox.min().lon());
//   grid_spec.set("bbox_n", bbox.max().lat());
//   grid_spec.set("bbox_e", bbox.max().lon());
//
//   if( N_ != 0 )
//     grid_spec.set("N", N_ );
//
//   return grid_spec;
// }


const std::vector<int>&  Structured::npts_per_lat() const
{
  if(nlons_int_.size() == 0) {
    nlons_int_.assign(nlons_.begin(), nlons_.end());
  }
  return nlons_int_;
}

std::string Structured::getOptimalMeshGenerator() const
{
    return "ReducedGrid";
}

size_t Structured::copyLonLatMemory(double* pts, size_t size) const
{
    size_t sizePts = 2*npts();

    ASSERT(size >= sizePts);

    for(size_t c = 0, jlat=0; jlat<nlat(); ++jlat )
    {
      double y = lat(jlat);
      for( size_t jlon=0; jlon<nlon(jlat); ++jlon )
      {
        pts[c++] = lon(jlat,jlon);
        pts[c++] = y;
      }
    }
    return sizePts;
}

void Structured::print(std::ostream& os) const
{
    os << "ReducedGrid(Name:" << shortName() << ")";
}

const std::vector<double>& Structured::latitudes() const
{
  return lat_;
}

std::string Structured::shortName() const {
  ASSERT(!shortName_.empty());
  return shortName_;
}

void Structured::hash(eckit::MD5& md5) const {
  // Through inheritance the grid_type_str() might differ while still being same grid
      //md5.add(grid_type_str());

  md5.add(latitudes().data(),    sizeof(double)*latitudes().size());
  md5.add(npts_per_lat().data(), sizeof(int)*npts_per_lat().size());
  bounding_box_.hash(md5);
}

//------------------------------------------------------------------------------

extern "C" {

int atlas__ReducedGrid__nlat(Structured* This)
{
  return This->nlat();
}


int atlas__ReducedGrid__nlon(Structured* This, int &jlat)
{
  return This->nlon(jlat);
}

void atlas__ReducedGrid__nlon__all(Structured* This, const int* &nlons, int &size)
{
  nlons = This->npts_per_lat().data();
  size  = This->npts_per_lat().size();
}

int atlas__ReducedGrid__nlonmax(Structured* This)
{
  return This->nlonmax();
}

int atlas__ReducedGrid__npts(Structured* This)
{
  return This->npts();
}

double atlas__ReducedGrid__lat(Structured* This,int jlat)
{
  return This->lat(jlat);
}

double atlas__ReducedGrid__lon(Structured* This,int jlat,int jlon)
{
  return This->lon(jlat, jlon);
}

void atlas__ReducedGrid__lonlat(Structured* This, int jlat, int jlon, double crd[])
{
  This->lonlat(jlat, jlon, crd);
}

void atlas__ReducedGrid__lat__all(Structured* This, const double* &lat, int &size)
{
  lat  = This->latitudes().data();
  size = This->latitudes().size();
}


Structured* atlas__new_reduced_grid(char* identifier)
{
  return Structured::create( std::string(identifier) );
}

void atlas__ReducedGrid__delete(Structured* This)
{
  delete This;
}

}

} // namespace global
} // namespace grid
} // namespace atlas
