.PHONY: help dev stop clean logs shell

# Default target
help: ## Show this help message
	@echo "Hack4Life Jekyll Site - Development Commands"
	@echo "==========================================="
	@echo "📍 Local dev: http://localhost:4000"
	@echo "🌐 Live site: https://cat101.github.io/hack4life"
	@echo "Note: GitHub Pages handles production builds automatically"
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-15s\033[0m %s\n", $$1, $$2}'

dev: ## Start development server with live reload
	@echo "🚀 Starting development server..."
	@echo "📍 Site will be available at http://localhost:4000"
	docker-compose up

dev-bg: ## Start development server in background
	@echo "🚀 Starting development server in background..."
	docker-compose up -d
	@echo "📍 Site is running at http://localhost:4000"

stop: ## Stop all running containers
	@echo "🛑 Stopping containers..."
	docker-compose down

clean: ## Clean up containers and rebuild
	@echo "🧹 Cleaning Hack4Life project artifacts..."
	docker-compose down
	@echo "🗑️  Removing project Docker image..."
	-docker rmi hack4life-jekyll:latest 2>/dev/null || true
	@echo "🗑️  Removing build directories..."
	rm -rf _site
	rm -rf .jekyll-cache
	rm -rf .jekyll-metadata
	rm -rf .sass-cache
	rm -rf vendor
	rm -rf .bundle
	rm -rf node_modules
	@echo "✅ Cleanup complete!"

rebuild: clean ## Clean and rebuild everything
	@echo "🔨 Rebuilding from scratch..."
	docker-compose build --no-cache

logs: ## Show container logs
	docker-compose logs -f

shell: ## Open shell in Jekyll container
	docker-compose exec jekyll sh

install: ## Install/update dependencies
	@echo "📦 Installing dependencies..."
	docker-compose build

serve: dev ## Alias for dev

# Quick commands
up: dev-bg ## Quick start in background
down: stop ## Quick stop
