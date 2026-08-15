# Install Horcrux Core with Docker Compose

## Prerequisites

Install docker and docker-compose:

```bash
# 1. Mettre à jour le système
sudo apt update && sudo apt upgrade -y

# 2. Installer Docker
curl -fsSL https://get.docker.com | sh

# 3. Activer Docker au démarrage et au groupe utilisateur
sudo systemctl enable docker
sudo usermod -aG docker $USER

# 4. Reconnecte-toi (ou reboot) pour que le groupe docker soit effectif
exit
# puis reconnecte-toi en SSH
```

## Create the `docker-compose.yml` file

Create a `docker-compose.yml`:

```yaml
services:
  horcrux:
    image: ghcr.io/ficaud/horcrux-wasm:latest
    container_name: horcrux
    ports:
      - "8080:80"
    restart: unless-stopped
```

### Identifying the image version

The image is tagged to reflect its source:

| Tag | Meaning |
|-----|---------|
| `latest` | Latest stable build (from `main` or a git tag) |
| `vX.Y.Z` | A specific release (tagged build) |
| `dev` | Development build (from the `dev` branch) |

Each image also embeds a machine-readable manifest at `/version.json` in the web root, so you can identify the exact source commit and branch:

In your docker-compose.yml, you can choose to pull a specific image by using the following syntax:

```yaml
image: ghcr.io/ficaud/horcrux-wasm:latest # latest stable build
image: ghcr.io/ficaud/horcrux-wasm:v1.2.0 # a specific release
image: ghcr.io/ficaud/horcrux-wasm:dev # development build
```

## Launch and manage the container

```bash
docker compose up -d
# update the container
docker compose pull
# force recreate the container with the latest image
docker compose up -d --force-recreate
```
