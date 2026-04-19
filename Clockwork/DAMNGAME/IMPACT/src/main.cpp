/* * * * * * * * * * * * * * * * * * *
 * Space Impact for PicoLibSDK v2.08 *
 *          ©DamianVCechov           *
 * * * * * * * * * * * * * * * * * * */

#include "include.h"

#ifdef HEIGHT
#undef HEIGHT
#endif

// Nastavíme vlastní
#define HEIGHT 240 

#define PLAYER_WIDTH            32
#define PLAYER_HEIGHT           21
#define PLAYER_SPEED             5
#define BULLET_MAX              13
#define BULLET_WIDTH            20
#define BULLET_HEIGHT            4
#define BULLET_SPEED             8
#define ENEMY_MAX             1000
#define ENEMY_WIDTH             39
#define ENEMY_HEIGHT            17
#define ENEMY_MIN_SPEED          5
#define ENEMY_MAX_SPEED         10
#define BOSS_WIDTH             100
#define BOSS_HEIGHT            100
#define BOSS_LIVES_MAX         100
#define BOSS_SPEED               3
#define BOSS_BULLET_MAX         20
#define BOSS_BULLET_WIDTH       15
#define BOSS_BULLET_HEIGHT       5
#define BOSS_BULLET_SPEED        7
#define BOSS_SHOOT_INTERVAL     60
#define BOSS_TIME               30 
#define SCENERY_HEIGHT          20
#define SCENERY_SPEED            1
#define BONUS                  500

const u8 ShootSound[] = { 150, 180, 200, 220, 200, 180, 150, 128 };
const u8 ExplosionSound[] = { 250, 200, 150, 100, 50, 20, 0, 0, 250, 200, 150, 100, 50, 20, 0, 0 };

typedef struct {
	int x, y;
	int lives;
} Player;

typedef struct {
	int x, y;
	bool active;
} Bullet;

typedef struct {
	int x, y;
	int speed;
	bool active;
} Enemy;

typedef struct {
	int x, y;
	int lives;
	bool active;
} Boss;

typedef struct {
	int x, y;
	bool active;
} BossBullet;

typedef struct {
	int y;
	int x1, x2;
} SceneryBand;

enum { STATE_TITLE, STATE_PLAYING, STATE_GAME_OVER, STATE_PAUSED };

Player player;
Boss boss;
Bullet bullets[BULLET_MAX];
Enemy enemies[ENEMY_MAX];
BossBullet bossBullets[BOSS_BULLET_MAX];
SceneryBand topBand;
SceneryBand bottomBand;
int score;
int gameState;
int enemySpawnCounter;
bool isBossActive;
int enemiesDestroyedInRound;
int roundNumber;
int bossShootCounter;

void InitGame()
{
	player.x = 10;
	player.y = (HEIGHT - PLAYER_HEIGHT) / 2;
	player.lives = 3;
	score = 0;
	enemySpawnCounter = 0;
	enemiesDestroyedInRound = 0;
	isBossActive = false;
	roundNumber = 1;
	bossShootCounter = 0;

	for (int i = 0; i < BULLET_MAX; i++) bullets[i].active = false;
	for (int i = 0; i < ENEMY_MAX; i++) enemies[i].active = false;
	for (int i = 0; i < BOSS_BULLET_MAX; i++) bossBullets[i].active = false;
	
	boss.active = false;

    // inicializace scenérie
	topBand.y = 0;
	bottomBand.y = HEIGHT - SCENERY_HEIGHT;
	topBand.x1 = 0;
	topBand.x2 = WIDTH;
	bottomBand.x1 = 0;
	bottomBand.x2 = WIDTH;
}

void FireBullet()
{
	for (int i = 0; i < BULLET_MAX; i++)
	{
		if (!bullets[i].active)
		{
			bullets[i].active = true;
			bullets[i].x = player.x + PLAYER_WIDTH;
			bullets[i].y = player.y + (PLAYER_HEIGHT / 2) - (BULLET_HEIGHT / 2);
			PlaySoundChan(0, ShootSound, sizeof(ShootSound), SNDREPEAT_NO, 1.5f, 1.0f, SNDFORM_PCM, 0);
			return;
		}
	}
}

void FireBossBullet() {
	for (int i = 0; i < BOSS_BULLET_MAX; i++)
	{
		if (!bossBullets[i].active)
		{
			bossBullets[i].active = true;
			bossBullets[i].x = boss.x;
			bossBullets[i].y = boss.y + (BOSS_HEIGHT / 2) - (BOSS_BULLET_HEIGHT / 2);
			return;
		}
	}
}

