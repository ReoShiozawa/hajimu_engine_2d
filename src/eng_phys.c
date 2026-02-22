/**
 * src/eng_phys.c — 軽量 AABB 物理エンジン
 *
 * Copyright (c) 2026 Reo Shiozawa — MIT License
 */
#include "eng_2d.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#define MAX_BODIES 512

typedef struct {
    float x, y, w, h;   /* AABB 左上+サイズ */
    float vx, vy;        /* 速度 */
    float grav_scale;    /* 重力スケール */
    float restitution;   /* 反発係数 */
    float damping;       /* 線形減衰 */
    float friction;      /* 摩擦係数 (0=なし, 1=即停止) */
    ENG_BodyType type;
    bool  used;
    bool  on_ground;
    bool  active;        /* false のとき物理更新・衝突をスキップ */
} Body;

struct ENG_Physics {
    Body   bodies[MAX_BODIES];
    float  grav_x, grav_y;
};

/* ── ライフサイクル ─────────────────────────────────────*/
ENG_Physics* eng_phys_create(float gravity_x, float gravity_y) {
    ENG_Physics* p = calloc(1, sizeof(ENG_Physics));
    if (!p) return NULL;
    p->grav_x = gravity_x;
    p->grav_y = gravity_y;
    return p;
}

void eng_phys_destroy(ENG_Physics* p) { free(p); }

/* ── ボディ ─────────────────────────────────────────────*/
ENG_BodyID eng_body_create(ENG_Physics* p, ENG_BodyType type,
                            float x, float y, float w, float h) {
    if (!p) return 0;
    for (int i = 0; i < MAX_BODIES; ++i) {
        if (!p->bodies[i].used) {
            Body* b = &p->bodies[i];
            b->x = x; b->y = y; b->w = w; b->h = h;
            b->vx = 0; b->vy = 0;
            b->grav_scale  = 1.0f;
            b->restitution = 0.0f;
            b->damping     = 0.0f;
            b->friction    = 0.0f;
            b->type        = type;
            b->used        = true;
            b->on_ground   = false;
            b->active      = true;
            return (ENG_BodyID)(i + 1);
        }
    }
    fprintf(stderr, "[eng_2d] ボディスロット満杯\n");
    return 0;
}

static Body* get_body(ENG_Physics* p, ENG_BodyID id) {
    if (!p || id == 0 || id > MAX_BODIES) return NULL;
    return p->bodies[id-1].used ? &p->bodies[id-1] : NULL;
}

void  eng_body_destroy(ENG_Physics* p, ENG_BodyID id) {
    Body* b = get_body(p, id);
    if (b) memset(b, 0, sizeof(*b));
}
float eng_body_x(ENG_Physics* p, ENG_BodyID id)  { Body* b=get_body(p,id); return b?b->x:0; }
float eng_body_y(ENG_Physics* p, ENG_BodyID id)  { Body* b=get_body(p,id); return b?b->y:0; }
float eng_body_vx(ENG_Physics* p, ENG_BodyID id) { Body* b=get_body(p,id); return b?b->vx:0; }
float eng_body_vy(ENG_Physics* p, ENG_BodyID id) { Body* b=get_body(p,id); return b?b->vy:0; }

void eng_body_set_pos(ENG_Physics* p, ENG_BodyID id, float x, float y) {
    Body* b = get_body(p,id); if(b){b->x=x;b->y=y;}
}
void eng_body_set_vel(ENG_Physics* p, ENG_BodyID id, float vx, float vy) {
    Body* b = get_body(p,id); if(b){b->vx=vx;b->vy=vy;}
}
void eng_body_add_vel(ENG_Physics* p, ENG_BodyID id, float dvx, float dvy) {
    Body* b = get_body(p,id); if(b){b->vx+=dvx;b->vy+=dvy;}
}
void eng_body_set_gravity_scale(ENG_Physics* p, ENG_BodyID id, float s) {
    Body* b = get_body(p,id); if(b) b->grav_scale=s;
}
void eng_body_set_restitution(ENG_Physics* p, ENG_BodyID id, float r) {
    Body* b = get_body(p,id); if(b) b->restitution=r;
}
void eng_body_set_damping(ENG_Physics* p, ENG_BodyID id, float d) {
    Body* b = get_body(p,id); if(b) b->damping=d;
}

