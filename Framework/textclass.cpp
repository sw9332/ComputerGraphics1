///////////////////////////////////////////////////////////////////////////////
// Filename: textclass.cpp
///////////////////////////////////////////////////////////////////////////////
#include "textclass.h"


TextClass::TextClass()
{
	m_Font = 0;
	m_FontShader = 0;

	m_sentence1 = 0;
	m_sentence2 = 0;
	m_sentence3 = 0;
	m_sentence4 = 0;
	m_sentence5 = 0;
	m_sentence6 = 0;
}


TextClass::TextClass(const TextClass& other)
{
}


TextClass::~TextClass()
{
}


bool TextClass::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, HWND hwnd,
	int screenWidth, int screenHeight, XMMATRIX baseViewMatrix)
{
	bool result;


	// Store the screen width and height.
	m_screenWidth = screenWidth;
	m_screenHeight = screenHeight;

	// Store the base view matrix.
	m_baseViewMatrix = baseViewMatrix;

	// Create the font object.
	m_Font = new FontClass;
	if (!m_Font)
	{
		return false;
	}

	// Initialize the font object.
	result = m_Font->Initialize(device, L"./data/fontdata.txt", L"./data/font.dds");
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the font object.", L"Error", MB_OK);
		return false;
	}

	// Create the font shader object.
	m_FontShader = new FontShaderClass;
	if (!m_FontShader)
	{
		return false;
	}

	// Initialize the font shader object.
	result = m_FontShader->Initialize(device, hwnd);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the font shader object.", L"Error", MB_OK);
		return false;
	}

	// Initialize the sentences.
	result = InitializeSentence(&m_sentence1, 128, device);
	if (!result) return false;
	result = InitializeSentence(&m_sentence2, 128, device);
	if (!result) return false;
	result = InitializeSentence(&m_sentence3, 128, device);
	if (!result) return false;
	result = InitializeSentence(&m_sentence4, 128, device);
	if (!result) return false;
	result = InitializeSentence(&m_sentence5, 128, device);
	if (!result) return false;
	result = InitializeSentence(&m_sentence6, 128, device);
	if (!result) return false;

	return true;
}


void TextClass::Shutdown()
{
	ReleaseSentence(&m_sentence1);
	ReleaseSentence(&m_sentence2);
	ReleaseSentence(&m_sentence3);
	ReleaseSentence(&m_sentence4);
	ReleaseSentence(&m_sentence5);
	ReleaseSentence(&m_sentence6);

	// Release the font shader object.
	if (m_FontShader)
	{
		m_FontShader->Shutdown();
		delete m_FontShader;
		m_FontShader = 0;
	}

	// Release the font object.
	if (m_Font)
	{
		m_Font->Shutdown();
		delete m_Font;
		m_Font = 0;
	}

	return;
}


bool TextClass::Render(ID3D11DeviceContext* deviceContext, XMMATRIX worldMatrix, XMMATRIX orthoMatrix)
{
	XMMATRIX borderMatrix;
	XMMATRIX finalWorldMatrix; // ?? 여기에 선언을 확실하게 추가했어!

	// 사방 오프셋
	float offsetsX[4] = { -1.0f, 1.0f, 0.0f, 0.0f };
	float offsetsY[4] = { 0.0f, 0.0f, 1.0f, -1.0f };

	// 테두리는 무조건 검은색
	XMFLOAT4 borderColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	// 6개의 모든 문장을 배열에 담음
	SentenceType* sentences[6] = { m_sentence1, m_sentence2, m_sentence3, m_sentence4, m_sentence5, m_sentence6 };

	for (int s = 0; s < 6; s++)
	{
		if (!sentences[s]) continue;

		// ?? 강제로 늘리던 스케일링 행렬을 완전히 빼고, 순수 원래 worldMatrix를 대입!
		finalWorldMatrix = worldMatrix;

		// 1. 테두리(외곽선) 4방향 렌더링
		for (int i = 0; i < 4; i++)
		{
			borderMatrix = finalWorldMatrix * XMMatrixTranslation(offsetsX[i], offsetsY[i], 0.0f);
			RenderSentenceWithColor(deviceContext, sentences[s], borderMatrix, orthoMatrix, borderColor);
		}

		// 2. 알맹이(본문) 원래 색상으로 렌더링
		XMFLOAT4 nativeColor = XMFLOAT4(sentences[s]->red, sentences[s]->green, sentences[s]->blue, 1.0f);
		RenderSentenceWithColor(deviceContext, sentences[s], finalWorldMatrix, orthoMatrix, nativeColor);
	}

	return true;
}

