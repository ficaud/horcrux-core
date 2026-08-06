# Contribution Guide

## Rules 

TBD

## Environment Setup

1. How to build and get into the .devcontainer

```bash
# Launch the dev container
devcontainer up --workspace-folder .

# Open a shell in the running dev container
devcontainer exec --workspace-folder . zsh
```

2. How to rebuild the devcontainer entirely

```bash
devcontainer up --workspace-folder . --remove-existing-container
```

3. How to exit the devcontainer

```bash
exit
```
