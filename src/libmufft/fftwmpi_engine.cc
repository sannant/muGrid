/**
 * @file   fftwmpi_engine.cc
 *
 * @author Lars Pastewka <lars.pastewka@imtek.uni-freiburg.de>
 *
 * @date   06 Mar 2017
 *
 * @brief  implements the MPI-parallel fftw engine
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

#include "fftwmpi_engine.hh"
#include <libmugrid/ccoord_operations.hh>

namespace muFFT {

  int FFTWMPIEngine::nb_engines{0};

  FFTWMPIEngine::FFTWMPIEngine(DynCcoord_t nb_grid_pts, Dim_t nb_dof_per_pixel,
                               Communicator comm)
      : Parent{nb_grid_pts, nb_dof_per_pixel, comm},
        plan_fft{nullptr}, plan_ifft{nullptr}, real_workspace{nullptr} {
    if (!this->nb_engines)
      fftw_mpi_init();
    this->nb_engines++;

    int dim = this->nb_domain_grid_pts.get_dim();
    std::vector<ptrdiff_t> narr(dim);
    for (Dim_t i = 0; i < dim; ++i) {
      narr[i] = this->nb_domain_grid_pts[dim - 1 - i];
    }
    narr[dim - 1] = this->nb_domain_grid_pts[0] / 2 + 1;
    ptrdiff_t res_x, loc_x, res_y, loc_y;
    this->workspace_size = fftw_mpi_local_size_many_transposed(
        dim, narr.data(), this->nb_dof_per_pixel, FFTW_MPI_DEFAULT_BLOCK,
        FFTW_MPI_DEFAULT_BLOCK, this->comm.get_mpi_comm(), &res_x, &loc_x,
        &res_y, &loc_y);
    // A factor of two is required because we are using the c2r/r2c DFTs.
    // See:
    // http://www.fftw.org/fftw3_doc/Multi_002ddimensional-MPI-DFTs-of-Real-Data.html
    this->workspace_size *= 2;
    if (dim > 1) {
      this->nb_fourier_grid_pts[dim-2] = this->nb_fourier_grid_pts[dim-1];
      this->fourier_locations[dim-2] = this->fourier_locations[dim-1];
    }
    this->nb_subdomain_grid_pts[dim-1] = res_x;
    this->subdomain_locations[dim-1] = loc_x;
    this->nb_fourier_grid_pts[dim-1] = res_y;
    this->fourier_locations[dim-1] = loc_y;

    for (auto & n : this->nb_subdomain_grid_pts) {
      if (n == 0) {
        this->active = false;
      }
    }
    for (auto & n : this->nb_fourier_grid_pts) {
      if (n == 0) {
        this->active = false;
      }
    }
  }

  /* ---------------------------------------------------------------------- */
  void FFTWMPIEngine::initialise(FFT_PlanFlags plan_flags) {
    if (this->initialised) {
      throw std::runtime_error("double initialisation, will leak memory");
    }

    /*
     * Initialize parent after the local number of grid points in each direction
     * have been determined and work space has been initialized
     */
    Parent::initialise(plan_flags);

    this->real_workspace = fftw_alloc_real(this->workspace_size);
    /*
     * We need to check whether the workspace provided by our field is large
     * enough. MPI parallel FFTW may request a workspace size larger than the
     * nominal size of the complex buffer.
     */
    if (static_cast<int>(this->work.size() * this->nb_dof_per_pixel) <
        this->workspace_size) {
      this->work.set_pad_size(this->workspace_size -
                              this->nb_dof_per_pixel * this->work.size());
    }

    unsigned int flags;
    switch (plan_flags) {
    case FFT_PlanFlags::estimate: {
      flags = FFTW_ESTIMATE;
      break;
    }
    case FFT_PlanFlags::measure: {
      flags = FFTW_MEASURE;
      break;
    }
    case FFT_PlanFlags::patient: {
      flags = FFTW_PATIENT;
      break;
    }
    default:
      throw std::runtime_error("unknown planner flag type");
      break;
    }

    int dim = this->nb_domain_grid_pts.get_dim();
    std::vector<ptrdiff_t> narr(dim);
    for (Dim_t i = 0; i < dim; ++i) {
      narr[i] = this->nb_domain_grid_pts[dim - 1 - i];
    }
    Real * in{this->real_workspace};
    fftw_complex * out{reinterpret_cast<fftw_complex *>(this->work.data())};
    this->plan_fft = fftw_mpi_plan_many_dft_r2c(
        dim, narr.data(), this->nb_dof_per_pixel, FFTW_MPI_DEFAULT_BLOCK,
        FFTW_MPI_DEFAULT_BLOCK, in, out, this->comm.get_mpi_comm(),
        FFTW_MPI_TRANSPOSED_OUT | flags);
    if (this->plan_fft == nullptr) {
      if (dim == 1)
        throw std::runtime_error("r2c plan failed; MPI parallel FFTW does not "
                                 "support 1D r2c FFTs");
      else
        throw std::runtime_error("r2c plan failed");
    }

    fftw_complex * i_in = reinterpret_cast<fftw_complex *>(this->work.data());
    Real * i_out = this->real_workspace;

    this->plan_ifft = fftw_mpi_plan_many_dft_c2r(
        dim, narr.data(), this->nb_dof_per_pixel, FFTW_MPI_DEFAULT_BLOCK,
        FFTW_MPI_DEFAULT_BLOCK, i_in, i_out, this->comm.get_mpi_comm(),
        FFTW_MPI_TRANSPOSED_IN | flags);
    if (this->plan_ifft == nullptr) {
      if (dim == 1)
        throw std::runtime_error("c2r plan failed; MPI parallel FFTW does not "
                                 "support 1D c2r FFTs");
      else
        throw std::runtime_error("c2r plan failed");
    }
    this->initialised = true;
  }

  /* ---------------------------------------------------------------------- */
  FFTWMPIEngine::~FFTWMPIEngine() noexcept {
    if (this->real_workspace != nullptr)
      fftw_free(this->real_workspace);
    if (this->plan_fft != nullptr)
      fftw_destroy_plan(this->plan_fft);
    if (this->plan_ifft != nullptr)
      fftw_destroy_plan(this->plan_ifft);
    // TODO(junge): We cannot run fftw_mpi_cleanup since also calls fftw_cleanup
    // and any running FFTWEngine will fail afterwards.
    // this->nb_engines--;
    // if (!this->nb_engines) fftw_mpi_cleanup();
  }

  /* ---------------------------------------------------------------------- */
  typename FFTWMPIEngine::Workspace_t &
  FFTWMPIEngine::fft(Field_t & field) {
    if (this->plan_fft == nullptr) {
      throw std::runtime_error("fft plan not initialised");
    }
    if (field.size() !=
        muGrid::CcoordOps::get_size(this->nb_subdomain_grid_pts)) {
      std::stringstream error;
      error << "The size of the field passed to the forward FFT is "
            << field.size() << " and does not match the size "
            << muGrid::CcoordOps::get_size(this->nb_subdomain_grid_pts)
            << " of the (sub)domain handled by FFTWMPIEngine.";
      throw std::runtime_error(error.str());
    }
    // Copy non-padded field to padded real_workspace.
    // Transposed output of M x N x L transform for >= 3 dimensions is padded
    // M x N x 2*(L/2+1).
    ptrdiff_t fstride =
        (this->nb_dof_per_pixel * this->nb_subdomain_grid_pts[0]);
    ptrdiff_t wstride = (this->nb_dof_per_pixel * 2 *
                         (this->nb_subdomain_grid_pts[0] / 2 + 1));
    ptrdiff_t n = field.size() / this->nb_subdomain_grid_pts[0];

    auto fdata = field.data();
    auto wdata = this->real_workspace;
    for (int i = 0; i < n; i++) {
      std::copy(fdata, fdata + fstride, wdata);
      fdata += fstride;
      wdata += wstride;
    }
    // Compute FFT
    fftw_mpi_execute_dft_r2c(
        this->plan_fft, this->real_workspace,
        reinterpret_cast<fftw_complex *>(this->work.data()));
    return this->work;
  }

  /* ---------------------------------------------------------------------- */
  void FFTWMPIEngine::ifft(Field_t & field) const {
    if (this->plan_ifft == nullptr) {
      throw std::runtime_error("ifft plan not initialised");
    }
    if (field.size() !=
        muGrid::CcoordOps::get_size(this->nb_subdomain_grid_pts)) {
      std::stringstream error;
      error << "The size of the field passed to the forward FFT is "
            << field.size() << " and does not match the size "
            << muGrid::CcoordOps::get_size(this->nb_subdomain_grid_pts)
            << " of the (sub)domain handled by FFTWMPIEngine.";
      throw std::runtime_error(error.str());
    }
    // Compute inverse FFT
    fftw_mpi_execute_dft_c2r(
        this->plan_ifft, reinterpret_cast<fftw_complex *>(this->work.data()),
        this->real_workspace);
    // Copy non-padded field to padded real_workspace.
    // Transposed output of M x N x L transform for >= 3 dimensions is padded
    // M x N x 2*(L/2+1).
    ptrdiff_t fstride{this->nb_dof_per_pixel *
                      this->nb_subdomain_grid_pts[0]};
    ptrdiff_t wstride{this->nb_dof_per_pixel * 2 *
                      (this->nb_subdomain_grid_pts[0] / 2 + 1)};
    ptrdiff_t n(field.size() / this->nb_subdomain_grid_pts[0]);

    auto fdata{field.data()};
    auto wdata{this->real_workspace};
    for (int i = 0; i < n; i++) {
      std::copy(wdata, wdata + fstride, fdata);
      fdata += fstride;
      wdata += wstride;
    }
  }

}  // namespace muFFT