bool TextClass::InitializeSentence(SentenceType** sentence, int maxLength, ID3D11Device* device)
{
	VertexType* vertices;
	unsigned long* indices;
	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;
	HRESULT result;
	int i;


	// Create a new sentence object.
	*sentence = new SentenceType;
	if (!*sentence)
	{
		return false;
	}

	// Initialize the sentence buffers to null.
	(*sentence)->vertexBuffer = 0;
	(*sentence)->indexBuffer = 0;

	// Set the maximum length of the sentence.
	(*sentence)->maxLength = maxLength;

	// Set the number of vertices in the vertex array.
	(*sentence)->vertexCount = 6 * maxLength;

	// Set the number of indexes in the index array.
	(*sentence)->indexCount = (*sentence)->vertexCount;

	// Create the vertex array.
	vertices = new VertexType[(*sentence)->vertexCount];
	if (!vertices)
	{
		return false;
	}

	// Create the index array.
	indices = new unsigned long[(*sentence)->indexCount];
	if (!indices)
	{
		return false;
	}

	// Initialize vertex array to zeros at first.
	memset(vertices, 0, (sizeof(VertexType) * (*sentence)->vertexCount));

	// Initialize the index array.
	for (i = 0; i < (*sentence)->indexCount; i++)
	{
		indices[i] = i;
	}

	// Set up the description of the dynamic vertex buffer.
	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * (*sentence)->vertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Create the vertex buffer.
	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &(*sentence)->vertexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Set up the description of the static index buffer.
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * (*sentence)->indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the index data.
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer.
	result = device->CreateBuffer(&indexBufferDesc, &indexData, &(*sentence)->indexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Release the vertex array as it is no longer needed.
	delete[] vertices;
	vertices = 0;

	// Release the index array as it is no longer needed.
	delete[] indices;
	indices = 0;

	return true;
}


bool TextClass::UpdateSentence(SentenceType* sentence, const char* text, int positionX, int positionY,
	float red, float green, float blue, ID3D11DeviceContext* deviceContext)
{
	int numLetters;
	VertexType* vertices;
	float drawX, drawY;
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	VertexType* verticesPtr;

	// 1. 색상 저장
	sentence->red = red;
	sentence->green = green;
	sentence->blue = blue;

	numLetters = (int)strlen(text);

	if (numLetters > sentence->maxLength)
	{
		return false;
	}

	// 2. Vertex 배열 생성 및 '전체 버퍼 크기만큼' 확실하게 0으로 초기화
	// sentence->vertexCount 전체를 밀어버려야 잔상이 안 남음!
	vertices = new VertexType[sentence->vertexCount];
	if (!vertices)
	{
		return false;
	}
	memset(vertices, 0, (sizeof(VertexType) * sentence->vertexCount));

	drawX = (float)(((m_screenWidth / 2) * -1) + positionX);
	drawY = (float)((m_screenHeight / 2) - positionY);

	// 3. 폰트 빌드
	m_Font->BuildVertexArray((void*)vertices, text, drawX, drawY);

	// 4. 맵핑 및 복사
	result = deviceContext->Map(sentence->vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		return false;
	}

	verticesPtr = (VertexType*)mappedResource.pData;

	// 여기도 넉넉하게 전체 vertexCount만큼 밀어서 복사해주자
	memcpy(verticesPtr, (void*)vertices, (sizeof(VertexType) * sentence->vertexCount));

	deviceContext->Unmap(sentence->vertexBuffer, 0);

	delete[] vertices;
	vertices = 0;

	return true;
}


void TextClass::ReleaseSentence(SentenceType** sentence)
{
	if (*sentence)
	{
		// Release the sentence vertex buffer.
		if ((*sentence)->vertexBuffer)
		{
			(*sentence)->vertexBuffer->Release();
			(*sentence)->vertexBuffer = 0;
		}

		// Release the sentence index buffer.
		if ((*sentence)->indexBuffer)
		{
			(*sentence)->indexBuffer->Release();
			(*sentence)->indexBuffer = 0;
		}

		// Release the sentence.
		delete* sentence;
		*sentence = 0;
	}

	return;
}


bool TextClass::RenderSentence(ID3D11DeviceContext* deviceContext, SentenceType* sentence, XMMATRIX worldMatrix,
	XMMATRIX orthoMatrix)
{
	unsigned int stride, offset;
	XMFLOAT4 pixelColor;
	bool result;


	// Set vertex buffer stride and offset.
	stride = sizeof(VertexType);
	offset = 0;

	// Set the vertex buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetVertexBuffers(0, 1, &sentence->vertexBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetIndexBuffer(sentence->indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Create a pixel color vector with the input sentence color.
	pixelColor = XMFLOAT4(sentence->red, sentence->green, sentence->blue, 1.0f);

	// Render the text using the font shader.
	result = m_FontShader->Render(deviceContext, sentence->indexCount, worldMatrix, m_baseViewMatrix,
		orthoMatrix, m_Font->GetTexture(), pixelColor);
	if (!result)
	{
		return false;
	}

	return true;
}

bool TextClass::RenderSentenceWithColor(ID3D11DeviceContext* deviceContext, TextClass::SentenceType* sentence,
	XMMATRIX worldMatrix, XMMATRIX orthoMatrix, XMFLOAT4 customColor)
{
	unsigned int stride, offset;
	bool result;

	stride = sizeof(VertexType);
	offset = 0;

	deviceContext->IASetVertexBuffers(0, 1, &sentence->vertexBuffer, &stride, &offset);
	deviceContext->IASetIndexBuffer(sentence->indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	result = m_FontShader->Render(deviceContext, sentence->indexCount, worldMatrix, m_baseViewMatrix,
		orthoMatrix, m_Font->GetTexture(), customColor);
	if (!result)
	{
		return false;
	}

	return true;
}


bool TextClass::SetFPS(int fps, ID3D11DeviceContext* deviceContext)
{
	char tempString[16];
	char fpsString[16];
	float red, green, blue;
	bool result;


	// Truncate the fps to below 10,000.
	if (fps > 9999)
	{
		fps = 9999;
	}

	// Convert the fps integer to string format.
	_itoa_s(fps, tempString, 10);

	// Setup the fps string.
	strcpy_s(fpsString, "FPS : ");
	strcat_s(fpsString, tempString);

	// If fps is 60 or above set the fps color to green.
	if (fps >= 60)
	{
		red = 0.0f;
		green = 1.0f;
		blue = 0.0f;
	}

	// If fps is below 60 set the fps color to yellow.
	if (fps < 60)
	{
		red = 1.0f;
		green = 1.0f;
		blue = 0.0f;
	}

	// If fps is below 30 set the fps color to red.
	if (fps < 30)
	{
		red = 1.0f;
		green = 0.0f;
		blue = 0.0f;
	}

	result = UpdateSentence(m_sentence1, fpsString, 20, 20, red, green, blue, deviceContext);
	if (!result)
	{
		return false;
	}

	return true;
}

bool TextClass::SetCPU(int cpu, ID3D11DeviceContext* deviceContext)
{
	char tempString[16];
	char cpuString[16];
	bool result;


	// Convert the cpu integer to string format.
	_itoa_s(cpu, tempString, 10);

	// Setup the cpu string.
	strcpy_s(cpuString, "CPU : ");
	strcat_s(cpuString, tempString);
	strcat_s(cpuString, "%");

	result = UpdateSentence(m_sentence2, cpuString, 20, 40, 0.0f, 1.0f, 0.0f, deviceContext);
	if (!result)
	{
		return false;
	}

	return true;
}

bool TextClass::SetObjectCount(int count, ID3D11DeviceContext* deviceContext)
{
	char tempString[16];
	char outString[32];
	_itoa_s(count, tempString, 10);

	strcpy_s(outString, "Objects : ");
	strcat_s(outString, tempString);

	return UpdateSentence(m_sentence3, outString, 20, 60, 0.0f, 1.0f, 0.0f, deviceContext);
}

bool TextClass::SetPolygonCount(int count, ID3D11DeviceContext* deviceContext)
{
	char tempString[16];
	char outString[32];
	_itoa_s(count, tempString, 10);

	strcpy_s(outString, "Polygons : ");
	strcat_s(outString, tempString);

	return UpdateSentence(m_sentence4, outString, 20, 80, 0.0f, 1.0f, 0.0f, deviceContext);
}

bool TextClass::SetResolution(int width, int height, ID3D11DeviceContext* deviceContext)
{
	char wStr[16], hStr[16];
	char outString[32];
	_itoa_s(width, wStr, 10);
	_itoa_s(height, hStr, 10);

	strcpy_s(outString, "Resolution : ");
	strcat_s(outString, wStr);
	strcat_s(outString, "x");
	strcat_s(outString, hStr);

	return UpdateSentence(m_sentence5, outString, 20, 100, 0.0f, 1.0f, 0.0f, deviceContext);
}

bool TextClass::SetTimer(const char* timerStr, ID3D11DeviceContext* deviceContext)
{
	char outString[32];

	strcpy_s(outString, "TIME : ");
	strcat_s(outString, timerStr);

	// 화면 상단 중앙 배치 계산
	int posX = (m_screenWidth / 2) - 50;
	int posY = 20;

	// 조준점과 깔맞춤인 형광 초록색(0, 1, 0)으로 전송!
	return UpdateSentence(m_sentence6, outString, posX, posY, 0.0f, 1.0f, 0.0f, deviceContext);
}