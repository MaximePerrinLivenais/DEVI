FROM gcc:9.3 AS builder

WORKDIR /app

COPY resources/code/ .

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
    && conan remote add lrde-public https://artifactory.lrde.epita.fr/artifactory/api/conan/lrde-public \
    && sh build.sh

# ----- Back runner -------

FROM python:3.7.10-slim
WORKDIR /app

ENV LD_LIBRARY_PATH=/app/lib \
    LC_ALL=C

COPY resources/requirements.txt requirements.txt
COPY --from=builder /app/build/soduco-py37-0.1.1-Linux.tar.gz /app

COPY --from=builder /usr/local/lib64/libstdc++.so.6 /usr/lib/x86_64-linux-gnu/

RUN apt-get update -y \
    && apt-get install -y tesseract-ocr-fra \
        libfreeimage3 \
        enchant \
        libpoppler-cpp0v5 \
        aspell-fr \
        libtesseract4 \
    && rm -rf /var/lib/apt/lists/* \
    && pip install --no-cache-dir -r requirements.txt \
    && tar xvf soduco-py37-0.1.1-Linux.tar.gz \
    && mv soduco-py37-0.1.1-Linux/lib . \
    && mv soduco-py37-0.1.1-Linux/back . \
    && rm -rf soduco-py37-0.1.1-Linux soduco-py37-0.1.1-Linux.tar.gz

COPY resources/code/back back/
COPY resources/code/resources/ resources/
COPY resources/code/server/ server/

EXPOSE 8000

CMD gunicorn -t 500 --bind 0.0.0.0:8000 --proxy-allow-from='*' server:app
