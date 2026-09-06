#include "graphicsclass.h"
#include "bitmapclass.h"

#pragma region 생성자, 소멸자
GraphicsClass::GraphicsClass()
{
    m_D3D = 0;
    m_Camera = 0;

    _biscuit_Model = 0;
    house_Model = 0;
    cake_Model = 0;
    ground_Model = 0;

    m_Bitmap = 0;
    m_TitleBitmap = 0;

    GreenCandyTreeModel = 0;
    YellowCandyTreeModel = 0;

    Airplane_Model = 0;
    CookieBike_Model = 0;
    object1_Model = 0;
    gun_Model = 0;
    
    m_mouseDownLastFrame = false;

    m_spawnAccumulator = 0.0f;
    m_nextSpawnTime = 3.0f; // 첫 스폰은 3초 뒤
    m_mouseDownLastFrame = false;

    for (int i = 0; i < 41; i++) {
        m_zombieWalkModels[i] = 0;
    }

    // 쿠키
    {
        for (int i = 0; i < 12; i++)
        {
            cookie_Model[i] = 0;
        }
    }
    m_TextureShader = 0;
}

GraphicsClass::~GraphicsClass() {}
GraphicsClass::GraphicsClass(const GraphicsClass& other) {}
#pragma endregion

#pragma region Frame
bool GraphicsClass::Frame(char* timerStr)
{
    bool result;
    static float rotation = 0.0f;

    m_Fps->Frame();
    m_Cpu->Frame();

    if (GetAsyncKeyState(VK_SPACE) & 0x8000)
    {
        m_showTitle = false;
    }

    rotation += (float)XM_PI * 0.005f;
    if (rotation > 360.0f) rotation -= 360.0f;

    if (GetAsyncKeyState('3') & 0x8000) m_filterType = 0;
    else if (GetAsyncKeyState('4') & 0x8000) m_filterType = 1;

    if (!m_soldierInitialized)
    {
        m_soldierTimer1 = 0.0f;
        m_soldierTimer2 = 3.5f;
        m_soldierInitialized = true;
    }

    float moveDuration = 4.0f;
    float waitDuration = 2.5f;
    float cycle = moveDuration + waitDuration;

    float startZ1 = -7.0f;
    float distance1 = 2.0f;
    float startZ2 = -3.0f;
    float distance2 = 2.0f;

    float dt = 0.016f;

    // 쿠키 1 데이터 계산
    m_soldierTimer1 += dt;
    float t1 = fmodf(m_soldierTimer1, cycle * 2.0f);

    if (t1 < moveDuration) {
        float progress = t1 / moveDuration;
        m_currentZ1 = startZ1 + (progress * distance1);
        m_rotationY1 = XM_PI;
    }
    else if (t1 < cycle) {
        m_currentZ1 = startZ1 + distance1;
        m_rotationY1 = XM_PI;
    }
    else if (t1 < cycle + moveDuration) {
        float progress = (t1 - cycle) / moveDuration;
        m_currentZ1 = (startZ1 + distance1) - (progress * distance1);
        m_rotationY1 = 0.0f;
    }
    else {
        m_currentZ1 = startZ1;
        m_rotationY1 = 0.0f;
    }

    // 쿠키 2 데이터 계산
    m_soldierTimer2 += dt;
    float t2 = fmodf(m_soldierTimer2, cycle * 2.0f);

    if (t2 < moveDuration) {
        float progress = t2 / moveDuration;
        m_currentZ2 = startZ2 + (progress * distance2);
        m_rotationY2 = XM_PI;
    }
    else if (t2 < cycle) {
        m_currentZ2 = startZ2 + distance2;
        m_rotationY2 = XM_PI;
    }
    else if (t2 < cycle + moveDuration) {
        float progress = (t2 - cycle) / moveDuration;
        m_currentZ2 = (startZ2 + distance2) - (progress * distance2);
        m_rotationY2 = 0.0f;
    }
    else {
        m_currentZ2 = startZ2;
        m_rotationY2 = 0.0f;
    }

    // 타이틀 화면이 아닐 때만 굴러가도록 제어
    if (!m_showTitle)
    {
        m_spawnAccumulator += dt;
        if (m_spawnAccumulator >= m_nextSpawnTime)
        {
            SpawnEnemy();
            m_spawnAccumulator = 0.0f;
            m_nextSpawnTime = static_cast<float>(rand() % 4) + 2.0f; // 2~6초 랜덤 스폰 (f 붙여서 경고 삭제)
        }

        // 에너미 이동 업데이트
        XMFLOAT3 playerPos = m_Camera->GetPosition();
        for (auto enemy : m_enemies)
        {
            enemy->Update(playerPos, dt);
        }

        // 단발 사격 처리
        bool isMouseDownThisFrame = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        if (isMouseDownThisFrame && !m_mouseDownLastFrame)
        {
            ProcessShooting();
        }
        m_mouseDownLastFrame = isMouseDownThisFrame;
    }

    bool isCollidingWithZombie = false;
    XMFLOAT3 playerPos = m_Camera->GetPosition();

    // 월드에 존재하는 모든 좀비(Enemy)들과의 거리 검사
    for (size_t i = 0; i < m_enemies.size(); ++i)
    {
        XMFLOAT3 enemyPos = m_enemies[i]->GetBoundingPosition();

        // 3D 평면(X, Z) 거리 계산
        float distX = enemyPos.x - playerPos.x;
        float distZ = enemyPos.z - playerPos.z;
        float distance = sqrt(distX * distX + distZ * distZ);

        // 좀비와 내 거리가 1.8f 이내라면 부딪힌 상태로 판정!
        if (distance < 1.8f)
        {
            isCollidingWithZombie = true;
            break;
        }
    }

    // 좀비랑 비비고 있는 중일 때
    if (isCollidingWithZombie)
    {
        m_hpDamageTimer += dt; // 프레임 시간(초)을 계속 누적

        if (m_hpDamageTimer >= 2.0f)  // 2초 도달!
        {
            m_playerHp -= 2;          // 체력 2 감소
            m_hpDamageTimer = 0.0f;   // 타이머 초기화

            if (m_playerHp < 0) m_playerHp = 0; // 음수 방지
        }
    }
    else
    {
        // 좀비에게서 떨어지면 타이머를 리셋해서, 다음에 다시 부딪혔을 때 "즉시" 반응하게 유도
        m_hpDamageTimer = 1.9f;
    }

    if (m_playerHp <= 0)
    {
        return false;
    }

    return Render(timerStr, rotation);
}
#pragma endregion

#pragma region Shutdown
void GraphicsClass::Shutdown()
{
    if (m_TextureShader)
    {
        m_TextureShader->Shutdown();
        delete m_TextureShader;
        m_TextureShader = 0;
    }

    if (m_Bitmap)
    {
        m_Bitmap->Shutdown();
        delete m_Bitmap; m_Bitmap = 0;
    }

    if (m_TitleBitmap)
    {
        m_TitleBitmap->Shutdown();
        delete m_TitleBitmap; m_TitleBitmap = 0;
    }

    if (_biscuit_Model)
    {
        _biscuit_Model->Shutdown();
        delete _biscuit_Model;
    }

    if (GreenCandyTreeModel)
    {
        GreenCandyTreeModel->Shutdown();
        delete GreenCandyTreeModel;
    }

    if (YellowCandyTreeModel)
    {
        YellowCandyTreeModel->Shutdown();
        delete YellowCandyTreeModel;
    }

    // 쿠키
    {
        for (int i = 0; i < 12; i++)
        {
            if (cookie_Model[i])
            {
                cookie_Model[i]->Shutdown();
                delete cookie_Model[i];
                cookie_Model[i] = 0;
            }
        }
    }

    for (auto enemy : m_enemies)
    {
        if (enemy)
        {
            delete enemy;
            enemy = nullptr;
        }
    }
    m_enemies.clear();

    if (gun_Model)
    {
        gun_Model->Shutdown();
        delete gun_Model;
    }

    if (CookieBike_Model)
    {
        CookieBike_Model->Shutdown();
        delete CookieBike_Model;
    }

    if (object1_Model)
    {
        object1_Model->Shutdown();
        delete object1_Model;
    }

    if (house_Model)
    {
        house_Model->Shutdown();
        delete house_Model;
        house_Model = 0;
    }

    if (cake_Model)
    {
        cake_Model->Shutdown();
        delete cake_Model;
        cake_Model = 0;
    }

    if (Airplane_Model)
    {
        Airplane_Model->Shutdown();
        delete Airplane_Model;
        Airplane_Model = 0;
    }

    if (ground_Model)
    {
        ground_Model->Shutdown();
        delete ground_Model;
        ground_Model = 0;
    }

    if (m_Fps)
    {
        delete m_Fps;
        m_Fps = 0;
    }

    if (m_Cpu)
    {
        m_Cpu->Shutdown();
        delete m_Cpu;
        m_Cpu = 0;
    }

    if (m_Camera)
    {
        delete m_Camera; m_Camera = 0;
    }

    if (m_D3D)
    {
        m_D3D->Shutdown();
        delete m_D3D; m_D3D = 0;
    }
}
#pragma endregion

