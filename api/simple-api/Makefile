include ../.env
export

SSH = $(REMOTE_USER)@$(REMOTE_HOST)
DIR = simple-api
IMAGE = simple-api:latest
TAR = simple-api-image.tar

.PHONY: all build save push copy down up restart logs clean

all: build save push copy restart

build:
	docker build --no-cache --platform linux/amd64 -t $(IMAGE) .

save:
	docker save -o $(TAR) $(IMAGE)

push:
	scp $(TAR) $(SSH):~/$(TAR)
	ssh $(SSH) "docker rmi $(IMAGE) || true"
	ssh $(SSH) "docker load -i ~/$(TAR) && rm ~/$(TAR)"
	rm $(TAR)

copy:
	ssh $(SSH) "mkdir -p $(DIR)"
	scp compose.yml $(SSH):$(DIR)/compose.yml
	scp ../.env $(SSH):$(DIR)/.env

down:
	ssh $(SSH) "cd $(DIR) && docker compose --env-file .env down"

up:
	ssh $(SSH) "cd $(DIR) && docker compose --env-file .env up -d"

restart: down up

logs:
	ssh $(SSH) "cd $(DIR) && docker compose --env-file .env logs -f"

clean: down
	ssh $(SSH) "docker rmi $(IMAGE) || true"
