#!/bin/sh
# Copyright (c) 2017-2019 The Bitcoin Core developers
# Copyright (c) 2022 The Evrmore Core developers
# Copyright (c) 2025 The Echelon Technology Group developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Install libdb4.8 (Berkeley DB v4.8.30).

set -e
export LC_ALL=C

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
PURPLE='\033[0;35m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[+]${NC} $1"
}

print_error() {
    echo -e "${RED}[!]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[*]${NC} $1"
}

# Check if running as root
if [ "$EUID" -eq 0 ]; then
   print_warning "Running as root. This is not recommended."
fi

# Parse command line arguments
if [ -z "${1}" ]; then
  echo "Usage: $0 <base-dir> [<extra-bdb-configure-flag> ...]"
  echo
  echo "Must specify a single argument: the directory in which db4 will be built."
  echo "This is probably \`pwd\` if you're at the root of the cryptocurrency repository."
  echo
  echo "Example: $0 /home/user/Evrmore"
  exit 1
fi

# Expand path function
expand_path() {
  cd "${1}" && pwd -P
}

# Set up variables
BDB_PREFIX="$(expand_path ${1})/db4"
shift
BDB_VERSION='db-4.8.30.NC'
BDB_VERSION_ZIP='db-4.8.30'  # ZIP file has different naming
BDB_HASH='12edc0df75bf9abd7f82f821795bcee50f42cb2e5f76a6a281b85732798364ef'
BDB_HASH_ZIP='4f538b56681a871cc71658ed9a6120081b74b474eaa73ba2c958abea04cc98ce'  # ZIP has different hash
BDB_URL="https://download.oracle.com/berkeley-db/${BDB_VERSION}.tar.gz"
BDB_URL_ZIP="https://download.oracle.com/berkeley-db/${BDB_VERSION_ZIP}.zip"

# Allow user to specify format preference via environment variable
USE_ZIP=${USE_ZIP:-false}

print_status "Berkeley DB 4.8.30 installation starting..."
print_status "Installation prefix: ${BDB_PREFIX}"

# Helper functions
check_exists() {
  command -v "$1" >/dev/null 2>&1
}

sha256_check() {
  if check_exists sha256sum; then
    echo "${1}  ${2}" | sha256sum -c
  elif check_exists sha256; then
    if [ "$(uname)" = "FreeBSD" ]; then
      sha256 -c "${1}" "${2}"
    else
      echo "${1}  ${2}" | sha256 -c
    fi
  else
    echo "${1}  ${2}" | shasum -a 256 -c
  fi
}

http_get() {
  if [ -f "${2}" ]; then
    print_status "File ${2} already exists; not downloading again"
  elif check_exists curl; then
    curl --insecure --retry 5 "${1}" -o "${2}"
  else
    wget --no-check-certificate "${1}" -O "${2}"
  fi

  sha256_check "${3}" "${2}"
}

# Install dependencies if needed
print_status "Checking for required tools..."
if ! check_exists make; then
    print_error "make not found. Please install build-essential"
    exit 1
fi

if ! check_exists g++; then
    print_error "g++ not found. Please install build-essential"
    exit 1
fi

# Create prefix directory
mkdir -p "${BDB_PREFIX}"

# Download and extract
if [ "${USE_ZIP}" = "true" ]; then
    print_status "Downloading Berkeley DB ${BDB_VERSION_ZIP}.zip..."
    http_get "${BDB_URL_ZIP}" "${BDB_VERSION_ZIP}.zip" "${BDB_HASH_ZIP}"

    # Check for unzip
    if ! check_exists unzip; then
        print_error "unzip not found. Please install it or use USE_ZIP=false for tar.gz"
        exit 1
    fi

    print_status "Extracting ZIP archive..."
    unzip -q "${BDB_VERSION_ZIP}.zip" -d "$BDB_PREFIX"
    cd "${BDB_PREFIX}/${BDB_VERSION_ZIP}/"
