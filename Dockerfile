FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /app
RUN apt-get update && apt-get install -y build-essential cmake git libssl-dev
RUN git clone --recurse-submodules https://github.com/Psyhich/rusnia_doser.git && \
    cd rusnia_doser && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make

FROM ubuntu:20.04  
ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /app
RUN apt-get update && apt-get install -y openssl
COPY --from=0 /app/rusnia_doser/build/rusnia_doser ./rusnia_doser
CMD ["/app/rusnia_doser"]
