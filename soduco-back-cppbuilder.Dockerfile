FROM gcc:9.3
RUN apt-get update -y \
    && apt-get install -y cmake \
        libboost-dev \
        libfreeimage-dev \
        libpoppler-cpp-dev \
        libtesseract-dev \
        ninja-build \
        python3 \
        python3-pip \
    && pip3 install setuptools \
        wheel \
        conan \
    && conan remote add lrde-public https://artifactory.lrde.epita.fr/artifactory/api/conan/lrde-public