bool GraphicsClass::Initialize(int screenWidth, int screenHeight, HWND hwnd)
{

#pragma region 시스템 초기화
    bool result;

    m_D3D = new D3DClass;
    if (!m_D3D) return false;

    result = m_D3D->Initialize(screenWidth, screenHeight, VSYNC_ENABLED, hwnd, FULL_SCREEN, SCREEN_DEPTH, SCREEN_NEAR);
    if (!result)
    {
        MessageBox(hwnd, L"Could not initialize Direct3D.", L"Error", MB_OK);
        return false;
    }

    m_Fps = new FpsClass;
    if (!m_Fps) return false;
    m_Fps->Initialize();

    m_Cpu = new CpuClass;
    if (!m_Cpu) return false;
    m_Cpu->Initialize();

    m_filterType = 1;
#pragma endregion

#pragma region 카메라 초기화
    m_Camera = new CameraClass;
    if (!m_Camera) return false;
    m_Camera->SetPosition(38.0f, 0.0f, -4.0f);
    m_Camera->SetRotation(0.0f, -90.0f, 0.0f);

    CameraClass* tempCamera = new CameraClass;
    if (!tempCamera) return false;

    tempCamera->SetPosition(0.0f, 0.0f, -1.0f);
    tempCamera->Render();

    XMMATRIX baseViewMatrix;
    tempCamera->GetViewMatrix(baseViewMatrix);

    delete tempCamera;
    tempCamera = 0;

    m_Text = new TextClass;
    if (!m_Text) return false;

    result = m_Text->Initialize(m_D3D->GetDevice(), m_D3D->GetDeviceContext(), hwnd, screenWidth, screenHeight, baseViewMatrix);
    if (!result)
    {
        MessageBox(hwnd, L"Could not initialize the text object.", L"Error", MB_OK);
        return false;
    }
#pragma endregion

#pragma region 2D 타이틀 배경 초기화
    m_showTitle = true;

    m_TitleBitmap = new BitmapClass;
    if (!m_TitleBitmap) return false;
    result = m_TitleBitmap->Initialize(m_D3D->GetDevice(), screenWidth, screenHeight, L"./data/Bitmap/TitleScene2.dds", screenWidth, screenHeight);
    if (!result)
    {
        MessageBox(hwnd, L"Could not initialize the bitmap object.", L"Error", MB_OK);
        return false;
    }

    m_TextureShader = new TextureShaderClass;
    if (!m_TextureShader) return false;
    result = m_TextureShader->Initialize(m_D3D->GetDevice(), hwnd);
    if (!result)
    {
        MessageBox(hwnd, L"Could not initialize the texture shader object.", L"Error", MB_OK);
        return false;
    }
#pragma endregion

#pragma region 인벤토리 UI 초기화
    m_inventoryUI = new BitmapClass;
    if (!m_inventoryUI) return false;
    result = m_inventoryUI->Initialize(m_D3D->GetDevice(), screenWidth, screenHeight, L"./data/UI/InventoryUI.dds", 430, 280);
    if (!result)
    {
        MessageBox(hwnd, L"Could not initialize the bitmap object.", L"Error", MB_OK);
        return false;
    }

    m_TextureShader = new TextureShaderClass;
    if (!m_TextureShader) return false;
    result = m_TextureShader->Initialize(m_D3D->GetDevice(), hwnd);
    if (!result)
    {
        MessageBox(hwnd, L"Could not initialize the texture shader object.", L"Error", MB_OK);
        return false;
    }
#pragma endregion

#pragma region Hp UI 초기화
    m_playerHp = 100.0f;
    m_playerMaxHp = 100.0f;

    m_HpUI = new BitmapClass;
    if (!m_HpUI) return false;
    result = m_HpUI->Initialize(m_D3D->GetDevice(), screenWidth, screenHeight, L"./data/UI/HpUI.dds", 400, 200);
    if (!result)
    {
        MessageBox(hwnd, L"Could not initialize the bitmap object.", L"Error", MB_OK);
        return false;
    }

    m_TextureShader = new TextureShaderClass;
    if (!m_TextureShader) return false;
    result = m_TextureShader->Initialize(m_D3D->GetDevice(), hwnd);
    if (!result)
    {
        MessageBox(hwnd, L"Could not initialize the texture shader object.", L"Error", MB_OK);
        return false;
    }
#pragma endregion

#pragma region Hp Value 초기화
    m_HpBarRed = new BitmapClass;
    if (!m_HpBarRed) return false;
    result = m_HpBarRed->Initialize(m_D3D->GetDevice(), screenWidth, screenHeight, L"./data/UI/HpValue.dds", 350, 170);
    if (!result)
    {
        MessageBox(hwnd, L"Could not initialize the bitmap object.", L"Error", MB_OK);
        return false;
    }

    m_TextureShader = new TextureShaderClass;
    if (!m_TextureShader) return false;
    result = m_TextureShader->Initialize(m_D3D->GetDevice(), hwnd);
    if (!result)
    {
        MessageBox(hwnd, L"Could not initialize the texture shader object.", L"Error", MB_OK);
        return false;
    }
#pragma endregion

#pragma region Point UI 초기화
    m_PointUI = new BitmapClass;
    if (!m_PointUI) return false;
    result = m_PointUI->Initialize(m_D3D->GetDevice(), screenWidth, screenHeight, L"./data/UI/Point.dds", 50, 50);
    if (!result)
    {
        MessageBox(hwnd, L"Could not initialize the bitmap object.", L"Error", MB_OK);
        return false;
    }

    m_TextureShader = new TextureShaderClass;
    if (!m_TextureShader) return false;
    result = m_TextureShader->Initialize(m_D3D->GetDevice(), hwnd);
    if (!result)
    {
        MessageBox(hwnd, L"Could not initialize the texture shader object.", L"Error", MB_OK);
        return false;
    }
#pragma endregion

#pragma region 바닥 초기화
    ground_Model = new ModelClass;
    ground_Model->Initialize(m_D3D->GetDevice(), L"./data/Ground/ground.obj", L"./data/Ground/Grass2.dds");
#pragma endregion

#pragma region 쿠키 초기화
    for (int i = 0; i < 12; i++) cookie_Model[i] = new ModelClass;

    // 사람 쿠키
    result = cookie_Model[0]->Initialize(m_D3D->GetDevice(), L"./data/Cookie1/model.obj", L"./data/Cookie1/diffuse.dds");
    if (!result) return false;
    result = cookie_Model[1]->Initialize(m_D3D->GetDevice(), L"./data/Cookie2/model.obj", L"./data/Cookie2/diffuse.dds");
    if (!result) return false;
    result = cookie_Model[2]->Initialize(m_D3D->GetDevice(), L"./data/Cookie3/model.obj", L"./data/Cookie3/diffuse.dds");
    if (!result) return false;
    result = cookie_Model[3]->Initialize(m_D3D->GetDevice(), L"./data/Cookie4/model.obj", L"./data/Cookie4/diffuse.dds");
    if (!result) return false;
    result = cookie_Model[4]->Initialize(m_D3D->GetDevice(), L"./data/Cookie5/model.obj", L"./data/Cookie5/diffuse.dds");
    if (!result) return false;
    result = cookie_Model[5]->Initialize(m_D3D->GetDevice(), L"./data/Cookie6/model.obj", L"./data/Cookie6/diffuse.dds");
    if (!result) return false;

    // 동물 쿠키
    result = cookie_Model[6]->Initialize(m_D3D->GetDevice(), L"./data/Cookie_Cat/model.obj", L"./data/Cookie_Cat/diffuse.dds");
    if (!result) return false;
    result = cookie_Model[7]->Initialize(m_D3D->GetDevice(), L"./data/Cookie_Dog/model.obj", L"./data/Cookie_Dog/diffuse.dds");
    if (!result) return false;
    result = cookie_Model[8]->Initialize(m_D3D->GetDevice(), L"./data/Cookie_Cat/model.obj", L"./data/Cookie_Cat/diffuse.dds");
    if (!result) return false;
    result = cookie_Model[9]->Initialize(m_D3D->GetDevice(), L"./data/Cookie_Dog/model.obj", L"./data/Cookie_Dog/diffuse.dds");
    if (!result) return false;

    // 병정 쿠키
    result = cookie_Model[10]->Initialize(m_D3D->GetDevice(), L"./data/Cookie7/model.obj", L"./data/Cookie7/diffuse.dds");
    if (!result) return false;
    result = cookie_Model[11]->Initialize(m_D3D->GetDevice(), L"./data/Cookie7/model.obj", L"./data/Cookie7/diffuse.dds");
    if (!result) return false;
#pragma endregion

#pragma region Enemy 초기화
    // Initialize 함수 내부 어딘가 (랜덤 시드 및 9개 모델 로드)
    srand(static_cast<unsigned int>(time(NULL)));

    WCHAR modelPath[128];
    for (int i = 0; i < 41; i++)
    {
        m_zombieWalkModels[i] = new ModelClass;
        swprintf_s(modelPath, L"./data/Enemy/Zombie_Crawl%d.obj", i);

        // 디퓨즈 텍스처 경로는 기존에 쓰던 경로 그대로 매칭해줘!
        result = m_zombieWalkModels[i]->Initialize(m_D3D->GetDevice(), modelPath, L"./data/Enemy/diffuse.dds");
        if (!result) return false;
    }
#pragma endregion

#pragma region 무기 초기화
    gun_Model = new ModelClass;
    result = gun_Model->Initialize(m_D3D->GetDevice(), L"./data/Weapon/model.obj", L"./data/Weapon/diffuse.dds");
    if (!result) return false;
#pragma endregion

#pragma region 집 초기화
    {
        house_Model = new ModelClass;
        result = house_Model->Initialize(m_D3D->GetDevice(), L"./data/House/model.obj", L"./data/House/diffuse.dds");
        if (!result) return false;
    }
#pragma endregion

#pragma region 블루 사탕 나무 (인스턴싱)
    {
        blueCandyTree_Model = new ModelClass;
        if (!blueCandyTree_Model) return false;
        result = blueCandyTree_Model->Initialize(m_D3D->GetDevice(), L"./data/BlueCandyTree/model.obj", L"./data/BlueCandyTree/diffuse.dds");
        if (!result) return false;
        result = blueCandyTree_Model->InitializeInstanceBuffer(m_D3D->GetDevice(), _blueCandyTreeMaxCount);
        if (!result) return false;

        D3D11_MAPPED_SUBRESOURCE initResource;
        if (SUCCEEDED(m_D3D->GetDeviceContext()->Map(blueCandyTree_Model->GetInstanceBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &initResource)))
        {
            struct InstanceType { XMFLOAT3 position; };
            InstanceType* InstancesPtr = (InstanceType*)initResource.pData;

            float y = -0.2f;
            int idx = 0; // 인덱스를 자동으로 카운트해서 실수 방지!

            // -0.2f ~ +0.2f 사이의 미세한 랜덤 오프셋을 주는 람다 함수
            auto GetRandomOffset = []() {
                return ((float)rand() / RAND_MAX * 0.4f) - 0.2f;
                };

            // 1번째 줄
            {
                float x = -7.0f;
                for (float z = -7.0f; z <= 7.0f; z += 1.0f)
                {
                    float randX = x + GetRandomOffset();
                    float randZ = z + GetRandomOffset();

                    InstancesPtr[idx++].position = XMFLOAT3(randX, y, randZ);
                }
            }

            // 2번째 줄 (idx: 15 ~ 29)
            {
                float x = -6.0f;
                for (float z = -7.0f; z <= 7.0f; z += 1.0f)
                {
                    float randX = x + GetRandomOffset();
                    float randZ = z + GetRandomOffset();

                    InstancesPtr[idx++].position = XMFLOAT3(randX, y, randZ);
                }
            }

            // 3번째 줄 (idx: 30 ~ 44)
            {
                float x = -5.0f;
                for (float z = -7.0f; z <= 7.0f; z += 1.0f)
                {
                    float randX = x + GetRandomOffset();
                    float randZ = z + GetRandomOffset();

                    InstancesPtr[idx++].position = XMFLOAT3(randX, y, randZ);
                }
            }

            // 4번째 줄 (idx: 45 ~ 59)
            {
                float x = -4.0f;
                for (float z = -7.0f; z <= 7.0f; z += 1.0f)
                {
                    float randX = x + GetRandomOffset();
                    float randZ = z + GetRandomOffset();

                    InstancesPtr[idx++].position = XMFLOAT3(randX, y, randZ);
                }
            }

            // 5번째 줄 (idx: 60 ~ 68)
            {
                float x = -3.0f;
                for (float z = -7.0f; z <= -4.0f; z += 1.0f) {
                    float randX = x + GetRandomOffset();
                    float randZ = z + GetRandomOffset();
                    InstancesPtr[idx++].position = XMFLOAT3(randX, y, randZ);
                }
                for (float z = 3.0f; z <= 7.0f; z += 1.0f) {
                    float randX = x + GetRandomOffset();
                    float randZ = z + GetRandomOffset();
                    InstancesPtr[idx++].position = XMFLOAT3(randX, y, randZ);
                }
            }

            // 6번째 줄 (idx: 69 ~ 77)
            {
                float x = -2.0f;
                for (float z = -7.0f; z <= -4.0f; z += 1.0f) {
                    float randX = x + GetRandomOffset();
                    float randZ = z + GetRandomOffset();
                    InstancesPtr[idx++].position = XMFLOAT3(randX, y, randZ);
                }
                for (float z = 3.0f; z <= 7.0f; z += 1.0f) {
                    float randX = x + GetRandomOffset();
                    float randZ = z + GetRandomOffset();
                    InstancesPtr[idx++].position = XMFLOAT3(randX, y, randZ);
                }
            }

            // 7번째 줄 (idx: 78 ~ 86)
            {
                float x = -1.0f;
                for (float z = -7.0f; z <= -4.0f; z += 1.0f) {
                    float randX = x + GetRandomOffset();
                    float randZ = z + GetRandomOffset();
                    InstancesPtr[idx++].position = XMFLOAT3(randX, y, randZ);
                }
                for (float z = 3.0f; z <= 7.0f; z += 1.0f) {
                    float randX = x + GetRandomOffset();
                    float randZ = z + GetRandomOffset();
                    InstancesPtr[idx++].position = XMFLOAT3(randX, y, randZ);
                }
            }

            // 8번째 줄 (idx: 87 ~ 95)
            {
                float x = 0.0f;
                for (float z = -7.0f; z <= -4.0f; z += 1.0f) {
                    float randX = x + GetRandomOffset();
                    float randZ = z + GetRandomOffset();
                    InstancesPtr[idx++].position = XMFLOAT3(randX, y, randZ);
                }
                for (float z = 3.0f; z <= 7.0f; z += 1.0f) {
                    float randX = x + GetRandomOffset();
                    float randZ = z + GetRandomOffset();
                    InstancesPtr[idx++].position = XMFLOAT3(randX, y, randZ);
                }
            }

            // 9번째 줄 (idx: 96 ~ 104)
            {
                float x = 1.0f;
                for (float z = -7.0f; z <= -4.0f; z += 1.0f) {
                    float randX = x + GetRandomOffset();
                    float randZ = z + GetRandomOffset();
                    InstancesPtr[idx++].position = XMFLOAT3(randX, y, randZ);
                }
                for (float z = 3.0f; z <= 7.0f; z += 1.0f) {
                    float randX = x + GetRandomOffset();
                    float randZ = z + GetRandomOffset();
                    InstancesPtr[idx++].position = XMFLOAT3(randX, y, randZ);
                }
            }

            // 10번째 줄 (idx: 105 ~ 113)
            {
                float x = 2.0f;
                for (float z = -7.0f; z <= -4.0f; z += 1.0f) {
                    float randX = x + GetRandomOffset();
                    float randZ = z + GetRandomOffset();
                    InstancesPtr[idx++].position = XMFLOAT3(randX, y, randZ);
                }
                for (float z = 3.0f; z <= 7.0f; z += 1.0f) {
                    float randX = x + GetRandomOffset();
                    float randZ = z + GetRandomOffset();
                    InstancesPtr[idx++].position = XMFLOAT3(randX, y, randZ);
                }
            }

            // 11번째 줄 (idx: 114 ~ 122)
            {
                float x = 3.0f;
                for (float z = -7.0f; z <= -4.0f; z += 1.0f) {
                    float randX = x + GetRandomOffset();
                    float randZ = z + GetRandomOffset();
                    InstancesPtr[idx++].position = XMFLOAT3(randX, y, randZ);
                }
                for (float z = 3.0f; z <= 7.0f; z += 1.0f) {
                    float randX = x + GetRandomOffset();
                    float randZ = z + GetRandomOffset();
                    InstancesPtr[idx++].position = XMFLOAT3(randX, y, randZ);
                }
            }

            // 12번째 줄 (idx: 123 ~ 131)
            {
                float x = 4.0f;
                for (float z = -7.0f; z <= -4.0f; z += 1.0f) {
                    float randX = x + GetRandomOffset();
                    float randZ = z + GetRandomOffset();
                    InstancesPtr[idx++].position = XMFLOAT3(randX, y, randZ);
                }
                for (float z = 3.0f; z <= 7.0f; z += 1.0f) {
                    float randX = x + GetRandomOffset();
                    float randZ = z + GetRandomOffset();
                    InstancesPtr[idx++].position = XMFLOAT3(randX, y, randZ);
                }
            }

            // 13번째 줄 (idx: 132 ~ 140)
            {
                float x = 5.0f;
                for (float z = -7.0f; z <= -4.0f; z += 1.0f) {
                    float randX = x + GetRandomOffset();
                    float randZ = z + GetRandomOffset();
                    InstancesPtr[idx++].position = XMFLOAT3(randX, y, randZ);
                }
                for (float z = 3.0f; z <= 7.0f; z += 1.0f) {
                    float randX = x + GetRandomOffset();
                    float randZ = z + GetRandomOffset();
                    InstancesPtr[idx++].position = XMFLOAT3(randX, y, randZ);
                }
            }

            // 14번째 줄 (idx: 141 ~ 153)
            {
                float x = 6.0f;
                for (float z = -7.0f; z <= -2.0f; z += 1.0f) {
                    float randX = x + GetRandomOffset();
                    float randZ = z + GetRandomOffset();
                    InstancesPtr[idx++].position = XMFLOAT3(randX, y, randZ);
                }
                for (float z = 1.0f; z <= 7.0f; z += 1.0f) {
                    float randX = x + GetRandomOffset();
                    float randZ = z + GetRandomOffset();
                    InstancesPtr[idx++].position = XMFLOAT3(randX, y, randZ);
                }
            }

            // 15번째 줄 (idx: 154 ~ 165)
            {
                float x = 7.0f;
                for (float z = -7.0f; z <= -2.0f; z += 1.0f) {
                    float randX = x + GetRandomOffset();
                    float randZ = z + GetRandomOffset();
                    InstancesPtr[idx++].position = XMFLOAT3(randX, y, randZ);
                }
                for (float z = 2.0f; z <= 7.0f; z += 1.0f) {
                    float randX = x + GetRandomOffset();
                    float randZ = z + GetRandomOffset();
                    InstancesPtr[idx++].position = XMFLOAT3(randX, y, randZ);
                }
            }

            // 최종적으로 몇 개가 실제로 생성되었는지 저장
            _blueCandyTreeCurrentCount = idx;

            m_D3D->GetDeviceContext()->Unmap(blueCandyTree_Model->GetInstanceBuffer(), 0);
        }
    }
#pragma endregion

#pragma region 초록 사탕 나무 초기화
    GreenCandyTreeModel = new ModelClass;
    result = GreenCandyTreeModel->Initialize(m_D3D->GetDevice(), L"./data/GreenCandyTree/model.obj", L"./data/GreenCandyTree/diffuse.dds");
    if (!result) return false;
#pragma endregion

#pragma region 쿠키 자전거 초기화
    CookieBike_Model = new ModelClass;
    result = CookieBike_Model->Initialize(m_D3D->GetDevice(), L"./data/CookieBike/model.obj", L"./data/CookieBike/diffuse.dds");
    if (!result) return false;
#pragma endregion

#pragma region 케이크 초기화
    cake_Model = new ModelClass;
    if (!cake_Model) return false;
    result = cake_Model->Initialize(m_D3D->GetDevice(), L"./data/Cake/model.obj", L"./data/Cake/diffuse.dds");
    if (!result) return false;
#pragma endregion

#pragma region 비행기 초기화
    Airplane_Model = new ModelClass;
    if (!Airplane_Model) return false;
    result = Airplane_Model->Initialize(m_D3D->GetDevice(), L"./data/Airplane/model.obj", L"./data/Airplane/diffuse.dds");
    if (!result) return false;
#pragma endregion

#pragma region 디저트 월드 입구 초기화
    dessertWorld_Model = new ModelClass;
    if (!dessertWorld_Model) return false;
    result = dessertWorld_Model->Initialize(m_D3D->GetDevice(), L"./data/DessertWorld/model.obj", L"./data/DessertWorld/diffuse.dds");
    if (!result) return false;
#pragma endregion

#pragma region 비스킷 초기화 (인스턴싱)
    _biscuit_Model = new ModelClass;
    if (!_biscuit_Model) return false;
    result = _biscuit_Model->Initialize(m_D3D->GetDevice(), L"./data/Biscuit/model.obj", L"./data/Biscuit/diffuse.dds");
    if (!result) return false;
    result = _biscuit_Model->InitializeInstanceBuffer(m_D3D->GetDevice(), _biscuitMaxCount);
    if (!result) return false;

    D3D11_MAPPED_SUBRESOURCE biscuitInitResource;
    if (SUCCEEDED(m_D3D->GetDeviceContext()->Map(_biscuit_Model->GetInstanceBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &biscuitInitResource)))
    {
        struct InstanceType { XMFLOAT3 position; };
        InstanceType* instancesPtr = (InstanceType*)biscuitInitResource.pData;

        float y = -2.0f;
        int idx = 0;

        // 첫 번째 집 길
        instancesPtr[idx++].position = XMFLOAT3(0.0f, y, 1.0f);
        instancesPtr[idx++].position = XMFLOAT3(0.0f, y, 0.5f);
        instancesPtr[idx++].position = XMFLOAT3(0.0f, y, 0.0f);
        instancesPtr[idx++].position = XMFLOAT3(0.0f, y, -0.5f);
        instancesPtr[idx++].position = XMFLOAT3(0.0f, y, -1.0f);
        instancesPtr[idx++].position = XMFLOAT3(0.0f, y, -1.5f);
        instancesPtr[idx++].position = XMFLOAT3(0.0f, y, -2.0f);

        // Left
        instancesPtr[idx++].position = XMFLOAT3(-0.5f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(0.0f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(-0.5f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(-1.0f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(-1.5f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(-2.0f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(-2.5f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(-3.0f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(-3.5f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(-4.0f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(-4.5f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(-5.0f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(-5.5f, y, -2.0f);

        // Right
        instancesPtr[idx++].position = XMFLOAT3(0.5f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(1.0f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(1.5f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(2.0f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(2.5f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(3.0f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(3.5f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(4.0f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(4.5f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(5.0f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(5.5f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(6.0f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(6.5f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(7.0f, y, -2.0f);

        // 두 번째 집 길
        instancesPtr[idx++].position = XMFLOAT3(3.8f, y, 1.0f);
        instancesPtr[idx++].position = XMFLOAT3(3.8f, y, 0.5f);
        instancesPtr[idx++].position = XMFLOAT3(3.8f, y, 0.0f);
        instancesPtr[idx++].position = XMFLOAT3(3.8f, y, -0.5f);
        instancesPtr[idx++].position = XMFLOAT3(3.8f, y, -1.0f);
        instancesPtr[idx++].position = XMFLOAT3(3.8f, y, -1.5f);
        instancesPtr[idx++].position = XMFLOAT3(3.8f, y, -2.0f);

        // 세 번재 집 길
        instancesPtr[idx++].position = XMFLOAT3(7.5f, y, 1.0f);
        instancesPtr[idx++].position = XMFLOAT3(7.5f, y, 0.5f);
        instancesPtr[idx++].position = XMFLOAT3(7.5f, y, 0.0f);
        instancesPtr[idx++].position = XMFLOAT3(7.5f, y, -0.5f);
        instancesPtr[idx++].position = XMFLOAT3(7.5f, y, -1.0f);
        instancesPtr[idx++].position = XMFLOAT3(7.5f, y, -1.5f);
        instancesPtr[idx++].position = XMFLOAT3(7.5f, y, -2.0f);

        // 케이크 주변 길
        instancesPtr[idx++].position = XMFLOAT3(-5.5f, y, -1.5f);
        instancesPtr[idx++].position = XMFLOAT3(-5.5f, y, -1.0f);
        instancesPtr[idx++].position = XMFLOAT3(-5.5f, y, -0.5f);
        instancesPtr[idx++].position = XMFLOAT3(-5.5f, y, 0.0f);
        instancesPtr[idx++].position = XMFLOAT3(-6.0f, y, 0.0f);
        instancesPtr[idx++].position = XMFLOAT3(-6.5f, y, 0.0f);
        instancesPtr[idx++].position = XMFLOAT3(-7.0f, y, 0.0f);
        instancesPtr[idx++].position = XMFLOAT3(-7.5f, y, 0.0f);
        instancesPtr[idx++].position = XMFLOAT3(-8.0f, y, 0.0f);
        instancesPtr[idx++].position = XMFLOAT3(-8.5f, y, 0.0f);
        instancesPtr[idx++].position = XMFLOAT3(-9.0f, y, 0.0f);
        instancesPtr[idx++].position = XMFLOAT3(-9.5f, y, 0.0f);
        instancesPtr[idx++].position = XMFLOAT3(-9.5f, y, -0.5f);
        instancesPtr[idx++].position = XMFLOAT3(-9.5f, y, -1.0f);
        instancesPtr[idx++].position = XMFLOAT3(-9.5f, y, -1.5f);
        instancesPtr[idx++].position = XMFLOAT3(-9.5f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(-9.5f, y, -2.5f);
        instancesPtr[idx++].position = XMFLOAT3(-9.5f, y, -3.0f);
        instancesPtr[idx++].position = XMFLOAT3(-9.5f, y, -3.5f);
        instancesPtr[idx++].position = XMFLOAT3(-9.5f, y, -4.0f);
        instancesPtr[idx++].position = XMFLOAT3(-9.0f, y, -4.0f);
        instancesPtr[idx++].position = XMFLOAT3(-8.5f, y, -4.0f);
        instancesPtr[idx++].position = XMFLOAT3(-8.0f, y, -4.0f);
        instancesPtr[idx++].position = XMFLOAT3(-7.5f, y, -4.0f);
        instancesPtr[idx++].position = XMFLOAT3(-7.0f, y, -4.0f);
        instancesPtr[idx++].position = XMFLOAT3(-6.5f, y, -4.0f);
        instancesPtr[idx++].position = XMFLOAT3(-6.0f, y, -4.0f);

        instancesPtr[idx++].position = XMFLOAT3(-5.5f, y, -4.0f);
        instancesPtr[idx++].position = XMFLOAT3(-5.5f, y, -3.5f);
        instancesPtr[idx++].position = XMFLOAT3(-5.5f, y, -3.0f);
        instancesPtr[idx++].position = XMFLOAT3(-5.5f, y, -2.5f);
        instancesPtr[idx++].position = XMFLOAT3(-5.5f, y, 0.5f);
        instancesPtr[idx++].position = XMFLOAT3(-5.5f, y, 1.0f);
        instancesPtr[idx++].position = XMFLOAT3(-5.5f, y, 1.5f);
        instancesPtr[idx++].position = XMFLOAT3(-5.5f, y, 2.0f);
        instancesPtr[idx++].position = XMFLOAT3(-5.5f, y, 2.5f);
        instancesPtr[idx++].position = XMFLOAT3(-5.5f, y, 3.0f);

        // 네 번째 집 길
        instancesPtr[idx++].position = XMFLOAT3(0.0f, y, -2.5f);
        instancesPtr[idx++].position = XMFLOAT3(0.0f, y, -3.0f);
        instancesPtr[idx++].position = XMFLOAT3(0.0f, y, -3.5f);
        instancesPtr[idx++].position = XMFLOAT3(0.0f, y, -4.0f);
        instancesPtr[idx++].position = XMFLOAT3(0.0f, y, -4.5f);
        instancesPtr[idx++].position = XMFLOAT3(0.0f, y, -5.0f);
        instancesPtr[idx++].position = XMFLOAT3(0.0f, y, -5.5f);

        // 다섯 번째 집 길
        instancesPtr[idx++].position = XMFLOAT3(3.8f, y, -2.5f);
        instancesPtr[idx++].position = XMFLOAT3(3.8f, y, -3.0f);
        instancesPtr[idx++].position = XMFLOAT3(3.8f, y, -3.5f);
        instancesPtr[idx++].position = XMFLOAT3(3.8f, y, -4.0f);
        instancesPtr[idx++].position = XMFLOAT3(3.8f, y, -4.5f);
        instancesPtr[idx++].position = XMFLOAT3(3.8f, y, -5.0f);
        instancesPtr[idx++].position = XMFLOAT3(3.8f, y, -5.5f);

        // 여섯 번째 집 길
        instancesPtr[idx++].position = XMFLOAT3(7.5f, y, -2.5f);
        instancesPtr[idx++].position = XMFLOAT3(7.5f, y, -3.0f);
        instancesPtr[idx++].position = XMFLOAT3(7.5f, y, -3.5f);
        instancesPtr[idx++].position = XMFLOAT3(7.5f, y, -4.0f);
        instancesPtr[idx++].position = XMFLOAT3(7.5f, y, -4.5f);
        instancesPtr[idx++].position = XMFLOAT3(7.5f, y, -5.0f);
        instancesPtr[idx++].position = XMFLOAT3(7.5f, y, -5.5f);

        // 입구 길
        instancesPtr[idx++].position = XMFLOAT3(7.5f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(8.0f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(8.5f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(9.0f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(9.5f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(10.0f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(10.5f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(11.0f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(11.5f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(12.0f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(12.5f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(13.0f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(13.5f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(14.0f, y, -2.0f);
        instancesPtr[idx++].position = XMFLOAT3(14.5f, y, -2.0f);

        _biscuitCurrentCount = idx;

        m_D3D->GetDeviceContext()->Unmap(_biscuit_Model->GetInstanceBuffer(), 0);
    }
#pragma endregion

#pragma region 오브젝트1 초기화
    object1_Model = new ModelClass;
    if (!object1_Model) return false;
    result = object1_Model->Initialize(m_D3D->GetDevice(), L"./data/Object1/model.obj", L"./data/Object1/diffuse.dds");
    if (!result) return false;
#pragma endregion

#pragma region 2D 배경 초기화
    m_Bitmap = new BitmapClass;
    if (!m_Bitmap) return false;
    result = m_Bitmap->Initialize(m_D3D->GetDevice(), screenWidth, screenHeight, L"./data/Bitmap/Night.dds", screenWidth, screenHeight);
    if (!result)
    {
        MessageBox(hwnd, L"Could not initialize the bitmap object.", L"Error", MB_OK);
        return false;
    }

    m_TextureShader = new TextureShaderClass;
    if (!m_TextureShader) return false;
    result = m_TextureShader->Initialize(m_D3D->GetDevice(), hwnd);
    if (!result)
    {
        MessageBox(hwnd, L"Could not initialize the texture shader object.", L"Error", MB_OK);
        return false;
    }
    return true;
#pragma endregion
}

bool GraphicsClass::Render(char* timerStr, float rotation)
{

#pragma region 뷰 / 프로젝션 매트릭스 설정 및 화면 초기화
    XMMATRIX worldMatrix, viewMatrix, projectionMatrix, orthoMatrix;
    bool result;

    {
        m_D3D->BeginScene(0.0f, 0.0f, 0.0f, 1.0f);
        m_Camera->Render();
        m_Camera->GetViewMatrix(viewMatrix);
        m_D3D->GetProjectionMatrix(projectionMatrix);
        m_D3D->GetOrthoMatrix(orthoMatrix);
    }
#pragma endregion

#pragma region 2D 하늘 배경 렌더링
    m_D3D->TurnZBufferOff();
    m_D3D->GetWorldMatrix(worldMatrix);

    m_Bitmap->Render(m_D3D->GetDeviceContext(), 0, 0);
    m_TextureShader->Render(m_D3D->GetDeviceContext(), m_Bitmap->GetIndexCount(), worldMatrix, XMMatrixIdentity(), orthoMatrix, m_Bitmap->GetTexture(), 0);

    m_D3D->TurnZBufferOn();
#pragma endregion

#pragma region 빌보드 처리
    XMMATRIX billboardMatrix;
    {
        // 뷰 행렬을 역행렬로 만들어 카메라의 위치와 방향을 가져옴
        XMVECTOR det;
        XMMATRIX invView = XMMatrixInverse(&det, viewMatrix);

        // 카메라가 바라보는 방향 벡터(Look 벡터)의 X, Z 성분만 추출
        float camLookX = XMVectorGetX(invView.r[2]);
        float camLookZ = XMVectorGetZ(invView.r[2]);

        // X, Z 평면에서의 각도를 계산 (카메라를 정면으로 바라보도록 피벗 조정)
        float angleY = atan2f(camLookX, camLookZ);

        billboardMatrix = XMMatrixRotationY(angleY);
    }
#pragma endregion

#pragma region 바닥 렌더링
    {
        XMMATRIX scale = XMMatrixScaling(50.0f, 1.0f, 50.0f);
        XMMATRIX translate = XMMatrixTranslation(0.0f, -1.0f, 0.0f);
        worldMatrix = scale * translate;

        XMFLOAT4X4 tempMatrix;
        XMStoreFloat4x4(&tempMatrix, worldMatrix);
        tempMatrix._22 = 1.005f;
        worldMatrix = XMLoadFloat4x4(&tempMatrix);

        ground_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), ground_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, ground_Model->GetTexture(), m_filterType);
    }
#pragma endregion

#pragma region 쿠키 렌더링
    m_D3D->TurnOnAlphaBlending();
    {
        m_D3D->GetWorldMatrix(worldMatrix);

        // 쿠키1
        {
            XMMATRIX scaleMatrix = XMMatrixScaling(1.0f, 1.0f, 1.0f);
            XMMATRIX rotateMatrix = XMMatrixRotationY(rotation);
            XMMATRIX translateMatrix = XMMatrixTranslation(-0.5, -0.5f, 2.0f);
            worldMatrix = scaleMatrix * rotateMatrix * translateMatrix;

            cookie_Model[0]->Render(m_D3D->GetDeviceContext());
            m_TextureShader->Render(m_D3D->GetDeviceContext(), cookie_Model[0]->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, cookie_Model[0]->GetTexture(), 0);
        }

        // 쿠키2
        {
            XMMATRIX scaleMatrix = XMMatrixScaling(1.0f, 1.0f, 1.0f);
            XMMATRIX rotateMatrix = XMMatrixRotationY(rotation);
            XMMATRIX translateMatrix = XMMatrixTranslation(7.0, -0.5f, 2.0f);
            worldMatrix = scaleMatrix * rotateMatrix * translateMatrix;

            cookie_Model[1]->Render(m_D3D->GetDeviceContext());
            m_TextureShader->Render(m_D3D->GetDeviceContext(), cookie_Model[1]->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, cookie_Model[1]->GetTexture(), 0);
        }

        // 쿠키3
        {
            XMMATRIX scaleMatrix = XMMatrixScaling(1.0f, 1.0f, 1.0f);
            XMMATRIX rotateMatrix = XMMatrixRotationY(rotation);
            XMMATRIX translateMatrix = XMMatrixTranslation(14.5, -1.0f, 2.0f);
            worldMatrix = scaleMatrix * rotateMatrix * translateMatrix;

            cookie_Model[2]->Render(m_D3D->GetDeviceContext());
            m_TextureShader->Render(m_D3D->GetDeviceContext(), cookie_Model[2]->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, cookie_Model[2]->GetTexture(), 0);
        }

        // 쿠키4
        {
            XMMATRIX scaleMatrix = XMMatrixScaling(1.0f, 1.0f, 1.0f);
            XMMATRIX rotateMatrix = XMMatrixRotationY(rotation);
            XMMATRIX translateMatrix = XMMatrixTranslation(0.5, -1.0f, -11.0f);
            worldMatrix = scaleMatrix * rotateMatrix * translateMatrix;

            cookie_Model[3]->Render(m_D3D->GetDeviceContext());
            m_TextureShader->Render(m_D3D->GetDeviceContext(), cookie_Model[3]->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, cookie_Model[3]->GetTexture(), 0);
        }

        // 쿠키5
        {
            XMMATRIX scaleMatrix = XMMatrixScaling(1.0f, 1.0f, 1.0f);
            XMMATRIX rotateMatrix = XMMatrixRotationY(rotation);
            XMMATRIX translateMatrix = XMMatrixTranslation(8.2, -0.5f, -11.0f);
            worldMatrix = scaleMatrix * rotateMatrix * translateMatrix;

            cookie_Model[4]->Render(m_D3D->GetDeviceContext());
            m_TextureShader->Render(m_D3D->GetDeviceContext(), cookie_Model[4]->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, cookie_Model[4]->GetTexture(), 0);
        }

        // 쿠키6
        {
            XMMATRIX scaleMatrix = XMMatrixScaling(1.0f, 1.0f, 1.0f);
            XMMATRIX rotateMatrix = XMMatrixRotationY(rotation);
            XMMATRIX translateMatrix = XMMatrixTranslation(15.6, -0.5f, -11.0f);
            worldMatrix = scaleMatrix * rotateMatrix * translateMatrix;

            cookie_Model[5]->Render(m_D3D->GetDeviceContext());
            m_TextureShader->Render(m_D3D->GetDeviceContext(), cookie_Model[5]->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, cookie_Model[5]->GetTexture(), 0);
        }

        // 고양이 쿠키1
        {
            XMMATRIX scaleMatrix = XMMatrixScaling(0.65f, 0.65f, 0.65f);
            XMMATRIX translateMatrix = XMMatrixTranslation(5.0f, -1.0f, -3.5f);
            worldMatrix = scaleMatrix * billboardMatrix * translateMatrix;

            cookie_Model[6]->Render(m_D3D->GetDeviceContext());
            m_TextureShader->Render(m_D3D->GetDeviceContext(), cookie_Model[6]->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, cookie_Model[6]->GetTexture(), 0);
        }

        // 강아지 쿠키1
        {
            XMMATRIX scaleMatrix = XMMatrixScaling(0.65f, 0.65f, 0.65f);
            XMMATRIX translateMatrix = XMMatrixTranslation(2.0f, -1.0f, -4.5f);
            worldMatrix = scaleMatrix * billboardMatrix * translateMatrix;

            cookie_Model[7]->Render(m_D3D->GetDeviceContext());
            m_TextureShader->Render(m_D3D->GetDeviceContext(), cookie_Model[7]->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, cookie_Model[7]->GetTexture(), 0);
        }

        // 고양이 쿠키2
        {
            XMMATRIX scaleMatrix = XMMatrixScaling(0.65f, 0.65f, 0.65f);
            XMMATRIX translateMatrix = XMMatrixTranslation(15.0f, -1.0f, -3.5f);
            worldMatrix = scaleMatrix * billboardMatrix * translateMatrix;

            cookie_Model[8]->Render(m_D3D->GetDeviceContext());
            m_TextureShader->Render(m_D3D->GetDeviceContext(), cookie_Model[8]->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, cookie_Model[8]->GetTexture(), 0);
        }

        // 강아지 쿠키2
        {
            XMMATRIX scaleMatrix = XMMatrixScaling(0.65f, 0.65f, 0.65f);
            XMMATRIX translateMatrix = XMMatrixTranslation(11.0f, -1.0f, -4.5f);
            worldMatrix = scaleMatrix * billboardMatrix * translateMatrix;

            cookie_Model[9]->Render(m_D3D->GetDeviceContext());
            m_TextureShader->Render(m_D3D->GetDeviceContext(), cookie_Model[9]->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, cookie_Model[9]->GetTexture(), 0);
        }

        // 병정 쿠키 1
        {
            XMMATRIX scaleMatrix = XMMatrixScaling(1.0f, 1.0f, 1.0f);
            XMMATRIX rotationMatrix = XMMatrixRotationY(m_rotationY1);
            XMMATRIX translateMatrix = XMMatrixTranslation(32.0f, -0.5f, m_currentZ1);
            worldMatrix = scaleMatrix * rotationMatrix * translateMatrix;

            cookie_Model[10]->Render(m_D3D->GetDeviceContext());
            m_TextureShader->Render(m_D3D->GetDeviceContext(), cookie_Model[10]->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, cookie_Model[10]->GetTexture(), 0);
        }

        // 병정 쿠키 2
        {
            XMMATRIX scaleMatrix = XMMatrixScaling(1.0f, 1.0f, 1.0f);
            XMMATRIX rotationMatrix = XMMatrixRotationY(m_rotationY2);
            XMMATRIX translateMatrix = XMMatrixTranslation(32.0f, -0.5f, m_currentZ2);
            worldMatrix = scaleMatrix * rotationMatrix * translateMatrix;

            cookie_Model[11]->Render(m_D3D->GetDeviceContext());
            m_TextureShader->Render(m_D3D->GetDeviceContext(), cookie_Model[11]->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, cookie_Model[11]->GetTexture(), 0);
        }
    }
    m_D3D->TurnOffAlphaBlending();
#pragma endregion

#pragma region 집 렌더링
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(5.0f, 5.0f, 5.0f);
        XMMATRIX translate = XMMatrixTranslation(0.0f, 1.1f, 5.5f);
        worldMatrix = scale * translate;

        house_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), house_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, house_Model->GetTexture(), 0);
    }
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(5.0f, 5.0f, 5.0f);
        XMMATRIX translate = XMMatrixTranslation(7.5f, 1.1f, 5.5f);
        worldMatrix = scale * translate;

        house_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), house_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, house_Model->GetTexture(), 0);
    }
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(5.0f, 5.0f, 5.0f);
        XMMATRIX translate = XMMatrixTranslation(15.0f, 1.1f, 5.5f);
        worldMatrix = scale * translate;

        house_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), house_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, house_Model->GetTexture(), 0);
    }
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(5.0f, 5.0f, 5.0f);
        XMMATRIX translate = XMMatrixTranslation(-11.0f, 1.1f, 9.5f);
        worldMatrix = scale * translate;

        house_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), house_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, house_Model->GetTexture(), 0);
    }

    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(5.0f, 5.0f, 5.0f);
        XMMATRIX rotate = XMMatrixRotationY(3.1f);
        XMMATRIX translate = XMMatrixTranslation(0.0f, 1.1f, -14.5f);
        worldMatrix = scale * rotate * translate;

        house_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), house_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, house_Model->GetTexture(), 0);
    }
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(5.0f, 5.0f, 5.0f);
        XMMATRIX rotate = XMMatrixRotationY(3.1f);
        XMMATRIX translate = XMMatrixTranslation(7.5f, 1.1f, -14.5f);
        worldMatrix = scale * rotate * translate;

        house_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), house_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, house_Model->GetTexture(), 0);
    }
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(5.0f, 5.0f, 5.0f);
        XMMATRIX rotate = XMMatrixRotationY(3.1f);
        XMMATRIX translate = XMMatrixTranslation(15.0f, 1.1f, -14.5f);
        worldMatrix = scale * rotate * translate;

        house_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), house_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, house_Model->GetTexture(), 0);
    }
#pragma endregion

#pragma region 블루 사탕 나무 렌더링 (인스턴싱)
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        worldMatrix = XMMatrixScaling(5.0f, 5.0f, 5.0f);

        unsigned int strides[2] = { sizeof(ModelClass::VertexType), sizeof(XMFLOAT3) };
        unsigned int offsets[2] = { 0, 0 };
        ID3D11Buffer* bufferPointers[2] = { blueCandyTree_Model->GetVertexBuffer(), blueCandyTree_Model->GetInstanceBuffer() };
        m_D3D->GetDeviceContext()->IASetVertexBuffers(0, 2, bufferPointers, strides, offsets);

        m_TextureShader->RenderInstance(m_D3D->GetDeviceContext(), blueCandyTree_Model->GetIndexCount(), _blueCandyTreeCurrentCount, worldMatrix, viewMatrix, projectionMatrix, blueCandyTree_Model->GetTexture());

        ID3D11Buffer* nullBuffer[1] = { nullptr };
        unsigned int nullStride[1] = { 0 };
        unsigned int nullOffset[1] = { 0 };
        m_D3D->GetDeviceContext()->IASetVertexBuffers(1, 1, nullBuffer, nullStride, nullOffset);
    }
#pragma endregion

#pragma region 초록 사탕 나무 렌더링
    {
        XMMATRIX scale = XMMatrixScaling(2.5f, 2.5f, 2.5f);
        XMMATRIX rotate = XMMatrixRotationY(rotation);

        XMMATRIX positions[] = {
            XMMatrixTranslation(-5.0f, 0.0f, -15.5f),
            XMMatrixTranslation(-10.0f, 0.0f, -15.5f),
            XMMatrixTranslation(-15.0f, 0.0f, -15.5f),
            XMMatrixTranslation(-18.0f, 0.0f, -13.0f),
            XMMatrixTranslation(-18.0f, 0.0f, 2.0f),
            XMMatrixTranslation(-18.0f, 0.0f, 6.0f),
            XMMatrixTranslation(-16.0f, 0.0f, 10.0f)
        };

        for (int i = 0; i < 7; i++) {
            m_D3D->GetWorldMatrix(worldMatrix);
            worldMatrix = scale * rotate * positions[i];
            GreenCandyTreeModel->Render(m_D3D->GetDeviceContext());
            m_TextureShader->Render(m_D3D->GetDeviceContext(), GreenCandyTreeModel->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, GreenCandyTreeModel->GetTexture(), 0);
        }
    }
#pragma endregion

#pragma region 쿠키 자전거 렌더링
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
        XMMATRIX rotate = XMMatrixRotationY(rotation);
        XMMATRIX translateSmall = XMMatrixTranslation(0.0f, 0.0f, 1.5f);
        XMMATRIX translateFinal = XMMatrixTranslation(25.0f, -0.5f, 0.0f);

        worldMatrix = scale * translateSmall * rotate * translateFinal;

        CookieBike_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), CookieBike_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, CookieBike_Model->GetTexture(), 0);
    }
#pragma endregion

#pragma region 케이크 렌더링
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(6.0f, 6.0f, 6.0f);
        XMMATRIX rotate = XMMatrixRotationY(-0.5f);
        XMMATRIX translate = XMMatrixTranslation(-15.0f, 2.0f, -4.0f);
        worldMatrix = scale * rotate * translate;

        cake_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), cake_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, cake_Model->GetTexture(), 0);
    }
#pragma endregion

#pragma region 비행기 렌더링
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(3.5f, 3.5f, 3.5f);
        XMMATRIX rotateX = XMMatrixRotationX(rotation * 0.6f);
        XMMATRIX rotateY = XMMatrixRotationY((rotation * 0.4f) + XM_PI);
        XMMATRIX translateSmall = XMMatrixTranslation(0.0f, 0.0f, -20.0f);
        XMMATRIX translateFinal = XMMatrixTranslation(-5.0f, 15.0f, -5.0f);
        worldMatrix = scale * rotateX * translateSmall * rotateY * translateFinal;

        Airplane_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), Airplane_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, Airplane_Model->GetTexture(), 0);
    }
#pragma endregion

#pragma region 디저트 월드 입구 렌더링
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(4.0f, 4.0f, 4.0f);
        XMMATRIX rotate = XMMatrixRotationY(-92.7f);
        XMMATRIX translate = XMMatrixTranslation(30.0f, -1.0f, -4.05f);
        worldMatrix = scale * rotate * translate;

        dessertWorld_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), dessertWorld_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, dessertWorld_Model->GetTexture(), 0);
    }
#pragma endregion

#pragma region 젤리 꽃 렌더링
    // 1
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
        XMMATRIX translate = XMMatrixTranslation(18.0f, -0.75f, -1.0f);
        worldMatrix = scale * translate;

        object1_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), object1_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, object1_Model->GetTexture(), 0);
    }
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
        XMMATRIX translate = XMMatrixTranslation(21.0f, -0.75f, -2.0f);
        worldMatrix = scale * translate;

        object1_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), object1_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, object1_Model->GetTexture(), 0);
    }
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
        XMMATRIX translate = XMMatrixTranslation(23.0f, -0.75f, 2.0f);
        worldMatrix = scale * translate;

        object1_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), object1_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, object1_Model->GetTexture(), 0);
    }

    // 2
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
        XMMATRIX translate = XMMatrixTranslation(19.0f, -0.75f, -7.0f);
        worldMatrix = scale * translate;

        object1_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), object1_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, object1_Model->GetTexture(), 0);
    }
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
        XMMATRIX translate = XMMatrixTranslation(20.0f, -0.75f, -9.0f);
        worldMatrix = scale * translate;

        object1_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), object1_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, object1_Model->GetTexture(), 0);
    }
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
        XMMATRIX translate = XMMatrixTranslation(23.0f, -0.75f, -7.5f);
        worldMatrix = scale * translate;

        object1_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), object1_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, object1_Model->GetTexture(), 0);
    }
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
        XMMATRIX translate = XMMatrixTranslation(25.0f, -0.75f, -12.0f);
        worldMatrix = scale * translate;

        object1_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), object1_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, object1_Model->GetTexture(), 0);
    }

    // 3
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
        XMMATRIX translate = XMMatrixTranslation(10.0f, -0.75f, -6.5f);
        worldMatrix = scale * translate;

        object1_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), object1_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, object1_Model->GetTexture(), 0);
    }
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
        XMMATRIX translate = XMMatrixTranslation(12.2f, -0.75f, -8.0f);
        worldMatrix = scale * translate;

        object1_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), object1_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, object1_Model->GetTexture(), 0);
    }
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
        XMMATRIX translate = XMMatrixTranslation(13.5f, -0.75f, -5.5f);
        worldMatrix = scale * translate;

        object1_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), object1_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, object1_Model->GetTexture(), 0);
    }
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
        XMMATRIX translate = XMMatrixTranslation(10.5f, -0.75f, -9.5f);
        worldMatrix = scale * translate;

        object1_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), object1_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, object1_Model->GetTexture(), 0);
    }

    // 5
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
        XMMATRIX translate = XMMatrixTranslation(9.0f, -0.75f, 2.0f);
        worldMatrix = scale * translate;

        object1_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), object1_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, object1_Model->GetTexture(), 0);
    }
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
        XMMATRIX translate = XMMatrixTranslation(9.5f, -0.75f, -0.5f);
        worldMatrix = scale *  translate;

        object1_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), object1_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, object1_Model->GetTexture(), 0);
    }
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
        XMMATRIX translate = XMMatrixTranslation(11.5f, -0.75f, 0.0f);
        worldMatrix = scale * translate;

        object1_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), object1_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, object1_Model->GetTexture(), 0);
    }

    // 6
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
        XMMATRIX translate = XMMatrixTranslation(1.5f, -0.75f, 1.5f);
        worldMatrix = scale * translate;

        object1_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), object1_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, object1_Model->GetTexture(), 0);
    }
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
        XMMATRIX translate = XMMatrixTranslation(3.0f, -0.75f, -0.45f);
        worldMatrix = scale * translate;

        object1_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), object1_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, object1_Model->GetTexture(), 0);
    }
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
        XMMATRIX translate = XMMatrixTranslation(4.5f, -0.75f, 0.0f);
        worldMatrix = scale * translate;

        object1_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), object1_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, object1_Model->GetTexture(), 0);
    }

    // 7
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
        XMMATRIX translate = XMMatrixTranslation(1.5f, -0.75f, -7.5f);
        worldMatrix = scale * translate;

        object1_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), object1_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, object1_Model->GetTexture(), 0);
    }
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
        XMMATRIX translate = XMMatrixTranslation(3.0f, -0.75f, -8.45f);
        worldMatrix = scale * translate;

        object1_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), object1_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, object1_Model->GetTexture(), 0);
    }
    {
        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
        XMMATRIX translate = XMMatrixTranslation(4.5f, -0.75f, -10.0f);
        worldMatrix = scale * translate;

        object1_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), object1_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, object1_Model->GetTexture(), 0);
    }
