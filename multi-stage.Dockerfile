FROM gcc:9.3 AS builder

# Set working directory to /app
WORKDIR /app

# Copying source code that will be used in build.sh script
COPY resources/code/ .

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
    # Add remote to conan \
    && conan remote add lrde-public https://artifactory.lrde.epita.fr/artifactory/api/conan/lrde-public \
    \
    # Call build.sh script \
    && sh build.sh

# ----- Back runner -------

# We used this image python because it is light enough
# and has >= 3.7 python versions
FROM python:3.7.10-slim

# Set working directory
WORKDIR /app

# Set environment variables
ENV LD_LIBRARY_PATH=/app/lib \
    LC_ALL=C

# Copying requirements.txt
COPY resources/requirements.txt requirements.txt

# Retrieve tar from builder
COPY --from=builder /app/build/soduco-py37-0.1.1-Linux.tar.gz /app

# Extract libstd++.so from builder
COPY --from=builder /usr/local/lib64/libstdc++.so.6 /usr/lib/x86_64-linux-gnu/

# Installing apt dependencies
RUN apt-get update -y \
    && apt-get install -y tesseract-ocr-fra \
        libfreeimage3 \
        enchant \
        libpoppler-cpp0v5 \
        aspell-fr \
        libtesseract4 \
    # Remove list of packages downloaded from the Ubuntu servers \
    && rm -rf /var/lib/apt/lists/* \
    \
    # Installing python dependencies with requirements.txt \
    && pip install --no-cache-dir -r requirements.txt \
    \
    # Untar the built tar \
    && tar xvf soduco-py37-0.1.1-Linux.tar.gz \
    \
    # Move lib and back directories \
    && mv soduco-py37-0.1.1-Linux/lib . \
    && mv soduco-py37-0.1.1-Linux/back . \
    \
    # Remove useless directories/files \
    && rm -rf soduco-py37-0.1.1-Linux soduco-py37-0.1.1-Linux.tar.gz

# Copying back, resources and server directories.
COPY resources/code/back back/
COPY resources/code/resources/ resources/
COPY resources/code/server/ server/

EXPOSE 8000

# The command to launch the server
CMD gunicorn -t 500 --bind 0.0.0.0:8000 --proxy-allow-from='*' server:app
