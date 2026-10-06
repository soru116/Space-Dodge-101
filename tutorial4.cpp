/*
 * 3D太空船星空探險遊戲
 *
 * 遊戲規則：
 * - 使用WASDXZ控制太空船移動
 * - 避開101障礙物
 * - 遊戲時間45秒
 * - 碰撞會扣血量，血量歸零遊戲結束
 * - 存活時間越長分數越高
 */

#include <windows.h>
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "tutorial4.h"
#include "texture.h"
#include "3dsloader.h"

 /**********************************************************
  * VARIABLES DECLARATION
  *********************************************************/

  // 螢幕設定
int screen_width = 800;
int screen_height = 600;

// 3D物件
obj_type rocket, obstacle;

// 太空船狀態
float rocket_pos_x = 0.0f;
float rocket_pos_y = 0.0f;
float rocket_pos_z = 0.0f;
float rocket_speed = 3.0f;

// 碰撞特效
int collision_occurred = 0; // 標記是否發生碰撞
int collision_timer = 0;    // 碰撞計時器
int blink_interval = 5;     // 閃爍間隔 (幀數)
int blink_duration = 240;   // 閃爍總時間 (幀數)
int is_rocket_visible = 1;  // 控制火箭是否可見

// 遊戲狀態
int game_time = 45;         // 遊戲時間45秒
int health = 100;           // 血量
int score = 0;              // 分數
int game_over = 0;          // 遊戲結束標誌
int game_start_time;        // 遊戲開始時間

// 補給品系統
obj_type health_pickup_model; // 愛心3D模型本體
#define MAX_HEALTH_PICKUPS 8  // 最多同時存在3個補血包

typedef struct {
    float x, y, z;
    int active; // 此補給品是否活躍在場景中 (0:不活躍, 1:活躍)
    float rotation; // 補給品自轉的角度
} HealthPickup;

HealthPickup health_pickup_items[MAX_HEALTH_PICKUPS]; // 用來管理多個補血包實例的陣列
float pickup_speed = 0.5f;        // 補給品移動速度
int active_health_pickups_count = 0; // 追蹤目前畫面上有多少個活躍的補血包

// 攝影機設定
float camera_distance = 250.0f;
float camera_height = 50.0f;
float camera_smooth = 0.1f;
float camera_x = 0.0f, camera_y = 50.0f, camera_z = 150.0f;

// 障礙物系統
#define MAX_OBSTACLES 20
typedef struct {
    float x, y, z;
    int active;
    float rotation;
    int cooldown_timer; // 碰撞冷卻計時器
} Obstacle;

Obstacle obstacles[MAX_OBSTACLES];
float obstacle_speed = 0.5f;

// Skybox貼圖ID
GLuint skybox_texture[6];

/**********************************************************
 * 初始化障礙物
 *********************************************************/
void InitObstacles() {
    srand(time(NULL));
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        obstacles[i].x = (rand() % 800) - 400;
        obstacles[i].y = (rand() % 400) - 200;
        obstacles[i].z = -500 - (rand() % 1500);
        obstacles[i].active = 1;
        obstacles[i].rotation = 0.0f;
        obstacles[i].cooldown_timer = 0;
    }
}

/**********************************************************
 * 更新障礙物位置
 *********************************************************/
void UpdateObstacles() {
    if (game_over) return;

    for (int i = 0; i < MAX_OBSTACLES; i++) {
        // 處理冷卻計時器
        if (obstacles[i].cooldown_timer > 0) {
            obstacles[i].cooldown_timer--;
            if (obstacles[i].cooldown_timer == 0) {
                // 冷卻結束，重新生成障礙物
                obstacles[i].active = 1;
                obstacles[i].x = (rand() % 800) - 400;
                obstacles[i].y = (rand() % 400) - 200;
                obstacles[i].z = rocket_pos_z - 1000 - (rand() % 1000);
                obstacles[i].rotation = 0.0f;
            }
        }
        if (obstacles[i].active) {
            obstacles[i].z += obstacle_speed;
            obstacles[i].rotation += 1.0f;

            // 如果障礙物移動到太空船後面，重新生成
            if (obstacles[i].z > rocket_pos_z + 100 && obstacles[i].cooldown_timer == 0) {
                obstacles[i].x = (rand() % 800) - 400;
                obstacles[i].y = (rand() % 400) - 200;
                obstacles[i].z = rocket_pos_z - 1000 - (rand() % 1000);
                obstacles[i].active = 1;
                obstacles[i].rotation = 0.0f;
            }
        }
    }
}

