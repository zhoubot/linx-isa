SHELL := /bin/sh

SPEC ?= ../../../spec/isa/spec/current/linxisa-v0.3.json
GEN_DIR ?= src/generated
DOC ?= src/linxisa-isa-manual.adoc

BUILD_DIR ?= build
PDF ?= $(BUILD_DIR)/linxisa-isa-manual.pdf
HTML ?= $(BUILD_DIR)/linxisa-isa-manual.html

.PHONY: all pdf html gen clean bundle-check

all: pdf

bundle-check:
	@command -v bundle >/dev/null 2>&1 || (echo "ERROR: bundler not found (expected 'bundle' in PATH)"; exit 1)

gen:
	@mkdir -p $(GEN_DIR)
	python3 ../../../tools/isa/gen_manual_adoc.py --spec $(SPEC) --out-dir $(GEN_DIR)
	python3 ../../../tools/isa/gen_ssr_adoc.py --spec $(SPEC) --out-dir $(GEN_DIR)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

pdf: bundle-check gen | $(BUILD_DIR)
	bundle install --path vendor/bundle
	bundle exec asciidoctor-pdf -D $(BUILD_DIR) -o $(notdir $(PDF)) $(DOC)
	@echo "Built $(PDF)"

html: bundle-check gen | $(BUILD_DIR)
	bundle install --path vendor/bundle
	bundle exec asciidoctor -D $(BUILD_DIR) -o $(notdir $(HTML)) $(DOC)
	@echo "Built $(HTML)"

clean:
	rm -rf $(BUILD_DIR) $(GEN_DIR) vendor/bundle .bundle
