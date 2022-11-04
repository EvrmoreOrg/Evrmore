FROM amd64/ubuntu:20.04 AS base

#If you found this docker image helpful please donate RVN to the maintainer
LABEL maintainer="RV9zdNeUTQUToZUcRp9uNF8gwH5LzDFtan"

EXPOSE 8819/tcp
EXPOSE 8820/tcp

ENV DEBIAN_FRONTEND=noninteractive

#Install runtime dependencies
RUN apt-get update && \
	apt-get install -y --no-install-recommends \
	bash wget net-tools libminiupnpc17 \
	libevent-2.1 libevent-pthreads-2.1 \
	libboost-system1.71 libboost-filesystem1.71 libboost-chrono1.71 \
	libboost-program-options1.71 libboost-thread1.71 \
	libzmq5 && \
	apt-get clean

FROM base AS build

#Install build dependencies
RUN apt-get update && \
	apt-get install -y --no-install-recommends \
	bash net-tools build-essential libtool autotools-dev automake \
	pkg-config libssl-dev libevent-dev bsdmainutils python3 \
	libboost-system1.71-dev libboost-filesystem1.71-dev libboost-chrono1.71-dev \
	libboost-program-options1.71-dev libboost-test1.71-dev libboost-thread1.71-dev \
	libzmq3-dev libminiupnpc-dev && \
	apt-get clean

#Copy source dir
COPY . /home/evrmore/build/Evrmore/
WORKDIR /home/evrmore/build/Evrmore

#build db4 from source
WORKDIR /home/evrmore/build/Evrmore/contrib
RUN ./install_db4.sh ../../

# build Evrmore core
WORKDIR /home/evrmore/build/Evrmore
RUN ./autogen.sh && ./configure --disable-tests BDB_LIBS="-L/home/evrmore/build/db4/lib -ldb_cxx-4.8" BDB_CFLAGS="-I/home/evrmore/build/db4/include" --with-gui=no  && make -j4

FROM base AS final

#Add our service account user
RUN useradd -ms /bin/bash evrmore && \
	mkdir /var/lib/evrmore && \
	chown evrmore:evrmore /var/lib/evrmore && \
	ln -s /var/lib/evrmore /home/evrmore/.evrmore && \
	chown -h evrmore:evrmore /home/evrmore/.evrmore

VOLUME /var/lib/evrmore

#Copy the compiled binaries from the build
COPY --from=build /home/evrmore/build/Evrmore/src/evrmored /usr/local/bin/evrmored
COPY --from=build /home/evrmore/build/Evrmore/src/evrmore-cli /usr/local/bin/evrmore-cli

WORKDIR /home/evrmore
USER evrmore

CMD /usr/local/bin/evrmored -datadir=/var/lib/evrmore -printtoconsole -onlynet=ipv4
