FROM gcc:9.3

# Set working directory to /app
WORKDIR /app

# Installing apt dependencies
RUN apt-get update -y \
    && apt-get install -y cmake \
        libboost-dev \
        libfreeimage-dev \
        libpoppler-cpp-dev \
        libtesseract-dev \
        ninja-build \
        python3 \
        python3-pip \
    \
    # Remove list of packages downloaded from the Ubuntu servers \
    && rm -rf /var/lib/apt/lists/* \
    \
    # Installing python dependencies with no cache option \
    && pip3 install --no-cache-dir setuptools \
        wheel \
        conan \
    \
    # Add remove to conan \
    && conan remote add lrde-public https://artifactory.lrde.epita.fr/artifactory/api/conan/lrde-public
