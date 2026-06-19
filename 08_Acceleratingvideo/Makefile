CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude -fPIC
LDFLAGS = -lm

SRC_DIR = src
BUILD_DIR = build
EXP_DIR = experiment

COMMON_OBJS = $(BUILD_DIR)/coord_transform.o $(BUILD_DIR)/rotation.o $(BUILD_DIR)/vector_math.o $(BUILD_DIR)/image_utils.o

SHARED_NAME = libomnigaze.so

.PHONY: shared

shared: $(BUILD_DIR)/$(SHARED_NAME)

$(BUILD_DIR)/$(SHARED_NAME): $(COMMON_OBJS) $(BUILD_DIR)/omnigaze_wrapper.o $(BUILD_DIR)/objective_3param.o
	@mkdir -p $(BUILD_DIR)
	$(CC) -shared -fPIC -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/omnigaze_wrapper.o: $(EXP_DIR)/omnigaze_wrapper.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(BUILD_DIR)/objective_3param.o: $(EXP_DIR)/objective_3param.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@


.PHONY: scan_3param scan_2d lm_estimate lm_convergence_cases lm_scan_omega1 lm_scan_omega2 create_reference_3param clean help

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

# 3パラメータ スキャン（目的関数・微分のCSV出力）
scan_3param: $(BUILD_DIR)/scan_3param

$(BUILD_DIR)/scan_3param: $(EXP_DIR)/scan_3param.c \
                           $(EXP_DIR)/objective_3param.c \
                           $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# ω₁-ω₂ 2次元スキャン（目的関数曲面の可視化）
scan_2d: $(BUILD_DIR)/scan_2d

$(BUILD_DIR)/scan_2d: $(EXP_DIR)/scan_2d.c \
                       $(EXP_DIR)/objective_3param.c \
                       $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# LM法による3パラメータ回転推定
lm_estimate: $(BUILD_DIR)/lm_estimate

$(BUILD_DIR)/lm_estimate: $(EXP_DIR)/lm_estimate.c \
                           $(EXP_DIR)/objective_3param.c \
                           $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 初期値別収束実験（3ケース）
lm_convergence_cases: $(BUILD_DIR)/lm_convergence_cases

$(BUILD_DIR)/lm_convergence_cases: $(EXP_DIR)/lm_convergence_cases.c \
                                    $(EXP_DIR)/objective_3param.c \
                                    $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# ω₁方向 収束域スキャン用バイナリ
lm_scan_omega1: $(BUILD_DIR)/lm_scan_omega1

$(BUILD_DIR)/lm_scan_omega1: $(EXP_DIR)/lm_scan_omega1.c \
                               $(EXP_DIR)/objective_3param.c \
                               $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# ω₂方向 収束域スキャン用バイナリ
lm_scan_omega2: $(BUILD_DIR)/lm_scan_omega2

$(BUILD_DIR)/lm_scan_omega2: $(EXP_DIR)/lm_scan_omega2.c \
                               $(EXP_DIR)/objective_3param.c \
                               $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# B方式平滑化 LM法 単一実行（収束域スキャン用）
lm_estimate_single: $(BUILD_DIR)/lm_estimate_single

$(BUILD_DIR)/lm_estimate_single: $(EXP_DIR)/lm_estimate_single.c \
                                   $(EXP_DIR)/objective_3param.c \
                                   $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 1フレーム分のLM法回転推定（動画パイプライン用）
lm_estimate_frame: $(BUILD_DIR)/lm_estimate_frame

$(BUILD_DIR)/lm_estimate_frame: $(EXP_DIR)/lm_estimate_frame.c \
                                  $(EXP_DIR)/objective_3param.c \
                                  $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 注視画像生成（動画パイプライン用）
# generate_gaze_frame: $(BUILD_DIR)/generate_gaze_frame
#
# $(BUILD_DIR)/generate_gaze_frame: $(EXP_DIR)/generate_gaze_frame.c $(COMMON_OBJS)
# 	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 3パラメータ参照画像生成（ω₁, ω₂, ω₃ 指定）
create_reference_3param: $(BUILD_DIR)/create_reference_3param

$(BUILD_DIR)/create_reference_3param: validation/create_reference_3param.c \
                                       $(EXP_DIR)/objective_3param.c \
                                       $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)/*

help:
	@echo "利用可能なターゲット:"
	@echo "  make scan_3param           - 3パラメータ スキャン（CSV出力）"
	@echo "  make scan_2d               - ω₁-ω₂ 2次元スキャン"
	@echo "  make lm_estimate           - LM法による3パラメータ回転推定"
	@echo "  make lm_convergence_cases  - 初期値別収束実験（3ケース)"
	@echo "  make lm_scan_omega1        - ω₁方向 収束域スキャン"
	@echo "  make create_reference_3param - 3パラメータ参照画像生成"
	@echo "  make clean                 - クリーンアップ"


lm_convergence_cases_base2: $(BUILD_DIR)/lm_convergence_cases_base2
$(BUILD_DIR)/lm_convergence_cases_base2: $(EXP_DIR)/lm_convergence_cases_base2.c \
						$(EXP_DIR)/objective_3param.c \
						$(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

lm_convergence_cases_base3: $(BUILD_DIR)/lm_convergence_cases_base3
$(BUILD_DIR)/lm_convergence_cases_base3: $(EXP_DIR)/lm_convergence_cases_base3.c \
						$(EXP_DIR)/objective_3param.c \
						$(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 勾配の1次元断面スキャン
scan_gradient_1d: $(BUILD_DIR)/scan_gradient_1d
$(BUILD_DIR)/scan_gradient_1d: $(EXP_DIR)/scan_gradient_1d.c \
						$(EXP_DIR)/objective_3param.c \
						$(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