#pragma endregion

#pragma region 비스킷 렌더링 (인스턴싱)
    {
        worldMatrix = XMMatrixScaling(2.0f, 0.5f, 2.0f);

        unsigned int strides[2] = { sizeof(ModelClass::VertexType), sizeof(XMFLOAT3) };
        unsigned int offsets[2] = { 0, 0 };
        ID3D11Buffer* bufferPointers[2] = { _biscuit_Model->GetVertexBuffer(), _biscuit_Model->GetInstanceBuffer() };

        m_D3D->GetDeviceContext()->IASetVertexBuffers(0, 2, bufferPointers, strides, offsets);
        m_TextureShader->RenderInstance(m_D3D->GetDeviceContext(), _biscuit_Model->GetIndexCount(), _biscuitCurrentCount, worldMatrix, viewMatrix, projectionMatrix, _biscuit_Model->GetTexture());

        ID3D11Buffer* nullBuffer[1] = { nullptr };
        unsigned int nullStride[1] = { 0 };
        unsigned int nullOffset[1] = { 0 };
        m_D3D->GetDeviceContext()->IASetVertexBuffers(1, 1, nullBuffer, nullStride, nullOffset);
    }
#pragma endregion

#pragma region 에너미 클래스 루프 렌더링
    for (auto enemy : m_enemies)
    {
        m_D3D->GetWorldMatrix(worldMatrix);

        XMFLOAT3 ePos = enemy->GetPosition();
        float eRotY = enemy->GetRotationY();
        int frameIndex = enemy->GetCurrentFrame(); // 현재 애니메이션 프레임 번호 따기

        XMMATRIX scaleMatrix = XMMatrixScaling(1.0f, 1.0f, 1.0f);
        XMMATRIX rotationMatrix = XMMatrixRotationY(eRotY);
        XMMATRIX translationMatrix = XMMatrixTranslation(ePos.x, ePos.y - 0.9f, ePos.z);

        worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;

        // 인덱스에 맞는 메쉬 출력
        m_zombieWalkModels[frameIndex]->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), m_zombieWalkModels[frameIndex]->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, m_zombieWalkModels[frameIndex]->GetTexture(), 0);
    }
