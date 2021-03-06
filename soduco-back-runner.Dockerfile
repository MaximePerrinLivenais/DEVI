FROM python:3.7.10-slim
WORKDIR /app/

ENV LD_LIBRARY_PATH=/app/lib \
    LC_ALL=C

COPY resources/code/back back/
COPY resources/code/resources/ resources/
COPY resources/code/server/ server/

COPY resources/code/build/soduco-py37-0.1.1-Linux/back back/
COPY resources/code/build/soduco-py37-0.1.1-Linux/lib lib/
COPY --from=soduco/back_builder:latest /usr/local/lib64/libstdc++.so.6 /usr/lib/x86_64-linux-gnu/

RUN apt-get update -y \
    && apt-get install -y tesseract-ocr-fra \
        libfreeimage3 \
        enchant \
        libpoppler-cpp0v5 \
        aspell-fr \
        libtesseract4 \
    && pip install unidecode \
        filelock \
        regex \
        gunicorn \
        fastspellchecker \
        pillow \
        python-dotenv \
        flask \
        pytest \
    && mkdir -p /data/annotations

CMD gunicorn -t 500 --bind 0.0.0.0:8000 --proxy-allow-from='*' server:app
