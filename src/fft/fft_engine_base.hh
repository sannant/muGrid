/**
 * @file   fft_engine_base.hh
 *
 * @author Till Junge <till.junge@epfl.ch>
 *
 * @date   01 Dec 2017
 *
 * @brief  Interface for FFT engines
 *
 * Copyright © 2017 Till Junge
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

#ifndef FFT_ENGINE_BASE_H
#define FFT_ENGINE_BASE_H

#include "common/common.hh"
#include "common/field_collection.hh"

namespace muSpectre {

  /**
   * Virtual base class for FFT engines. To be implemented by all
   * FFT_engine implementations.
   */
  template <Dim_t DimS, Dim_t DimM>
  class FFT_Engine_base
  {
  public:
    constexpr static Dim_t sdim{DimS}; //!< spatial dimension of the cell
    constexpr static Dim_t mdim{DimM}; //!< material dimension of the cell
    //! cell coordinates type
    using Ccoord = Ccoord_t<DimS>;
    //! spatial coordinates type
    using Rcoord = std::array<Real, DimS>;
    //! global FieldCollection
    using GFieldCollection_t = GlobalFieldCollection<DimS, DimM>;
    //! local FieldCollection (for Fourier-space pixels)
    using LFieldCollection_t = LocalFieldCollection<DimS, DimM>;
    //! Field type on which to apply the projection
    using Field_t = TensorField<GFieldCollection_t, Real, 2, DimM>;
    /**
     * Field type holding a Fourier-space representation of a
     * real-valued second-order tensor field
     */
    using Workspace_t = TensorField<LFieldCollection_t, Complex, 2, DimM>;
    /**
     * iterator over Fourier-space discretisation point
     */
    using iterator = typename LFieldCollection_t::iterator;

    //! Default constructor
    FFT_Engine_base() = delete;

    //! Constructor with cell resolutions
    FFT_Engine_base(Ccoord resolutions, Rcoord lengths);

    //! Copy constructor
    FFT_Engine_base(const FFT_Engine_base &other) = delete;

    //! Move constructor
    FFT_Engine_base(FFT_Engine_base &&other) = default;

    //! Destructor
    virtual ~FFT_Engine_base() = default;

    //! Copy assignment operator
    FFT_Engine_base& operator=(const FFT_Engine_base &other) = delete;

    //! Move assignment operator
    FFT_Engine_base& operator=(FFT_Engine_base &&other) = default;

    //! compute the plan, etc
    virtual void initialise(FFT_PlanFlags /*plan_flags*/);

    //! forward transform (dummy for interface)
    virtual Workspace_t & fft(Field_t & /*field*/) = 0;

    //! inverse transform (dummy for interface)
    virtual void ifft(Field_t & /*field*/) const = 0;

    /**
     * iterators over only those pixels that exist in frequency space
     * (i.e. about half of all pixels, see rfft)
     */
    //! returns an iterator to the first pixel in Fourier space
    inline iterator begin() {return this->work_space_container.begin();}
    //! returns an iterator past to the last pixel in Fourier space
    inline iterator end()  {return this->work_space_container.end();}

    //! nb of pixels (mostly for debugging)
    size_t size() const;
    //! nb of pixels in Fourier space
    size_t workspace_size() const;

    //! returns the resolutions of the cell
    const Ccoord & get_resolutions() const {return this->resolutions;}
    //! returns the physical sizes of the cell
    const Rcoord & get_lengths() const {return this->lengths;}

    //! only required for testing and debugging
    LFieldCollection_t & get_field_collection() {
      return this->work_space_container;}
    //! only required for testing and debugging
    Workspace_t& get_work_space() {return this->work;}

    //! factor by which to multiply projection before inverse transform (this is
    //! typically 1/nb_pixels for so-called unnormalized transforms (see,
    //! e.g. http://www.fftw.org/fftw3_doc/Multi_002dDimensional-DFTs-of-Real-Data.html#Multi_002dDimensional-DFTs-of-Real-Data
    //! or https://docs.scipy.org/doc/numpy-1.13.0/reference/routines.fft.html
    //! . Rather than scaling the inverse transform (which would cost one more
    //! loop), FFT engines provide this value so it can be used in the
    //! projection operator (where no additional loop is required)
    inline Real normalisation() const {return norm_factor;};

  protected:
    /**
     * Field collection in which to store fields associated with
     * Fourier-space points
     */
    LFieldCollection_t work_space_container{};
    const Ccoord resolutions; //!< resolutions of the cell
    const Rcoord lengths;     //!< physical sizes of the cell
    Workspace_t & work;       //!< field to store the Fourier transform of P
    const Real norm_factor; //!< normalisation coefficient of fourier transform
  private:
  };

}  // muSpectre

#endif /* FFT_ENGINE_BASE_H */