/**********************************************************
 * 初始化所有血量補給品
 *********************************************************/
void InitHealthPickups() {
    for (int i = 0; i < MAX_HEALTH_PICKUPS; i++) {
        health_pickup_items[i].active = 0; // 將陣列中每個補血包都設為不活躍
    }
    active_health_pickups_count = 0; // 重置活躍補血包的計數器
}

/**********************************************************
 * 生成一個血量補給品 
 *********************************************************/
void SpawnHealthPickup() {
    if (active_health_pickups_count >= MAX_HEALTH_PICKUPS) {
        return; // 如果活躍補血包數量已經達到上限，就不再生成新的
    }

    // 找到一個不活躍的補血包槽位
    for (int i = 0; i < MAX_HEALTH_PICKUPS; i++) {
        if (!health_pickup_items[i].active) {
            health_pickup_items[i].x = (rand() % 800) - 400; // 隨機 X 座標與障礙物範圍一致
            health_pickup_items[i].y = (rand() % 400) - 200; // 隨機 Y 座標與障礙物範圍一致
            health_pickup_items[i].z = rocket_pos_z - 800 - (rand() % 1200); // 在太空船前方隨機 Z 座標
            health_pickup_items[i].active = 1;
            health_pickup_items[i].rotation = 0.0f;
            active_health_pickups_count++; // 活躍補血包數量增加1
            break; // 找到一個就生成，然後退出迴圈
        }
    }
}

/**********************************************************
 * 更新血量補給品位置
 *********************************************************/
void UpdateHealthPickup() {
    if (game_over) return;

    for (int i = 0; i < MAX_HEALTH_PICKUPS; i++) {
        if (health_pickup_items[i].active) {
            health_pickup_items[i].z += pickup_speed;
            health_pickup_items[i].rotation += 2.0f; // 讓補給品自轉

            // 如果補給品移動到太空船後面 (超出畫面)，則設為不活躍
            if (health_pickup_items[i].z > rocket_pos_z + 100) {
                health_pickup_items[i].active = 0;
                active_health_pickups_count--; // 活躍補血包數量減少1
            }
        }
    }

    // 有一定機率生成新的補血包，直到達到最大數量
    // rand() % 300 == 0 大約每300幀（約5秒）嘗試生成一個新的，確保稀疏性
    if (active_health_pickups_count < MAX_HEALTH_PICKUPS && (rand() % 300 == 0)) {
        SpawnHealthPickup();
    }
}

/**********************************************************
 * 碰撞檢測
 *********************************************************/
void CheckCollisions() {
    if (game_over) return;

    float collision_distance = 80.0f; // 障礙物的碰撞距離閾值
    float pickup_collision_distance = 80.0f; // 補給品碰撞距離閾值，設定為與障礙物一樣大小

    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (obstacles[i].active && obstacles[i].cooldown_timer == 0) {
            float dx = rocket_pos_x - obstacles[i].x;
            float dy = rocket_pos_y - obstacles[i].y;
            float dz = rocket_pos_z - obstacles[i].z;
            float distance = sqrt(dx * dx + dy * dy + dz * dz);

            if (distance < collision_distance) {
                health -= 20;
                // 觸發碰撞特效
                collision_occurred = 1;
                collision_timer = 0;

                // 啟動障礙物的冷卻計時器
                obstacles[i].active = 0;
                obstacles[i].cooldown_timer = 60;

                if (health <= 0) {
                    game_over = 1;
                    health = 0;
                }
                break;
            }
        }
    }
    // 檢查與血量補給品的碰撞
    for (int i = 0; i < MAX_HEALTH_PICKUPS; i++) {
        if (health_pickup_items[i].active) {
            float dx = rocket_pos_x - health_pickup_items[i].x;
            float dy = rocket_pos_y - health_pickup_items[i].y;
            float dz = rocket_pos_z - health_pickup_items[i].z;
            float distance = sqrt(dx * dx + dy * dy + dz * dz);

            if (distance < pickup_collision_distance) {
                health += 30; // 增加血量
                if (health > 100) health = 100; // 血量上限
                health_pickup_items[i].active = 0; // 補給品被吃掉，設為不活躍
                active_health_pickups_count--; // 活躍補血包數量減少1
                break;
            }
        }
    }
}

