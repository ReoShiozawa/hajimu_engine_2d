/**
 * src/plugin.c — はじむ 2D エンジン プラグインエントリー
 *
 * Copyright (c) 2026 Reo Shiozawa — MIT License
 */
#include "hajimu_plugin.h"
#include "eng_2d.h"

static ENG_Physics* g_phys = NULL;

/* ── ヘルパーマクロ ─────────────────────────────────────*/
#define ARG_NUM(i) ((i) < argc && args[(i)].type == VALUE_NUMBER ? args[(i)].number : 0.0)
#define ARG_STR(i) ((i) < argc && args[(i)].type == VALUE_STRING  ? args[(i)].string.data : "")
#define ARG_INT(i) ((int)ARG_NUM(i))
#define ARG_F(i)   ((float)ARG_NUM(i))
#define ARG_B(i)   ((i) < argc && args[(i)].type == VALUE_BOOL ? args[(i)].boolean : false)
#define NUM(v)     hajimu_number((double)(v))
#define BVAL(v)    hajimu_bool((bool)(v))
#define NUL        hajimu_null()

/* ── 物理ワールド ────────────────────────────────────────*/
static Value fn_物理初期化(int argc, Value* args) {
    if (g_phys) { eng_phys_destroy(g_phys); g_phys = NULL; }
    float gx = argc > 0 ? ARG_F(0) : 0.0f;
    float gy = argc > 1 ? ARG_F(1) : 980.0f;
    g_phys = eng_phys_create(gx, gy);
    return NUM(g_phys ? 0 : -1);
}
static Value fn_物理終了(int argc, Value* args) {
    (void)argc; (void)args;
    if (g_phys) { eng_phys_destroy(g_phys); g_phys = NULL; }
    return NUL;
}
static Value fn_物理更新(int argc, Value* args) {
    if (g_phys) eng_phys_update(g_phys, ARG_F(0));
    return NUL;
}

/* ── ボディ ─────────────────────────────────────────────*/
static Value fn_ボディ作成(int argc, Value* args) {
    return NUM(eng_body_create(g_phys, (ENG_BodyType)ARG_INT(0),
                               ARG_F(1), ARG_F(2), ARG_F(3), ARG_F(4)));
}
static Value fn_ボディ削除(int argc, Value* args)    { eng_body_destroy(g_phys, ARG_INT(0)); return NUL; }
static Value fn_ボディX取得(int argc, Value* args)   { return NUM(eng_body_x(g_phys, ARG_INT(0))); }
static Value fn_ボディY取得(int argc, Value* args)   { return NUM(eng_body_y(g_phys, ARG_INT(0))); }
static Value fn_ボディW取得(int argc, Value* args)   { return NUM(eng_body_w(g_phys, ARG_INT(0))); }
static Value fn_ボディH取得(int argc, Value* args)   { return NUM(eng_body_h(g_phys, ARG_INT(0))); }
static Value fn_ボディVX取得(int argc, Value* args)  { return NUM(eng_body_vx(g_phys, ARG_INT(0))); }
static Value fn_ボディVY取得(int argc, Value* args)  { return NUM(eng_body_vy(g_phys, ARG_INT(0))); }
static Value fn_ボディ位置設定(int argc, Value* args) { eng_body_set_pos(g_phys,ARG_INT(0),ARG_F(1),ARG_F(2)); return NUL; }
static Value fn_ボディ速度設定(int argc, Value* args) { eng_body_set_vel(g_phys,ARG_INT(0),ARG_F(1),ARG_F(2)); return NUL; }
static Value fn_ボディ速度加算(int argc, Value* args) { eng_body_add_vel(g_phys,ARG_INT(0),ARG_F(1),ARG_F(2)); return NUL; }
static Value fn_ボディ重力スケール設定(int argc, Value* args) { eng_body_set_gravity_scale(g_phys,ARG_INT(0),ARG_F(1)); return NUL; }
static Value fn_ボディ反発設定(int argc, Value* args) { eng_body_set_restitution(g_phys,ARG_INT(0),ARG_F(1)); return NUL; }
static Value fn_ボディ減衰設定(int argc, Value* args) { eng_body_set_damping(g_phys,ARG_INT(0),ARG_F(1)); return NUL; }
static Value fn_ボディ衝突中(int argc, Value* args)  { return BVAL(eng_body_overlaps(g_phys,ARG_INT(0),ARG_INT(1))); }
static Value fn_ボディ矩形衝突(int argc, Value* args) {
    return BVAL(eng_body_overlaps_rect(g_phys,ARG_INT(0),ARG_F(1),ARG_F(2),ARG_F(3),ARG_F(4)));
}
static Value fn_接地判定(int argc, Value* args) { return BVAL(eng_body_on_ground(g_phys,ARG_INT(0))); }

