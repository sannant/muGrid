/**
 * @file   mpi_test_fft_engine.cc
 *
 * @author Lars Pastewka <lars.pastewka@imtek.uni-freiburg.de>
 *
 * @date   06 Mar 2017
 *
 * @brief  tests for MPI-parallel fft engine implementations
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

#include <boost/mpl/list.hpp>

#include "tests.hh"
#include "mpi_context.hh"
#include "fft/fftwmpi_engine.hh"
#include "common/ccoord_operations.hh"
#include "common/field_collection.hh"
#include "common/field_map.hh"
#include "common/iterators.hh"

namespace muSpectre {

  BOOST_AUTO_TEST_SUITE(fftwmpi_engine);

  /* ---------------------------------------------------------------------- */
  template <typename Engine, Dim_t resolution>
  struct FFTW_fixture {
    constexpr static Dim_t box_resolution{resolution};
    constexpr static Real box_length{4.5};
    constexpr static Dim_t sdim{Engine::sdim};
    constexpr static Dim_t mdim{Engine::mdim};
    constexpr static Ccoord_t<sdim> res() {
      return CcoordOps::get_cube<sdim>(box_resolution);
    }
    FFTW_fixture(): engine(res(), CcoordOps::get_cube<sdim>(box_length),
                           MPIContext::get_context().comm) {}
    Engine engine;
  };

  template <typename Engine>
  struct FFTW_fixture_python_segfault{
    constexpr static Dim_t  dim{twoD};
    constexpr static Dim_t sdim{twoD};
    constexpr static Dim_t mdim{twoD};
    constexpr static Ccoord_t<sdim> res() {return {6, 4};}
    FFTW_fixture_python_segfault():
      engine{res(), {3., 3}, MPIContext::get_context().comm} {}
    Engine engine;
  };

  using fixlist = boost::mpl::list<FFTW_fixture<FFTWMPIEngine<  twoD,   twoD>, 3>,
                                   FFTW_fixture<FFTWMPIEngine<  twoD, threeD>, 3>,
                                   FFTW_fixture<FFTWMPIEngine<threeD, threeD>, 3>,
                                   FFTW_fixture<FFTWMPIEngine<  twoD,   twoD>, 4>,
                                   FFTW_fixture<FFTWMPIEngine<  twoD, threeD>, 4>,
                                   FFTW_fixture<FFTWMPIEngine<threeD, threeD>, 4>,
                                   FFTW_fixture_python_segfault<FFTWMPIEngine<twoD, twoD>>>;


  /* ---------------------------------------------------------------------- */
  BOOST_FIXTURE_TEST_CASE_TEMPLATE(Constructor_test, Fix, fixlist, Fix) {
    Communicator &comm = MPIContext::get_context().comm;
    BOOST_CHECK_NO_THROW(Fix::engine.initialise(FFT_PlanFlags::estimate));
    BOOST_CHECK_EQUAL(comm.sum(Fix::engine.size()),
                      CcoordOps::get_size(Fix::res()));
  }

  /* ---------------------------------------------------------------------- */
  BOOST_FIXTURE_TEST_CASE_TEMPLATE(fft_test, Fix, fixlist, Fix) {
    Fix::engine.initialise(FFT_PlanFlags::estimate);
    constexpr Dim_t order{2};
    using FC_t = GlobalFieldCollection<Fix::sdim, Fix::mdim>;
    FC_t fc;
    auto & input{make_field<TensorField<FC_t, Real, order, Fix::mdim>>("input", fc)};
    auto & ref  {make_field<TensorField<FC_t, Real, order, Fix::mdim>>("reference", fc)};
    auto & result{make_field<TensorField<FC_t, Real, order, Fix::mdim>>("result", fc)};
    fc.initialise(Fix::engine.get_resolutions());

    using map_t = MatrixFieldMap<FC_t, Real, Fix::mdim, Fix::mdim>;
    map_t inmap{input};
    auto refmap{map_t{ref}};
    auto resultmap{map_t{result}};
    size_t cntr{0};
    for (auto tup: akantu::zip(inmap, refmap)) {
      cntr++;
      auto & in_{std::get<0>(tup)};
      auto & ref_{std::get<1>(tup)};
      in_.setRandom();
      ref_ = in_;
    }
    auto & complex_field = Fix::engine.fft(input);
    using cmap_t = MatrixFieldMap<LocalFieldCollection<Fix::sdim, Fix::mdim>, Complex, Fix::mdim, Fix::mdim>;
    cmap_t complex_map(complex_field);
    Real error = complex_map[0].imag().norm();
    BOOST_CHECK_LT(error, tol);

    /* make sure, the engine has not modified input (which is
       unfortunately const-casted internally, hence this test) */
    for (auto && tup: akantu::zip(inmap, refmap)) {
      Real error{(std::get<0>(tup) - std::get<1>(tup)).norm()};
      BOOST_CHECK_LT(error, tol);
    }

    /* make sure that the ifft of fft returns the original*/
    Fix::engine.ifft(result);
    for (auto && tup: akantu::zip(resultmap, refmap)) {
      Real error{(std::get<0>(tup)*Fix::engine.normalisation() - std::get<1>(tup)).norm()};
      BOOST_CHECK_LT(error, tol);
      if (error > tol) {
        std::cout << std::get<0>(tup).array()/std::get<1>(tup).array() << std::endl << std::endl;
      }
    }
  }

  BOOST_AUTO_TEST_SUITE_END();

}  // muSpectre