#pragma endregion

#pragma region 무기 렌더링
    {
        m_D3D->TurnZBufferOff();
        m_D3D->GetWorldMatrix(worldMatrix);

        XMMATRIX scale = XMMatrixScaling(1.8f, 1.8f, 1.8f);
        XMMATRIX localRot = XMMatrixRotationY(4.75f);
        XMMATRIX localPos = XMMatrixTranslation(0.6f, -0.4f, 1.2f);

        XMMATRIX gunLocal = scale * localRot * localPos;

        XMFLOAT3 camRot = m_Camera->GetRotation();
        XMFLOAT3 camPos = m_Camera->GetPosition();

        float pitch = camRot.x * 0.0174532925f;
        float yaw = camRot.y * 0.0174532925f;
        float roll = camRot.z * 0.0174532925f;

        XMMATRIX cameraRotation = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
        XMMATRIX cameraTranslation = XMMatrixTranslation(camPos.x, camPos.y, camPos.z);

        worldMatrix = gunLocal * cameraRotation * cameraTranslation;

        gun_Model->Render(m_D3D->GetDeviceContext());
        m_TextureShader->Render(m_D3D->GetDeviceContext(), gun_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, gun_Model->GetTexture(), 0);
        m_D3D->TurnZBufferOn();
    }
