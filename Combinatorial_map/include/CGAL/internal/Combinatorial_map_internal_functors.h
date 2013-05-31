// Copyright (c) 2010-2011 CNRS and LIRIS' Establishments (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org); you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 3 of the License,
// or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
//
// Author(s)     : Guillaume Damiand <guillaume.damiand@liris.cnrs.fr>
//
#ifndef CGAL_COMBINATORIAL_MAP_INTERNAL_FUNCTORS_H
#define CGAL_COMBINATORIAL_MAP_INTERNAL_FUNCTORS_H

#include <CGAL/Dart_const_iterators.h>
#include <CGAL/Combinatorial_map_basic_operations.h>
#include <CGAL/Dimension.h>
#include <CGAL/Kernel_traits.h>
#include <CGAL/Cartesian_converter.h>
#include <vector>

/* Definition of functors used internally to manage attributes (we need
 * functors as attributes are stored in tuple, thus all the access must be
 * done at compiling time). Some of these functors are used with
 * Foreach_enabled_attributes to iterate through all the non void attribs.
 * Functors allowing to group/ungroup attributes are defined in
 * internal/Combinatorial_map_group_functors.h. Public functions are defined
 * in Combinatorial_map_functors.h.
 *
 * internal::Call_split_functor<CMap,i> to call the OnSplit functors on two
 *    given i-attributes.
 *
 * internal::Call_merge_functor<CMap,i> to call the OnMerge functors on two
 *    given i-attributes.
 *
 * internal::Test_is_valid_attribute_functor<CMap> to test if a given i-cell is
 *    valid (all its darts are linked to the same attribute, no other dart is
 *    linked with this attribute).
 *
 * internal::Count_cell_functor<CMap> to count the nuber of i-cells.
 *
 * internal::Count_bytes_one_attribute_functor<CMap> to count the memory
 *    occupied by i-attributes.
 *
 * internal::Decrease_attribute_functor<CMap> to decrease by one the ref
 *    counting of a given i-attribute.
 *
 * internal::Beta_functor<Dart, i...> to call several beta on the given dart.
 *   Indices are given as parameter of the run function.
 *
 * internal::Beta_functor_static<Dart, i...> to call several beta on the given
 *   dart. Indices are given as template arguments.
 *
 * internal::Set_i_attribute_of_dart_functor<CMap, i> to set the i-attribute
 *   of a given dart.
 */

namespace CGAL
{
// ****************************************************************************
namespace internal
{
// Struct to test if the given class has a functor with a map as first
// parameter.
template <typename CMap, typename Attribute, typename Functor>
struct FuctorWithMap
{
  template <typename T, T> struct TypeCheck;

  typedef char Yes;
  struct No{ char c[2]; };

  template <typename T> struct Fct
  {
    // The function we want to test.
    typedef void (T::*fptr)(CMap*, Attribute&, Attribute&);
  };

  template <typename T>
  static Yes
  HasFunctorWithMap(TypeCheck< typename Fct<T>::fptr, &T::operator() >*);
  template <typename T> static No  HasFunctorWithMap(...);

public:
  static bool const
  value=(sizeof(HasFunctorWithMap<Functor>(0))==sizeof(Yes));
};
// ****************************************************************************
// Functor which call Functor::operator() on the two given cell_attributes
template<typename CMap, typename Cell_attribute, typename Functor,
         bool FunctorWithMap=
         FuctorWithMap<CMap, Cell_attribute, Functor>::value>
struct Apply_cell_functor
{
  static void run(CMap*, Cell_attribute& acell1, Cell_attribute& acell2)
  {
    Functor() (acell1,acell2);
  }
};
template<typename CMap, typename Cell_attribute, typename Functor>
struct Apply_cell_functor<CMap, Cell_attribute, Functor, true>
{
  static void run(CMap* amap, Cell_attribute& acell1, Cell_attribute& acell2)
  {
    Functor() (amap, acell1, acell2);
  }
};
//...except for Null_functor.
template<typename CMap, typename Cell_attribute, bool FunctorWithMap>
struct Apply_cell_functor<CMap, Cell_attribute, CGAL::Null_functor,
    FunctorWithMap>
{
  static void run(CMap*, Cell_attribute&, Cell_attribute&)
  {}
};
//...even with true.
template<typename CMap, typename Cell_attribute>
struct Apply_cell_functor<CMap, Cell_attribute, CGAL::Null_functor, true>
{
  static void run(CMap*, Cell_attribute&, Cell_attribute&)
  {}
};
// ****************************************************************************
// Functor used to call the On_split functor between the two given darts.
template<typename CMap, unsigned int i,
         typename Enabled=typename CMap::
       #ifndef CGAL_CFG_TEMPLATE_IN_DEFAULT_PARAMETER_BUG
         template
       #endif
         Attribute_type<i>::type>
struct Call_split_functor
{
  typedef typename CMap::template Attribute_type<i>::type Attribute;
  typedef typename Attribute::On_split On_split;

