# We used this image python because it is light enough
# and has >= 3.7 python versions
FROM python:3.7.10-slim

# Set working directory to /app
WORKDIR /app/

# Set environment variables
ENV LD_LIBRARY_PATH=/app/lib \
    LC_ALL=C

# Copying the requirements.txt
COPY resources/requirements.txt requirements.txt

# Copy the tarball with the previously built lib and decompress it.
ADD resources/code/build/soduco-py37-0.1.1-Linux.tar.gz .

# Extract libstd++.so from the cpp build image.
COPY --from=soduco/back_builder:latest /usr/local/lib64/libstdc++.so.6 /usr/lib/x86_64-linux-gnu/

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
    # Installing python dependencies \
    && pip install --no-cache-dir -r requirements.txt \
    \
    # Moving lib and back directories from the decompressed tarball \
    # in the current directory \
    && mv soduco-py37-0.1.1-Linux/lib . \
    && mv soduco-py37-0.1.1-Linux/back . \
    \
    # Removing the tarball directory\
    && rm -rf soduco-py37-0.1.1-Linux

# Copying back, resources and server directories.
COPY resources/code/back back/
COPY resources/code/resources/ resources/
COPY resources/code/server/ server/

EXPOSE 8000

# The command to launch
CMD gunicorn -t 500 --bind 0.0.0.0:8000 --proxy-allow-from='*' server:app
