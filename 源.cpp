#include <graphics.h>
#include <conio.h>
#include <windows.h>
#include <cmath>
#include <string>
using namespace std;

int idx_current_anim = 0; //动画帧

//Player动画载入
const int Player_anim_num = 3;
IMAGE img_player_left[Player_anim_num];
IMAGE img_player_right[Player_anim_num];
void LoadAnim_player() {
	for (size_t i = 0; i < Player_anim_num; i++) {
		wstring path = L"img/player_left_" + to_wstring(i) + L".png";
		loadimage(&img_player_left[i], path.c_str());
	}
	for (size_t i = 0; i < Player_anim_num; i++) {
		wstring path = L"img/player_right_" + to_wstring(i) + L".png";
		loadimage(&img_player_right[i], path.c_str());
	}
}


int main() {
	initgraph(1280, 720);

	bool running = true;

	ExMessage msg;


	//载入图片
	LoadAnim_player();


	//双缓冲
	BeginBatchDraw();


	while (running) {

		DWORD start_time = GetTickCount();	// 用以节约CPU资源

		while (peekmessage(&msg)) {

		}

		//动画播放：每20帧播放一张图片
		static int counter = 0;
		if (++counter % 20 == 0) {		// 调节被取模的数以调整播放速率
			idx_current_anim++;
		}
		//循环播放
		idx_current_anim = idx_current_anim % Player_anim_num;
		
		cleardevice();

		//动画渲染
		putimage(500, 500, &img_player_left[idx_current_anim]);

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