#include "wallhack.h"
#include "gui.h"

// SOURCE: https://niemand.com.ar/2019/01/08/fingerprinting-models-when-hooking-directx-vermintide-2/
// SOURCE: https://niemand.com.ar/2019/01/13/creating-your-own-wallhack/

bool operator==(const propertiesModel& lhs, const propertiesModel& rhs)
{
    if (lhs.stride != rhs.stride
        || lhs.vedesc_ByteWidth != rhs.vedesc_ByteWidth
        || lhs.indesc_ByteWidth != rhs.indesc_ByteWidth
        || lhs.pscdesc_ByteWidth != rhs.pscdesc_ByteWidth)
    {
        return false;
    }
    else
    {
        return true;
    }
}

ID3D11PixelShader* pShaderRed = NULL;
ID3D11PixelShader* pShaderBlue = NULL;
float redColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
float blueColor[4] = { 0.0f, 0.0f, 1.0f, 1.0f };

UINT veStartSlot;
UINT veNumBuffers;

ID3D11Buffer* veBuffer;
UINT Stride;
UINT veBufferOffset;
D3D11_BUFFER_DESC vedesc;

ID3D11Buffer* inBuffer;
DXGI_FORMAT inFormat;
UINT inOffset;
D3D11_BUFFER_DESC indesc;

UINT pscStartSlot;
UINT pscNumBuffers;
ID3D11Buffer* pscBuffer;
D3D11_BUFFER_DESC pscdesc;

struct propertiesModel currentParams;
std::unordered_set<propertiesModel> seenParams;
std::unordered_set<propertiesModel> wallhackParams;
int currentParamPosition = 1;
std::mutex g_propertiesModels;

ID3D11DepthStencilState* m_DepthStencilState;
ID3D11DepthStencilState* m_DepthStencilStateFalse;
ID3D11DepthStencilState* m_origDepthStencilState;
UINT pStencilRef;

// Own contribution
void UpdateRedShader(float r, float g, float b, float a)
{
    if (pShaderRed) pShaderRed->Release();
    gui::GenerateShader(&pShaderRed, r, g, b);
}
// Own contribution
void UpdateBlueShader(float r, float g, float b, float a)
{
    if (pShaderBlue) pShaderBlue->Release();
    gui::GenerateShader(&pShaderBlue, r, g, b);
}