  static void run(CMap* amap, typename CMap::Dart_handle adart1,
                  typename CMap::Dart_handle adart2)
  {
    // Static version
    CGAL::internal::Apply_cell_functor<CMap, Attribute, On_split>::
        run(amap, *(adart1->template attribute<i>()),
          *(adart2->template attribute<i>()));
    // Dynamic version
    if ( CGAL::cpp11::get<CMap::Helper::template Dimension_index<i>::value>
         (amap->m_onsplit_functors) )
      CGAL::cpp11::get<CMap::Helper::template Dimension_index<i>::value>
          (amap->m_onsplit_functors)
          (*(adart1->template attribute<i>()),
           *(adart2->template attribute<i>()));
  }
  static void
  run(CMap* amap, typename CMap::template Attribute_handle<i>::type a1,
      typename CMap::template Attribute_handle<i>::type a2)
  {
    // Static version
    CGAL::internal::Apply_cell_functor<CMap, Attribute, On_split>::
        run(amap, *a1, *a2);
    // Dynamic version
    if ( CGAL::cpp11::get<CMap::Helper::template Dimension_index<i>::value>
         (amap->m_onsplit_functors) )
      CGAL::cpp11::get<CMap::Helper::template Dimension_index<i>::value>
          (amap->m_onsplit_functors)(*a1, *a2);
  }
};
// Specialization for disabled attributes.
template<typename CMap,unsigned int i>
struct Call_split_functor<CMap, i, CGAL::Void>
{
  static void run(typename CMap::Dart_handle,
                  typename CMap::Dart_handle)
  {}
};
// ****************************************************************************
// Functor used to call the On_merge functor between the two given darts.
template<typename CMap,unsigned int i,
         typename Enabled=typename CMap::
       #ifndef CGAL_CFG_TEMPLATE_IN_DEFAULT_PARAMETER_BUG
         template
       #endif
         Attribute_type<i>::type>
struct Call_merge_functor
{
  typedef typename CMap::template Attribute_type<i>::type Attribute;
  typedef typename Attribute::On_merge On_merge;

