# STEP 1

docker build -t soduco/back_builder -f soduco-back-cppbuilder.Dockerfile .
docker run --rm -it -v ${PWD}/resources/code:/app/ -w /app soduco/back_builder sh build.sh


# STEP 2

docker build -t soduco/back_runner -f soduco-back-runner.Dockerfile .
docker run -v ${PWD}/resources/data:/data -p 8000:8000 soduco/back_runner
