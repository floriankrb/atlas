#include "atlas/atlas_f.h"

module atlas_HybridElements_module

use fckit_owned_object_module, only: fckit_owned_object
use atlas_Connectivity_module, only: atlas_MultiBlockConnectivity
use atlas_Field_module, only: atlas_Field
use atlas_ElementType_module, only: atlas_ElementType
use atlas_Elements_module, only: atlas_Elements

implicit none

private :: fckit_owned_object
private :: atlas_MultiBlockConnectivity
private :: atlas_Field
private :: atlas_ElementType
private :: atlas_Elements

public :: atlas_HybridElements

private

!-----------------------------
! atlas_HybridElements       !
!-----------------------------

type, extends(fckit_owned_object) :: atlas_HybridElements
contains

! Public methods
  procedure, public :: size     => atlas_HybridElements__size

  procedure, public ::  node_connectivity
  procedure, public ::  edge_connectivity
  procedure, public ::  cell_connectivity

  generic, public :: add => add_elements, add_elements_with_nodes, add_field

  generic, public :: field => field_by_idx_size_t, field_by_idx_int, field_by_name

  generic, public :: elements => elements_int, elements_size_t

  procedure, public :: nb_fields
  procedure, public :: has_field
  procedure, public :: global_index
  procedure, public :: remote_index
  procedure, public :: partition
  procedure, public :: halo

  procedure, public :: nb_types

! Private methods
  procedure, private :: add_elements
  procedure, private :: add_elements_with_nodes
  procedure, private :: add_field
  procedure, private :: field_by_idx_int
  procedure, private :: field_by_idx_size_t
  procedure, private :: field_by_name

  procedure, private :: elements_int
  procedure, private :: elements_size_t

#if FCKIT_FINAL_NOT_INHERITING
  final :: atlas_HybridElements__final_auto
#endif

end type

interface atlas_HybridElements
  module procedure atlas_HybridElements__cptr
  module procedure atlas_HybridElements__constructor
end interface

!========================================================
contains
!========================================================

function atlas_HybridElements__cptr(cptr) result(this)
  use, intrinsic :: iso_c_binding
  use atlas_hybridelements_c_binding
  type(atlas_HybridElements) :: this
  type(c_ptr), intent(in) :: cptr
  call this%reset_c_ptr( cptr )
  call this%return()
end function

function atlas_HybridElements__constructor() result(this)
  use atlas_hybridelements_c_binding
  type(atlas_HybridElements) :: this
  call this%reset_c_ptr( atlas__mesh__HybridElements__create() )
  call this%return()
end function

function atlas_HybridElements__size(this) result(val)
  use, intrinsic :: iso_c_binding
  use atlas_hybridelements_c_binding
  integer(c_size_t) :: val
  class(atlas_HybridElements), intent(in) :: this
  val = atlas__mesh__HybridElements__size(this%c_ptr())
end function

function node_connectivity(this) result(connectivity)
  use atlas_hybridelements_c_binding
  class(atlas_HybridElements), intent(in) :: this
  type(atlas_MultiBlockConnectivity) :: connectivity
  connectivity = atlas_MultiBlockConnectivity( &
      atlas__mesh__HybridElements__node_connectivity(this%c_ptr()) )
  call connectivity%return()
end function

function edge_connectivity(this) result(connectivity)
  use atlas_hybridelements_c_binding
  class(atlas_HybridElements), intent(in) :: this
  type(atlas_MultiBlockConnectivity) :: connectivity
  connectivity = atlas_MultiBlockConnectivity( &
      atlas__mesh__HybridElements__edge_connectivity(this%c_ptr()) )
  call connectivity%return()
end function

function cell_connectivity(this) result(connectivity)
  use atlas_hybridelements_c_binding
  class(atlas_HybridElements), intent(in) :: this
  type(atlas_MultiBlockConnectivity) :: connectivity
  connectivity = atlas_MultiBlockConnectivity( &
      atlas__mesh__HybridElements__cell_connectivity(this%c_ptr()) )
  call connectivity%return()
end function

subroutine add_elements(this,elementtype,nb_elements)
  use, intrinsic :: iso_c_binding
  use atlas_hybridelements_c_binding
  class(atlas_HybridElements), intent(inout) :: this
  type(atlas_ElementType) :: elementtype
  integer(c_size_t) :: nb_elements
  call atlas__mesh__HybridElements__add_elements(this%c_ptr(),elementtype%c_ptr(),nb_elements)
end subroutine

subroutine add_elements_with_nodes(this,elementtype,nb_elements,node_connectivity)
  use, intrinsic :: iso_c_binding
  use atlas_hybridelements_c_binding
  class(atlas_HybridElements), intent(inout) :: this
  type(atlas_ElementType), intent(in) :: elementtype
  integer(c_size_t), intent(in) :: nb_elements
  integer(c_int), intent(in) :: node_connectivity(:)
  call atlas__mesh__HybridElements__add_elements_with_nodes(this%c_ptr(),&
    & elementtype%c_ptr(),nb_elements,node_connectivity,1)
end subroutine

