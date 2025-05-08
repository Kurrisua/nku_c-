#include <graphics.h>
#include <conio.h>
#include <windows.h>
#include <cmath>
#include <string>
#include<vector>
using namespace std;

//putimage_alpha的创建，用来实现图片透明部分的播放
#pragma comment(lib,"MSIMG32.LIB")
inline void putimage_alpha(int x, int y, IMAGE* img) {
	int w = img->getwidth();
	int h = img->getheight();
	AlphaBlend(GetImageHDC(NULL), x, y, w, h, GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER,0,255,AC_SRC_ALPHA });
}
inline void putimage_alpha_1(int x, int y, const IMAGE* img) {
	int w = img->getwidth();
	int h = img->getheight();
	AlphaBlend(GetImageHDC(NULL), x, y, w, h, GetImageHDC((IMAGE*)img), 0, 0, w, h, { AC_SRC_OVER,0,255,AC_SRC_ALPHA });
}

#pragma comment(lib,"Winmm.lib")

//光标的彻底隐藏
WNDPROC oldProc = NULL;  // 保存原窗口过程

LRESULT CALLBACK CustomWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// 屏蔽系统鼠标光标设置
	if (message == WM_SETCURSOR) {
		SetCursor(NULL);  // 不显示系统光标
		return TRUE;
	}

	// 其他交给原窗口过程处理
	return CallWindowProc(oldProc, hWnd, message, wParam, lParam);
}



//创建动画类
class Animation {
private:
	vector<IMAGE*> frame_list;			// 动画帧序列
	int interval_ms = 0;				// 动画帧间隔
	int timer = 0;						// 计时器
	int idx_frame = 0;					// 动画帧索引(第几帧)

public:
	Animation(LPCTSTR path, int num, int interval) // 图片路径，图片数量，动画帧间隔
	{
		interval_ms = interval;

		TCHAR path_file[256];
		for (size_t i = 0; i < num; i++) {
			_stprintf_s(path_file, path, i);

			IMAGE* frame = new IMAGE();
			loadimage(frame, path_file);
			frame_list.push_back(frame);
		}
	}
	~Animation()
	{
		for (size_t i = 0; i < frame_list.size(); i++) {
			delete frame_list[i];
		}
	}

	void Play(int x, int y, int delta)	//播放动画（x坐标，y坐标，时间间隔）
	{
		timer += delta;					//计时器控制时间间隔
		if (timer >= interval_ms)		//播放下一帧动画
		{
			idx_frame = (idx_frame + 1) % frame_list.size();
			timer = 0;
		}

		putimage_alpha(x, y, frame_list[idx_frame]);
	}
};

//创建子弹类
class Bullet
{
	POINT pos;						//建立子弹的坐标
	const int speed = 7;
	double dir_x,dir_y;						//子弹射击的方向；
	bool active = true;				//子弹是否还应存在
	IMAGE img_Bullet;
public:
	Bullet(POINT start_pos, POINT target_pos):pos(start_pos)
	{
		loadimage(&img_Bullet, _T("img/Bullet.png"));

		int dx = target_pos.x - start_pos.x;
		int dy = target_pos.y - start_pos.y;
		double length = sqrt(dx * dx + dy * dy);

		if (length > 0)
		{
			dir_x = dx / length;
			dir_y = dy / length;
		}
		else
		{
			dir_x = 0;	dir_y = -1;
		}
	}
	void Move()
	{
		if (!active) return;
		pos.x += (int)(speed * dir_x);
		pos.y += (int)(speed * dir_y);

		if (pos.x < -50 || pos.x > 1280 + 50 || pos.y < -50 || pos.y > 720 + 50) //检查子弹是否超出边界，超出则算作无效
		{
			active = false;
		}
	}
	void Draw()const
	{
		if (!active) return;
		putimage_alpha_1(pos.x, pos.y, &img_Bullet);
	}
	bool IsActive() const { return active; }
	POINT GetPosition() const { return pos; }
	void Deactivate() { active = false; }
};