/* AABB 重なり判定 */
static bool aabb_overlap(const Body* a, const Body* b) {
    return a->x < b->x+b->w && a->x+a->w > b->x &&
           a->y < b->y+b->h && a->y+a->h > b->y;
}
bool eng_body_overlaps(ENG_Physics* p, ENG_BodyID a, ENG_BodyID b) {
    Body* ba = get_body(p,a); Body* bb = get_body(p,b);
    return ba && bb && aabb_overlap(ba, bb);
}
bool eng_body_overlaps_rect(ENG_Physics* p, ENG_BodyID id,
                             float rx, float ry, float rw, float rh) {
    Body* b = get_body(p,id);
    if (!b) return false;
    Body r = {.x=rx,.y=ry,.w=rw,.h=rh,.used=true};
    return aabb_overlap(b, &r);
}
bool eng_body_on_ground(ENG_Physics* p, ENG_BodyID id) {
    Body* b = get_body(p,id); return b && b->on_ground;
}
float eng_body_w(ENG_Physics* p, ENG_BodyID id) { Body* b=get_body(p,id); return b?b->w:0; }
float eng_body_h(ENG_Physics* p, ENG_BodyID id) { Body* b=get_body(p,id); return b?b->h:0; }

/* ── v1.1.0 追加: インパルス / 有効化 / フリクション ─────*/
void eng_body_apply_impulse(ENG_Physics* p, ENG_BodyID id, float ix, float iy) {
    Body* b = get_body(p, id);
    if (b && b->type != ENG_BODY_STATIC) {
        b->vx += ix;
        b->vy += iy;
    }
}

void eng_body_set_active(ENG_Physics* p, ENG_BodyID id, bool active) {
    Body* b = get_body(p, id);
    if (b) b->active = active;
}

bool eng_body_get_active(ENG_Physics* p, ENG_BodyID id) {
    Body* b = get_body(p, id);
    return b ? b->active : false;
}

void eng_body_set_friction(ENG_Physics* p, ENG_BodyID id, float friction) {
    Body* b = get_body(p, id);
    if (b) b->friction = friction < 0.0f ? 0.0f : friction;
}

