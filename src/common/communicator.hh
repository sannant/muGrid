/**
 * @file   communicator.hh
 *
 * @author Lars Pastewka <lars.pastewka@imtek.uni-freiburg.de>
 *
 * @date   07 Mar 2018
 *
 * @brief  abstraction layer for the distributed memory communicator object 
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

#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#ifdef WITH_MPI
#include <mpi.h>
#endif

namespace muSpectre {

#ifdef WITH_MPI

  template<typename T> decltype(auto) mpi_type() { };
  template<> inline decltype(auto) mpi_type<char>() { return MPI_CHAR; }
  template<> inline decltype(auto) mpi_type<short>() { return MPI_SHORT; }
  template<> inline decltype(auto) mpi_type<int>() { return MPI_INT; }
  template<> inline decltype(auto) mpi_type<long>() { return MPI_LONG; }
  template<> inline decltype(auto) mpi_type<unsigned char>() { return MPI_UNSIGNED_CHAR; }
  template<> inline decltype(auto) mpi_type<unsigned short>() { return MPI_UNSIGNED_SHORT; }
  template<> inline decltype(auto) mpi_type<unsigned int>() { return MPI_UNSIGNED; }
  template<> inline decltype(auto) mpi_type<unsigned long>() { return MPI_UNSIGNED_LONG; }
  template<> inline decltype(auto) mpi_type<float>() { return MPI_FLOAT; }
  template<> inline decltype(auto) mpi_type<double>() { return MPI_DOUBLE; }

  //! lightweight abstraction for the MPI communicator object
  class Communicator {
  public:
    Communicator(MPI_Comm comm=MPI_COMM_NULL): comm{comm} {};
    ~Communicator() {};

    //! get rank of present process
    int rank() const {
      if (comm == MPI_COMM_NULL) return 0;
      int res;
      MPI_Comm_rank(this->comm, &res);
      return res;
    }

    //! sum reduction on scalar types
    template<typename T>
    T sum(const T &arg) const {
      if (comm == MPI_COMM_NULL) return arg;
      T res;
      MPI_Allreduce(&arg, &res, 1, mpi_type<T>(), MPI_SUM, this->comm);
      return res;
    }

    MPI_Comm get_mpi_comm() { return this->comm; }

  private:
    MPI_Comm comm;
  };

#else /* WITH_MPI */

  //! stub communicator object that doesn't communicate anything
  class Communicator {
  public:
    Communicator() {};
    ~Communicator() {};

    //! get rank of present process
    int rank() {
      return 0;
    }

    //! sum reduction on scalar types
    template<typename T>
    T sum(const T &arg) { return arg; }
  };

#endif

}

#endif /* COMMUNICATOR_H */
