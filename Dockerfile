FROM ubuntu:latest
RUN apt update -y
RUN apt -y install cmake build-essential libboost-all-dev
#RUN apt -y install sphinx python3-sphinx-rtd-theme

COPY . /mriFilter

RUN mkdir /mriBin
RUN mkdir /data

WORKDIR /mriBin
RUN cmake ../mriFilter
RUN make -j20
#WORKDIR /mriBin/docs
#RUN make html

WORKDIR /data

# ENTRYPOINT ["mpirun", "--allow-run-as-root", "-np", "4", "/mriBin/bin/mpfilterapp"]