else
    print_status "Downloading Berkeley DB ${BDB_VERSION}.tar.gz..."
    http_get "${BDB_URL}" "${BDB_VERSION}.tar.gz" "${BDB_HASH}"

    print_status "Extracting TAR.GZ archive..."
    tar -xzf ${BDB_VERSION}.tar.gz -C "$BDB_PREFIX"
    cd "${BDB_PREFIX}/${BDB_VERSION}/"
fi

# Apply comprehensive C++11 compatibility patches
print_status "Applying C++11 compatibility patches..."

# First, rename __atomic_compare_exchange (with two underscores) to avoid conflicts
print_status "Renaming __atomic_compare_exchange to __atomic_compare_exchange_db..."
sed -i 's/__atomic_compare_exchange/__atomic_compare_exchange_db/g' dbinc/atomic.h

# Now rename atomic_compare_exchange (without underscores) - using word boundaries to avoid matching the above
print_status "Renaming atomic_compare_exchange to atomic_compare_exchange_db..."
sed -i 's/\<atomic_compare_exchange\>/atomic_compare_exchange_db/g' dbinc/atomic.h
sed -i 's/\<atomic_compare_exchange\>/atomic_compare_exchange_db/g' mutex/mut_method.c
sed -i 's/\<atomic_compare_exchange\>/atomic_compare_exchange_db/g' mutex/mut_win32.c
sed -i 's/\<atomic_compare_exchange\>/atomic_compare_exchange_db/g' mutex/mut_tas.c
sed -i 's/\<atomic_compare_exchange\>/atomic_compare_exchange_db/g' dbinc/mutex_int.h

# Now apply sed commands for atomic_init
print_status "Renaming atomic_init to atomic_init_db..."
sed -i 's/\<atomic_init\>/atomic_init_db/g' dbinc/atomic.h
sed -i 's/\<atomic_init\>/atomic_init_db/g' mp/mp_fget.c
sed -i 's/\<atomic_init\>/atomic_init_db/g' mp/mp_mvcc.c
sed -i 's/\<atomic_init\>/atomic_init_db/g' mp/mp_region.c
sed -i 's/\<atomic_init\>/atomic_init_db/g' mutex/mut_method.c
sed -i 's/\<atomic_init\>/atomic_init_db/g' mutex/mut_tas.c

# Update config.guess and config.sub
print_status "Updating config.guess and config.sub for modern systems..."

# Try to download from multiple sources
CONFIG_GUESS_URLS=(
    'https://raw.githubusercontent.com/gcc-mirror/gcc/master/config.guess'
    'https://git.savannah.gnu.org/cgit/config.git/plain/config.guess'
    'http://git.savannah.gnu.org/cgit/config.git/plain/config.guess'
)

CONFIG_SUB_URLS=(
    'https://raw.githubusercontent.com/gcc-mirror/gcc/master/config.sub'
    'https://git.savannah.gnu.org/cgit/config.git/plain/config.sub'
    'http://git.savannah.gnu.org/cgit/config.git/plain/config.sub'
)

# Function to try downloading from multiple URLs
download_with_fallback() {
    local filename="$1"
    shift
    local urls=("$@")

    for url in "${urls[@]}"; do
        print_status "Trying to download ${filename} from ${url}..."
        if check_exists curl; then
            if curl -fsSL "${url}" -o "${filename}.tmp" 2>/dev/null; then
                # Check if we got an HTML file instead of the script
                if ! head -n1 "${filename}.tmp" | grep -q "^#!" ; then
                    print_warning "Got HTML instead of script from ${url}, trying next source..."
                    rm -f "${filename}.tmp"
                    continue
                fi
                mv "${filename}.tmp" "${filename}"
                chmod +x "${filename}"
                print_status "Successfully downloaded ${filename}"
                return 0
            fi
        else
            if wget -q "${url}" -O "${filename}.tmp" 2>/dev/null; then
                if ! head -n1 "${filename}.tmp" | grep -q "^#!" ; then
                    print_warning "Got HTML instead of script from ${url}, trying next source..."
                    rm -f "${filename}.tmp"
                    continue
                fi
                mv "${filename}.tmp" "${filename}"
                chmod +x "${filename}"
                print_status "Successfully downloaded ${filename}"
                return 0
            fi
        fi
        rm -f "${filename}.tmp"
    done

    return 1
}

