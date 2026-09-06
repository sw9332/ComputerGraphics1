#include "systemclass.h"

#pragma region 생성자, 소멸자
SystemClass::SystemClass()
{
	m_Input = 0;
	m_Graphics = 0;
}


SystemClass::SystemClass(const SystemClass& other)
{
}


SystemClass::~SystemClass()
{
}
#pragma endregion

#pragma region Initialize
bool SystemClass::Initialize()
{
	int screenWidth, screenHeight;
	bool result;


	// Initialize the width and height of the screen to zero before sending the variables into the function.
	screenWidth = 0;
	screenHeight = 0;

	// Initialize the windows api.
	InitializeWindows(screenWidth, screenHeight);

	// Create the input object.  This object will be used to handle reading the keyboard input from the user.
	m_Input = new InputClass;
	if (!m_Input)
	{
		return false;
	}

	// Initialize the input object.
	m_Input->Initialize();

	// Create the graphics object.  This object will handle rendering all the graphics for this application.
	m_Graphics = new GraphicsClass;
	if (!m_Graphics)
	{
		return false;
	}

	// Initialize the graphics object.
	result = m_Graphics->Initialize(screenWidth, screenHeight, m_hwnd);
	if (!result)
	{
		return false;
	}

	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	m_frequency = frequency.QuadPart;

	// 시작 시간 초기화
	LARGE_INTEGER startTime;
	QueryPerformanceCounter(&startTime);
	m_startTime = startTime.QuadPart;

	m_deltaTime = 0.0f;
	m_gameTimer = 120.0f;

	return true;
}
#pragma endregion

#pragma region Shutdown
void SystemClass::Shutdown()
{
	// Release the graphics object.
	if (m_Graphics)
	{
		m_Graphics->Shutdown();
		delete m_Graphics;
		m_Graphics = 0;
	}

	// Release the input object.
	if (m_Input)
	{
		delete m_Input;
		m_Input = 0;
	}

	// Shutdown the window.
	ShutdownWindows();

	return;
}
#pragma endregion

#pragma region Run
void SystemClass::Run()
{
	MSG msg;
	bool done, result;

	ZeroMemory(&msg, sizeof(MSG));

	done = false;
	while (!done)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT)
		{
			done = true;
		}
		else
		{
			LARGE_INTEGER currentTime;
			QueryPerformanceCounter(&currentTime);

			m_deltaTime = (float)(currentTime.QuadPart - m_startTime) / (float)m_frequency;

			m_startTime = currentTime.QuadPart;

			if (m_deltaTime > 0.1f) m_deltaTime = 0.1f;

			result = Frame();
			if (!result)
			{
				done = true;
			}
		}
	}
}
#pragma endregion

#pragma region Frame
bool SystemClass::Frame()
{
	bool result;

	if (m_Graphics && !m_Graphics->m_showTitle)
	{
		if (m_gameTimer > 0.0f)
		{
			m_gameTimer -= m_deltaTime;
			if (m_gameTimer < 0.0f) m_gameTimer = 0.0f;
		}

		if (m_gameTimer <= 0.0f)
		{
			return false;
		}
	}

	int minutes = static_cast<int>(m_gameTimer) / 60;
	int seconds = static_cast<int>(m_gameTimer) % 60;

	// 3. 문자열 조립
	char timerString[16];
	sprintf_s(timerString, "%02d:%02d", minutes, seconds);

	if (m_Input->IsKeyDown(VK_ESCAPE))
	{
		return false;
	}

	if (GetForegroundWindow() == m_hwnd && m_Graphics)
	{
		RECT rect;
		GetWindowRect(m_hwnd, &rect);

		int centerX = rect.left + (rect.right - rect.left) / 2;
		int centerY = rect.top + (rect.bottom - rect.top) / 2;

		POINT currentPos;
		GetCursorPos(&currentPos);

		int deltaX = currentPos.x - centerX;
		int deltaY = currentPos.y - centerY;

		if (!m_Graphics->m_showTitle)
		{
			ClipCursor(&rect);
			SetCursorPos(centerX, centerY);

			float sensitivity = 0.15f;
			if (m_Graphics->m_Camera && (deltaX != 0 || deltaY != 0))
			{
				XMFLOAT3 rot = m_Graphics->m_Camera->GetRotation();

				rot.y += (float)deltaX * sensitivity;
				rot.x += (float)deltaY * sensitivity;

				if (rot.x > 89.0f)  rot.x = 89.0f;
				if (rot.x < -89.0f) rot.x = -89.0f;

				m_Graphics->m_Camera->SetRotation(rot.x, rot.y, rot.z);
			}
		}
		else
		{
			ClipCursor(NULL);
			SetCursorPos(centerX, centerY);
		}
	}
	else
	{
		ClipCursor(NULL);
	}

	if (m_Graphics && m_Graphics->m_Camera && !m_Graphics->m_showTitle)
	{
		XMFLOAT3 pos = m_Graphics->m_Camera->GetPosition();
		XMFLOAT3 forward = m_Graphics->m_Camera->GetForward();
		XMFLOAT3 right = m_Graphics->m_Camera->GetRight();

		XMVECTOR hForwardVec = XMVectorSet(forward.x, 0.0f, forward.z, 0.0f);
		hForwardVec = XMVector3Normalize(hForwardVec);

		XMVECTOR rightVec = XMVectorSet(right.x, 0.0f, right.z, 0.0f);
		rightVec = XMVector3Normalize(rightVec);

		XMVECTOR moveDir = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

		if ((GetAsyncKeyState(VK_UP) & 0x8000) || (GetAsyncKeyState('W') & 0x8000))
		{
			moveDir += hForwardVec;
		}
		if ((GetAsyncKeyState(VK_DOWN) & 0x8000) || (GetAsyncKeyState('S') & 0x8000))
		{
			moveDir -= hForwardVec;
		}
		if ((GetAsyncKeyState(VK_LEFT) & 0x8000) || (GetAsyncKeyState('A') & 0x8000))
		{
			moveDir -= rightVec;
		}
		if ((GetAsyncKeyState(VK_RIGHT) & 0x8000) || (GetAsyncKeyState('D') & 0x8000))
		{
			moveDir += rightVec;
		}

		if (XMVector3Less(XMVector3LengthSq(moveDir), XMVectorSet(0.0001f, 0.0001f, 0.0001f, 0.0001f)) == false)
		{
			moveDir = XMVector3Normalize(moveDir);

			XMFLOAT3 finalDir;
			XMStoreFloat3(&finalDir, moveDir);

			pos.x += finalDir.x * _moveSpeed * m_deltaTime;
			pos.z += finalDir.z * _moveSpeed * m_deltaTime;
		}

		m_Graphics->m_Camera->SetPosition(pos.x, pos.y, pos.z);
	}

	result = m_Graphics->Frame(timerString);
	if (!result)
	{
		return false;
	}

	return true;
}
#pragma endregion