void UpdateBossBullets() {
	for (int i = 0; i < BOSS_BULLET_MAX; i++)
	{
		if (bossBullets[i].active)
		{
			bossBullets[i].x -= BOSS_BULLET_SPEED; // Střely bosse míří doleva
			if (bossBullets[i].x + BOSS_BULLET_WIDTH < 0) bossBullets[i].active = false;
		}
	}
}

void UpdateScenery()
{
    topBand.x1 -= SCENERY_SPEED;
    topBand.x2 -= SCENERY_SPEED;
    bottomBand.x1 -= SCENERY_SPEED;
    bottomBand.x2 -= SCENERY_SPEED;

    if (topBand.x1 <= -WIDTH)
    {
        topBand.x1 = topBand.x2 + WIDTH;
    }
    if (topBand.x2 <= -WIDTH)
    {
        topBand.x2 = topBand.x1 + WIDTH;
    }

    if (bottomBand.x1 <= -WIDTH)
    {
        bottomBand.x1 = bottomBand.x2 + WIDTH;
    }
    if (bottomBand.x2 <= -WIDTH)
    {
        bottomBand.x2 = bottomBand.x1 + WIDTH;
    }
}

void SpawnEnemy()
{
	for (int i = 0; i < ENEMY_MAX; i++)
	{
		if (!enemies[i].active)
		{
			enemies[i].active = true;
			enemies[i].x = WIDTH;
			enemies[i].y = RandU16Max(HEIGHT - ENEMY_HEIGHT);
			enemies[i].speed = RandU8MinMax(ENEMY_MIN_SPEED, ENEMY_MAX_SPEED);
			return;
		}
	}
}

void SpawnBoss() 
{
	boss.active = true;
	boss.x = WIDTH - BOSS_WIDTH;
	boss.y = (HEIGHT - BOSS_HEIGHT) / 2;
	boss.lives = BOSS_LIVES_MAX + (roundNumber * 20); 
}

void UpdateBullets()
{
	for (int i = 0; i < BULLET_MAX; i++)
	{
		if (bullets[i].active)
		{
			bullets[i].x += BULLET_SPEED;
			if (bullets[i].x > WIDTH) bullets[i].active = false;
		}
	}
}

void UpdateEnemies()
{
	if (!isBossActive) {
		enemySpawnCounter++;
		if (enemySpawnCounter > 10)
		{
			enemySpawnCounter = 0;
			if (RandU8Max(100) < 40) SpawnEnemy();
		}
	}

	for (int i = 0; i < ENEMY_MAX; i++)
	{
		if (enemies[i].active)
		{
			enemies[i].x -= enemies[i].speed;
			if (enemies[i].x + ENEMY_WIDTH < 0) enemies[i].active = false;
		}
	}
	
	if (!isBossActive && enemiesDestroyedInRound >= BOSS_TIME) {
		isBossActive = true;
		SpawnBoss();
		for (int i = 0; i < ENEMY_MAX; i++) {
			enemies[i].active = false;
		}
	}
}

void DrawAnimExplosion()
{
    DispUpdate();
    DrawBlit(Sprite,  3, 18, player.x, player.y - 2, 32, 32, 320, COL_MAGENTA);
    DispUpdate();
    WaitMs(125);
    DrawBlit(Sprite,  3, 48, player.x, player.y - 2, 32, 32, 320, COL_MAGENTA);
    DispUpdate();
    WaitMs(125);
    DrawBlit(Sprite, 35, 18, player.x, player.y - 2, 32, 32, 320, COL_MAGENTA);
    DispUpdate();
    WaitMs(125);
    DrawBlit(Sprite, 35, 48, player.x, player.y - 2, 32, 32, 320, COL_MAGENTA);
    DispUpdate();
    WaitMs(125);
    DrawBlit(Sprite, 67, 18, player.x, player.y - 2, 32, 32, 320, COL_MAGENTA);
    DispUpdate();
    WaitMs(125);
    DrawBlit(Sprite, 67, 48, player.x, player.y - 2, 32, 32, 320, COL_MAGENTA);
    DispUpdate();
    WaitMs(125);
    DrawBlit(Sprite, 99, 18, player.x, player.y - 2, 32, 32, 320, COL_MAGENTA);
    DispUpdate();
    WaitMs(125);
    DrawBlit(Sprite, 99, 48, player.x, player.y - 2, 32, 32, 320, COL_MAGENTA);
    DispUpdate();
    WaitMs(125);
    DrawBlit(Sprite,  3, 78, player.x, player.y - 2, 32, 32, 320, COL_MAGENTA);
    DispUpdate();
    WaitMs(125);
    DrawBlit(Sprite, 35, 78, player.x, player.y - 2, 32, 32, 320, COL_MAGENTA);
    DispUpdate();
    WaitMs(125);
    DrawBlit(Sprite, 67, 78, player.x, player.y - 2, 32, 32, 320, COL_MAGENTA);
    DispUpdate();
    WaitMs(125);
    DrawBlit(Sprite, 99, 78, player.x, player.y - 2, 32, 32, 320, COL_MAGENTA);
    DispUpdate();
    WaitMs(1000);
}