//创建Player类
class Player {
	const int speed = 3;
	const int PLAYER_WIDTH = 70;		// 角色宽度
	const int PLAYER_HEIGHT = 70;		// 角色高度
	const int SHADOW_WIDTH = 70;		// 阴影宽度
	IMAGE img_shadow;
	Animation* anim_left;
	Animation* anim_right;
	POINT pos = { 500,500 };			// 角色当前坐标
	bool is_move_up = false;
	bool is_move_down = false;
	bool is_move_left = false;
	bool is_move_right = false;
	bool is_alive = true;				// 玩家存活状态
	vector<Bullet*>bullets;				// 储存子弹
	const int SHOOT_COOLDOWN = 500;		// 射击冷却时间（毫秒）
	DWORD lastShootTime = 0;			// 上次射击时间
public:
	Player()							// 载入角色在此处已经实现
	{
		loadimage(&img_shadow, _T("img/player_shadow.png"));
		anim_left = new Animation(_T("img/player_left_%d.png"), 3, 70);
		anim_right = new Animation(_T("img/player_right_%d.png"), 3, 70);
	}
	~Player()
	{
		delete anim_left;
		delete anim_right;
	}
	void ProcessEvent(const ExMessage& msg)		//输出信息
	{
		if (msg.message == WM_KEYDOWN)
		{
			switch (msg.vkcode)
			{
			case VK_UP:
				is_move_up = true;
				break;
			case VK_DOWN:
				is_move_down = true;
				break;
			case VK_LEFT:
				is_move_left = true;
				break;
			case VK_RIGHT:
				is_move_right = true;
				break;
			}
		}
		else if (msg.message == WM_KEYUP)
		{
			switch (msg.vkcode)
			{
			case VK_UP:
				is_move_up = false;
				break;
			case VK_DOWN:
				is_move_down = false;
				break;
			case VK_LEFT:
				is_move_left = false;
				break;
			case VK_RIGHT:
				is_move_right = false;
				break;
			}
		}
	}
	void Move()
	{
		//包含45°方向的移动
		int dir_x = is_move_right - is_move_left;
		int dir_y = is_move_down - is_move_up;
		double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);
		if (len_dir != 0)
		{
			double normalized_x = dir_x / len_dir;
			double normalized_y = dir_y / len_dir;
			pos.x += (int)(speed * normalized_x);
			pos.y += (int)(speed * normalized_y);
		}
		//使角色在边界之内
		if (pos.x < 0)						pos.x = 0;
		if (pos.y < 0)						pos.y = 0;
		if (pos.x + PLAYER_WIDTH > 1280)	pos.x = 1280 - PLAYER_WIDTH;
		if (pos.y + PLAYER_HEIGHT > 720)	pos.y = 720 - PLAYER_HEIGHT;
	}
	void Draw(int delta)
	{
		//生成阴影
		int shadow_pos_x = pos.x + (PLAYER_WIDTH / 2 - 35);//
		int shadow_pos_y = pos.y + PLAYER_HEIGHT - 22;
		putimage_alpha(shadow_pos_x, shadow_pos_y, &img_shadow);

		static bool facing_left = false;
		int direct_x = is_move_right - is_move_left;
		if (direct_x < 0)		facing_left = true;
		else if (direct_x > 0)	facing_left = false;

		if (facing_left)
			anim_left->Play(pos.x, pos.y, delta);
		else
			anim_right->Play(pos.x, pos.y, delta);
	}
	const POINT Getposition() const
	{
		return pos;
	}
	// 发射子弹（在ProcessEvent中调用）
	void Shoot(POINT target_pos)
	{
		DWORD currentTime = GetTickCount();
		if (currentTime - lastShootTime < SHOOT_COOLDOWN) return;

		// 计算发射位置（玩家中心）后续微调
		POINT center =
		{
			pos.x + PLAYER_WIDTH / 2 - 25,
			pos.y + PLAYER_HEIGHT / 2 - 25
		};
		bullets.push_back(new Bullet(center, target_pos));
		lastShootTime = currentTime;
	}
	void UpdateBullets()
	{
		// 移除失效子弹
		auto it = bullets.begin();
		while (it != bullets.end())
		{
			if (!(*it)->IsActive())
			{
				delete* it;
				it = bullets.erase(it);
			}
			else
			{
				(*it)->Move();
				++it;
			}
		}
	}
	// 绘制所有子弹
	void DrawBullets() const
	{
		for (const auto& bullet : bullets)
		{
			bullet->Draw();
		}
	}
	const vector<Bullet*>& Getbullets() const
	{
		return bullets;
	}
};


