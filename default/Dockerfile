FROM ubuntu:20.04

RUN apt-get -y update && apt-get -y upgrade \
    && apt-get -y install make gcc g++ git

COPY ./app ~
RUN chmod +x ~/Makefile && \
    chmod +x ~/run.sh && \
    cd ~ && make
ENTRYPOINT ~/run.sh
