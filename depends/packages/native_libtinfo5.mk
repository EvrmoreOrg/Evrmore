package=native_libtinfo5
$(package)_version=6.3-2ubuntu0.1
$(package)_download_path=http://archive.ubuntu.com/ubuntu/pool/universe/n/ncurses/
$(package)_file_name=libtinfo5_$($(package)_version)_amd64.deb
$(package)_sha256_hash=ab89265d8dd18bda6a29d7c796367d6d9f22a39a8fa83589577321e7caf3857b

define $(package)_extract_cmds
  cd $($(package)_source_dir) && \
  ar x $($(package)_file_name) && \
  mkdir -p $($(package)_extract_dir) && \
  if [ -f data.tar.zst ]; then \
    tar -xf data.tar.zst -C $($(package)_extract_dir); \
  else \
    tar -xf data.tar.xz -C $($(package)_extract_dir); \
  fi
endef

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/lib && \
  cp -P $($(package)_extract_dir)/lib/x86_64-linux-gnu/libtinfo.so.5* $($(package)_staging_prefix_dir)/lib/
endef

define $(package)_postprocess_cmds
  cd $($(package)_staging_prefix_dir)/lib && \
  if [ ! -e libtinfo.so.5 ]; then \
    ln -sf libtinfo.so.5.* libtinfo.so.5; \
  fi
endef
