# 3D 太空飛行遊戲
### Space Dodge 101

> 3D 太空飛行遊戲｜C++ 以 OpenGL 結合 GLUT 與 3DS 模型載入技術

駕駛太空船穿越星空，閃避迎面飛來的**台北 101**，收集愛心補血，**45 秒內存活越久分數越高**！

---

## ✨ 特色

-  **3D 星空場景**：六面 Skybox 貼圖，營造身處宇宙的沉浸感
-  **自製 101 障礙物**：以 Maya／3ds Max 建模，匯出 3DS 格式載入遊戲，並持續自轉
-  **六方向飛行**：前後、左右、上下自由移動太空船
-  **平滑跟隨攝影機**：鏡頭以插值方式跟在太空船後方，畫面不突兀
-  **生命值系統**：碰撞扣血、吃愛心補血，畫面左上角即時顯示血條
-  **碰撞閃爍特效**：撞到障礙物後太空船會閃爍，提示受到傷害

##  技術

| 項目 | 使用技術 |
|------|----------|
| 語言 | C++（C 風格寫法） |
| 繪圖 API | OpenGL（GL、GLU） |
| 視窗與輸入 | GLUT |
| 3D 模型 | 3DS 格式載入器（spacesimulator.net 教學程式） |
| 貼圖 | BMP 紋理載入 |
| 建模工具 | Maya、3ds Max |
| 開發環境 | Visual Studio 2022（Windows） |

## 🎮 操作

| 按鍵 | 功能 |
|------|------|
| `W` / `S` | 向前／向後 |
| `A` / `D` | 向左／向右 |
| `X` / `Z` | 上升／下降 |
| `R` | 遊戲結束後重新開始 |
| `ESC` | 離開遊戲 |

##  遊戲規則

| 項目 | 規則 |
|------|------|
| ⏱️ 時間 | 每局 45 秒，時間到即結束 |
| ❤️ 初始血量 | 100 |
| 💥 撞到 101 | 扣 20 血，血量歸零立即結束 |
| 💛 吃到愛心 | 回 30 血，上限 100 |
| 🏆 分數 | 依存活時間累積，每 0.1 秒 +1 分 |

結束畫面會顯示 **GAME OVER** 與最終分數，按 `R` 再玩一局。

##  執行方式

**直接遊玩**：執行資料夾中的 `tutorial4.exe`（`glut32.dll` 與模型、貼圖檔需放在同一資料夾）

**從原始碼編譯**：
1. 用 Visual Studio 2022 開啟 `tutorial4.sln`
2. 確認已設定 GLUT（`glut.h`、`glut32.lib`、`glut32.dll`）
3. 選擇 **Debug** 組態後執行

## 📁 檔案結構

```
├── tutorial4.cpp        # 遊戲主程式：遊戲邏輯、碰撞、繪圖、UI
├── 3dsloader.cpp / .h   # 3DS 模型載入
├── texture.cpp / .h     # BMP 貼圖載入
├── spaceship.3DS        # 太空船模型
├── 101.3ds              # 台北 101 模型（原始檔 101.max / 101.mb）
├── heart.3ds            # 愛心補給品模型
├── nx/px/ny/py/nz/pz.bmp # Skybox 六面貼圖
└── tutorial4.sln        # Visual Studio 方案檔
```

##  遊戲畫面


<p align="center">
  <img width="49%" alt="遊戲畫面" src="https://github.com/user-attachments/assets/80dc2c03-569c-48d5-a77c-a5d0625c1ce8" />
  <img width="49%" alt="結束畫面" src="https://github.com/user-attachments/assets/80abbb70-7c34-4c4d-972c-19771c140cc5" />
</p>

## 引用

3DS 模型載入與貼圖程式改寫自 [spacesimulator.net](http://www.spacesimulator.net) Tutorial 4（作者 Damiano Vitulli，BSD 授權）。
