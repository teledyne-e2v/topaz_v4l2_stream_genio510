SDK_DIR ?= /home/niziak/sdk
SHELL := /bin/bash

BUILD_DIR := $(shell pwd)/build

all: $(BUILD_DIR)/Makefile
	source $(SDK_DIR)/environment-setup-armv8a-poky-linux && cd $(BUILD_DIR) && make -j

$(BUILD_DIR)/Makefile: $(BUILD_DIR)
	source $(SDK_DIR)/environment-setup-armv8a-poky-linux && cd $(BUILD_DIR) && cmake ..

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

DST_DIR ?= /home/root
DST_HOST ?= 192.168.177.157

SSH_OPTS = -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null

target_install:
	scp $(SSH_OPTS) $(BUILD_DIR)/sensortest root@$(DST_HOST):$(DST_DIR)

target_get:
	scp $(SSH_OPTS) root@$(DST_HOST):$(DST_DIR)/image* .
