include ../.env
export

SSH = $(REMOTE_USER)@$(REMOTE_HOST)
DIR = rasa

.PHONY: all copy down build up train restart clean

all: copy down build up train restart

copy:
	ssh $(SSH) "mkdir -p $(DIR)"
	-rsync -avz --progress --no-perms --no-owner --no-group --no-times ./ $(SSH):$(DIR)/
	scp ../.env $(SSH):$(DIR)/.env

down:
	ssh $(SSH) "cd $(DIR) && docker compose --env-file .env down"

build:
	ssh $(SSH) "cd $(DIR) && docker compose --env-file .env build actions-de"

up:
	ssh $(SSH) "cd $(DIR) && docker compose --env-file .env up -d"

train:
	ssh $(SSH) "cd $(DIR) && rm -f rasa-de/models/*.tar.gz"
	ssh $(SSH) "cd $(DIR) && docker compose --env-file .env exec rasa-de rasa train"

restart: down build up

clean: down
