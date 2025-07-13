FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y g++ cmake make libcurl4-openssl-dev git libsqlite3-dev zlib1g-dev libminizip-dev

RUN git clone --depth 1 --branch v3.4.0 https://github.com/catchorg/Catch2.git /usr/local/include/Catch2
RUN git clone --depth 1 --branch v3.11.2 https://github.com/nlohmann/json.git /usr/local/include/json

WORKDIR /app
COPY . .