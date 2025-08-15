package=bdb
$(package)_version=4.8.30
$(package)_download_path=https://download.oracle.com/berkeley-db
$(package)_file_name=db-$($(package)_version).NC.tar.gz
$(package)_sha256_hash=12edc0df75bf9abd7f82f821795bcee50f42cb2e5f76a6a281b85732798364ef
$(package)_build_subdir=build_unix

define $(package)_set_vars
$(package)_config_opts=--disable-shared --enable-cxx --disable-replication
$(package)_config_opts_mingw32=--enable-mingw --with-mutex=win32/gcc
$(package)_config_opts_linux=--with-pic --with-mutex=POSIX/pthreads
$(package)_cxxflags=-std=c++11
endef

# NOTE: fcntl mutex support fix for future reference
# On Ubuntu 25+ and other modern systems, BDB 4.8.30's fcntl mutex implementation causes segfaults, at least in the qt.
# We currently force pthread mutexes with --with-mutex=POSIX/pthreads above.
# If fcntl mutex support needs to be re-enabled in the future (not recommended), add this to preprocess_cmds:
#   chmod +w dist/configure && \
#   sed -i 's/\*mut_pthread\*|\*mut_tas\*|\*mut_win32\*)/\*mut_pthread\*|\*mut_tas\*|\*mut_win32\*|\*mut_fcntl\*)/g' dist/configure && \
#   chmod -w dist/configure && \
# However, be aware that the fcntl mutex implementation in BDB 4.8.30 has known issues with modern kernels
# and will likely cause segmentation faults. Consider upgrading to a newer BDB version instead.

define $(package)_preprocess_cmds
  sed -i.old 's/__atomic_compare_exchange/__atomic_compare_exchange_db/g' dbinc/atomic.h && \
  sed -i.old 's/\<atomic_compare_exchange\>/atomic_compare_exchange_db/g' dbinc/atomic.h dbinc/mutex_int.h mutex/mut_method.c mutex/mut_tas.c mutex/mut_win32.c && \
  sed -i.old 's/\<atomic_init\>/atomic_init_db/g' dbinc/atomic.h mp/mp_region.c mp/mp_mvcc.c mp/mp_fget.c mutex/mut_method.c mutex/mut_tas.c && \
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub dist
endef

define $(package)_config_cmds
  ../dist/$($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE) libdb_cxx-4.8.a libdb-4.8.a
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install_lib install_include
endef
