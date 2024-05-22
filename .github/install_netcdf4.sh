curl -L https://download.gnome.org/sources/libxml2/2.12/libxml2-2.12.7.tar.xz | tar -Jx
cd libxml2-2.12.7
./configure --without-python
make
make install
curl -L https://downloads.unidata.ucar.edu/netcdf-c/4.9.2/netcdf-c-4.9.2.tar.gz | tar -zx
cd netcdf-c-4.9.2
./configure --disable-hdf5 --disable-byterange --disable-shared --with-pic
make
make install
