include .env
export

SSH = $(REMOTE_USER)@$(REMOTE_HOST)

.PHONY: postgres rasa api dashboard help

postgres:
	ssh $(SSH) "mkdir -p postgres"
	rsync -avz --progress --no-perms --no-owner --no-group --no-times postgres/ $(SSH):postgres/
	scp .env $(SSH):postgres/.env
	ssh $(SSH) "cd postgres && docker compose --env-file .env down && docker compose --env-file .env up -d"

rasa:
	ssh $(SSH) "mkdir -p rasa"
	rsync -avz --progress --no-perms --no-owner --no-group --no-times rasa/ $(SSH):rasa/
	scp .env $(SSH):rasa/.env
	ssh $(SSH) "cd rasa && docker compose --env-file .env down"
	ssh $(SSH) "cd rasa && docker compose --env-file .env build actions-de"
	ssh $(SSH) "cd rasa && docker compose --env-file .env up -d"
	ssh $(SSH) "cd rasa && rm -f rasa-de/models/*.tar.gz"
	ssh $(SSH) "cd rasa && docker compose --env-file .env exec rasa-de rasa train"
	ssh $(SSH) "cd rasa && docker compose --env-file .env restart"

api:
	cd simple-api && docker build --no-cache --platform linux/amd64 -t simple-api:latest .
	cd simple-api && docker save -o simple-api-image.tar simple-api:latest
	scp simple-api/simple-api-image.tar $(SSH):~/simple-api-image.tar
	ssh $(SSH) "docker rmi simple-api:latest || true"
	ssh $(SSH) "docker load -i ~/simple-api-image.tar && rm ~/simple-api-image.tar"
	rm simple-api/simple-api-image.tar
	ssh $(SSH) "mkdir -p simple-api"
	scp simple-api/compose.yml $(SSH):simple-api/compose.yml
	scp .env $(SSH):simple-api/.env
	ssh $(SSH) "cd simple-api && docker compose --env-file .env down && docker compose --env-file .env up -d"

dashboard:
	cd simple-dashboard && docker build --no-cache --platform linux/amd64 \
		--build-arg VITE_API_BASE_URL=https://smuhr.tailbcecf4.ts.net/api \
		--build-arg VITE_API_TOKEN=$(API_TOKEN) \
		-t simple-dashboard:latest .
	cd simple-dashboard && docker save -o simple-dashboard-image.tar simple-dashboard:latest
	scp simple-dashboard/simple-dashboard-image.tar $(SSH):~/simple-dashboard-image.tar
	ssh $(SSH) "docker rmi simple-dashboard:latest || true"
	ssh $(SSH) "docker load -i ~/simple-dashboard-image.tar && rm ~/simple-dashboard-image.tar"
	rm simple-dashboard/simple-dashboard-image.tar
	ssh $(SSH) "mkdir -p simple-dashboard"
	scp simple-dashboard/compose.yml $(SSH):simple-dashboard/compose.yml
	ssh $(SSH) "cd simple-dashboard && docker compose down && docker compose up -d"

help:
	@echo "make postgres   - Deploy PostgreSQL"
	@echo "make rasa       - Deploy Rasa NLU + train model"
	@echo "make api        - Deploy Voice API"
	@echo "make dashboard  - Deploy React Dashboard"
