Hi !

Group : Youssef Ouhmmou && Maxime Perrin-Livenais

# STEP 1 : Cpp builder

```bash
docker build -t soduco/back_builder -f soduco-back-cppbuilder.Dockerfile .
docker run --rm -it -v ${PWD}/resources/code:/app/ soduco/back_builder sh build.sh
```

# STEP 2 : Back runner

```bash
docker build -t soduco/back_runner -f soduco-back-runner.Dockerfile .
docker run -v ${PWD}/resources/data:/data -v ${PWD}/resources/annotations:/data/annotations -p 8000:8000 soduco/back_runner
```

# STEP 3 : Docker Compose

This step uses the dockerfile `multi-stage.Dockerfile` from the step 4.

```bash
docker-compose up
```

# STEP 4 : Multi-stage

```bash
docker build -t soduco/multi_stage -f multi-stage.Dockerfile .
docker run -v ${PWD}/resources/data:/data -v ${PWD}/resources/annotations:/data/annotations -p 8000:8000 soduco/multi_stage
```