  static void run(CMap* amap, typename CMap::Dart_handle adart1,
                  typename CMap::Dart_handle adart2)
  {
    // Static version
    CGAL::internal::Apply_cell_functor<CMap, Attribute, On_merge>::
        run(amap, *(adart1->template attribute<i>()),
            *(adart2->template attribute<i>()));
    // Dynamic version
    if ( CGAL::cpp11::get<CMap::Helper::template Dimension_index<i>::value>
         (amap->m_onmerge_functors) )
      CGAL::cpp11::get<CMap::Helper::template Dimension_index<i>::value>
          (amap->m_onmerge_functors)
          (*(adart1->template attribute<i>()),
           *(adart2->template attribute<i>()));
  }
  static void
  run(CMap* amap, typename CMap::template Attribute_handle<i>::type a1,
      typename CMap::template Attribute_handle<i>::type a2)
  {
    // Static version
    CGAL::internal::Apply_cell_functor<CMap, Attribute, On_merge>::
        run(amap, *a1, *a2);
    // Dynamic version
    if ( CGAL::cpp11::get<CMap::Helper::template Dimension_index<i>::value>
         (amap->m_onmerge_functors) )
      CGAL::cpp11::get<CMap::Helper::template Dimension_index<i>::value>
          (amap->m_onmerge_functors)(*a1, *a2);
  }
};
// Specialization for disabled attributes.
template<typename CMap,unsigned int i>
struct Call_merge_functor<CMap, i, CGAL::Void>
{
  static void run(CMap*, typename CMap::Dart_handle,
                  typename CMap::Dart_handle)
  {}
};
// ****************************************************************************
/// Functor used to test if a cell is valid
template<typename CMap>
struct Test_is_valid_attribute_functor
{
  /** Test the validity of a i-cell-attribute.
   * ie all the darts belonging to a i-cell are linked to the same attribute.
   * @param adart a dart.
   * @param amark a mark used to mark darts of the i-cell.
   * @return true iff all the darts of the i-cell link to the same attribute.
   */
  template <unsigned int i>
  static void run(const CMap* amap,
                  typename CMap::Dart_const_handle adart,
                  std::vector<int>* marks, bool *ares)
  {
    CGAL_static_assertion_msg(CMap::Helper::template
                              Dimension_index<i>::value>=0,
                              "Test_is_valid_attribute_functor<i> but "
                              " i-attributes are disabled");

    int amark = (*marks)[i];
    if ( amap->is_marked(adart, amark) ) return; // dart already test.

    bool valid = true;
    bool found_dart = false;

    typename CMap::template Attribute_const_handle<i>::type
        a=adart->template attribute<i>();

    unsigned int nb = 0;
    for ( CGAL::CMap_dart_const_iterator_basic_of_cell<CMap,i>
          it(*amap, adart, amark); it.cont(); ++it )
    {
      if ( it->template attribute<i>() != a )
      {
        std::cout<<"ERROR: an attribute of the "<<i<<"-cell is different. cur:"
                <<&*a<<" != first:"<<&*it->template attribute<i>()
               <<" for dart "<<&*it<<std::endl;
        valid=false;
      }

      if ( a!=NULL && it==a->dart() ) found_dart=true;

      amap->mark(it, amark);
      ++nb;
    }

    if ( a!=NULL )
    {
      if ( a->get_nb_refs()!=nb )
      {
        std::cout<<"ERROR: the number of reference of an "<<i
                <<"-attribute is not correct. Count: "<<nb
               <<" != Store in the attribute: "<<a->get_nb_refs()<<" for dart "
              <<&*adart<<std::endl;
        valid=false;
      }
      if ( !a->is_valid() )
      {
        std::cout<<"ERROR: the dart associated with an "<<i
                <<"-attribute is NULL for dart "
               <<&*adart<<std::endl;
        valid=false;
      }
      if ( a->dart()!=NULL && !found_dart )
      {
        std::cout<<"ERROR: the non NULL dart of an "<<i
                <<"-attribute does not belong to the cell. a->dart()= "
               <<&*a->dart()<<" for dart "<<&*adart<<std::endl;
        valid=false;
      }
    }

    if ( !valid ) (*ares)=false;
  }
};
// ****************************************************************************
/// Functor for counting i-cell
template<typename CMap>
struct Count_cell_functor
{
  template <unsigned int i>
  static void run( const CMap* amap,
                   typename CMap::Dart_const_handle adart,
                   std::vector<int>* amarks,
                   std::vector<unsigned int>* ares )
  {
    if ( (*amarks)[i]!=-1 && !amap->is_marked(adart, (*amarks)[i]) )
    {
      ++ (*ares)[i];
      CGAL::mark_cell<CMap,i>(*amap, adart, (*amarks)[i]);
    }
  }
};
// ****************************************************************************
/// Functor for counting the memory occupation of attributes
/// Be careful not reentrant !!! TODO a  Foreach_enabled_attributes
/// taking an instance of a functor as argument allowing to compute
/// and return values.
template<typename CMap>
struct Count_bytes_one_attribute_functor
{
  template <unsigned int i>
  static void run( const CMap* amap )
  {
    res += amap->template attributes<i>().capacity()*
      sizeof(typename CMap::template Attribute_type<i>::type);
  }

