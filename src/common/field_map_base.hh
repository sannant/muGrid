/**
 * file   field_map.hh
 *
 * @author Till Junge <till.junge@epfl.ch>
 *
 * @date   12 Sep 2017
 *
 * @brief  Defined a strongly defines proxy that iterates efficiently over a field
 *
 * @section LICENCE
 *
 * Copyright (C) 2017 Till Junge
 *
 * µSpectre is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3, or (at
 * your option) any later version.
 *
 * µSpectre is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Emacs; see the file COPYING. If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#ifndef FIELD_MAP_H
#define FIELD_MAP_H

#include <array>
#include <string>
#include <memory>
#include <type_traits>
#include <unsupported/Eigen/CXX11/Tensor>

#include "common/field.hh"
#include "common/common.hh"


namespace muSpectre {

  namespace internal {
    //----------------------------------------------------------------------------//
    //! little helper to automate creation of const maps without duplication
    template<class T, bool isConst>
    struct const_corrector {
      using type = typename T::reference;
    };
    template<class T>
    struct const_corrector<T, true> {
      using type = typename T::const_reference;
    };

    template<class T, bool isConst>
    using const_corrector_t = typename const_corrector<T, isConst>::type;

    //----------------------------------------------------------------------------//
    template <class FieldCollection, typename T, Dim_t NbComponents>
    class FieldMap
    {
    public:
      using TypedField = ::muSpectre::internal::TypedFieldBase
        <FieldCollection, T, NbComponents>;
      using Field = typename TypedField::parent;
      using size_type = std::size_t;

      //! Default constructor
      FieldMap() = delete;

      FieldMap(Field & field);

      //! Copy constructor
      FieldMap(const FieldMap &other) = delete;

      //! Move constructor
      FieldMap(FieldMap &&other) noexcept = delete;

      //! Destructor
      virtual ~FieldMap() noexcept = default;

      //! Copy assignment operator
      FieldMap& operator=(const FieldMap &other) = delete;

      //! Move assignment operator
      FieldMap& operator=(FieldMap &&other) noexcept = delete;

      //! give static access to nb_components
      inline constexpr static size_t nb_components();

      //! give human-readable field map type
      virtual std::string info_string() const = 0;

      //! return field name
      inline const std::string & get_name() const;

      //! return my collection (for iterating)
      inline const FieldCollection & get_collection() const;

      //! member access needs to be implemented by inheriting classes
      //inline value_type operator[](size_t index);
      //inline value_type operator[](Ccoord ccord);

      //! check compatibility (must be called by all inheriting classes at the
      //! end of their constructors
      inline void check_compatibility();

      //! convenience call to collection's size method
      inline size_t size() const;

      template <class FullyTypedFieldMap, bool isConst=false>
      class iterator
      {
      public:
        using value_type =
          const_corrector_t<FullyTypedFieldMap, isConst>;
        using pointer = typename FullyTypedFieldMap::pointer;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::random_access_iterator_tag;
        using Ccoord = typename FieldCollection::Ccoord;
        //! Default constructor
        iterator() = delete;

        //! constructor
        inline iterator(FullyTypedFieldMap & fieldmap, bool begin=true);

        //! constructor for random access
        inline iterator(FullyTypedFieldMap & fieldmap, size_t index);

        //! Copy constructor
        iterator(const iterator &other)= default;

        //! Move constructor
        iterator(iterator &&other) noexcept = default;

        //! Destructor
        virtual ~iterator() noexcept = default;

        //! Copy assignment operator
        iterator& operator=(const iterator &other) = default;

        //! Move assignment operator
        iterator& operator=(iterator &&other) noexcept = default;

        //! pre-increment
        inline iterator & operator++();
        //! post-increment
        inline iterator operator++(int);
        //! dereference
        inline value_type operator*();
        //! member of pointer
        inline pointer operator->();
        //! pre-decrement
        inline iterator & operator--();
        //! post-decrement
        inline iterator operator--(int);
        //! access subscripting
        inline value_type operator[](difference_type diff);
        //! equality
        inline bool operator==(const iterator & other) const;
        //! inequality
        inline bool operator!=(const iterator & other) const;
        //! div. comparisons
        inline bool operator<(const iterator & other) const;
        inline bool operator<=(const iterator & other) const;
        inline bool operator>(const iterator & other) const;
        inline bool operator>=(const iterator & other) const;
        //! additions, subtractions and corresponding assignments
        inline iterator operator+(difference_type diff) const;
        inline iterator operator-(difference_type diff) const;
        inline iterator& operator+=(difference_type diff);
        inline iterator& operator-=(difference_type diff);

        //! get pixel coordinates
        inline Ccoord get_ccoord() const;

        //! ostream operator (mainly for debug
        friend std::ostream & operator<<(std::ostream & os,
                                         const iterator& it) {
          if (isConst) {
            os << "const ";
          }
          os << "iterator on field '"
             << it.fieldmap.get_name()
             << "', entry " << it.index;
          return os;
        }

      protected:
        const FieldCollection & collection;
        FullyTypedFieldMap  & fieldmap;
        size_t index;
      private:
      };


    protected:
      inline T* get_ptr_to_entry(size_t index);
      inline T& get_ref_to_entry(size_t index);
      const FieldCollection & collection;
      TypedField  & field;
    private:
    };
  }  // internal


  namespace internal {

    /* ---------------------------------------------------------------------- */
    template<class FieldCollection, typename T, Dim_t NbComponents>
    FieldMap<FieldCollection, T, NbComponents>::
    FieldMap(Field & field)
      :collection(field.get_collection()), field(static_cast<TypedField&>(field)) {
      static_assert(NbComponents>0,
                    "Only fields with more than 0 components allowed");
    }

    /* ---------------------------------------------------------------------- */
    template<class FieldCollection, typename T, Dim_t NbComponents>
    void
    FieldMap<FieldCollection, T, NbComponents>::
    check_compatibility() {
     if (typeid(T).hash_code() !=
          this->field.get_stored_typeid().hash_code()) {
        throw FieldInterpretationError
          ("Cannot create a Map of type '" +
           this->info_string() +
           "' for field '" + this->field.get_name() + "' of type '" +
           this->field.get_stored_typeid().name() + "'");
      }
      //check size compatibility
      if (NbComponents != this->field.get_nb_components()) {
        throw FieldInterpretationError
          ("Cannot create a Map of type '" +
           this->info_string() +
           "' for field '" + this->field.get_name() + "' with " +
           std::to_string(this->field.get_nb_components()) + " components");
      }
    }

    /* ---------------------------------------------------------------------- */
    template<class FieldCollection, typename T, Dim_t NbComponents>
    size_t
    FieldMap<FieldCollection, T, NbComponents>::
    size() const {
      return this->collection.size();
    }

    /* ---------------------------------------------------------------------- */
    template<class FieldCollection, typename T, Dim_t NbComponents>
    constexpr size_t
    FieldMap<FieldCollection, T, NbComponents>::
    nb_components() {
      return NbComponents;
    }

    /* ---------------------------------------------------------------------- */
    template<class FieldCollection, typename T, Dim_t NbComponents>
    const std::string &
    FieldMap<FieldCollection, T, NbComponents>::
    get_name() const {
      return this->field.get_name();
    }

    /* ---------------------------------------------------------------------- */
    template<class FieldCollection, typename T, Dim_t NbComponents>
    const FieldCollection &
    FieldMap<FieldCollection, T, NbComponents>::
    get_collection() const {
      return this->collection;
    }

    /* ---------------------------------------------------------------------- */
    /* ---------------------------------------------------------------------- */
    // Iterator implementations
    //! constructor
    template<class FieldCollection, typename T, Dim_t NbComponents>
    template<class FullyTypedFieldMap, bool isConst>
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst>::
    iterator(FullyTypedFieldMap & fieldmap, bool begin)
      :collection(fieldmap.get_collection()), fieldmap(fieldmap),
       index(begin ? 0 : fieldmap.field.size()) {}

    /* ---------------------------------------------------------------------- */
    //! constructor for random access
    template<class FieldCollection, typename T, Dim_t NbComponents>
    template<class FullyTypedFieldMap, bool isConst>
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst>::
    iterator(FullyTypedFieldMap & fieldmap, size_t index)
      :collection(fieldmap.collection), fieldmap(fieldmap),
       index(index) {}

    /* ---------------------------------------------------------------------- */
    //! pre-increment
    template<class FieldCollection, typename T, Dim_t NbComponents>
    template<class FullyTypedFieldMap, bool isConst>
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst> &
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst>::
    operator++() {
      this->index++;
      return *this;
    }

    /* ---------------------------------------------------------------------- */
    //! post-increment
    template<class FieldCollection, typename T, Dim_t NbComponents>
    template<class FullyTypedFieldMap, bool isConst>
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst>
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst>::
    operator++(int) {
      iterator current = *this;
      this->index++;
      return current;
    }

    /* ---------------------------------------------------------------------- */
    //! dereference
    template<class FieldCollection, typename T, Dim_t NbComponents>
    template<class FullyTypedFieldMap, bool isConst>
    typename FieldMap<FieldCollection, T, NbComponents>::template iterator<FullyTypedFieldMap, isConst>::value_type
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst>::
    operator*() {
      return this->fieldmap.template operator[]<value_type>(this->index);
    }

    /* ---------------------------------------------------------------------- */
    //! member of pointer
    template<class FieldCollection, typename T, Dim_t NbComponents>
    template<class FullyTypedFieldMap, bool isConst>
    typename FullyTypedFieldMap::pointer
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst>::
    operator->() {
      return this->fieldmap.ptr_to_val_t(this->index);
    }

    /* ---------------------------------------------------------------------- */
    //! pre-decrement
    template<class FieldCollection, typename T, Dim_t NbComponents>
    template<class FullyTypedFieldMap, bool isConst>
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst> &
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst>::
    operator--() {
      this->index--;
      return *this;
    }

    /* ---------------------------------------------------------------------- */
    //! post-decrement
    template<class FieldCollection, typename T, Dim_t NbComponents>
    template<class FullyTypedFieldMap, bool isConst>
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst>
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst>::
    operator--(int) {
      iterator current = *this;
      this->index--;
      return current;
    }

    /* ---------------------------------------------------------------------- */
    //! Access subscripting
    template<class FieldCollection, typename T, Dim_t NbComponents>
    template<class FullyTypedFieldMap, bool isConst>
    typename FieldMap<FieldCollection, T, NbComponents>::template iterator<FullyTypedFieldMap, isConst>::value_type
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst>::
    operator[](difference_type diff) {
      return this->fieldmap[this->index+diff];
    }

    /* ---------------------------------------------------------------------- */
    //! equality
    template<class FieldCollection, typename T, Dim_t NbComponents>
    template<class FullyTypedFieldMap, bool isConst>
    bool
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst>::
    operator==(const iterator & other) const {
      return (this->index == other.index &&
              &this->fieldmap == &other.fieldmap);
    }

    /* ---------------------------------------------------------------------- */
    //! inquality
    template<class FieldCollection, typename T, Dim_t NbComponents>
    template<class FullyTypedFieldMap, bool isConst>
    bool
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst>::
    operator!=(const iterator & other) const {
      return !(*this == other);
    }

    /* ---------------------------------------------------------------------- */
    //! div. comparisons
    template<class FieldCollection, typename T, Dim_t NbComponents>
    template<class FullyTypedFieldMap, bool isConst>
    bool
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst>::
    operator<(const iterator & other) const {
      return (this->index < other.index);
    }
    template<class FieldCollection, typename T, Dim_t NbComponents>
    template<class FullyTypedFieldMap, bool isConst>
    bool
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst>::
    operator<=(const iterator & other) const {
      return (this->index <= other.index);
    }
    template<class FieldCollection, typename T, Dim_t NbComponents>
    template<class FullyTypedFieldMap, bool isConst>
    bool
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst>::
    operator>(const iterator & other) const {
      return (this->index > other.index);
    }
    template<class FieldCollection, typename T, Dim_t NbComponents>
    template<class FullyTypedFieldMap, bool isConst>
    bool
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst>::
    operator>=(const iterator & other) const {
      return (this->index >= other.index);
    }

    /* ---------------------------------------------------------------------- */
    //! additions, subtractions and corresponding assignments
    template<class FieldCollection, typename T, Dim_t NbComponents>
    template<class FullyTypedFieldMap, bool isConst>
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst>
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst>::
    operator+(difference_type diff) const {
      return iterator(this->fieldmap, this->index + diff);
    }
    template<class FieldCollection, typename T, Dim_t NbComponents>
    template<class FullyTypedFieldMap, bool isConst>
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst>
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst>::
    operator-(difference_type diff) const {
      return iterator(this->fieldmap, this->index - diff);
    }
    template<class FieldCollection, typename T, Dim_t NbComponents>
    template<class FullyTypedFieldMap, bool isConst>
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst> &
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst>::
    operator+=(difference_type diff) {
      this->index += diff;
      return *this;
    }
    template<class FieldCollection, typename T, Dim_t NbComponents>
    template<class FullyTypedFieldMap, bool isConst>
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst> &
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst>::
    operator-=(difference_type diff) {
      this->index -= diff;
      return *this;
    }

    /* ---------------------------------------------------------------------- */
    //! get pixel coordinates
    template<class FieldCollection, typename T, Dim_t NbComponents>
    template<class FullyTypedFieldMap, bool isConst>
    typename FieldCollection::Ccoord
    FieldMap<FieldCollection, T, NbComponents>::iterator<FullyTypedFieldMap, isConst>::
    get_ccoord() const {
      return this->collection.get_ccoord(this->index);
    }

////----------------------------------------------------------------------------//
//template<class FieldCollection, typename T, Dim_t NbComponents, class FullyTypedFieldMap>
//std::ostream & operator <<
//(std::ostream &os,
// const typename FieldMap<FieldCollection, T, NbComponents>::
// template iterator<FullyTypedFieldMap, isConst> & it) {
//  os << "iterator on field '"
//     << it.field.get_name()
//     << "', entry " << it.index;
//  return os;
//}

    /* ---------------------------------------------------------------------- */
    template<class FieldCollection, typename T, Dim_t NbComponents>
    T*
    FieldMap<FieldCollection, T, NbComponents>::get_ptr_to_entry(size_t index) {
      return this->field.get_ptr_to_entry(std::move(index));
    }

    /* ---------------------------------------------------------------------- */
    template<class FieldCollection, typename T, Dim_t NbComponents>
    T&
    FieldMap<FieldCollection, T, NbComponents>::get_ref_to_entry(size_t index) {
      return this->field.get_ref_to_entry(std::move(index));
    }


  }  // internal


}  // muSpectre

#endif /* FIELD_MAP_H */