subroutine add_field(this,field)
  use atlas_hybridelements_c_binding
  class(atlas_HybridElements), intent(inout) :: this
  type(atlas_Field), intent(in) :: field
  call atlas__mesh__HybridElements__add_field(this%c_ptr(), field%c_ptr())
end subroutine


function nb_types(this) result(val)
  use, intrinsic :: iso_c_binding
  use atlas_hybridelements_c_binding
  integer(c_size_t) :: val
  class(atlas_HybridElements), intent(in) :: this
  val = atlas__mesh__HybridElements__nb_types(this%c_ptr())
end function

function nb_fields(this) result(val)
  use, intrinsic :: iso_c_binding
  use atlas_hybridelements_c_binding
  integer(c_size_t) :: val
  class(atlas_HybridElements), intent(in) :: this
  val = atlas__mesh__HybridElements__nb_fields(this%c_ptr())
end function

function has_field(this,name) result(val)
  use fckit_c_interop_module, only: c_str
  use atlas_hybridelements_c_binding
  logical :: val
  class(atlas_HybridElements), intent(in) :: this
  character(len=*), intent(in) :: name
  if( atlas__mesh__HybridElements__has_field(this%c_ptr(),c_str(name)) == 0 ) then
    val = .False.
  else
    val = .True.
  endif
end function

function field_by_name(this,name) result(field)
  use fckit_c_interop_module, only: c_str
  use atlas_hybridelements_c_binding
  type(atlas_Field) :: field
  class(atlas_HybridElements), intent(in) :: this
  character(len=*), intent(in) :: name
  field = atlas_Field( atlas__mesh__HybridElements__field_by_name(this%c_ptr(),c_str(name)) )
  call field%return()
end function

function field_by_idx_size_t(this,idx) result(field)
  use, intrinsic :: iso_c_binding
  use atlas_hybridelements_c_binding
  type(atlas_Field) :: field
  class(atlas_HybridElements), intent(in) :: this
  integer(c_size_t), intent(in) :: idx
  field = atlas_Field( atlas__mesh__HybridElements__field_by_idx(this%c_ptr(),idx-1_c_size_t) )
  call field%return()
end function

function field_by_idx_int(this,idx) result(field)
  use, intrinsic :: iso_c_binding
  use atlas_hybridelements_c_binding
  type(atlas_Field) :: field
  class(atlas_HybridElements), intent(in) :: this
  integer(c_int), intent(in) :: idx
  field = atlas_Field( atlas__mesh__HybridElements__field_by_idx(this%c_ptr(),int(idx-1,c_size_t)) )
  call field%return()
end function

function global_index(this) result(field)
  use atlas_hybridelements_c_binding
  type(atlas_Field) :: field
  class(atlas_HybridElements), intent(in) :: this
  field = atlas_Field( atlas__mesh__HybridElements__global_index(this%c_ptr()) )
  call field%return()
end function

function remote_index(this) result(field)
  use atlas_hybridelements_c_binding
  type(atlas_Field) :: field
  class(atlas_HybridElements), intent(in) :: this
  field = atlas_Field( atlas__mesh__HybridElements__remote_index(this%c_ptr()) )
  call field%return()
end function

function partition(this) result(field)
  use atlas_hybridelements_c_binding
  type(atlas_Field) :: field
  class(atlas_HybridElements), intent(in) :: this
  field = atlas_Field( atlas__mesh__HybridElements__partition(this%c_ptr()) )
  call field%return()
end function

function halo(this) result(field)
  use atlas_hybridelements_c_binding
  type(atlas_Field) :: field
  class(atlas_HybridElements), intent(in) :: this
  field = atlas_Field( atlas__mesh__HybridElements__halo(this%c_ptr()) )
  call field%return()
end function

function elements_size_t(this,idx) result(elements)
  use, intrinsic :: iso_c_binding
  use atlas_hybridelements_c_binding
  type(atlas_Elements) :: elements
  class(atlas_HybridElements), intent(in) :: this
  integer(c_size_t), intent(in) :: idx
  elements = atlas_Elements( atlas__mesh__HybridElements__elements(this%c_ptr(),idx-1_c_size_t) )
  call elements%return()
end function

function elements_int(this,idx) result(elements)
  use, intrinsic :: iso_c_binding
  use atlas_hybridelements_c_binding
  type(atlas_Elements) :: elements
  class(atlas_HybridElements), intent(in) :: this
  integer(c_int), intent(in) :: idx
  elements = atlas_Elements( atlas__mesh__HybridElements__elements(this%c_ptr(),int(idx-1,c_size_t)) )
  call elements%return()
end function

!-------------------------------------------------------------------------------

subroutine atlas_HybridElements__final_auto(this)
  type(atlas_HybridElements) :: this
#if FCKIT_FINAL_DEBUGGING
  write(0,*) "atlas_HybridElements__final_auto"
#endif
#if FCKIT_FINAL_NOT_PROPAGATING
  call this%final()
#endif
  FCKIT_SUPPRESS_UNUSED( this )
end subroutine

end module atlas_HybridElements_module

