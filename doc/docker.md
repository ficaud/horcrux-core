# Run the Horcrux WASM demo with Docker Compose

The Horcrux Core web UI (the same split/unsplit pages as the ESP32 captive
portal) is published as a static image on GitHub Container Registry (GHCR).
It runs entirely client-side in the browser and works on Linux, macOS,
Windows and Raspberry Pi (`amd64`, `arm64`, `arm/v7`).

## Prerequisites

Install Docker and the Compose plugin:

```bash
# 1. Update the system
sudo apt update && sudo apt upgrade -y

# 2. Install Docker (bundles the compose plugin)
curl -fsSL https://get.docker.com | sh

# 3. Enable Docker on boot and add yourself to the docker group
sudo systemctl enable docker
sudo usermod -aG docker $USER

# 4. Log out and back in (or reboot) for the group change to take effect
exit
```

## Choosing an image tag

The image is tagged to reflect its source:

| Tag | Meaning |
|-----|---------|
| `latest` | Latest stable build (`main` or a release tag) |
| `vX.Y.Z` | A specific release (e.g. `v1.4.1`) |
| `dev` | Development build (from the `dev/jfi` branch) |

## Create the `docker-compose.yml` file

```yaml
services:
  horcrux:
    image: ghcr.io/ficaud/horcrux-wasm:latest
    container_name: horcrux
    ports:
      - "8080:80"
    restart: unless-stopped
```

Replace `latest` with a specific tag (e.g. `v1.4.1`) to pin a release.

## Launch and manage the container

```bash
# start in the background
docker compose up -d
# view logs
docker compose logs -f
# update to the latest image and recreate the container
docker compose pull && docker compose up -d --force-recreate
# stop and remove it
docker compose down
```

Open `http://localhost:8080`.

## Identify the running version

Each image embeds a manifest at `/version.json` and OCI labels:

```bash
curl http://localhost:8080/version.json
docker inspect horcrux --format '{{index .Config.Labels "org.opencontainers.image.version"}}'
```

To build the image locally instead of pulling it, use the
`docker-compose.yml` at the repository root.