# Remove old files
rm -f "dist/config.guess" "dist/config.sub"

# Try to download config.guess
if ! download_with_fallback "dist/config.guess" "${CONFIG_GUESS_URLS[@]}"; then
    print_warning "Could not download config.guess from any source"
    print_warning "Continuing with original config.guess (build may fail on newer systems)"
fi

# Try to download config.sub
if ! download_with_fallback "dist/config.sub" "${CONFIG_SUB_URLS[@]}"; then
    print_warning "Could not download config.sub from any source"
    print_warning "Continuing with original config.sub (build may fail on newer systems)"
fi

# DO NOT Apply the mutex_fcntl fix from manual approach, this is causing a segmentation fault on QT when compiling on Ubuntu 25+, using POSIX pthreads flag instead
#print_status "Applying mutex_fcntl fix to dist/configure..."
#if [ -f "dist/configure" ]; then
#    # First, let's check if the line exists and what it looks like
#    if grep -n '\*mut_pthread\*|\*mut_tas\*|\*mut_win32\*)' dist/configure > /dev/null; then
#        # Apply the fix
#        sed -i 's/\*mut_pthread\*|\*mut_tas\*|\*mut_win32\*)/\*mut_pthread\*|\*mut_tas\*|\*mut_win32\*|\*mut_fcntl\*)/g' dist/configure
#        print_status "mutex_fcntl fix applied successfully"
#    else
#        print_warning "Could not find the expected pattern in dist/configure - skipping mutex_fcntl fix"
#    fi
#fi

# Configure and build
print_status "Configuring Berkeley DB..."
cd build_unix/

# Determine the correct source path based on which format we used
if [ "${USE_ZIP}" = "true" ]; then
    SOURCE_PATH="${BDB_PREFIX}/${BDB_VERSION_ZIP}"
else
    SOURCE_PATH="${BDB_PREFIX}/${BDB_VERSION}"
fi

"${SOURCE_PATH}/dist/configure" \
  --enable-cxx \
  --prefix="${BDB_PREFIX}" \
  --with-mutex=POSIX/pthreads \
  "${@}"

print_status "Building Berkeley DB (this may take a while)..."
make -j$(nproc)

print_status "Installing Berkeley DB..."
make install

# Display completion message
echo
print_status "Berkeley DB 4.8.30 build complete!"
echo
echo -e "Installation prefix: ${GREEN}${BDB_PREFIX}${NC}"
echo
echo "When compiling your cryptocurrency node (e.g., Evrmore), configure with:"
echo
echo "  export BDB_PREFIX='${BDB_PREFIX}'"
echo '  ./configure BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx-4.8" BDB_CFLAGS="-I${BDB_PREFIX}/include" ...'
echo
echo "Or directly:"
echo "  ./configure BDB_LIBS=\"-L${BDB_PREFIX}/lib -ldb_cxx-4.8\" BDB_CFLAGS=\"-I${BDB_PREFIX}/include\" ..."
echo

# Optional: Create a config file for easy sourcing
CONFIG_FILE="${BDB_PREFIX}/db4-config.sh"
cat > "${CONFIG_FILE}" << EOF
# Berkeley DB 4.8.30 Configuration
# Source this file before building your cryptocurrency node
# Usage: source ${CONFIG_FILE}

export BDB_PREFIX='${BDB_PREFIX}'
export BDB_LIBS="-L\${BDB_PREFIX}/lib -ldb_cxx-4.8"
export BDB_CFLAGS="-I\${BDB_PREFIX}/include"

echo "Berkeley DB 4.8.30 environment configured:"
echo "  BDB_PREFIX: \${BDB_PREFIX}"
echo "  BDB_LIBS: \${BDB_LIBS}"
echo "  BDB_CFLAGS: \${BDB_CFLAGS}"
EOF

print_status "Configuration saved to: ${CONFIG_FILE}"
echo -e "You can source this file before building: ${GREEN}source ${CONFIG_FILE}${NC}"
echo
echo -e "${RED}(㇏(•̀ᵥᵥ•́)ノ)${PURPLE} This shit ain't nothin' to me man!${NC}"
echo
