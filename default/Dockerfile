FROM ubuntu:20.04

RUN apt-get -y update && apt-get -y upgrade \
    && apt-get -y install make gcc g++ git gdb

COPY ./app .
RUN chmod +x Makefile && \
    chmod +x run.sh && make
ENTRYPOINT /run.sh
