SHELL := /bin/sh

PROJECT_ROOT := $(CURDIR)

LOCAL_UV_CACHE := $(PROJECT_ROOT)/.uv-cache
LOCAL_HF_HOME := $(PROJECT_ROOT)/.cache/huggingface
LOCAL_TMP_DIR := $(PROJECT_ROOT)/.tmp

export UV_CACHE_DIR := $(LOCAL_UV_CACHE)
export HF_HOME := $(LOCAL_HF_HOME)
export TMPDIR := $(LOCAL_TMP_DIR)

.PHONY: install run debug clean lint lint-strict prepare paths

prepare:
	@mkdir -p "$(LOCAL_UV_CACHE)"
	@mkdir -p "$(LOCAL_HF_HOME)"
	@mkdir -p "$(LOCAL_TMP_DIR)"

install: prepare
	uv sync

run: prepare
	uv run python -m src

debug: prepare
	PYTHONFAULTHANDLER=1 uv run python -X dev -m src

lint: prepare
	uv run flake8 src/
	uv run mypy src/ --warn-return-any --warn-unused-ignores \
		--ignore-missing-imports --disallow-untyped-defs \
		--check-untyped-defs

lint-strict: lint

paths:
	@echo "Project root:  $(PROJECT_ROOT)"
	@echo "uv cache:     $(LOCAL_UV_CACHE)"
	@echo "HF cache:     $(LOCAL_HF_HOME)"
	@echo "Temporary:    $(LOCAL_TMP_DIR)"
	@echo "Virtual env:  $(PROJECT_ROOT)/.venv"

clean:
	rm -rf .venv
	rm -rf .uv-cache
	rm -rf .cache
	rm -rf .tmp
	rm -rf data/output
	rm -rf .mypy_cache
	rm -rf .pytest_cache
	find src llm_sdk -type d -name "__pycache__" \
		-prune -exec rm -rf {} +
	find src llm_sdk -type f \
		\( -name "*.pyc" -o -name "*.pyo" \) -delete