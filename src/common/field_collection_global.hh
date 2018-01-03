/**
 * file   field_collection_global.hh
 *
 * @author Till Junge <till.junge@altermail.ch>
 *
 * @date   05 Nov 2017
 *
 * @brief  FieldCollection base-class for global fields
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


#ifndef FIELD_COLLECTION_GLOBAL_H
#define FIELD_COLLECTION_GLOBAL_H

#include "common/field_collection_base.hh"
#include "common/ccoord_operations.hh"


namespace muSpectre {

  /* ---------------------------------------------------------------------- */
  //! DimS spatial dimension (dimension of problem)
  //! DimM material_dimension (dimension of constitutive law)
  template<Dim_t DimS, Dim_t DimM>
  class GlobalFieldCollection:
    public FieldCollectionBase<DimS, DimM, GlobalFieldCollection<DimS, DimM>>
  {
  public:
    using Parent = FieldCollectionBase
      <DimS, DimM, GlobalFieldCollection<DimS, DimM>>;
    using Ccoord = typename Parent::Ccoord;
    using Field_p = typename Parent::Field_p;
    using iterator = typename CcoordOps::Pixels<DimS>::iterator;
    //! Default constructor
    GlobalFieldCollection();

    //! Copy constructor
    GlobalFieldCollection(const GlobalFieldCollection &other) = delete;

    //! Move constructor
    GlobalFieldCollection
      (GlobalFieldCollection &&other) noexcept = default;

    //! Destructor
    virtual ~GlobalFieldCollection() noexcept = default;

    //! Copy assignment operator
    GlobalFieldCollection&
    operator=(const GlobalFieldCollection &other) = delete;

    //! Move assignment operator
    GlobalFieldCollection&
    operator=(GlobalFieldCollection &&other) noexcept = default;

    /** allocate memory, etc. At this point, the collection is
        informed aboud the size and shape of the domain (through the
        sizes parameter). The job of initialise is to make sure that
        all fields are either of size 0, in which case they need to be
        allocated, or are of the same size as the product of 'sizes'
        (if standard strides apply) any field of a different size is
        wrong.

        Attention: The strides are typically not needed, as by
        default, the strides can be inferred from the sizes (e.g.,
        using Ccoordops::get_default_strides). FFTW3-MPI, however,
        need a padding in the case of an even number of pixels in the
        contiguous direction with r2c and c2r transforms. This means
        that the data is not necessarily contiguous in the input array

        TODO: check whether it makes sense to put a runtime check here
     **/
    inline void initialise(Ccoord sizes, Ccoord strides=CcoordOps::get_cube<DimS>(-1));

    //! return the pixel sizes
    inline const Ccoord & get_sizes() const;

    //! returns the linear index corresponding to cell coordinates
    template <class CcoordRef>
    inline size_t get_index(CcoordRef && ccoord) const;
    //! returns the cell coordinates corresponding to a linear index
    inline Ccoord get_ccoord(size_t index) const;

    inline iterator begin();
    inline iterator end();


    static constexpr inline Dim_t spatial_dim() {return DimS;}
    static constexpr inline Dim_t material_dim() {return DimM;}
  protected:
    //! number of discretisation cells in each of the DimS spatial directions
    Ccoord sizes{};
    Ccoord strides{};
    CcoordOps::Pixels<DimS> pixels;
  private:
  };

  /* ---------------------------------------------------------------------- */
  template <Dim_t DimS, Dim_t DimM>
  GlobalFieldCollection<DimS, DimM>::GlobalFieldCollection()
    :Parent(), pixels{{0}}{}



  /* ---------------------------------------------------------------------- */
  template <Dim_t DimS, Dim_t DimM>
  void GlobalFieldCollection<DimS, DimM>::
  initialise(Ccoord sizes, Ccoord strides) {
    if (this->is_initialised) {
      throw std::runtime_error("double initialisation");
    }
    if (strides == CcoordOps::get_cube<DimS>(-1)) {
      this->strides = CcoordOps::get_default_strides(sizes);
    } else {
      this->strides = strides;
    }
    this->pixels = CcoordOps::Pixels<DimS>(sizes);
    this->size_ = CcoordOps::get_size(sizes);
    this->sizes = sizes;
    size_t logical_size{CcoordOps::get_size_from_strides(this->sizes, this->strides)};
    if (logical_size != this->size()) {
      std::cout << "TODO: size warning" << std::endl;
    } else {
      std::cout << "TODO: standard sizing" << std::endl;
    }

    std::for_each(std::begin(this->fields), std::end(this->fields),
                  [this](auto && item) {
                    auto && field = *item.second;
                    const auto field_size = field.size();
                    if (field_size == 0) {
                      field.resize(this->size());
                    } else if (field_size != this->size()) {
                      std::stringstream err_stream;
                      err_stream << "Field '" << field.get_name()
                                 << "' contains " << field_size
                                 << " entries, but the field collection "
                                 << "has " << this->size() << " pixels";
                      throw FieldCollectionError(err_stream.str());
                    }
                  });
    this->is_initialised = true;
  }

  //----------------------------------------------------------------------------//
  //! return the pixel sizes
  template <Dim_t DimS, Dim_t DimM>
  const typename GlobalFieldCollection<DimS, DimM>::Ccoord &
  GlobalFieldCollection<DimS, DimM>::get_sizes() const {
    return this->sizes;
  }

  //----------------------------------------------------------------------------//
  //! returns the cell coordinates corresponding to a linear index
  template <Dim_t DimS, Dim_t DimM>
  typename GlobalFieldCollection<DimS, DimM>::Ccoord
  GlobalFieldCollection<DimS, DimM>::get_ccoord(size_t index) const {
    return CcoordOps::get_ccoord(this->get_sizes(), std::move(index));
  }


  /* ---------------------------------------------------------------------- */
  template <Dim_t DimS, Dim_t DimM>
  typename GlobalFieldCollection<DimS, DimM>::iterator
  GlobalFieldCollection<DimS, DimM>::begin() {
    return this->pixels.begin();
  }

  /* ---------------------------------------------------------------------- */
  template <Dim_t DimS, Dim_t DimM>
  typename GlobalFieldCollection<DimS, DimM>::iterator
  GlobalFieldCollection<DimS, DimM>::end() {
    return this->pixels.end();
  }
  //----------------------------------------------------------------------------//
  //! returns the linear index corresponding to cell coordinates
  template <Dim_t DimS, Dim_t DimM>
  template <class CcoordRef>
  size_t
  GlobalFieldCollection<DimS, DimM>::get_index(CcoordRef && ccoord) const {
    static_assert(std::is_same<
                    Ccoord,
                    std::remove_const_t<
                      std::remove_reference_t<CcoordRef>>>::value,
                  "can only be called with values or references of Ccoord");
    return CcoordOps::get_index(this->get_sizes(),
                                std::forward<CcoordRef>(ccoord));
  }

}  // muSpectre

#endif /* FIELD_COLLECTION_GLOBAL_H */
