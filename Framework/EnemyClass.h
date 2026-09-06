#pragma once
#include <directxmath.h>

using namespace DirectX;

class EnemyClass
{
public:
    EnemyClass();
    ~EnemyClass();

    // 초기화 및 업데이트 함수
    void Initialize(XMFLOAT3 spawnPos, float speed);
    void Update(XMFLOAT3 playerPos, float dt);

    // Get 함수들 (렌더링 및 충돌용)
    XMFLOAT3 GetPosition() const { return m_position; }
    float GetRotationY() const { return m_rotationY; }
    int GetCurrentFrame() const { return m_currentFrame; }
    float GetBoundingRadius() const { return 0.2f; }

    XMFLOAT3 GetBoundingPosition() const
    {
        return XMFLOAT3(m_position.x, m_position.y - 0.35f, m_position.z);
    }

private:
    XMFLOAT3 m_position;     // 현재 위치
    float m_speed;           // 이동 속도
    float m_rotationY;       // Y축 회전값 (플레이어 조준)

    float m_animTimer;       // 애니메이션 재생 타이머
    int m_currentFrame;      // 현재 재생 중인 프레임 번호 (0 ~ 8)
};