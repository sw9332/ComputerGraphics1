#ifndef _GRAPHICSCLASS_H_
#define _GRAPHICSCLASS_H_

#include "d3dclass.h"
#include "cameraclass.h"
#include "modelclass.h"
#include "textureshaderclass.h"
#include "textclass.h"
#include "fpsclass.h"
#include "cpuclass.h"
#include "EnemyClass.h"
#include <vector>

const bool FULL_SCREEN = false;
const bool VSYNC_ENABLED = true;
const float SCREEN_DEPTH = 1000.0f;
const float SCREEN_NEAR = 0.1f;

class BitmapClass;

class GraphicsClass
{
public:
	GraphicsClass();
	~GraphicsClass();
	GraphicsClass(const GraphicsClass&);

	bool Initialize(int, int, HWND);
	void Shutdown();
	bool Frame(char* timerStr);

private:
	bool Render(char*, float);

public:
	CameraClass* m_Camera;
	bool m_showTitle;

private:
	TextureShaderClass* m_TextureShader;
	BitmapClass* m_BackgroundSky;
	TextClass* m_Text;
	BitmapClass* m_Bitmap;
	BitmapClass* m_TitleBitmap;
	BitmapClass* m_inventoryUI;
	BitmapClass* m_PointUI;

	BitmapClass* m_HpUI;
	BitmapClass* m_HpBarRed;

	D3DClass* m_D3D;

	FpsClass* m_Fps = nullptr;
	CpuClass* m_Cpu = nullptr;

private:
	int m_playerMaxHp;        // 최대 체력 (예: 100)
	int m_playerHp;           // 현재 체력
	float m_hpDamageTimer;    // 2초 지속 대미지 계산용 타이머

private:
	// Biscuit Model
	int _biscuitMaxCount = 150;
	int _biscuitCurrentCount = 115;

	// BlueCandyTree Model
	int _blueCandyTreeMaxCount = 250;
	int _blueCandyTreeCurrentCount = 166;

	// Fillter Mode
	int m_filterType = 1;

	// Scrren Size
	int m_screenWidth;
	int m_screenHeight;

private:
	bool  m_soldierInitialized = false;
	float m_soldierTimer1 = 0.0f;
	float m_soldierTimer2 = 0.0f;
	float m_currentZ1 = -7.0f;
	float m_currentZ2 = -3.0f;
	float m_rotationY1 = 0.0f;
	float m_rotationY2 = 0.0f;

private:
	ModelClass* m_zombieWalkModels[41]; // 24 프레임
	std::vector<EnemyClass*> m_enemies; // 스폰된 에너미 객체 리스트

	float m_spawnAccumulator; // 스폰 타이머 누적값
	float m_nextSpawnTime; // 다음 스폰 간격 (랜덤)
	bool m_mouseDownLastFrame; // 마우스 단발 사격 체크용 변수

	void SpawnEnemy(); // 에너미 생성 함수
	void ProcessShooting(); // 사격 판정 함수

private:
	// instancing model
	ModelClass* _biscuit_Model;
	ModelClass* house_Model;
	ModelClass* blueCandyTree_Model;

	// not instancing model
	ModelClass* PinkCandyTree_Model;
	ModelClass* YellowCandyTreeModel;
	ModelClass* GreenCandyTreeModel;
	ModelClass* cookie_Model[12];
	ModelClass* cake_Model;
	ModelClass* dessertWorld_Model;
	ModelClass* ground_Model;
	ModelClass* Airplane_Model;
	ModelClass* CookieBike_Model;
	ModelClass* object1_Model;

	ModelClass* gun_Model;
};

#endif