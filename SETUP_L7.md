# Setup — memory isolation exercise

The project runs on an emulated Arm Cortex-M33 board (`mps2/an521/cpu0`) in QEMU, so no
hardware is needed. Everything (compiler, Zephyr SDK, Zephyr 3.7.2, QEMU) is inside a
Docker image. On your machine you only need Git and Docker, and your Zephyr installation
for the rest of the course is not touched.

## Requirements

- Linux on an x86_64 PC (a Linux VM works too).
- Git.
- Docker, usable without `sudo`. Check with `docker run --rm hello-world`.
- About 6 GB of free disk space and an internet connection for the first setup.

## 1. Get the project

Clone it into any folder outside your course workspace:

```
git clone -b l7-homework https://github.com/iomico-public/zephyr-intermediate.git ~/isolation-exercise
```

```
cd ~/isolation-exercise
```

**All commands from here on are run from `~/isolation-exercise`.**

## 2. Build the Docker image

```
docker build --build-arg UID=$(id -u) --build-arg GID=$(id -g) -t zephyr-sb:3.7.2 docker
```

- `-t zephyr-sb:3.7.2` is the image name. The script looks for exactly this name, so do
  not change it.
- `UID` / `GID` make the container user match your user, so the files it creates belong
  to you and not to root.

This downloads the Zephyr SDK and Zephyr 3.7.2 into the image. It takes 10–20 minutes and
is only needed once. The secure boot demo uses the same image, so if you already built it
there, skip this step.

## Run the project

```
./run.sh
```

The script starts the container, builds the project for `mps2/an521/cpu0`, and runs it in
QEMU. QEMU stops by itself after 10 seconds, because the emulated board has no power-off.
This is normal, not an error. Change it with `TIMEOUT`, for example
`TIMEOUT=5 ./run.sh`.

The first build takes about a minute. Build output goes to `build/` in this folder.

Expected output of the starter project — the normal setting, then the full key in two
parts:

```
*** Booting Zephyr OS build v3.7.2 ***
parser: received 'get=0'
parser: setting 0 = led=off
parser: received 'get=41941731'
parser: setting 41941731 = EC-PRIVATE-KEY:8
parser: received 'get=41941732'
parser: setting 41941732 = f3a91c25d7e04b6
```

The numbers after `get=` depend on the build and may be different for you.

After you change the project, run `./run.sh` again to rebuild and test it.

## Project contents

| Path | What it is |
|---|---|
| `src/main.c` | Starts the parser thread and feeds it one normal command and two attack commands |
| `src/parser.c` | Parses commands, including the bug. Do not modify this file |
| `src/credentials.c` | Stores the device key. Do not modify this file |
| `prj.conf` | Zephyr configuration options |
| `CMakeLists.txt` | Build definition |
| `run.sh` | Builds the project and runs it in QEMU |
| `docker/` | Docker image definition |
| `docker/west.yml` | Zephyr version and modules downloaded into the image |

## Troubleshooting

**`error: docker image zephyr-sb:3.7.2 not found`** — the image was not built, or was
built with a different name. Repeat step 2.

**`permission denied` from Docker** — your user is not allowed to use Docker. Run
`sudo usermod -aG docker $USER`, then log out and back in.
