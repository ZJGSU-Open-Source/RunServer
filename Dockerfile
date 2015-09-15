FROM golang:1.4.2
MAINTAINER Sakeven "sakeven.jiang@daocloud.io"

# Install base software.
RUN \
  sed -i 's/# \(.*multiverse$\)/\1/g' /etc/apt/sources.list && \
  apt-get update && \
  apt-get -y upgrade && \
  apt-get install -y build-essential flex openjdk-7-jdk && \
  rm -rf /var/lib/apt/lists/*


ADD . $GOPATH/src/RunServer

RUN git clone https://github.com/ZJGSU-Open-Source/vjudger.git $GOPATH/src/vjudger
RUN git clone https://github.com/ZJGSU-Open-Source/GoOnlineJudge.git $GOPATH/src/GoOnlineJudge
RUN git clone https://github.com/sakeven/restweb.git $GOPATH/src/restweb
RUN go get -t RunServer

# Build OJ
RUN \
  cd $GOPATH/src/RunServer && \
  ./make.sh

# Define working directory.
WORKDIR $GOPATH/src

# Expose ports
EXPOSE 8888

CMD RunServer