void CheckCollisions()
{
	for (int i = 0; i < BULLET_MAX; i++)
	{
		if (bullets[i].active)
		{
			// Zničení normálního nepřítele
			for (int j = 0; j < ENEMY_MAX; j++)
			{
				if (enemies[j].active)
				{
					if ((bullets[i].x < enemies[j].x + ENEMY_WIDTH) && (bullets[i].x + BULLET_WIDTH > enemies[j].x) &&
						(bullets[i].y < enemies[j].y + ENEMY_HEIGHT) && (bullets[i].y + BULLET_HEIGHT > enemies[j].y))
					{
						bullets[i].active = false;
						enemies[j].active = false;
						score += 10;
						enemiesDestroyedInRound++;
						PlaySoundChan(1, ExplosionSound, sizeof(ExplosionSound), SNDREPEAT_NO, 1.0f, 1.0f, SNDFORM_PCM, 0);
					}
				}
			}

			// Zničení bosse
			if (isBossActive && boss.active) 
            {
				if ((bullets[i].x < boss.x + BOSS_WIDTH) && (bullets[i].x + BULLET_WIDTH > boss.x) &&
					(bullets[i].y < boss.y + BOSS_HEIGHT) && (bullets[i].y + BULLET_HEIGHT > boss.y))
				{
					bullets[i].active = false;
					boss.lives--;
					PlaySoundChan(1, ExplosionSound, sizeof(ExplosionSound), SNDREPEAT_NO, 1.0f, 1.0f, SNDFORM_PCM, 0);

					if (boss.lives <= 0) {
						boss.active = false;
						isBossActive = false;
						roundNumber++;
						enemiesDestroyedInRound = 0;
						score += BONUS; // Bonus za zabití bosse
						PlaySoundChan(2, ExplosionSound, sizeof(ExplosionSound), SNDREPEAT_NO, 1.0f, 1.0f, SNDFORM_PCM, 0);
					}
				}
			}
		}
	}

	// Kolize hráče s nepřáteli
	for (int i = 0; i < ENEMY_MAX; i++)
	{
		if (enemies[i].active)
		{
			if ((player.x < enemies[i].x + ENEMY_WIDTH) && (player.x + PLAYER_WIDTH > enemies[i].x) &&
				(player.y < enemies[i].y + ENEMY_HEIGHT) && (player.y + PLAYER_HEIGHT > enemies[i].y))
			{
				enemies[i].active = false;
				player.lives--;
				PlaySoundChan(2, ExplosionSound, sizeof(ExplosionSound), SNDREPEAT_NO, 1.0f, 1.0f, SNDFORM_PCM, 0);
                DrawAnimExplosion();
				if (player.lives <= 0) gameState = STATE_GAME_OVER;
			}
		}
	}
	
	// Kolize hráče s bossem
	if (isBossActive && boss.active) {
		if ((player.x < boss.x + BOSS_WIDTH) && (player.x + PLAYER_WIDTH > boss.x) &&
			(player.y < boss.y + BOSS_HEIGHT) && (player.y + PLAYER_HEIGHT > boss.y))
		{
			player.lives--;
			PlaySoundChan(2, ExplosionSound, sizeof(ExplosionSound), SNDREPEAT_NO, 1.0f, 1.0f, SNDFORM_PCM, 0);
            DrawAnimExplosion();
			if (player.lives <= 0) gameState = STATE_GAME_OVER;
		}
	}

    // Kolize střel bosse s hráčem
	for (int i = 0; i < BOSS_BULLET_MAX; i++)
	{
		if (bossBullets[i].active)
		{
			if ((player.x < bossBullets[i].x + BOSS_BULLET_WIDTH) && (player.x + PLAYER_WIDTH > bossBullets[i].x) &&
				(player.y < bossBullets[i].y + BOSS_BULLET_HEIGHT) && (player.y + PLAYER_HEIGHT > bossBullets[i].y))
			{
				bossBullets[i].active = false;
				player.lives--;
				PlaySoundChan(2, ExplosionSound, sizeof(ExplosionSound), SNDREPEAT_NO, 1.0f, 1.0f, SNDFORM_PCM, 0);
                DrawAnimExplosion();
				if (player.lives <= 0) gameState = STATE_GAME_OVER;
			}
		}
	}

    // Kolize hráče s horním pásem
	if (player.y < topBand.y + SCENERY_HEIGHT)
    {
		player.lives--;
		PlaySoundChan(2, ExplosionSound, sizeof(ExplosionSound), SNDREPEAT_NO, 1.0f, 1.0f, SNDFORM_PCM, 0);
        DrawAnimExplosion();
		if (player.lives <= 0) gameState = STATE_GAME_OVER;
	}

	// Kolize hráče s dolním pásem
	if (player.y + PLAYER_HEIGHT > bottomBand.y) 
    {
		player.lives--;
		PlaySoundChan(2, ExplosionSound, sizeof(ExplosionSound), SNDREPEAT_NO, 1.0f, 1.0f, SNDFORM_PCM, 0);
        DrawAnimExplosion();
		if (player.lives <= 0) gameState = STATE_GAME_OVER;
	}
}

