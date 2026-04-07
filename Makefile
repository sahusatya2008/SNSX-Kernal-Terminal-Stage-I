SHELL := /bin/zsh

.PHONY: all build run headless clean

all: build

build:
	./scripts/build.sh

run:
	./scripts/run.sh

headless:
	SNSX_HEADLESS=1 ./scripts/run.sh

clean:
	rm -rf build dist
