FROM gcc:9.3

WORKDIR /app

RUN apt-get update -y \
    && apt-get install -y cmake \
        libboost-dev \
        libfreeimage-dev \
        libpoppler-cpp-dev \
        libtesseract-dev \
        ninja-build \
        python3 \
        python3-pip \
    && rm -rf /var/lib/apt/lists/* \
    && pip3 install --no-cache-dir setuptools \
        wheel \
        conan \
    && conan remote add lrde-public https://artifactory.lrde.epita.fr/artifactory/api/conan/lrde-public