void UpdateBoss() 
{
	if (isBossActive && boss.active) 
    {
		// Pohyb bosse (sleduje hráče)
		if (player.y < (boss.y + BOSS_HEIGHT / 2)) boss.y -= BOSS_SPEED;
		if (player.y > (boss.y + BOSS_HEIGHT / 2)) boss.y += BOSS_SPEED;

		// Omezení bosse
		if (boss.y < 0) boss.y = 0;
		if (boss.y > HEIGHT - BOSS_HEIGHT) boss.y = HEIGHT - BOSS_HEIGHT;

		// Najíždění bosse 
		if (roundNumber >= 3) 
        { 
			if (boss.x > 0 ) boss.x -= (BOSS_SPEED / 3); 
        }
        else 
        {
		    // Boss je na pravém okraji, pokud nenajíždí
			if (boss.x < WIDTH - BOSS_WIDTH) boss.x += BOSS_SPEED;
			if (boss.x > WIDTH - BOSS_WIDTH) boss.x = WIDTH - BOSS_WIDTH;
		}
		
		// Střelba bosse
		bossShootCounter++;
		if (bossShootCounter >= BOSS_SHOOT_INTERVAL) 
        {
			FireBossBullet();
			bossShootCounter = 0;
		}
	}
}

void DrawGame()
{
    DrawClear();
    // Vykreslení pozadí
    DrawImg(Wall, 0, 0, 0, 0, WIDTH, HEIGHT, WIDTH);

    // Horní a dolní pás
    DrawBlit(Sprite, 0, 210, topBand.x1, topBand.y, WIDTH, SCENERY_HEIGHT, WIDTH, COL_MAGENTA);
    DrawBlit(Sprite, 0, 220, bottomBand.x1, bottomBand.y, WIDTH, SCENERY_HEIGHT, WIDTH, COL_MAGENTA);

    // Plynulý pohyb pásů...
    DrawBlit(Sprite, 0, 210, topBand.x2, topBand.y, WIDTH, SCENERY_HEIGHT, WIDTH, COL_MAGENTA);
    DrawBlit(Sprite, 0, 220, bottomBand.x2, bottomBand.y, WIDTH, SCENERY_HEIGHT, WIDTH, COL_MAGENTA);

    DrawBlit(Sprite, 0, 0, player.x, player.y, PLAYER_WIDTH, PLAYER_HEIGHT, WIDTH, COL_MAGENTA);

    for (int i = 0; i < BULLET_MAX; i++)
        if (bullets[i].active) DrawBlit(Sprite, 104, 0, bullets[i].x, bullets[i].y, BULLET_WIDTH, BULLET_HEIGHT, WIDTH, COL_MAGENTA);

    for (int i = 0; i < ENEMY_MAX; i++)
		if (enemies[i].active) DrawBlit(Sprite, 41, 0, enemies[i].x, enemies[i].y, ENEMY_WIDTH, ENEMY_HEIGHT, WIDTH, COL_MAGENTA);
	
	// Boss
	if (isBossActive && boss.active) {
        if ( roundNumber < 3) DrawBlit(Sprite, 236, 0, boss.x, boss.y, BOSS_WIDTH, BOSS_HEIGHT, WIDTH, COL_MAGENTA);
        else DrawBlit(Sprite, 134, 0, boss.x, boss.y, BOSS_WIDTH, BOSS_HEIGHT, WIDTH, COL_MAGENTA);
	}

	// Střely bosse
	for (int i = 0; i < BOSS_BULLET_MAX; i++) 
    {
		if (bossBullets[i].active) DrawRect(bossBullets[i].x, bossBullets[i].y, BOSS_BULLET_WIDTH, BOSS_BULLET_HEIGHT, COL_YELLOW);
	}
    
	char score_buf[40];
	snprintf(score_buf, sizeof(score_buf), "Score: %d  Lives: %d  Round: %d", score, player.lives, roundNumber);
	DrawText(score_buf, 5, 5, COL_WHITE);
	
	if (isBossActive) {
		char boss_lives_buf[20];
		snprintf(boss_lives_buf, sizeof(boss_lives_buf), "%d HP", boss.lives);
		DrawText(boss_lives_buf, WIDTH - 50, 5, COL_RED);
	}
}

