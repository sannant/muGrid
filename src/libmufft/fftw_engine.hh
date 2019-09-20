/**
 * @file   fftw_engine.hh
 *
 * @author Till Junge <till.junge@altermail.ch>
 *
 * @date   03 Dec 2017
 *
 * @brief  FFT engine using FFTW
 *
 * Copyright © 2017 Till Junge
 *
 * µFFT is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3, or (at
 * your option) any later version.
 *
 * µFFT is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with µFFT; see the file COPYING. If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * * Boston, MA 02111-1307, USA.
 *
 * Additional permission under GNU GPL version 3 section 7
 *
 * If you modify this Program, or any covered work, by linking or combining it
 * with proprietary FFT implementations or numerical libraries, containing parts
 * covered by the terms of those libraries' licenses, the licensors of this
 * Program grant you additional permission to convey the resulting work.
 *
 */

#ifndef SRC_LIBMUFFT_FFTW_ENGINE_HH_
#define SRC_LIBMUFFT_FFTW_ENGINE_HH_

#include "fft_engine_base.hh"

#include <fftw3.h>

namespace muFFT {

  /**
   * implements the `muFFT::FftEngine_Base` interface using the
   * FFTW library
   */
  class FFTWEngine : public FFTEngineBase {
   public:
    using Parent = FFTEngineBase;       //!< base class
    //! field for Fourier transform of second-order tensor
    using Workspace_t = typename Parent::Workspace_t;
    //! real-valued second-order tensor
    using Field_t = typename Parent::Field_t;
    //! Default constructor
    FFTWEngine() = delete;

    /**
     * Constructor with the domain's number of grid points in each direciton,
     * the number of components to transform, and the communicator
     */
    FFTWEngine(const DynCcoord_t & nb_grid_pts, Dim_t nb_dof_per_pixel,
               Communicator comm = Communicator());

    //! Copy constructor
    FFTWEngine(const FFTWEngine & other) = delete;

    //! Move constructor
    FFTWEngine(FFTWEngine && other) = delete;

    //! Destructor
    virtual ~FFTWEngine() noexcept;

    //! Copy assignment operator
    FFTWEngine & operator=(const FFTWEngine & other) = delete;

    //! Move assignment operator
    FFTWEngine & operator=(FFTWEngine && other) = delete;

    // compute the plan, etc
    void initialise(FFT_PlanFlags plan_flags) override;

    //! forward transform
    Workspace_t & fft(Field_t & field) override;

    //! inverse transform
    void ifft(Field_t & field) const override;

   protected:
    fftw_plan plan_fft{};     //!< holds the plan for forward fourier transform
    fftw_plan plan_ifft{};    //!< holds the plan for inverse fourier transform
    bool initialised{false};  //!< to prevent double initialisation
  };

}  // namespace muFFT

#endif  // SRC_LIBMUFFT_FFTW_ENGINE_HH_