/**********************************************************
 * 載入Skybox貼圖
 *********************************************************/
void LoadSkybox() {
    skybox_texture[0] = LoadBitmap("nx.bmp"); // Right
    skybox_texture[1] = LoadBitmap("px.bmp");  // Left
    skybox_texture[2] = LoadBitmap("nz.bmp");    // Top
    skybox_texture[3] = LoadBitmap("pz.bmp");// Bottom
    skybox_texture[4] = LoadBitmap("ny.bmp"); // Front
    skybox_texture[5] = LoadBitmap("py.bmp");  // Back

}

/**********************************************************
 * 繪製Skybox
 *********************************************************/
void DrawSkybox() {
    glDisable(GL_DEPTH_TEST);
    glPushMatrix();

    float size = 2000.0f;

    // 前面
    glBindTexture(GL_TEXTURE_2D, skybox_texture[3]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-size, -size, -size);
    glTexCoord2f(1, 0); glVertex3f(size, -size, -size);
    glTexCoord2f(1, 1); glVertex3f(size, size, -size);
    glTexCoord2f(0, 1); glVertex3f(-size, size, -size);
    glEnd();

    // 後面
    glBindTexture(GL_TEXTURE_2D, skybox_texture[2]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(size, -size, size);
    glTexCoord2f(1, 0); glVertex3f(-size, -size, size);
    glTexCoord2f(1, 1); glVertex3f(-size, size, size);
    glTexCoord2f(0, 1); glVertex3f(size, size, size);
    glEnd();

    // 左面
    glBindTexture(GL_TEXTURE_2D, skybox_texture[0]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-size, -size, size);
    glTexCoord2f(1, 0); glVertex3f(-size, -size, -size);
    glTexCoord2f(1, 1); glVertex3f(-size, size, -size);
    glTexCoord2f(0, 1); glVertex3f(-size, size, size);
    glEnd();

    // 右面
    glBindTexture(GL_TEXTURE_2D, skybox_texture[1]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(size, -size, -size);
    glTexCoord2f(1, 0); glVertex3f(size, -size, size);
    glTexCoord2f(1, 1); glVertex3f(size, size, size);
    glTexCoord2f(0, 1); glVertex3f(size, size, -size);
    glEnd();

    // 上面
    glBindTexture(GL_TEXTURE_2D, skybox_texture[5]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-size, size, -size);
    glTexCoord2f(1, 0); glVertex3f(size, size, -size);
    glTexCoord2f(1, 1); glVertex3f(size, size, size);
    glTexCoord2f(0, 1); glVertex3f(-size, size, size);
    glEnd();

    // 下面
    glBindTexture(GL_TEXTURE_2D, skybox_texture[4]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-size, -size, size);
    glTexCoord2f(1, 0); glVertex3f(size, -size, size);
    glTexCoord2f(1, 1); glVertex3f(size, -size, -size);
    glTexCoord2f(0, 1); glVertex3f(-size, -size, -size);
    glEnd();

    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
}

/**********************************************************
 * 更新攝影機位置（平滑跟隨）
 *********************************************************/
void UpdateCamera() {
    float target_x = rocket_pos_x;
    float target_y = rocket_pos_y + camera_height;
    float target_z = rocket_pos_z + camera_distance;

    // 平滑插值
    camera_x += (target_x - camera_x) * camera_smooth;
    camera_y += (target_y - camera_y) * camera_smooth;
    camera_z += (target_z - camera_z) * camera_smooth;
}

/**********************************************************
 * 繪製UI資訊
 *********************************************************/
void DrawUI() {
    // 設定2D渲染模式
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, screen_width, 0, screen_height, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);

    char info[100];

    // 顯示血量文字
    glColor3f(1.0f, 0.0f, 0.0f);
    glRasterPos2f(10, screen_height - 20);
    sprintf(info, "Health: %d", health);
    for (char* c = info; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    // 繪製血條背景 (紅色)
    glColor3f(0.5f, 0.0f, 0.0f); // 深紅色
    glBegin(GL_QUADS);
    glVertex2f(10, screen_height - 40);
    glVertex2f(10 + 100, screen_height - 40); // 寬度 100
    glVertex2f(10 + 100, screen_height - 30); // 高度 10
    glVertex2f(10, screen_height - 30);
    glEnd();

    // 繪製血條 (綠色)
    glColor3f(0.0f, 1.0f, 0.0f); // 綠色
    float health_bar_width = (float)health / 100.0f * 100.0f; // 血量百分比轉換為寬度
    glBegin(GL_QUADS);
    glVertex2f(10, screen_height - 40);
    glVertex2f(10 + health_bar_width, screen_height - 40);
    glVertex2f(10 + health_bar_width, screen_height - 30);
    glVertex2f(10, screen_height - 30);
    glEnd();


    // 顯示分數
    glColor3f(0.0f, 1.0f, 0.0f);
    glRasterPos2f(10, screen_height - 60); // 調整 Y 座標，避免與血條重疊
    sprintf(info, "Score: %d", score);
    for (char* c = info; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    // 顯示剩餘時間
    glColor3f(1.0f, 1.0f, 0.0f);
    glRasterPos2f(10, screen_height - 90); // 調整 Y 座標
    int remaining_time = game_time - (glutGet(GLUT_ELAPSED_TIME) - game_start_time) / 1000;
    if (remaining_time < 0) remaining_time = 0;
    sprintf(info, "Time: %d", remaining_time);
    for (char* c = info; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    // 遊戲結束畫面
    if (game_over || remaining_time <= 0) {
        if (remaining_time <= 0 && !game_over) {
            game_over = 1;
        }

        glColor3f(1.0f, 1.0f, 1.0f);
        glRasterPos2f(screen_width / 2 - 50, screen_height / 2);
        sprintf(info, "GAME OVER!");
        for (char* c = info; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        }

        glRasterPos2f(screen_width / 2 - 60, screen_height / 2 - 30);
        sprintf(info, "Final Score: %d", score);
        for (char* c = info; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        }

        glRasterPos2f(screen_width / 2 - 80, screen_height / 2 - 60);
        sprintf(info, "Press R to Restart");
        for (char* c = info; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        }
    }

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glColor3f(1.0f, 1.0f, 1.0f);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

/**********************************************************
 * 初始化遊戲
 *********************************************************/
void RestartGame() {
    game_over = 0;
    health = 100;
    score = 0;
    rocket_pos_x = 0.0f;
    rocket_pos_y = 0.0f;
    rocket_pos_z = 0.0f;
    game_start_time = glutGet(GLUT_ELAPSED_TIME);
    InitObstacles();
    InitHealthPickups(); // 重新開始時，初始化所有補血包為不活躍
}

/**********************************************************
 * SUBROUTINE init()
 *********************************************************/
void init(void) {
    glClearColor(0.0, 0.0, 0.1, 0.0);
    glShadeModel(GL_SMOOTH);

    glViewport(0, 0, screen_width, screen_height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (GLfloat)screen_width / (GLfloat)screen_height, 1.0f, 5000.0f);

    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_TEXTURE_2D);

    // 載入太空船模型
    Load3DS(&rocket, "spaceship.3ds");
    rocket.id_texture = LoadBitmap("spaceshiptexture.bmp");
    if (rocket.id_texture == -1) {
        MessageBox(NULL, "Image file: spaceshiptexture.bmp not found", "Error", MB_OK | MB_ICONERROR);
        exit(0);
    }

    // 載入障礙物模型
    Load3DS(&obstacle, "101.3ds");
    obstacle.id_texture = LoadBitmap("101.bmp");
    if (obstacle.id_texture == -1) {
        MessageBox(NULL, "Image file: 101.bmp not found", "Error", MB_OK | MB_ICONERROR);
        exit(0);
    }

    // 載入血量補給模型
    Load3DS(&health_pickup_model, "heart.3ds"); // 使用 health_pickup_model
    health_pickup_model.id_texture = LoadBitmap("heart_texture.bmp"); // 使用 health_pickup_model
    if (health_pickup_model.id_texture == -1) {
        MessageBox(NULL, "Image file: heart_texture.bmp not found", "Error", MB_OK | MB_ICONERROR);
        exit(0);
    }

    // 載入Skybox
    LoadSkybox();

    // 初始化遊戲
    game_start_time = glutGet(GLUT_ELAPSED_TIME);
    InitObstacles();
    InitHealthPickups(); // 初始時，初始化所有補血包為不活躍
}

/**********************************************************
 * SUBROUTINE keyboard()
 *********************************************************/
void keyboard(unsigned char key, int x, int y) {
    if (game_over) {
        if (key == 'r' || key == 'R') {
            RestartGame();
        }
        return;
    }

    switch (key) {
    case 'w': case 'W':
        rocket_pos_z -= rocket_speed;
        break;
    case 's': case 'S':
        rocket_pos_z += rocket_speed;
        break;
    case 'a': case 'A':
        rocket_pos_x -= rocket_speed;
        break;
    case 'd': case 'D':
        rocket_pos_x += rocket_speed;
        break;
    case 'z': case 'Z':
        rocket_pos_y -= rocket_speed;
        break;
    case 'x': case 'X':
        rocket_pos_y += rocket_speed;
        break;
    case 27: // ESC
        exit(0);
        break;
    }
}

/**********************************************************
 * SUBROUTINE display()
 *********************************************************/
void display(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // 更新遊戲狀態
    UpdateObstacles();
    UpdateHealthPickup();
    CheckCollisions();
    UpdateCamera();

    // 處理碰撞特效
    if (collision_occurred) {
        collision_timer++;
        if (collision_timer < blink_duration) {
            if (collision_timer % blink_interval == 0) {
                is_rocket_visible = !is_rocket_visible; // 切換可見性
            }
        }
        else {
            collision_occurred = 0; // 閃爍結束，重置碰撞標誌
            is_rocket_visible = 1;  // 確保火箭再次可見
        }
    }

    // 更新分數（基於存活時間）
    if (!game_over) {
        score = (glutGet(GLUT_ELAPSED_TIME) - game_start_time) / 100;
    }

    // 設定攝影機
    gluLookAt(camera_x, camera_y, camera_z,
        rocket_pos_x, rocket_pos_y, rocket_pos_z,
        0, 1, 0);

    // 繪製Skybox
    DrawSkybox();

    // 只有當 is_rocket_visible 為真時才繪製太空船
    if (is_rocket_visible) {
        // 繪製太空船
        glBindTexture(GL_TEXTURE_2D, rocket.id_texture);
        glPushMatrix();
        glTranslatef(rocket_pos_x, rocket_pos_y, rocket_pos_z);
        glScalef(0.5f, 0.5f, 0.5f);
        glRotatef(180, 0, 0, 1);
        glRotatef(90, 1, 0, 0);

        glBegin(GL_TRIANGLES);
        for (int l_index = 0; l_index < rocket.polygons_qty; l_index++) {
            glTexCoord2f(rocket.mapcoord[rocket.polygon[l_index].a].u,
                rocket.mapcoord[rocket.polygon[l_index].a].v);
            glVertex3f(rocket.vertex[rocket.polygon[l_index].a].x,
                rocket.vertex[rocket.polygon[l_index].a].y,
                rocket.vertex[rocket.polygon[l_index].a].z);

            glTexCoord2f(rocket.mapcoord[rocket.polygon[l_index].b].u,
                rocket.mapcoord[rocket.polygon[l_index].b].v);
            glVertex3f(rocket.vertex[rocket.polygon[l_index].b].x,
                rocket.vertex[rocket.polygon[l_index].b].y,
                rocket.vertex[rocket.polygon[l_index].b].z);

            glTexCoord2f(rocket.mapcoord[rocket.polygon[l_index].c].u,
                rocket.mapcoord[rocket.polygon[l_index].c].v);
            glVertex3f(rocket.vertex[rocket.polygon[l_index].c].x,
                rocket.vertex[rocket.polygon[l_index].c].y,
                rocket.vertex[rocket.polygon[l_index].c].z);
        }
        glEnd();
        glPopMatrix();
    }
    // 繪製障礙物
    glBindTexture(GL_TEXTURE_2D, obstacle.id_texture);
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (obstacles[i].active) {
            glPushMatrix();
            glTranslatef(obstacles[i].x, obstacles[i].y, obstacles[i].z);
            glScalef(2.0f, 2.0f, 2.0f);
            glRotatef(obstacles[i].rotation, 0, 1, 0);
            glRotatef(180, 0, 0, 1);
            glRotatef(90, 1, 0, 0);

            glBegin(GL_TRIANGLES);
            for (int l_index = 0; l_index < obstacle.polygons_qty; l_index++) {
                glTexCoord2f(obstacle.mapcoord[obstacle.polygon[l_index].a].u,
                    obstacle.mapcoord[obstacle.polygon[l_index].a].v);
                glVertex3f(obstacle.vertex[obstacle.polygon[l_index].a].x,
                    obstacle.vertex[obstacle.polygon[l_index].a].y,
                    obstacle.vertex[obstacle.polygon[l_index].a].z);

                glTexCoord2f(obstacle.mapcoord[obstacle.polygon[l_index].b].u,
                    obstacle.mapcoord[obstacle.polygon[l_index].b].v);
                glVertex3f(obstacle.vertex[obstacle.polygon[l_index].b].x,
                    obstacle.vertex[obstacle.polygon[l_index].b].y,
                    obstacle.vertex[obstacle.polygon[l_index].b].z);

                glTexCoord2f(obstacle.mapcoord[obstacle.polygon[l_index].c].u,
                    obstacle.mapcoord[obstacle.polygon[l_index].c].v);
                glVertex3f(obstacle.vertex[obstacle.polygon[l_index].c].x,
                    obstacle.vertex[obstacle.polygon[l_index].c].y,
                    obstacle.vertex[obstacle.polygon[l_index].c].z);
            }
            glEnd();
            glPopMatrix();
        }
    }
    // 繪製血量補給品
    glBindTexture(GL_TEXTURE_2D, health_pickup_model.id_texture); // 綁定愛心模型的紋理
    for (int i = 0; i < MAX_HEALTH_PICKUPS; i++) { // 遍歷所有補血包
        if (health_pickup_items[i].active) {
            glPushMatrix();
            glTranslatef(health_pickup_items[i].x, health_pickup_items[i].y, health_pickup_items[i].z);
            glScalef(20.1f, 20.1f, 20.1f); // 將補給品縮放與障礙物相同的2.0倍大小
            //glRotatef(health_pickup_items[i].rotation, 0, 1, 0); // 讓補給品自轉
            //glRotatef(00.0f, 1.0f, 00.0f, 0.0f); // 示例：圍繞 X 軸旋轉 90 度
            glBegin(GL_TRIANGLES);
            for (int l_index = 0; l_index < health_pickup_model.polygons_qty; l_index++) { // 使用 health_pickup_model 的資料
                glTexCoord2f(health_pickup_model.mapcoord[health_pickup_model.polygon[l_index].a].u,
                    health_pickup_model.mapcoord[health_pickup_model.polygon[l_index].a].v);
                glVertex3f(health_pickup_model.vertex[health_pickup_model.polygon[l_index].a].x,
                    health_pickup_model.vertex[health_pickup_model.polygon[l_index].a].y,
                    health_pickup_model.vertex[health_pickup_model.polygon[l_index].a].z);

                glTexCoord2f(health_pickup_model.mapcoord[health_pickup_model.polygon[l_index].b].u,
                    health_pickup_model.mapcoord[health_pickup_model.polygon[l_index].b].v);
                glVertex3f(health_pickup_model.vertex[health_pickup_model.polygon[l_index].b].x,
                    health_pickup_model.vertex[health_pickup_model.polygon[l_index].b].y,
                    health_pickup_model.vertex[health_pickup_model.polygon[l_index].b].z);

                glTexCoord2f(health_pickup_model.mapcoord[health_pickup_model.polygon[l_index].c].u,
                    health_pickup_model.mapcoord[health_pickup_model.polygon[l_index].c].v);
                glVertex3f(health_pickup_model.vertex[health_pickup_model.polygon[l_index].c].x,
                    health_pickup_model.vertex[health_pickup_model.polygon[l_index].c].y,
                    health_pickup_model.vertex[health_pickup_model.polygon[l_index].c].z);
            }
            glEnd();
            glPopMatrix();
        }
    }

    // 繪製UI
    DrawUI();

    glFlush();
    glutSwapBuffers();
}

/**********************************************************
 * SUBROUTINE resize()
 *********************************************************/
void resize(int width, int height) {
    screen_width = width;
    screen_height = height;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, screen_width, screen_height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (GLfloat)screen_width / (GLfloat)screen_height, 1.0f, 5000.0f);

    glutPostRedisplay();
}

/**********************************************************
 * Main routine
 *********************************************************/
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(screen_width, screen_height);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("3D太空船星空探險 - 3D Spaceship Adventure");

    glutDisplayFunc(display);
    glutIdleFunc(display);
    glutReshapeFunc(resize);
    glutKeyboardFunc(keyboard);

    init();
    glutMainLoop();

    return 0;
}