#pragma endregion

#pragma region 인벤토리 UI 렌더링
    {
        m_D3D->TurnZBufferOff();
        m_D3D->TurnOnAlphaBlending();
        m_D3D->GetWorldMatrix(worldMatrix);
        m_inventoryUI->Render(m_D3D->GetDeviceContext(), 470, 590);

        m_TextureShader->Render(
            m_D3D->GetDeviceContext(),
            m_inventoryUI->GetIndexCount(),
            worldMatrix,
            XMMatrixIdentity(),
            orthoMatrix,
            m_inventoryUI->GetTexture(),
            0
        );
        m_D3D->TurnOffAlphaBlending();
        m_D3D->TurnZBufferOn();
    }
#pragma endregion

#pragma region Hp UI 렌더링
    {
        m_D3D->TurnZBufferOff();
        m_D3D->TurnOnAlphaBlending();

        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX uiViewMatrix = XMMatrixIdentity();

        m_HpUI->Render(m_D3D->GetDeviceContext(), -5, 620);
        m_TextureShader->Render(
            m_D3D->GetDeviceContext(),
            m_HpUI->GetIndexCount(),
            worldMatrix,
            uiViewMatrix,
            orthoMatrix,
            m_HpUI->GetTexture(), 0
        );
    }
#pragma endregion

