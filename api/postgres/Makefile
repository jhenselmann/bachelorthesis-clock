include ../.env
export

SSH = $(REMOTE_USER)@$(REMOTE_HOST)
DIR = postgres

.PHONY: all copy down up restart clean backup restore

all: copy restart

copy:
	ssh $(SSH) "mkdir -p $(DIR)"
	-rsync -avz --progress --no-perms --no-owner --no-group --no-times ./ $(SSH):$(DIR)/
	scp ../.env $(SSH):$(DIR)/.env

down:
	ssh $(SSH) "cd $(DIR) && docker compose --env-file .env down"

up:
	ssh $(SSH) "cd $(DIR) && docker compose --env-file .env up -d"

restart: down up

clean:
	ssh $(SSH) "cd $(DIR) && docker compose --env-file .env down -v"

backup:
	ssh $(SSH) "cd $(DIR) && docker compose --env-file .env exec postgres pg_dump -U $(POSTGRES_USER) $(POSTGRES_DB) > backup_$$(date +%Y%m%d_%H%M%S).sql"

restore:
	@test -n "$(FILE)" || (echo "Usage: make restore FILE=backup.sql" && exit 1)
	ssh $(SSH) "cd $(DIR) && docker compose --env-file .env exec -T postgres psql -U $(POSTGRES_USER) $(POSTGRES_DB) < $(FILE)"
