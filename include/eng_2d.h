/**
 * include/eng_2d.h — はじむ 2D 物理・タイルマップ 公開 API
 *
 * 外部ライブラリ不要の軽量 AABB 物理エンジンと
 * CSV タイルマップローダーを提供する。
 *
 * Copyright (c) 2026 Reo Shiozawa — MIT License
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── 型定義 ─────────────────────────────────────────────*/
typedef struct ENG_Physics ENG_Physics;
typedef struct ENG_TileMap ENG_TileMap;
typedef uint32_t ENG_BodyID; /* 0 = 無効 */
typedef uint32_t ENG_MapID;  /* 0 = 無効 */

/** ボディ種別 */
typedef enum {
    ENG_BODY_STATIC    = 0, /* 移動しない */
    ENG_BODY_DYNAMIC   = 1, /* 物理演算対象 */
    ENG_BODY_KINEMATIC = 2, /* velocity のみ、重力なし */
} ENG_BodyType;

/* ── 物理ワールド ────────────────────────────────────────*/

/** 物理ワールド生成。gravity_x/y は重力加速度 (px/s²)。 */
ENG_Physics* eng_phys_create(float gravity_x, float gravity_y);

/** ワールド破棄。 */
void eng_phys_destroy(ENG_Physics* p);

/** 物理更新。dt = デルタ時間 (秒)。 */
void eng_phys_update(ENG_Physics* p, float dt);

/* ── ボディ ─────────────────────────────────────────────*/

/** ボディ作成。x/y はボディ左上座標。 */
ENG_BodyID eng_body_create(ENG_Physics* p, ENG_BodyType type,
                            float x, float y, float w, float h);

/** ボディ削除。 */
void eng_body_destroy(ENG_Physics* p, ENG_BodyID id);

/* 位置 */
float eng_body_x(ENG_Physics* p, ENG_BodyID id);
float eng_body_y(ENG_Physics* p, ENG_BodyID id);
void  eng_body_set_pos(ENG_Physics* p, ENG_BodyID id, float x, float y);

/* 速度 */
float eng_body_vx(ENG_Physics* p, ENG_BodyID id);
float eng_body_vy(ENG_Physics* p, ENG_BodyID id);
void  eng_body_set_vel(ENG_Physics* p, ENG_BodyID id, float vx, float vy);
void  eng_body_add_vel(ENG_Physics* p, ENG_BodyID id, float dvx, float dvy);

/* 重力スケール (dynamic のみ有効、デフォルト 1.0) */
void  eng_body_set_gravity_scale(ENG_Physics* p, ENG_BodyID id, float s);

/* 反発 (0.0=完全非弾性, 1.0=完全弾性) */
void  eng_body_set_restitution(ENG_Physics* p, ENG_BodyID id, float r);

/* 線形減衰 (空気抵抗; 0.0=なし〜1.0=即停止) */
void  eng_body_set_damping(ENG_Physics* p, ENG_BodyID id, float d);

/** 即時に速度を加算する (ジャンプ等に使用)。 */
void  eng_body_apply_impulse(ENG_Physics* p, ENG_BodyID id, float ix, float iy);

/** ボディの物理更新・衝突を有効/無効にする。 */
void  eng_body_set_active(ENG_Physics* p, ENG_BodyID id, bool active);

/** ボディの有効/無効状態を取得する。 */
bool  eng_body_get_active(ENG_Physics* p, ENG_BodyID id);

/** 摩擦係数設定 (接地中に水平速度を減衰; 0=なし)。 */
void  eng_body_set_friction(ENG_Physics* p, ENG_BodyID id, float friction);

/** 2 つのボディが現在接触しているかどうか。 */
bool eng_body_overlaps(ENG_Physics* p, ENG_BodyID a, ENG_BodyID b);

/** ボディと座標矩形が接触しているかどうか。 */
bool eng_body_overlaps_rect(ENG_Physics* p, ENG_BodyID id,
                             float rx, float ry, float rw, float rh);

/** 接地判定 (下端が静的ボディ上端に接触)。 */
bool eng_body_on_ground(ENG_Physics* p, ENG_BodyID id);

/** ボディのサイズ (w, h) を変更する。 */
void eng_body_set_size(ENG_Physics* p, ENG_BodyID id, float w, float h);

/** レイキャスト: 始点 (ox,oy) から方向 (dx,dy) に最大 len ピクセル進み、
 *  最初に当たったボディIDを返す (なければ 0)。
 *  hit_x / hit_y に接触座標を書き込む (NULLも可)。 */
ENG_BodyID eng_phys_raycast(ENG_Physics* p,
                              float ox, float oy,
                              float dx, float dy, float len,
                              float* hit_x, float* hit_y);

/* サイズ取得 */
float eng_body_w(ENG_Physics* p, ENG_BodyID id);
float eng_body_h(ENG_Physics* p, ENG_BodyID id);

/* ── タイルマップ ────────────────────────────────────────*/

/**
 * CSV タイルマップを読み込む。
 * CSV フォーマット: 行=タイル行, 列=タイル列, 値=タイルインデックス (0=空)。
 * tile_w / tile_h: 1タイルのピクセル寸法。
 * 戻り値: MapID (0=失敗)
 */
ENG_MapID eng_map_load_csv(const char* path, int tile_w, int tile_h);

/** タイルマップを整数配列から直接作成。tiles[row * cols + col] = タイルインデックス。 */
ENG_MapID eng_map_create(const int* tiles, int cols, int rows,
                          int tile_w, int tile_h);

/** タイルマップ解放。 */
void eng_map_destroy(ENG_MapID id);

/* 寸法 */
int   eng_map_cols(ENG_MapID id);
int   eng_map_rows(ENG_MapID id);
float eng_map_pixel_w(ENG_MapID id);
float eng_map_pixel_h(ENG_MapID id);
int   eng_map_tile_w(ENG_MapID id);
int   eng_map_tile_h(ENG_MapID id);

/* タイル値アクセス */
int  eng_map_get(ENG_MapID id, int col, int row);
void eng_map_set(ENG_MapID id, int col, int row, int tile_idx);

/**
 * タイルマップとボディの衝突応答を行う。
 * tile_idx > 0 のセルをソリッドとして扱い、ボディを押し出す。
 * eng_phys_update() の後に呼ぶこと。
 */
void eng_map_resolve_body(ENG_MapID map_id, ENG_Physics* p, ENG_BodyID body_id);

/**
 * 矩形がソリッドタイルと重なっているかどうか (センサー判定)。
 */
bool eng_map_overlaps_rect(ENG_MapID id, float x, float y, float w, float h);

/** ピクセル座標 (px, py) が含まれるタイルのインデックスを返す (-1=範囲外)。 */
int eng_map_get_tile_at_pixel(ENG_MapID id, float px, float py);

#ifdef __cplusplus
}
#endif