//创建敌人类
class Enemy
{
	const int speed = 2;
	const int ENEMY_WIDTH = 70;
	const int ENEMY_HEIGHT = 70;
	const int SHADOW_WIDTH = 70;
	IMAGE img_shadow;
	Animation* anim_left;
	Animation* anim_right;
	POINT pos = { 0,0 };
	bool facing_left = false;
public:
	Enemy()
	{
		loadimage(&img_shadow, _T("img/enemy_shadow.png"));
		anim_left = new Animation(_T("img/enemy_left_%d.png"), 3, 70);
		anim_right = new Animation(_T("img/enemy_right_%d.png"), 3, 70);

		//实现敌人在边界随机生成
		//放置在随机边界
		enum class RandomEdge
		{
			Up = 0,
			Down,
			Left,
			Right
		};
		RandomEdge edge = (RandomEdge)(rand() % 4);
		//在边界的随机位置生成
		switch (edge)
		{
		case RandomEdge::Up:
			pos.x = rand() % 1280;
			pos.y = -ENEMY_HEIGHT;
			break;
		case RandomEdge::Down:
			pos.x = rand() % 1280;
			pos.y = 720;
			break;
		case RandomEdge::Left:
			pos.x = -ENEMY_WIDTH;
			pos.y = rand() % 720;
			break;
		case RandomEdge::Right:
			pos.x = 1280;
			pos.y = rand() % 720;
			break;
		default:
			break;
		}
	}
	~Enemy()
	{
		delete anim_left;
		delete anim_right;
	}
	bool CheckBulletCollision(const Bullet& bullet)
	{
		const POINT& bullet_pos = bullet.GetPosition();
		// 敌人碰撞箱 微调
		RECT enemy_rect = { pos.x-20, pos.y-20, pos.x + ENEMY_WIDTH-20, pos.y + ENEMY_HEIGHT-20 };

		return (bullet_pos.x >= enemy_rect.left && bullet_pos.x <= enemy_rect.right &&
			bullet_pos.y >= enemy_rect.top && bullet_pos.y <= enemy_rect.bottom);
	}
	bool CheckPlayerCollision(const Player& player)
	{
		//使用敌人的中心点进行判定
		POINT check_pos = { pos.x + ENEMY_WIDTH/2,pos.y + ENEMY_HEIGHT/2 };
		if (check_pos.x >= player.Getposition().x && check_pos.x <= player.Getposition().x + 70
			&& check_pos.y >= player.Getposition().y && check_pos.y <= player.Getposition().y + 70)
		{
			return true;
		}
		return false;
	}
	void Move(const Player& player)
	{
		const POINT& player_pos = player.Getposition();
		int dir_x = player_pos.x - pos.x;
		int dir_y = player_pos.y - pos.y;
		double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);
		if (len_dir != 0)
		{
			double normalized_x = dir_x / len_dir;
			double normalized_y = dir_y / len_dir;
			pos.x += (int)(speed * normalized_x);
			pos.y += (int)(speed * normalized_y);
		}
		if (dir_x < 0)
			facing_left = true;
		else if (dir_x > 0)
			facing_left = false;
	}
	void Draw(int delta)
	{
		//生成阴影
		int shadow_pos_x = pos.x + (ENEMY_WIDTH / 2 - 33);//
		int shadow_pos_y = pos.y + ENEMY_HEIGHT - 22;
		putimage_alpha(shadow_pos_x, shadow_pos_y, &img_shadow);

		if (facing_left)
			anim_left->Play(pos.x, pos.y, delta);
		else
			anim_right->Play(pos.x, pos.y, delta);
	}
};

//生成敌人函数
void GenerateEnemy(vector<Enemy*>& enemy_list) {
	const int INTERVAL = 100;	//控制生成敌人的时间间隔
	static int counter = 0;
	if ((++counter) % INTERVAL == 0)
		enemy_list.push_back(new Enemy());
};