#pragma region MessageHandler
LRESULT CALLBACK SystemClass::MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	switch (umsg)
	{
	case WM_KEYDOWN:
	{
		m_Input->KeyDown((unsigned int)wparam);
		return 0;
	}

	case WM_KEYUP:
	{
		m_Input->KeyUp((unsigned int)wparam);
		return 0;
	}

	default:
	{
		return DefWindowProc(hwnd, umsg, wparam, lparam);
	}
	}
}
#pragma endregion

#pragma region InitializeWindows
void SystemClass::InitializeWindows(int& screenWidth, int& screenHeight)
{
	WNDCLASSEX wc;
	DEVMODE dmScreenSettings;
	int posX, posY;


	ApplicationHandle = this;
	m_hinstance = GetModuleHandle(NULL);
	m_applicationName = L"Game";

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = m_hinstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = m_applicationName;
	wc.cbSize = sizeof(WNDCLASSEX);

	RegisterClassEx(&wc);

	screenWidth = GetSystemMetrics(SM_CXSCREEN);
	screenHeight = GetSystemMetrics(SM_CYSCREEN);

	if (FULL_SCREEN)
	{
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth = (unsigned long)screenWidth;
		dmScreenSettings.dmPelsHeight = (unsigned long)screenHeight;
		dmScreenSettings.dmBitsPerPel = 32;
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

		posX = posY = 0;
	}
	else
	{
		screenWidth = 1366;
		screenHeight = 768;

		posX = (GetSystemMetrics(SM_CXSCREEN) - screenWidth) / 2;
		posY = (GetSystemMetrics(SM_CYSCREEN) - screenHeight) / 2;
	}

	m_hwnd = CreateWindowEx(WS_EX_APPWINDOW, m_applicationName, m_applicationName,
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW,
		posX, posY, screenWidth, screenHeight, NULL, NULL, m_hinstance, NULL);

	ShowWindow(m_hwnd, SW_SHOW);
	SetForegroundWindow(m_hwnd);
	SetFocus(m_hwnd);

	// Hide the mouse cursor.
	ShowCursor(false);

	return;
}
#pragma endregion

#pragma region ShutdownWindows
void SystemClass::ShutdownWindows()
{
	ClipCursor(NULL);

	// Show the mouse cursor.
	ShowCursor(true);

	if (FULL_SCREEN)
	{
		ChangeDisplaySettings(NULL, 0);
	}

	DestroyWindow(m_hwnd);
	m_hwnd = NULL;

	UnregisterClass(m_applicationName, m_hinstance);
	m_hinstance = NULL;

	ApplicationHandle = NULL;

	return;
}
#pragma endregion

#pragma region WndProc
LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
	switch (umessage)
	{
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}

	case WM_CLOSE:
	{
		PostQuitMessage(0);
		return 0;
	}

	default:
	{
		return ApplicationHandle->MessageHandler(hwnd, umessage, wparam, lparam);
	}
	}
}
#pragma endregion