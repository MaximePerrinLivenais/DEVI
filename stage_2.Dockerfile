FROM python:3.7.10-buster

RUN apt-get update -y \
    && apt-get install -y build-essential \
        tesseract-ocr-fra \
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
        flask