#pragma region Hp Value 렌더링
    {
        m_D3D->TurnZBufferOff();
        m_D3D->TurnOnAlphaBlending();

        m_D3D->GetWorldMatrix(worldMatrix);
        XMMATRIX uiViewMatrix = XMMatrixIdentity();

        float hpRatio = (float)m_playerHp / (float)m_playerMaxHp;
        if (hpRatio < 0.0f) hpRatio = 0.0f;
        if (hpRatio > 1.0f) hpRatio = 1.0f;

        int originalRedWidth = 280;
        int currentRedWidth = static_cast<int>(originalRedWidth * hpRatio);

        m_HpBarRed->UpdateBuffers(m_D3D->GetDeviceContext(), 80, 637, currentRedWidth);

        m_HpBarRed->Render(m_D3D->GetDeviceContext(), 80, 637);
        m_TextureShader->Render(m_D3D->GetDeviceContext(), m_HpBarRed->GetIndexCount(), worldMatrix, uiViewMatrix, orthoMatrix, m_HpBarRed->GetTexture(), 0);
    }
#pragma endregion

#pragma region Point UI 렌더링
    m_D3D->TurnZBufferOff();
    m_D3D->TurnOnAlphaBlending();

    m_D3D->GetWorldMatrix(worldMatrix);
    XMMATRIX uiViewMatrix = XMMatrixIdentity();

    m_PointUI->Render(m_D3D->GetDeviceContext(), 660, 350);
    m_TextureShader->Render(
        m_D3D->GetDeviceContext(),
        m_PointUI->GetIndexCount(),
        worldMatrix,
        uiViewMatrix,
        orthoMatrix,
        m_PointUI->GetTexture(), 0
    );
