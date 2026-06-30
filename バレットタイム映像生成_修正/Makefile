CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude
LDFLAGS = -lm

SRC_DIR = src
BUILD_DIR = build
EXP_DIR = experiment

COMMON_OBJS = $(BUILD_DIR)/coord_transform.o $(BUILD_DIR)/rotation.o $(BUILD_DIR)/vector_math.o $(BUILD_DIR)/image_utils.o

.PHONY: create_reference_3param libomnigaze.so clean help

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

# 3パラメータ参照画像生成（ω₁, ω₂, ω₃ 指定）
create_reference_3param: $(BUILD_DIR)/create_reference_3param

$(BUILD_DIR)/create_reference_3param: validation/create_reference_3param.c \
                                       $(EXP_DIR)/objective_3param.c \
                                       $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# バレットタイム映像生成用共有ライブラリ
libomnigaze.so: $(EXP_DIR)/lm_video.c $(EXP_DIR)/objective_3param.c \
                $(SRC_DIR)/coord_transform.c $(SRC_DIR)/rotation.c \
                $(SRC_DIR)/vector_math.c $(SRC_DIR)/image_utils.c
	@mkdir -p $(BUILD_DIR)
	gcc -O3 -fPIC -shared -o $(BUILD_DIR)/libomnigaze.so $^ -Iinclude -lm

clean:
	rm -rf $(BUILD_DIR)/*

help:
	@echo "利用可能なターゲット:"
	@echo "  make libomnigaze.so          - バレットタイム映像生成用共有ライブラリ"
	@echo "  make create_reference_3param - 3パラメータ参照画像生成"
	@echo "  make clean                   - クリーンアップ"
