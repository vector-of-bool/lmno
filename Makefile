.SILENT:
.PHONY: build

.PHONY: poetry-install
poetry-install:
	echo "Installing Python tooling..."
	poetry -q install

.PHONY: build
build: poetry-install
	poetry run dagon build

.PHONY: docs
docs: poetry-install
	poetry run dagon docs

.PHONY: docs-server
docs-server: poetry-install
	poetry run \
		sphinx-autobuild docs/ _build/docs \
			--re-ignore "docs/_build" \
			--re-ignore docs/ref/api \
			-j auto