/* ── 物理更新 ────────────────────────────────────────────*/
void eng_phys_update(ENG_Physics* p, float dt) {
    if (!p || dt <= 0.0f) return;

    /* 重力・速度統合 */
    for (int i = 0; i < MAX_BODIES; ++i) {
        Body* b = &p->bodies[i];
        if (!b->used || !b->active || b->type == ENG_BODY_STATIC) continue;
        if (b->type == ENG_BODY_DYNAMIC) {
            b->vx += p->grav_x * b->grav_scale * dt;
            b->vy += p->grav_y * b->grav_scale * dt;
        }
        /* 減衰 */
        float damp = 1.0f - b->damping * dt;
        if (damp < 0.0f) damp = 0.0f;
        b->vx *= damp; b->vy *= damp;
        /* 摩擦 (接地中のみ水平方向に適用) */
        if (b->on_ground && b->friction > 0.0f) {
            float fric = 1.0f - b->friction * dt;
            if (fric < 0.0f) fric = 0.0f;
            b->vx *= fric;
        }
        /* 移動 */
        b->x += b->vx * dt;
        b->y += b->vy * dt;
        b->on_ground = false;
    }

    /* AABB 衝突応答 (simple swept-AABB push-out) */
    for (int i = 0; i < MAX_BODIES; ++i) {
        Body* a = &p->bodies[i];
        if (!a->used || !a->active || a->type == ENG_BODY_STATIC) continue;
        for (int j = 0; j < MAX_BODIES; ++j) {
            if (i == j) continue;
            Body* b = &p->bodies[j];
            if (!b->used || !b->active) continue;
            if (a->type == ENG_BODY_DYNAMIC && b->type == ENG_BODY_DYNAMIC && i > j) continue;
            if (!aabb_overlap(a, b)) continue;

            float ox = 0, oy = 0;
            float ax_ctr = a->x + a->w*0.5f, ay_ctr = a->y + a->h*0.5f;
            float bx_ctr = b->x + b->w*0.5f, by_ctr = b->y + b->h*0.5f;
            float over_x = (a->w + b->w)*0.5f - fabsf(ax_ctr - bx_ctr);
            float over_y = (a->h + b->h)*0.5f - fabsf(ay_ctr - by_ctr);

            if (over_x < over_y) {
                ox = (ax_ctr < bx_ctr) ? -over_x : over_x;
            } else {
                oy = (ay_ctr < by_ctr) ? -over_y : over_y;
            }

            /* 押し出し */
            if (b->type == ENG_BODY_STATIC) {
                a->x += ox; a->y += oy;
                if (oy < 0) { a->on_ground = true; }
                if (ox != 0 && a->vx * ox < 0) {
                    a->vx = -a->vx * a->restitution;
                }
                if (oy != 0 && a->vy * oy < 0) {
                    if (oy < 0) a->on_ground = true;
                    a->vy = -a->vy * a->restitution;
                }
            } else {
                /* dynamic vs dynamic: 等質量で半分ずつ */
                a->x += ox*0.5f; a->y += oy*0.5f;
                b->x -= ox*0.5f; b->y -= oy*0.5f;
                if (ox != 0) { float tmp=(a->vx+b->vx)*0.5f; a->vx=tmp; b->vx=tmp; }
                if (oy != 0) { float tmp=(a->vy+b->vy)*0.5f; a->vy=tmp; b->vy=tmp; }
            }
        }
    }
}

/* ── ボディサイズ設定 ────────────────────────────────────*/
void eng_body_set_size(ENG_Physics* p, ENG_BodyID id, float w, float h) {
    Body* b = get_body(p, id);
    if (b) { b->w = w; b->h = h; }
}

/* ── レイキャスト (AABB スラブ法) ───────────────────────*/
ENG_BodyID eng_phys_raycast(ENG_Physics* p,
                              float ox, float oy,
                              float dx, float dy, float len,
                              float* hit_x, float* hit_y) {
    if (!p || len <= 0.0f) return 0;
    float mag = sqrtf(dx*dx + dy*dy);
    if (mag < 1e-6f) return 0;
    float ux = dx / mag, uy = dy / mag;

    ENG_BodyID best_id = 0;
    float best_t = len;

    for (int i = 0; i < MAX_BODIES; i++) {
        Body* b = &p->bodies[i];
        if (!b->used || !b->active) continue;
        float inv_ux = (ux != 0.0f) ? 1.0f/ux : 1e30f;
        float inv_uy = (uy != 0.0f) ? 1.0f/uy : 1e30f;
        float tx1 = (b->x        - ox) * inv_ux;
        float tx2 = (b->x + b->w - ox) * inv_ux;
        float ty1 = (b->y        - oy) * inv_uy;
        float ty2 = (b->y + b->h - oy) * inv_uy;
        if (tx1 > tx2) { float tmp=tx1; tx1=tx2; tx2=tmp; }
        if (ty1 > ty2) { float tmp=ty1; ty1=ty2; ty2=tmp; }
        float tmin = tx1 > ty1 ? tx1 : ty1;
        float tmax = tx2 < ty2 ? tx2 : ty2;
        if (tmax < 0.0f || tmin > tmax || tmin > best_t) continue;
        float t = tmin < 0.0f ? 0.0f : tmin;
        best_t  = t;
        best_id = (ENG_BodyID)(i + 1);
    }
    if (best_id && hit_x && hit_y) {
        *hit_x = ox + ux * best_t;
        *hit_y = oy + uy * best_t;
    }
    return best_id;
}
