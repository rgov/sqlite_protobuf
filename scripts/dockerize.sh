#!/bin/sh
# A very simple script to give you a Docker container running the same image
# that CircleCI uses.

REPO_ROOT=$(cd "$(dirname "$0")"; git rev-parse --show-toplevel)

docker run \
  --interactive \
  --tty \
  --cap-add=SYS_PTRACE \
  --security-opt seccomp=unconfined \
  --volume "${REPO_ROOT}:/$(basename "${REPO_ROOT}")" \
  --workdir "/$(basename "${REPO_ROOT}")" \
  rgov/sqlite_protobuf_build:latest \
  /bin/bash
