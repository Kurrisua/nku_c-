#include <graphics.h>
#include <conio.h>
#include <windows.h>
#include <cmath>
#include <string>
#include<vector>
using namespace std;

//putimage_alpha�Ĵ���������ʵ��ͼƬ͸�����ֵĲ���
#pragma comment(lib,"MSIMG32.LIB")
inline void putimage_alpha(int x, int y, IMAGE* img) {
	int w = img->getwidth();
	int h = img->getheight();
	AlphaBlend(GetImageHDC(NULL), x, y, w, h, GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER,0,255,AC_SRC_ALPHA });
}

//����������
class Animation {
private:
	vector<IMAGE*> frame_list;			// ����֡����
	int interval_ms = 0;				// ����֡���
	int timer = 0;						// ��ʱ��
	int idx_frame = 0;					// ����֡����(�ڼ�֡)

public:
	Animation(LPCTSTR path, int num, int interval) // ͼƬ·����ͼƬ����������֡���
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

	void Play(int x, int y, int delta)	//���Ŷ�����x���꣬y���꣬ʱ������
	{
		timer += delta;					//��ʱ������ʱ����
		if (timer >= interval_ms)		//������һ֡����
		{
			idx_frame = (idx_frame + 1) % frame_list.size();
			timer = 0;
		}

		putimage_alpha(x, y, frame_list[idx_frame]);
	}
};

//����Player��
class Player {
	const int speed = 3;
	const int PLAYER_WIDTH = 70;		//��ɫ���
	const int PLAYER_HEIGHT = 70;	//��ɫ�߶�
	const int SHADOW_WIDTH = 70;	//��Ӱ���
	IMAGE img_shadow;
	Animation* anim_left;
	Animation* anim_right;
	POINT pos = { 500,500 };		//��ɫ��ǰ����
	bool is_move_up = false;
	bool is_move_down = false;
	bool is_move_left = false;
	bool is_move_right = false;
public:
	Player()	//�����ɫ�ڴ˴��Ѿ�ʵ��
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
	void ProcessEvent(const ExMessage& msg)		//�����Ϣ
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
		//����45�㷽����ƶ�
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
		//ʹ��ɫ�ڱ߽�֮��
		if (pos.x < 0)						pos.x = 0;
		if (pos.y < 0)						pos.y = 0;
		if (pos.x + PLAYER_WIDTH > 1280)	pos.x = 1280 - PLAYER_WIDTH;
		if (pos.y + PLAYER_HEIGHT > 720)	pos.y = 720 - PLAYER_HEIGHT;
	}
	void Draw(int delta)
	{
		//������Ӱ
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
};

//�����ӵ���
class Bullet
{

};

//����������
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

		//ʵ�ֵ����ڱ߽��������
		//����������߽�
		enum class RandomEdge
		{
			Up = 0,
			Down,
			Left,
			Right
		};
		RandomEdge edge = (RandomEdge)(rand() % 4);
		//�ڱ߽�����λ������
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
		return false;
	}
	bool CheckPlayerCollision(const Player& player)
	{
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
		//������Ӱ
		int shadow_pos_x = pos.x + (ENEMY_WIDTH / 2 - 33);//
		int shadow_pos_y = pos.y + ENEMY_HEIGHT - 22;
		putimage_alpha(shadow_pos_x, shadow_pos_y, &img_shadow);

		if (facing_left)
			anim_left->Play(pos.x, pos.y, delta);
		else
			anim_right->Play(pos.x, pos.y, delta);
	}
};

//���ɵ��˺���
void GenerateEnemy(vector<Enemy*>& enemy_list) {
	const int INTERVAL = 100;	//�������ɵ��˵�ʱ����
	static int counter = 0;
	if ((++counter) % INTERVAL == 0)
		enemy_list.push_back(new Enemy());
};


int main() {
	initgraph(1280, 720);

	bool running = true;

	ExMessage msg;

	//���뱳��
	IMAGE img_background;
	loadimage(&img_background, _T("img/background.png"));

	//����Player
	Player player;

	//����Enemy
	vector<Enemy*> enemy_list;

	BeginBatchDraw();

	/////��Ϸ����/////
	while (running)
	{
		DWORD start_time = GetTickCount();	// ���Խ�ԼCPU��Դ


		///��Ϣ������///
		while (peekmessage(&msg))
		{
			player.ProcessEvent(msg);
		}


		///���ݴ�����////
		player.Move();
		GenerateEnemy(enemy_list);
		for (Enemy* enemy : enemy_list)
			enemy->Move(player);




		cleardevice();

		///��Ⱦ����///
		putimage(0, 0, &img_background);
		player.Draw(5);
		for (Enemy* enemy : enemy_list)
			enemy->Draw(5);



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