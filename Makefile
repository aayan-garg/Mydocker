CC = gcc
CFLAGS = -Wall -Wextra -std=gnu11 -Iinclude -g
LDFLAGS = -lseccomp

SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = bin
TEST_DIR = tests
ROOTFS_DIR = rootfs

# Explicitly list runtime C files (exclude standalone test utilities)
SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/container.c $(SRC_DIR)/cgroups.c $(SRC_DIR)/fs.c $(SRC_DIR)/net.c $(SRC_DIR)/seccomp_filter.c $(SRC_DIR)/utils.c
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
TARGET = $(BIN_DIR)/mydocker

ALPINE_URL = https://dl-cdn.alpinelinux.org/alpine/v3.20/releases/x86_64/alpine-minirootfs-3.20.0-x86_64.tar.gz

.PHONY: all clean test setup-rootfs dirs

all: dirs $(TARGET) setup-helpers

dirs:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR) $(ROOTFS_DIR) $(TEST_DIR)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "[BUILD] Successfully compiled $(TARGET)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

setup-helpers: dirs
	@if [ -d $(ROOTFS_DIR)/bin ]; then \
		$(CC) -static -O2 $(SRC_DIR)/mem_alloc.c -o $(ROOTFS_DIR)/bin/mem_alloc 2>/dev/null || true; \
		$(CC) -static -O2 $(SRC_DIR)/reboot_test.c -o $(ROOTFS_DIR)/bin/reboot_test 2>/dev/null || true; \
	fi

setup-rootfs: dirs
	@if [ ! -f $(ROOTFS_DIR)/bin/sh ]; then \
		echo "[ROOTFS] Downloading Alpine Linux minirootfs..."; \
		curl -sSL $(ALPINE_URL) -o $(ROOTFS_DIR)/alpine.tar.gz; \
		echo "[ROOTFS] Extracting into $(ROOTFS_DIR)..."; \
		tar -xzf $(ROOTFS_DIR)/alpine.tar.gz -C $(ROOTFS_DIR); \
		rm $(ROOTFS_DIR)/alpine.tar.gz; \
		echo "[ROOTFS] RootFS setup complete."; \
	else \
		echo "[ROOTFS] RootFS already exists in $(ROOTFS_DIR)"; \
	fi
	@$(CC) -static -O2 $(SRC_DIR)/mem_alloc.c -o $(ROOTFS_DIR)/bin/mem_alloc
	@$(CC) -static -O2 $(SRC_DIR)/reboot_test.c -o $(ROOTFS_DIR)/bin/reboot_test

test: all
	@echo "[TEST] Running stage tests..."
	@for test_script in $(wildcard $(TEST_DIR)/stage*.sh); do \
		echo "Running $$test_script..."; \
		bash $$test_script || exit 1; \
	done

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)/mydocker
	@echo "[CLEAN] Build artifacts removed."
