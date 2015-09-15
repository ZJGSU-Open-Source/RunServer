FROM golang:1.4.2
MAINTAINER Sakeven "sakeven.jiang@daocloud.io"

# Install base software.
RUN \
  sed -i 's/# \(.*multiverse$\)/\1/g' /etc/apt/sources.list && \
  apt-get update && \
  apt-get -y upgrade && \
  apt-get install -y build-essential flex openjdk-7-jdk && \
  rm -rf /var/lib/apt/lists/*


# Build OJ
RUN \
  cd $GOPATH/src/RunServer && \
  ./make.sh

# Define working directory.
WORKDIR $GOPATH/src

# Expose ports
EXPOSE 8888

CMD RunServer