#pragma endregion


#pragma region 타이틀 2D 렌더링 (맨 위에 그려져야 함)
    if (m_showTitle)
    {
        m_D3D->TurnZBufferOff();
        m_D3D->GetWorldMatrix(worldMatrix);

        m_TitleBitmap->Render(m_D3D->GetDeviceContext(), 0, 0);
        m_TextureShader->Render(m_D3D->GetDeviceContext(), m_TitleBitmap->GetIndexCount(), worldMatrix, XMMatrixIdentity(), orthoMatrix, m_TitleBitmap->GetTexture(), 0);

        m_D3D->TurnZBufferOn();
    }
#pragma endregion

#pragma region 2D 성능 정보 렌더링
    m_D3D->TurnZBufferOff();
    int fpsValue = m_Fps->GetFps();
    int cpuValue = m_Cpu->GetCpuPercentage();

    // 객체 수 계산
    int ground = 1;
    int cake = 1;
    int dessertWorld = 1;
    int object1 = 20;
    int greenCandyTree = 15;
    int house = 7;
    int cookieCount = sizeof(cookie_Model) / sizeof(cookie_Model[0]);
    int airplane = 1;

    // 단일 모델 (바닥(1), 케이크(1), 디저트 입구(1), 분수대(1), 
    int singleModel = ground + cake + dessertWorld + greenCandyTree + house + object1 + cookieCount + airplane;

    // 인스턴싱 모델은 따로 계산
    int objectCount = singleModel + _blueCandyTreeCurrentCount + _biscuitCurrentCount;
    int polygonCount = 0;

    // 폴리곤 수 계산
    polygonCount += (ground_Model->GetIndexCount() / 3) * ground;
    polygonCount += (cake_Model->GetIndexCount() / 3) * cake;
    polygonCount += (dessertWorld_Model->GetIndexCount() / 3) * dessertWorld;
    polygonCount += (Airplane_Model->GetIndexCount() / 3) * airplane;
    polygonCount += (GreenCandyTreeModel->GetIndexCount() / 3) * greenCandyTree;
    polygonCount += (object1_Model->GetIndexCount() / 3) * object1;
    polygonCount += (house_Model->GetIndexCount() / 3) * house;

    for (int i = 0; i < cookieCount; i++)
    {
        polygonCount += cookie_Model[i]->GetIndexCount() / 3;
    }

    m_D3D->TurnOnAlphaBlending();

    int screenWidth = m_Text->m_screenWidth;
    int screenHeight = m_Text->m_screenHeight;

    m_Text->SetFPS(fpsValue, m_D3D->GetDeviceContext());
    m_Text->SetCPU(cpuValue, m_D3D->GetDeviceContext());
    m_Text->SetObjectCount(objectCount, m_D3D->GetDeviceContext());
    m_Text->SetPolygonCount(polygonCount, m_D3D->GetDeviceContext());
    m_Text->SetResolution(screenWidth, screenHeight, m_D3D->GetDeviceContext());
    m_Text->SetResolution(screenWidth, screenHeight, m_D3D->GetDeviceContext());
    m_Text->SetTimer(timerStr, m_D3D->GetDeviceContext());

    m_D3D->GetWorldMatrix(worldMatrix);
    m_Text->Render(m_D3D->GetDeviceContext(), worldMatrix, orthoMatrix);

    m_D3D->TurnOffAlphaBlending();
    m_D3D->TurnZBufferOn();
    m_D3D->EndScene();
    return true;
