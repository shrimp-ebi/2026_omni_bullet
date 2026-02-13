CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude
LDFLAGS = -lm

SRC_DIR = src
BUILD_DIR = build
TEST_DIR = test
EXP_DIR = experiment

COMMON_OBJS = $(BUILD_DIR)/coord_transform.o $(BUILD_DIR)/rotation.o $(BUILD_DIR)/vector_math.o $(BUILD_DIR)/image_utils.o

.PHONY: all clean test experiment help

all: $(BUILD_DIR)/main

$(BUILD_DIR)/main: $(SRC_DIR)/main.c $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/coord_transform.o: $(SRC_DIR)/coord_transform.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/rotation.o: $(SRC_DIR)/rotation.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/vector_math.o: $(SRC_DIR)/vector_math.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/image_utils.o: $(SRC_DIR)/image_utils.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

experiment: $(BUILD_DIR)/experiment

$(BUILD_DIR)/experiment: $(EXP_DIR)/experiment.c $(EXP_DIR)/objective.c $(EXP_DIR)/derivative.c $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test: $(BUILD_DIR)/test_coord_transform $(BUILD_DIR)/test_rotation $(BUILD_DIR)/test_vector_math

$(BUILD_DIR)/test_coord_transform: $(TEST_DIR)/test_coord_transform.c $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/test_rotation: $(TEST_DIR)/test_rotation.c $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/test_vector_math: $(TEST_DIR)/test_vector_math.c $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)/*

help:
	@echo "make          - 注視画像生成プログラムをビルド"
	@echo "make test     - テストプログラムをビルド"
	@echo "make experiment - 検証実験プログラムをビルド"
	@echo "make clean    - クリーンアップ"
