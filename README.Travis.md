# Travis CI

By default Travis uses Ubuntu 14.04 LTS for building, but it is possible to run
the build in any Linux distribution using [Docker](https://www.docker.com/).

## Setup

For each target distribution there is a `Dockerfile` and the respective
`.travis.sh` script. The `Dockerfile` defines the steps needed for building
the Docker image, the script runs the build in the specific distribution.

The `.travis.yml` file defines the build matrix which runs builds for all
configured distributions in parallel. It is defined in the `env` section,
see more details in the [Travis documentation](
https://docs.travis-ci.com/user/customizing-the-build#Build-Matrix).

## Running the Build Locally

- [Install and start Docker](https://docs.docker.com/engine/installation/linux/)
- Run (as `root`) the same commands as in the `.travis.yml` file, that means:
- First build the docker image locally, e.g. `docker build -f
  Dockerfile.tumbleweed -t snapper-devel .`, the Docker image automatically
  includes also the copy of the current Snapper sources.
- Then run the build: `docker run -it --rm snapper-devel ./.travis.tumbleweed.sh`
  (The `--rm` will cleanup the new layer created by the build, if want to
  inspect it then remove it.)
- If you need to debug a failure then run `bash` instead of the Travis script
  and run the build steps manually. If you need an editor or some other tool
  you can install them via the respective packaging tool, see the `Dockerfile`
  for examples.

