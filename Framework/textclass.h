////////////////////////////////////////////////////////////////////////////////
// Filename: textclass.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _TEXTCLASS_H_
#define _TEXTCLASS_H_

///////////////////////
// MY CLASS INCLUDES //
///////////////////////
#include "fontclass.h"
#include "fontshaderclass.h"
#include "AlignedAllocationPolicy.h"

////////////////////////////////////////////////////////////////////////////////
// Class name: TextClass
////////////////////////////////////////////////////////////////////////////////
class TextClass : public AlignedAllocationPolicy<16>
{
private:
	struct SentenceType
	{
		ID3D11Buffer *vertexBuffer, *indexBuffer;
		int vertexCount, indexCount, maxLength;
		float red, green, blue;
	};

	struct VertexType
	{
		XMFLOAT3 position;
	    XMFLOAT2 texture;
	};

public:
	TextClass();
	TextClass(const TextClass&);
	~TextClass();

	bool Initialize(ID3D11Device*, ID3D11DeviceContext*, HWND, int, int, XMMATRIX);
	void Shutdown();
	bool Render(ID3D11DeviceContext*, XMMATRIX, XMMATRIX);
	bool SetFPS(int, ID3D11DeviceContext*);
	bool SetCPU(int, ID3D11DeviceContext*);
	bool SetObjectCount(int count, ID3D11DeviceContext* deviceContext);
	bool SetPolygonCount(int count, ID3D11DeviceContext* deviceContext);
	bool SetResolution(int width, int height, ID3D11DeviceContext* deviceContext);
	bool SetTimer(const char* timerStr, ID3D11DeviceContext* deviceContext);

	int m_screenWidth, m_screenHeight;

private:
	bool InitializeSentence(SentenceType**, int, ID3D11Device*);
	bool UpdateSentence(SentenceType*, const char*, int, int, float, float, float, ID3D11DeviceContext*);
	void ReleaseSentence(SentenceType**);
	bool RenderSentence(ID3D11DeviceContext*, SentenceType*, XMMATRIX, XMMATRIX);
	bool RenderSentenceWithColor(ID3D11DeviceContext* deviceContext, TextClass::SentenceType* sentence,
		XMMATRIX worldMatrix, XMMATRIX orthoMatrix, XMFLOAT4 customColor);

private:
	FontClass* m_Font;
	FontShaderClass* m_FontShader;
	
	XMMATRIX m_baseViewMatrix;

	SentenceType* m_sentence1; // FPS
	SentenceType* m_sentence2; // CPU
	SentenceType* m_sentence3; // Object Count
	SentenceType* m_sentence4; // Polygon Count
	SentenceType* m_sentence5; // Resolution
	SentenceType* m_sentence6;
};

#endif