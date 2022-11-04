 #!/usr/bin/env bash

 # Execute this file to install the evrmore cli tools into your path on OS X

 CURRENT_LOC="$( cd "$(dirname "$0")" ; pwd -P )"
 LOCATION=${CURRENT_LOC%Evrmore-Qt.app*}

 # Ensure that the directory to symlink to exists
 sudo mkdir -p /usr/local/bin

 # Create symlinks to the cli tools
 sudo ln -s ${LOCATION}/Evrmore-Qt.app/Contents/MacOS/evrmored /usr/local/bin/evrmored
 sudo ln -s ${LOCATION}/Evrmore-Qt.app/Contents/MacOS/evrmore-cli /usr/local/bin/evrmore-cli