/* ── タイルマップ ────────────────────────────────────────*/
static Value fn_タイルマップ読込(int argc, Value* args) {
    return NUM(eng_map_load_csv(ARG_STR(0), ARG_INT(1), ARG_INT(2)));
}
static Value fn_タイルマップ削除(int argc, Value* args) { eng_map_destroy(ARG_INT(0)); return NUL; }
static Value fn_タイルマップ列数(int argc, Value* args) { return NUM(eng_map_cols(ARG_INT(0))); }
static Value fn_タイルマップ行数(int argc, Value* args) { return NUM(eng_map_rows(ARG_INT(0))); }
static Value fn_タイルインデックス取得(int argc, Value* args) {
    return NUM(eng_map_get(ARG_INT(0), ARG_INT(1), ARG_INT(2)));
}
static Value fn_タイルインデックス設定(int argc, Value* args) {
    eng_map_set(ARG_INT(0), ARG_INT(1), ARG_INT(2), ARG_INT(3)); return NUL;
}
static Value fn_タイルマップ矩形衝突(int argc, Value* args) {
    return BVAL(eng_map_overlaps_rect(ARG_INT(0), ARG_F(1), ARG_F(2), ARG_F(3), ARG_F(4)));
}
static Value fn_タイルマップ衝突応答(int argc, Value* args) {
    eng_map_resolve_body(ARG_INT(0), g_phys, ARG_INT(1)); return NUL;
}

/* ── プラグイン登録 ─────────────────────────────────────*/
#define FN(name, mn, mx) { #name, fn_##name, mn, mx }

static HajimuPluginFunc funcs[] = {
    /* 物理ワールド */
    FN(物理初期化, 0, 2),
    FN(物理終了,   0, 0),
    FN(物理更新,   1, 1),
    /* ボディ */
    FN(ボディ作成,        5, 5),
    FN(ボディ削除,        1, 1),
    FN(ボディX取得,       1, 1),
    FN(ボディY取得,       1, 1),
    FN(ボディW取得,       1, 1),
    FN(ボディH取得,       1, 1),
    FN(ボディVX取得,      1, 1),
    FN(ボディVY取得,      1, 1),
    FN(ボディ位置設定,    3, 3),
    FN(ボディ速度設定,    3, 3),
    FN(ボディ速度加算,    3, 3),
    FN(ボディ重力スケール設定, 2, 2),
    FN(ボディ反発設定,    2, 2),
    FN(ボディ減衰設定,    2, 2),
    FN(ボディ衝突中,      2, 2),
    FN(ボディ矩形衝突,    5, 5),
    FN(接地判定,          1, 1),
    /* タイルマップ */
    FN(タイルマップ読込,       3, 3),
    FN(タイルマップ削除,       1, 1),
    FN(タイルマップ列数,       1, 1),
    FN(タイルマップ行数,       1, 1),
    FN(タイルインデックス取得, 3, 3),
    FN(タイルインデックス設定, 4, 4),
    FN(タイルマップ矩形衝突,   5, 5),
    FN(タイルマップ衝突応答,   2, 2),
};

HAJIMU_PLUGIN_EXPORT HajimuPluginInfo* hajimu_plugin_init(void) {
    static HajimuPluginInfo info = {
        .name           = "engine_2d",
        .version        = "1.0.0",
        .author         = "Reo Shiozawa",
        .description    = "はじむ用 2D 物理・タイルマップエンジン",
        .functions      = funcs,
        .function_count = sizeof(funcs) / sizeof(funcs[0]),
    };
    return &info;
}
