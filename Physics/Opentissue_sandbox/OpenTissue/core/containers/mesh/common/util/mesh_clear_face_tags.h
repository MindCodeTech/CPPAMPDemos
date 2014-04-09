#ifndef OPENTISSUE_CORE_CONTAINERS_MESH_COMMON_UTIL_MESH_MESH_CLEAR_FACE_TAGS_H
#define OPENTISSUE_CORE_CONTAINERS_MESH_COMMON_UTIL_MESH_MESH_CLEAR_FACE_TAGS_H
//
// OpenTissue Template Library
// - A generic toolbox for physics-based modeling and simulation.
// Copyright (C) 2008 Department of Computer Science, University of Copenhagen.
//
// OTTL is licensed under zlib: http://opensource.org/licenses/zlib-license.php
//
#include <OpenTissue/configuration.h>

namespace OpenTissue
{
  namespace mesh
  {
    template<typename mesh_type>
    void clear_face_tags(mesh_type & mesh, int tag_value = 0)
    {
      typename mesh_type::face_iterator fend   = mesh.face_end();
      typename mesh_type::face_iterator f      = mesh.face_begin();
      for(;f!=fend;++f)
        f->m_tag = tag_value;
    }

  } // namespace mesh
} // namespace OpenTissue

//OPENTISSUE_CORE_CONTAINERS_MESH_COMMON_UTIL_MESH_MESH_CLEAR_FACE_TAGS_H
#endif