#pragma endregion
}

#pragma region Enemy
void GraphicsClass::SpawnEnemy()
{
    float randomX = static_cast<float>((rand() % 80) - 40);
    float randomZ = static_cast<float>((rand() % 80) - 40);
    float randomSpeed = 5.0f + static_cast<float>(rand() % 3);

    EnemyClass* newEnemy = new EnemyClass();
    newEnemy->Initialize(XMFLOAT3(randomX, 0.0f, randomZ), randomSpeed);

    m_enemies.push_back(newEnemy);
}

void GraphicsClass::ProcessShooting()
{
    XMFLOAT3 rayOrigin = m_Camera->GetPosition();

    XMMATRIX viewMatrix;
    m_Camera->GetViewMatrix(viewMatrix);
    XMMATRIX invView = XMMatrixInverse(nullptr, viewMatrix);

    XMFLOAT4X4 m;
    XMStoreFloat4x4(&m, invView);
    XMVECTOR rayDirVec = XMVectorSet(m._31, m._32, m._33, 0.0f);
    rayDirVec = XMVector3Normalize(rayDirVec);

    XMFLOAT3 rayDirection;
    XMStoreFloat3(&rayDirection, rayDirVec);

    int targetIndex = -1;
    float closestDist = 999999.0f;

    for (size_t i = 0; i < m_enemies.size(); ++i)
    {
        XMFLOAT3 enemyPos = m_enemies[i]->GetBoundingPosition();
        float radius = m_enemies[i]->GetBoundingRadius();

        float vX = enemyPos.x - rayOrigin.x;
        float vY = enemyPos.y - rayOrigin.y;
        float vZ = enemyPos.z - rayOrigin.z;

        float t = vX * rayDirection.x + vY * rayDirection.y + vZ * rayDirection.z;
        if (t < 0.0f) continue;

        float closestX = rayOrigin.x + rayDirection.x * t;
        float closestY = rayOrigin.y + rayDirection.y * t;
        float closestZ = rayOrigin.z + rayDirection.z * t;

        float dX = enemyPos.x - closestX;
        float dY = enemyPos.y - closestY;
        float dZ = enemyPos.z - closestZ;
        float minDistanceSq = dX * dX + dY * dY + dZ * dZ;

        if (minDistanceSq <= (radius * radius))
        {
            if (t < closestDist)
            {
                closestDist = t;
                targetIndex = static_cast<int>(i);
            }
        }
    }

    if (targetIndex != -1)
    {
        delete m_enemies[targetIndex];
        m_enemies.erase(m_enemies.begin() + targetIndex);
    }
}
#pragma endregion