//玩家积分系统,打印玩家分数
void GetScore(int score)
{
	static TCHAR text[64];
	_stprintf_s(text, _T("SCORE: %d"), score);
	setbkmode(TRANSPARENT);
	settextcolor(WHITE);
	settextstyle(30, 19,_T("Comic Sans MS"));
	outtextxy(20, 20, text);
}


int main() {
	initgraph(1280, 720);

	//载入背景音乐和受击音效
	mciSendString(_T("open music/background.mp3 alias background"), NULL, 0, NULL);
	mciSendString(_T("Open music/hit.mp3 alias hit"), NULL, 0, NULL);

	mciSendString(_T("play background repeat from 0"), NULL, 0, NULL);

	HWND hWnd = GetHWnd();
	// 保存原窗口过程，并替换为自定义
	oldProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)CustomWndProc);
	// 同时隐藏系统类光标（预防性设置）
	SetClassLongPtr(hWnd, GCLP_HCURSOR, (LONG_PTR)NULL);

	int score = 0;	//创建计分系统

	bool running = true;

	ExMessage msg;

	//载入背景
	IMAGE img_background;
	loadimage(&img_background, _T("img/background.png"));

	//载入光标图案
	IMAGE img_mouse;
	loadimage(&img_mouse, _T("img/mouse.png"));

	//载入Player
	Player player;

	//载入Enemy
	vector<Enemy*> enemy_list;

	BeginBatchDraw();

	/////游戏主体/////
	while (running)
	{
		while (ShowCursor(FALSE) >= 0);
		DWORD start_time = GetTickCount();	// 用以节约CPU资源


		///消息处理部分///
		while (peekmessage(&msg))
		{
			player.ProcessEvent(msg);
			// 添加鼠标左键射击
			if (msg.message == WM_LBUTTONDOWN) {
				player.Shoot({ msg.x, msg.y }); // 目标位置为鼠标点击坐标
			}
		}


		///数据处理部分////
		player.Move();
		player.UpdateBullets();
		GenerateEnemy(enemy_list);
		for (Enemy* enemy : enemy_list)
			enemy->Move(player);

		//检查敌人与玩家的碰撞
		for (Enemy* enemy : enemy_list)
		{
			if (enemy->CheckPlayerCollision(player))
			{
				// 停止背景音乐
				mciSendString(_T("stop background"), NULL, 0, NULL);

				//绘制战败画面
				static TCHAR text[128];
				_stprintf_s(text, _T("FINAL SCORE : %d !"), score);
				MessageBox(GetHWnd(), text, _T("GAME OVER!"), MB_OK);
				running = false;
				break;
			}
		}

		// 检查子弹与敌人碰撞
		auto enemy_it = enemy_list.begin();
		while (enemy_it != enemy_list.end()) {
			bool hit = false;

			for (Bullet* bullet : player.Getbullets()) {
				if ((*enemy_it)->CheckBulletCollision(*bullet)) {
					mciSendString(_T("play hit from 0"), NULL, 0, NULL);
					bullet->Deactivate(); 
					hit = true;
					score++;	//打死一个敌人，加一分
					break;
				}
			}

			if (hit) {
				delete* enemy_it;
				enemy_it = enemy_list.erase(enemy_it); // 删除敌人
			}
			else {
				++enemy_it;
			}
		}


		cleardevice();

		///渲染部分///
		putimage(0, 0, &img_background);
		player.Draw(5);
		player.DrawBullets();
		for (Enemy* enemy : enemy_list)
			enemy->Draw(5);
		POINT mousePos;
		GetCursorPos(&mousePos);
		ScreenToClient(GetHWnd(), &mousePos);  // 将屏幕坐标转换为窗口内坐标

		putimage_alpha_1(mousePos.x - 21, mousePos.y - 9, &img_mouse); // 调整偏移以居中

		GetScore(score);

		FlushBatchDraw();
		DWORD end_time = GetTickCount();
		DWORD delta_time = end_time - start_time;
		if (delta_time < 1000 / 144) {
			Sleep(1000 / 144 - delta_time);
		}
	}

	EndBatchDraw();

	return 0;
}