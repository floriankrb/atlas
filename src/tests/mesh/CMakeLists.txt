# (C) Copyright 2013 ECMWF.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation nor
# does it submit to any jurisdiction.

if( HAVE_FCTEST )

  add_fctest( TARGET atlas_fctest_mesh
    LINKER_LANGUAGE Fortran
    CONDITION       NOT atlas_fctest_mesh_DISABLED
    SOURCES         fctest_mesh.F90
    LIBS            atlas_f
  )

  add_fctest( TARGET atlas_fctest_meshgen
    LINKER_LANGUAGE Fortran
    CONDITION       ON
    SOURCES         fctest_meshgen.F90
    LIBS            atlas_f
  )

  add_fctest( TARGET atlas_fctest_connectivity
    LINKER_LANGUAGE Fortran
    SOURCES         fctest_connectivity.F90
    LIBS            atlas_f
  )

  add_fctest( TARGET atlas_fctest_elements
    LINKER_LANGUAGE Fortran
    SOURCES         fctest_elements.F90
    LIBS            atlas_f
  )

endif()

ecbuild_add_test( TARGET atlas_test_parfields
  MPI        2
  CONDITION  ECKIT_HAVE_MPI AND TRANSI_HAVE_MPI
  SOURCES    test_parfields.cc
  LIBS       atlas
)

ecbuild_add_test( TARGET atlas_test_halo
  MPI        5
  CONDITION  ECKIT_HAVE_MPI AND TRANSI_HAVE_MPI
  SOURCES    test_halo.cc ../TestMeshes.h
  LIBS       atlas
)

ecbuild_add_test( TARGET atlas_test_distmesh
  MPI        5
  CONDITION  ECKIT_HAVE_MPI AND TRANSI_HAVE_MPI
  SOURCES    test_distmesh.cc ../TestMeshes.h
  LIBS       atlas
)

ecbuild_add_test(
  TARGET      atlas_test_cgal_mesh_gen_from_points
  SOURCES     test_cgal_mesh_gen_from_points.cc
  CONDITION   ATLAS_HAVE_TESSELATION
  LIBS        atlas
)

ecbuild_add_test(
  TARGET atlas_test_accumulate_facets
  SOURCES test_accumulate_facets.cc
  LIBS atlas
)

foreach( test connectivity elements ll meshgen3d rgg )
  ecbuild_add_test( TARGET atlas_test_${test}
    SOURCES test_${test}.cc
    LIBS atlas
  )
endforeach()

atlas_add_cuda_test(
  TARGET atlas_test_connectivity_kernel
  SOURCES test_connectivity_kernel.cu
  LIBS atlas
)