  static typename CMap::size_type res;
};
template<typename CMap>
typename CMap::size_type Count_bytes_one_attribute_functor<CMap>::res = 0;

template<typename CMap>
struct Count_bytes_all_attributes_functor
{
  static typename CMap::size_type run( const CMap& amap )
  {
    CGAL::internal::Count_bytes_one_attribute_functor<CMap>::res = 0;
    CMap::Helper::template Foreach_enabled_attributes
      <CGAL::internal::Count_bytes_one_attribute_functor<CMap> >::run(&amap);
    return CGAL::internal::Count_bytes_one_attribute_functor<CMap>::res;
  }
};
// ****************************************************************************
/// Decrease the cell attribute reference counting of the given dart.
/// The attribute is removed if there is no more darts linked with it.
template<typename CMap, unsigned int i, typename T=
         typename CMap::template Attribute_type<i>::type>
struct Decrease_attribute_functor_run
{
  static void run(CMap* amap, typename CMap::Dart_handle adart)
  {
    if ( adart->template attribute<i>()!=NULL )
    {
      adart->template attribute<i>()->dec_nb_refs();
      if ( adart->template attribute<i>()->get_nb_refs()==0 )
        amap->template erase_attribute<i>(adart->template attribute<i>());
    }
  }
};
/// Specialization for void attributes.
template<typename CMap, unsigned int i>
struct Decrease_attribute_functor_run<CMap, i, CGAL::Void>
{
  static void run(CMap*, typename CMap::Dart_handle)
  {}
};
// ****************************************************************************
/// Functor used to call decrease_attribute_ref_counting<i>
/// on each i-cell attribute enabled
template<typename CMap>
struct Decrease_attribute_functor
{
  template <unsigned int i>
  static void run(CMap* amap, typename CMap::Dart_handle adart)
  { CGAL::internal::Decrease_attribute_functor_run<CMap,i>::run(amap, adart); }
};
// ****************************************************************************
/// Functor used to set the i-attribute of a given dart.
template<typename CMap, unsigned int i, typename T=
         typename CMap::template Attribute_type<i>::type>
struct Set_i_attribute_of_dart_functor
{
  static void run( CMap* amap, typename CMap::Dart_handle dh,
                   typename CMap::template Attribute_handle<i>::type ah )
  {
    CGAL_static_assertion(i<=CMap::dimension);
    CGAL_assertion( dh!=NULL && dh!=CMap::null_dart_handle );

    if ( dh->template attribute<i>()==ah ) return;

    CGAL::internal::Decrease_attribute_functor_run<CMap, i>::run(amap, dh);
    dh->template set_attribute<i>(ah);
    if ( ah!=NULL ) ah->set_dart(dh);
  }
};
/// Specialization for void attributes.
template<typename CMap, unsigned int i>
struct Set_i_attribute_of_dart_functor<CMap, i, CGAL::Void>
{
  static void run( CMap*, typename CMap::Dart_handle,
                   typename CMap::template Attribute_handle<i>::type)
  {}
};
// ****************************************************************************
// Functor allowing to set the value of a point if point exist, have
// same dimension. For dim>3, if type of points are the same (because no converter).
template<typename Point1, typename Point2>
struct Set_point_d_if_same
{
  static void run(Point1&, const Point2&)
  {}
};

template<typename Point1>
struct Set_point_d_if_same<Point1, Point1>
{
  static void run(Point1& p1, const Point1& p2)
  {
    p1 = p2; // Copy of Point_d having same type
  }
};

template<typename Point1, typename Point2,
         typename T1=typename Ambient_dimension<Point1>::type,
         typename T2=typename Ambient_dimension<Point2>::type>
struct Set_point_if_possible
{
  static void run(Point1&, const Point2&)
  {}
};

template<typename Point1, typename Point2>
struct Set_point_if_possible<Point1, Point2, Dimension_tag<2>, Dimension_tag<2> >
{
  static void run(Point1& p1, const Point2& p2)
  {
    p1 = Cartesian_converter<typename Kernel_traits<Point1>::Kernel,
        typename Kernel_traits<Point2>::Kernel>(p2);
  }
};

template<typename Point1, typename Point2>
struct Set_point_if_possible<Point1, Point2, Dimension_tag<3>, Dimension_tag<3> >
{
  static void run(Point1& p1, const Point2& p2)
  {
    p1 = Cartesian_converter<typename Kernel_traits<Point1>::Kernel,
        typename Kernel_traits<Point2>::Kernel>(p2);
  }
};

template<typename Point1, typename Point2>
struct Set_point_if_possible<Point1, Point2, Dynamic_dimension_tag,
    Dynamic_dimension_tag>
{
  static void run(Point1& p1, const Point2& p2)
  {
    if ( p1.dimension()==p2.dimension() )
      Set_point_d_if_same<Point1, Point2>::run(p1, p2);
  }
};

// Set_point_if_exist ensure that Attr2 has a point
template<typename Point1, typename Attr2,
         typename Point2=typename Attr2::Point>
struct Set_point_if_exist
{
  static void run(Point1& p1, const Attr2& a2)
  {
    Set_point_if_possible<Point1, Point2>(p1, a2.point());
  }
};

template<typename Point1, typename Attr2>
struct Set_point_if_exist<Point1, Attr2, CGAL::Void>
{
  static void run(Point1&, const Attr2& )
  {}
};

// Functor allowing to set a value if two info are the same or not
// TODO replace these version by using is_constructible ?
// if no support for gcc11, use this simplified version ? (same type)
template<typename Attr1, typename Attr2,
         typename T1=typename Attr1::Info, typename T2=typename Attr2::Info>
struct Set_info_if_same_type
{
  static void run(Attr1&, const Attr2&)
  {}
};

template<typename Attr1, typename Attr2, typename T1>
struct Set_info_if_same_type<Attr1, Attr2, T1, void>
{
  static void run(Attr1&, const Attr2&)
  {}
};

template<typename Attr1, typename Attr2, typename T1>
struct Set_info_if_same_type<Attr1, Attr2, void, T1>
{
  static void run(Attr1&, const Attr2&)
  {}
};

template<typename Attr1, typename Attr2>
struct Set_info_if_same_type<Attr1, Attr2, void, void>
{
  static void run(Attr1&, const Attr2&)
  {}
};

template<typename Attr1, typename Attr2, typename T1>
struct Set_info_if_same_type<Attr1, Attr2, T1, T1>
{
  static void run(Attr1& a1, const Attr2& a2)
  { a1.info()=a2.info(); }
};

// Functor allowing to copy attributes whey info are convertible
template<typename Map1, typename Map2, int i,
         typename T1, typename T2,
         typename Info1=typename T1::Info, typename Info2=typename T2::Info>
struct Copy_attr_if_same_type
{
  static void const run(Map1* cmap1, const Map2* cmap2,
                        typename Map1::Dart_handle dh1,
                        typename Map2::Dart_const_handle dh2)
  {}
};

template<typename Map1, typename Map2, int i,
         typename T1, typename T2, typename Info>
struct Copy_attr_if_same_type<Map1, Map2, i, T1, T2, Info, Info>
{
  static void const run(Map1* cmap1, const Map2* cmap2,
                        typename Map1::Dart_handle dh1,
                        typename Map2::Dart_const_handle dh2)
  {
    CGAL_static_assertion(i<=Map2::dimension);
    if (dh1->template attribute<i>()==NULL &&
        dh2->template attribute<i>()!=NULL)
    {
      cmap1->template
        set_attribute<i>(dh1, cmap1->template create_attribute<i>());
      dh1->template attribute<i>()->copy(*dh2->template attribute<i>());
    }
  }
};

// Case of two non void type
template <typename Map1, typename Map2, int i,
          typename T1, typename T2>
struct Copy_attr_functor
{
  static void const run(Map1* cmap1, const Map2* cmap2,
                        typename Map1::Dart_handle dh1,
                        typename Map2::Dart_const_handle dh2)
  {
    Copy_attr_if_same_type<Map1, Map2, i, T1, T2>::
        run(cmap1, cmap2, dh1, dh2);
  }
};

// Case T1==void
template <typename Map1, typename Map2, int i, typename T1>
struct Copy_attr_functor<Map1, Map2, i, CGAL::Void, T1>
{
  static void const run(Map1*, const Map2*,
                        typename Map1::Dart_handle,
                        typename Map2::Dart_const_handle)
  {}
};

// Case T2==void
template <typename Map1, typename Map2, int i, typename T1>
struct Copy_attr_functor<Map1, Map2, i, T1, CGAL::Void>
{
  static void const run(Map1*, const Map2*,
                        typename Map1::Dart_handle,
                        typename Map2::Dart_const_handle)
  {}
};

// Case T1==T2==void
template <typename Map1, typename Map2, int i>
struct Copy_attr_functor<Map1, Map2, i, CGAL::Void, CGAL::Void>
{
  static void const run(Map1*, const Map2*,
                        typename Map1::Dart_handle,
                        typename Map2::Dart_const_handle)
  {}
};

/// Copy enabled attributes from one cmap to other
template<typename Map1, typename Map2>
struct Copy_attributes_functor{
  template<unsigned int i>
  static void run( Map1* cmap1,
                   const Map2* cmap2,
                   typename Map1::Dart_handle dh1,
                   typename Map2::Dart_const_handle dh2 )
  {
    Copy_attr_functor<Map1, Map2, i,
                      typename Map1::Helper::template Attribute_type<i>::type,
                      typename Map2::Helper::template Attribute_type<i>::type>
    ::run(cmap1, cmap2, dh1, dh2);
  }
};
// ****************************************************************************
// Functor allowing to test if two info are the same or not
template<typename T1, typename T2>
struct Is_same_info
{
  static bool run(const T1&, const T2&)
  { return false; }
};

template<typename T1>
struct Is_same_info<T1,T1>
{
  static bool run(const T1&e1, const T1&e2)
  { return e1==e2; }
};

// Case of two non void type
template<typename Map1, typename Map2,
         typename T1, typename T2, int i>
struct Is_same_attribute_functor
{
  static bool const run(typename Map1::Dart_const_handle dh1,
                        typename Map2::Dart_const_handle dh2)
  {
    if (dh1->template attribute<i>()==NULL &&
        dh2->template attribute<i>()==NULL)
      return true;

    if (dh1->template attribute<i>()==NULL ||
        dh2->template attribute<i>()==NULL)
      return false;

    return
      Is_same_info<typename T1::Info,typename T2::Info>::
      run(dh1->template attribute<i>()->info(),
          dh2->template attribute<i>()->info());
  }
};

// Case T1==void
template <typename Map1, typename Map2,
          typename T1, int i>
struct Is_same_attribute_functor<Map1, Map2, Void, T1, i>
{
  static bool run(typename Map1::Dart_const_handle,
                  typename Map2::Dart_const_handle dh2)
  { return dh2->template attribute<i>()==NULL; }
};

// Case T2==void
template <typename Map1, typename Map2,
          typename T1, int i>
struct Is_same_attribute_functor<Map1, Map2, T1, Void, i>
{
  static bool run(typename Map1::Dart_const_handle dh1,
                  typename Map2::Dart_const_handle)
  { return dh1->template attribute<i>()==NULL; }
};

// Case T1==T2==void
template <typename Map1, typename Map2, int i>
struct Is_same_attribute_functor<Map1, Map2, Void, Void, i>
{
  static bool run(typename Map1::Dart_const_handle,
                  typename Map2::Dart_const_handle)
  { return true; }
};

/// Test if the two darts are associated with the same attribute.
template<typename Map1, typename Map2>
struct Test_is_same_attribute_functor
{
  template<unsigned int i>
  static void run(typename Map1::Dart_const_handle dh1,
                  typename Map2::Dart_const_handle dh2 )
  {
    if (value)
      value = Is_same_attribute_functor
        <Map1, Map2,
         typename Map1::Helper::template Attribute_type<i>::type,
         typename Map2::Helper::template Attribute_type<i>::type,
         i>::run(dh1, dh2);
  }
  static bool value;
};
template<typename Map1, typename Map2>
bool Test_is_same_attribute_functor<Map1, Map2>::value = true;
// ****************************************************************************
// Beta functor, used to combine several beta.
#ifndef CGAL_CFG_NO_CPP0X_VARIADIC_TEMPLATES
template<typename Dart_handle, typename ... Betas>
struct Beta_functor;
template<typename Dart_handle, typename ... Betas>
struct Beta_functor<Dart_handle, int, Betas...>
{
  static Dart_handle run(Dart_handle ADart, int B, Betas... betas)
  { return Beta_functor<Dart_handle, Betas...>::run(ADart->beta(B),
                                                    betas...); }
};
template<typename Dart_handle>
struct Beta_functor<Dart_handle, int>
{
  static Dart_handle run(Dart_handle ADart, int B)
  {
    CGAL_assertion( ADart!=NULL );
    return ADart->beta(B);
  }
};
// ****************************************************************************
template<typename Dart_handle, int ... Betas>
struct Beta_functor_static;
template<typename Dart_handle, int B, int ... Betas>
struct Beta_functor_static<Dart_handle, B, Betas...>
{
  static Dart_handle run(Dart_handle ADart)
  { return Beta_functor_static<Dart_handle, Betas...>::
        run(ADart->template beta<B>()); }
};
template<typename Dart_handle, int B>
struct Beta_functor_static<Dart_handle, B>
{
  static Dart_handle run(Dart_handle ADart)
  {
    CGAL_assertion( ADart!=NULL );
    return ADart->template beta<B>();
  }
};
#endif //CGAL_CFG_NO_CPP0X_VARIADIC_TEMPLATES
// ****************************************************************************
} // namespace internal
} // namespace CGAL

#endif // CGAL_COMBINATORIAL_MAP_INTERNAL_FUNCTORS_H
