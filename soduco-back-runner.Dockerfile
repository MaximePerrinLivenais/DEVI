FROM python:3.7.10-slim
WORKDIR /app/

ENV LD_LIBRARY_PATH=/app/lib \
    LC_ALL=C

COPY resources/requirements.txt requirements.txt

ADD resources/code/build/soduco-py37-0.1.1-Linux.tar.gz .
COPY --from=soduco/back_builder:latest /usr/local/lib64/libstdc++.so.6 /usr/lib/x86_64-linux-gnu/

RUN apt-get update -y \
    && apt-get install -y tesseract-ocr-fra \
        libfreeimage3 \
        enchant \
        libpoppler-cpp0v5 \
        aspell-fr \
        libtesseract4 \
    && rm -rf /var/lib/apt/lists/* \
    && pip install --no-cache-dir -r requirements.txt \
    #&& mkdir -p /data/annotations \
    && mv soduco-py37-0.1.1-Linux/lib . \
    && mv soduco-py37-0.1.1-Linux/back . \
    && rm -rf soduco-py37-0.1.1-Linux

COPY resources/code/back back/
COPY resources/code/resources/ resources/
COPY resources/code/server/ server/

CMD gunicorn -t 500 --bind 0.0.0.0:8000 --proxy-allow-from='*' server:app
