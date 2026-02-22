/**
 * src/eng_map.c — CSV タイルマップ実装
 *
 * Copyright (c) 2026 Reo Shiozawa — MIT License
 */
#include "eng_2d.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define MAX_MAPS 16

typedef struct {
    int*  tiles;
    int   cols, rows;
    int   tile_w, tile_h;
    bool  used;
} TileMap;

static TileMap g_maps[MAX_MAPS];

static TileMap* get_map(ENG_MapID id) {
    if (id == 0 || id > MAX_MAPS) return NULL;
    return g_maps[id-1].used ? &g_maps[id-1] : NULL;
}

ENG_MapID eng_map_create(const int* tiles, int cols, int rows,
                          int tile_w, int tile_h) {
    for (int i = 0; i < MAX_MAPS; ++i) {
        if (!g_maps[i].used) {
            TileMap* m = &g_maps[i];
            m->tiles  = malloc(sizeof(int) * cols * rows);
            if (!m->tiles) return 0;
            memcpy(m->tiles, tiles, sizeof(int) * cols * rows);
            m->cols   = cols;
            m->rows   = rows;
            m->tile_w = tile_w;
            m->tile_h = tile_h;
            m->used   = true;
            return (ENG_MapID)(i + 1);
        }
    }
    fprintf(stderr, "[eng_2d] タイルマップスロット満杯\n");
    return 0;
}

ENG_MapID eng_map_load_csv(const char* path, int tile_w, int tile_h) {
    FILE* f = fopen(path, "r");
    if (!f) { fprintf(stderr, "[eng_2d] CSV読込失敗: %s\n", path); return 0; }

    /* まず行数・列数を計測 */
    int cols = 0, rows = 0, cur_cols = 0;
    int c;
    while ((c = fgetc(f)) != EOF) {
        if      (c == ',')  { cur_cols++; }
        else if (c == '\n') {
            if (cur_cols > 0 || rows == 0) {
                if (cols == 0) cols = cur_cols + 1;
                rows++;
            }
            cur_cols = 0;
        }
    }
    if (cur_cols > 0) rows++; /* 最終行に改行がない場合 */
    rewind(f);

    if (cols <= 0 || rows <= 0) { fclose(f); return 0; }

    int* tiles = calloc(cols * rows, sizeof(int));
    if (!tiles) { fclose(f); return 0; }

    int col = 0, row = 0;
    while (fscanf(f, "%d", &tiles[row * cols + col]) == 1) {
        c = fgetc(f);
        if (c == ',') { col++; }
        else if (c == '\n' || c == EOF) { col = 0; row++; }
    }
    fclose(f);

    ENG_MapID id = eng_map_create(tiles, cols, rows, tile_w, tile_h);
    free(tiles);
    return id;
}

void eng_map_destroy(ENG_MapID id) {
    TileMap* m = get_map(id);
    if (m) { free(m->tiles); memset(m, 0, sizeof(*m)); }
}

int   eng_map_cols(ENG_MapID id)     { TileMap* m=get_map(id); return m?m->cols:0; }
int   eng_map_rows(ENG_MapID id)     { TileMap* m=get_map(id); return m?m->rows:0; }
int   eng_map_tile_w(ENG_MapID id)   { TileMap* m=get_map(id); return m?m->tile_w:0; }
int   eng_map_tile_h(ENG_MapID id)   { TileMap* m=get_map(id); return m?m->tile_h:0; }
float eng_map_pixel_w(ENG_MapID id)  { TileMap* m=get_map(id); return m?(float)(m->cols*m->tile_w):0; }
float eng_map_pixel_h(ENG_MapID id)  { TileMap* m=get_map(id); return m?(float)(m->rows*m->tile_h):0; }

int eng_map_get(ENG_MapID id, int col, int row) {
    TileMap* m = get_map(id);
    if (!m || col<0 || row<0 || col>=m->cols || row>=m->rows) return -1;
    return m->tiles[row * m->cols + col];
}
void eng_map_set(ENG_MapID id, int col, int row, int tile_idx) {
    TileMap* m = get_map(id);
    if (!m || col<0 || row<0 || col>=m->cols || row>=m->rows) return;
    m->tiles[row * m->cols + col] = tile_idx;
}

bool eng_map_overlaps_rect(ENG_MapID id, float x, float y, float w, float h) {
    TileMap* m = get_map(id);
    if (!m) return false;
    int c0 = (int)(x / m->tile_w);
    int r0 = (int)(y / m->tile_h);
    int c1 = (int)((x + w - 1) / m->tile_w);
    int r1 = (int)((y + h - 1) / m->tile_h);
    for (int r = r0; r <= r1; ++r)
        for (int c = c0; c <= c1; ++c) {
            int ti = eng_map_get(id, c, r);
            if (ti > 0) return true;
        }
    return false;
}

/* タイルマップとの衝突応答 (X/Y 軸分離) */
void eng_map_resolve_body(ENG_MapID map_id, ENG_Physics* p, ENG_BodyID body_id) {
    TileMap* m = get_map(map_id);
    if (!m || !p) return;

    float bx  = eng_body_x(p, body_id);
    float by  = eng_body_y(p, body_id);
    float bvx = eng_body_vx(p, body_id);
    float bvy = eng_body_vy(p, body_id);
    float bw  = eng_body_w(p, body_id);
    float bh  = eng_body_h(p, body_id);

    /* X 軸押し出し */
    if (bvx != 0.0f && eng_map_overlaps_rect(map_id, bx, by, bw, bh)) {
        /* 1px 単位で戻す */
        float dir = (bvx > 0) ? -1.0f : 1.0f;
        while (eng_map_overlaps_rect(map_id, bx, by, bw, bh)) {
            bx += dir;
        }
        bvx = 0;
    }

    /* Y 軸押し出し */
    if (eng_map_overlaps_rect(map_id, bx, by, bw, bh)) {
        float dir = (bvy > 0) ? -1.0f : 1.0f;
        bool was_falling = bvy > 0;
        while (eng_map_overlaps_rect(map_id, bx, by, bw, bh)) {
            by += dir;
        }
        if (was_falling) {
            /* set on_ground via zero vy */
        }
        bvy = 0;
    }

    eng_body_set_pos(p, body_id, bx, by);
    eng_body_set_vel(p, body_id, bvx, bvy);
}

/* ── ピクセル座標からタイルインデックスを返す ─────────────*/
int eng_map_get_tile_at_pixel(ENG_MapID id, float px, float py) {
    TileMap* m = get_map(id);
    if (!m) return -1;
    int col = (int)(px / m->tile_w);
    int row = (int)(py / m->tile_h);
    if (col < 0 || col >= m->cols || row < 0 || row >= m->rows) return -1;
    return m->tiles[row * m->cols + col];
}
