#include "EnemyClass.h"
#include <cmath>

EnemyClass::EnemyClass()
{
    m_position = XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_speed = 0.0f;
    m_rotationY = 0.0f;
    m_animTimer = 0.0f;
    m_currentFrame = 0;
}

EnemyClass::~EnemyClass() {}

void EnemyClass::Initialize(XMFLOAT3 spawnPos, float speed)
{
    m_position = spawnPos;
    m_speed = speed;
}

void EnemyClass::Update(XMFLOAT3 playerPos, float dt)
{
    float dirX = playerPos.x - m_position.x;
    float dirZ = playerPos.z - m_position.z;
    float distance = sqrt(dirX * dirX + dirZ * dirZ);

    if (distance > 1.0f)
    {
        dirX /= distance;
        dirZ /= distance;

        m_position.x += dirX * m_speed * 0.1f * dt;
        m_position.z += dirZ * m_speed * 0.1f * dt;

        m_animTimer += dt;
        if (m_animTimer >= 0.06f)
        {
            m_currentFrame = (m_currentFrame + 1) % 41;
            m_animTimer = 0.0f;
        }
    }
    else
    {
        m_currentFrame = 0; // 멈추면 기본 자세
    }
    m_rotationY = atan2(dirX, dirZ) + 3.141592f;
}