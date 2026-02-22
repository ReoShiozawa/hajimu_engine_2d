# hajimu_engine_2d

> **はじむ言語**向け 軽量 2D 物理エンジン + CSV タイルマップシステム

外部ライブラリ不要の AABB 物理エンジンと、CSV ファイルからタイルマップを読み込むシステムを提供します。  
`hajimu_engine_render` と組み合わせてプラットフォーマーゲームやタイルマップゲームを作れます。

---

## 特徴

- **外部ライブラリ不要** — 標準 C11 のみ (math.h)  
- **AABB 物理** — Static / Dynamic / Kinematic の 3 種ボディ、重力・反発・減衰  
- **接地判定** — `物体接地中()` で地面に立っているか確認  
- **CSV タイルマップ** — `tile_index > 0` のセルを自動でソリッド判定  
- **ボディ ↔ マップ衝突解決** — `タイルマップ衝突解決()` 1 関数で完結  

---

## 依存

| 要件 | 詳細 |
|---|---|
| はじむ (nihongo) | v0.9 以上 |
| C11 コンパイラ | clang / gcc |
| CMake | 3.15 以上 |

---

## ビルド

```bash
git clone https://github.com/ReoShiozawa/hajimu_engine_2d.git
cd hajimu_engine_2d

make          # → build/engine_2d.hjp
make install  # → ~/.hajimu/plugins/engine_2d/
```

---

## クイックスタート

```jp
取込「engine_2d」

# 物理ワールド作成 (重力: x=0, y=500 px/s²)
変数「ワールド」 = 物理ワールド作成(0, 500)

# 地面 (Static)
変数「地面」 = 物体作成(「ワールド」, 0, 50, 300, 800, 20)

# プレイヤー (Dynamic)
変数「自機」 = 物体作成(「ワールド」, 1, 100, 100, 32, 48)

ループ開始
    変数「dt」 = デルタ時間()
    物理更新(「ワールド」, 「dt」)

    # 表示
    スプライト描画(プレイヤーTexID, 物体X(「ワールド」, 「自機」), 物体Y(「ワールド」, 「自機」), 32, 48)
ループ終わり
```

---

## API リファレンス

### 物理ワールド

| 関数 | 引数 | 戻り値 | 説明 |
|---|---|---|---|
| `物理ワールド作成(重力X, 重力Y)` | float, float | int (ワールドID) | ワールド生成 |
| `物理ワールド削除(id)` | int | null | ワールド解放 |
| `物理更新(id, dt)` | int, float | null | 毎フレーム呼ぶ |

### ボディ (物体)

| 関数 | 引数 | 戻り値 | 説明 |
|---|---|---|---|
| `物体作成(world, type, x, y, w, h)` | — | int (BodyID) | type: 0=static 1=dynamic 2=kinematic |
| `物体削除(world, body)` | — | null | ボディ解放 |
| `物体X(world, body)` | — | float | 左上 X |
| `物体Y(world, body)` | — | float | 左上 Y |
| `物体幅(world, body)` | — | float | 幅 |
| `物体高さ(world, body)` | — | float | 高さ |
| `物体位置設定(world, body, x, y)` | — | null | テレポート |
| `物体速度X(world, body)` | — | float | X 方向速度 |
| `物体速度Y(world, body)` | — | float | Y 方向速度 |
| `物体速度設定(world, body, vx, vy)` | — | null | 速度変更 |
| `物体速度加算(world, body, dvx, dvy)` | — | null | 速度加算 |
| `重力スケール設定(world, body, s)` | — | null | 1.0=標準, 0=無重力 |
| `反発設定(world, body, r)` | — | null | 0=吸収 〜 1=完全弾性 |
| `減衰設定(world, body, d)` | — | null | 0=なし 〜 1=即停止 |
| `物体重複確認(world, a, b)` | — | bool | 2 ボディが接触中か |
| `物体矩形重複(world, body, x,y,w,h)` | — | bool | ボディと矩形が接触中か |
| `物体接地中(world, body)` | — | bool | 地面に立っているか |

### タイルマップ

| 関数 | 引数 | 戻り値 | 説明 |
|---|---|---|---|
| `タイルマップCSV読込(path, tileW, tileH)` | str, int, int | int (MapID) | CSV ファイルから読込 |
| `タイルマップ削除(map)` | int | null | 解放 |
| `タイル列数(map)` | int | int | — |
| `タイル行数(map)` | int | int | — |
| `マップ幅(map)` | int | float | px |
| `マップ高さ(map)` | int | float | px |
| `タイル幅(map)` | int | int | px |
| `タイル高さ(map)` | int | int | px |
| `タイル取得(map, col, row)` | — | int | タイルインデックス |
| `タイル設定(map, col, row, idx)` | — | null | タイル書き換え |
| `タイルマップ衝突解決(map, world, body)` | — | null | ボディ押し出し |
| `マップ矩形重複(map, x, y, w, h)` | — | bool | センサー判定 |

### CSV フォーマット

```csv
0,0,0,0,0
0,0,1,0,0
1,1,1,1,1
```

- `0` = 空気 (通り抜け可)  
- `1` 以上 = ソリッドタイル  
- 任意のインデックスでスプライトシート描画が可能

---

## サンプル

- [examples/hello_2d.jp](examples/hello_2d.jp) — 物理ボールとタイルマップ

---

## ライセンス

MIT License — Copyright (c) 2026 Reo Shiozawa
