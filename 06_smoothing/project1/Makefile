CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude
LDFLAGS = -lm

SRC_DIR = src
BUILD_DIR = build
EXP_DIR = experiment

COMMON_OBJS = $(BUILD_DIR)/coord_transform.o $(BUILD_DIR)/rotation.o $(BUILD_DIR)/vector_math.o $(BUILD_DIR)/image_utils.o $(BUILD_DIR)/y_rotation.o

.PHONY: all clean experiment validation help

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

$(BUILD_DIR)/y_rotation.o: $(SRC_DIR)/y_rotation.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

experiment: $(BUILD_DIR)/experiment

# 3パラメータ検証実験（LM法）
experiment3: $(BUILD_DIR)/experiment_3param

$(BUILD_DIR)/experiment_3param: $(EXP_DIR)/experiment_3param.c \
                                 $(EXP_DIR)/objective_3param.c \
                                 $(COMMON_OBJS_NO_LM)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# omega2方向スキャン（Y軸版と同形式のグラフ出力用）
scan: $(BUILD_DIR)/scan_omega2

# lm_rotation.o を除いたオブジェクトファイル
COMMON_OBJS_NO_LM = $(BUILD_DIR)/coord_transform.o $(BUILD_DIR)/rotation.o \
                    $(BUILD_DIR)/vector_math.o $(BUILD_DIR)/image_utils.o \
                    $(BUILD_DIR)/y_rotation.o

$(BUILD_DIR)/scan_omega2: $(EXP_DIR)/scan_omega2.c \
                          $(EXP_DIR)/objective_3param.c \
                          $(COMMON_OBJS_NO_LM)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 3パラメータ スキャン（目的関数・微分のCSV出力）
scan_3param: $(BUILD_DIR)/scan_3param

$(BUILD_DIR)/scan_3param: $(EXP_DIR)/scan_3param.c \
                           $(EXP_DIR)/objective_3param.c \
                           $(COMMON_OBJS_NO_LM)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 勾配検証テスト（解析微分 vs 数値微分）
test_gradient: $(BUILD_DIR)/test_gradient

$(BUILD_DIR)/test_gradient: $(EXP_DIR)/test_gradient.c \
                             $(EXP_DIR)/objective_3param.c \
                             $(COMMON_OBJS_NO_LM)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 1画素デバッグ（勾配内部値の表示）
debug_grad: $(BUILD_DIR)/debug_grad

$(BUILD_DIR)/debug_grad: $(EXP_DIR)/debug_grad.c \
                          $(EXP_DIR)/objective_3param.c \
                          $(COMMON_OBJS_NO_LM)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# LM法による3パラメータ回転推定
lm_estimate: $(BUILD_DIR)/lm_estimate

$(BUILD_DIR)/lm_estimate: $(EXP_DIR)/lm_estimate.c \
                           $(EXP_DIR)/objective_3param.c \
                           $(COMMON_OBJS_NO_LM)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# LM法による3パラメータ回転推定（小領域版: 380×190 px）
lm_estimate_small: $(BUILD_DIR)/lm_estimate_small

$(BUILD_DIR)/lm_estimate_small: $(EXP_DIR)/lm_estimate_small.c \
                                  $(EXP_DIR)/objective_3param.c \
                                  $(COMMON_OBJS_NO_LM)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# ω₁方向 収束域スキャン用バイナリ
lm_scan_omega1: $(BUILD_DIR)/lm_scan_omega1

$(BUILD_DIR)/lm_scan_omega1: $(EXP_DIR)/lm_scan_omega1.c \
                               $(EXP_DIR)/objective_3param.c \
                               $(COMMON_OBJS_NO_LM)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# ω₁-ω₂ 2次元スキャン（目的関数曲面の可視化）
scan_2d: $(BUILD_DIR)/scan_2d

$(BUILD_DIR)/scan_2d: $(EXP_DIR)/scan_2d.c \
                       $(EXP_DIR)/objective_3param.c \
                       $(COMMON_OBJS_NO_LM)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 初期値別収束実験（3ケース）
lm_convergence_cases: $(BUILD_DIR)/lm_convergence_cases

$(BUILD_DIR)/lm_convergence_cases: $(EXP_DIR)/lm_convergence_cases.c \
                                    $(EXP_DIR)/objective_3param.c \
                                    $(COMMON_OBJS_NO_LM)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)



$(BUILD_DIR)/experiment: $(EXP_DIR)/experiment.c $(EXP_DIR)/objective.c $(EXP_DIR)/derivative.c $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

validation: validation/create_reference validation/validate_y_rotation

validation/create_reference: validation/create_reference.c $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

validation/validate_y_rotation: validation/validate_y_rotation.c $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 3パラメータ参照画像生成（ω₁, ω₂, ω₃ 指定）
create_reference_3param: $(BUILD_DIR)/create_reference_3param

$(BUILD_DIR)/create_reference_3param: validation/create_reference_3param.c \
                                       $(EXP_DIR)/objective_3param.c \
                                       $(COMMON_OBJS_NO_LM)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)/*

help:
	@echo "make          - 注視画像生成プログラムをビルド"
	@echo "make validation - 検証プログラムをビルド"
	@echo "make experiment - 検証実験プログラムをビルド"
	@echo "make clean    - クリーンアップ"