void DrawTitleScreen()
{
	DrawClear();
    DrawBlit(Sprite, 0, 140, (WIDTH - 134) / 2, 30, 134, 66, WIDTH, COL_MAGENTA);
    DrawBlit(Sprite, 0, 121, (WIDTH - 170) / 2, 120, 170, 16, WIDTH, COL_MAGENTA);
	DrawText("Press 'A' to Start", (WIDTH - 18 * 8) / 2, 180, COL_GRAY);
    DrawText("Press 'Y' to Quit", (WIDTH - 17 * 8) / 2, 200, COL_GRAY);
}

void DrawGameOverScreen()
{
	DrawClear();
	char score_buf[40];
	snprintf(score_buf, sizeof(score_buf), "Final Score: %d", score);

	DrawText2("GAME OVER", (WIDTH - 9 * 16) / 2, 40, COL_RED);
	DrawText(score_buf, (WIDTH - strlen(score_buf) * 8) / 2, 110, COL_WHITE);
	DrawText("Press 'A' to Restart", (WIDTH - 20 * 8) / 2, 140, COL_GRAY);
    DrawText("Press 'Y' to Quit", (WIDTH - 17 * 8) / 2, 160, COL_GRAY);
    DrawBlit(Sprite, 0, 121, (WIDTH - 170) / 2, 220, 170, 16, WIDTH, COL_MAGENTA);
}

void DrawPauseScreen()
{
    DrawText2("PAUSED", (WIDTH - 6 * 16) / 2, (HEIGHT - 32) / 2, COL_YELLOW);
    DrawText("Press 'A' to continue", (WIDTH - 21 * 8) / 2, (HEIGHT + 50) / 2, COL_YELLOW);
}

// Ovládání při hře
void HandleInput()
{
	if (KeyPressed(KEY_UP) && player.y > 0) player.y -= PLAYER_SPEED;
	if (KeyPressed(KEY_DOWN) && player.y < HEIGHT - PLAYER_HEIGHT) player.y += PLAYER_SPEED;
    if (KeyPressed(KEY_LEFT) && player.x > 0) player.x -= PLAYER_SPEED;
    if (KeyPressed(KEY_RIGHT) && player.x < WIDTH - PLAYER_WIDTH) player.x += PLAYER_SPEED;
	if (KeyPressed(KEY_A)) FireBullet();
    if (KeyPressed(KEY_X)) gameState = STATE_PAUSED;
    if (KeyPressed(KEY_Y)) gameState = STATE_TITLE;
}

// ------------------------------- Hlavní funkce -----------------------------------------
int main()
{
	gameState = STATE_TITLE;
	
	while (true)
	{
		switch(gameState)
		{
			case STATE_TITLE:
				DrawTitleScreen();
				if (KeyPressed(KEY_A))
				{
					InitGame();
					gameState = STATE_PLAYING;
				}
                if (KeyPressed(KEY_Y)) ResetToBootLoader();
				break;

			case STATE_PLAYING:

				HandleInput();
				UpdateBullets();
				UpdateEnemies();
				UpdateBoss();
                UpdateBossBullets();
                UpdateScenery();
				CheckCollisions();
				DrawGame();
				break;

            case STATE_PAUSED:
				DrawGame(); 
				DrawPauseScreen();
                if (KeyPressed(KEY_A)) gameState = STATE_PLAYING;
                if (KeyPressed(KEY_Y)) ResetToBootLoader();
				break;

            case STATE_GAME_OVER:
				DrawGameOverScreen();
				if (KeyPressed(KEY_A)) gameState = STATE_TITLE;
                if (KeyPressed(KEY_Y)) ResetToBootLoader();
				break;
		}

        DispUpdate();
	}
}
