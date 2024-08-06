#pragma once
#include <cstddef>
#include <unordered_set>
#include <mutex>
#include <d3d11.h>
#ifndef WALLHACK_H
#define WALLHACK_H

// TODO: später sachen löschen, die nicht mehr gebraucht werden

// Model Structures
struct propertiesModel
{
    UINT stride;
    UINT vedesc_ByteWidth;
    UINT indesc_ByteWidth;
    UINT pscdesc_ByteWidth;

    friend bool operator==(const propertiesModel& lhs, const propertiesModel& rhs);
};

bool operator==(const propertiesModel& lhs, const propertiesModel& rhs);

namespace std {
    template<> struct hash<propertiesModel>
    {
        std::size_t operator()(const propertiesModel& obj) const noexcept
        {
            std::size_t h1 = std::hash<int>{}(obj.stride);
            std::size_t h2 = std::hash<int>{}(obj.vedesc_ByteWidth);
            std::size_t h3 = std::hash<int>{}(obj.indesc_ByteWidth);
            std::size_t h4 = std::hash<int>{}(obj.pscdesc_ByteWidth);
            return (h1 + h3 + h4) ^ (h2 << 1);
        }
    };
}

// Texture for the shader
extern ID3D11PixelShader* pShaderRed;
extern ID3D11PixelShader* pShaderBlue;
// Texture color for the shader imgui
extern float redColor[4];
extern float blueColor[4];

// vertex
extern UINT veStartSlot;
extern UINT veNumBuffers;
extern ID3D11Buffer* veBuffer;
extern UINT Stride;
extern UINT veBufferOffset;
extern D3D11_BUFFER_DESC vedesc;

// index
extern ID3D11Buffer* inBuffer;
extern DXGI_FORMAT inFormat;
extern UINT inOffset;
extern D3D11_BUFFER_DESC indesc;

// psgetConstantbuffers
extern UINT pscStartSlot;
extern UINT pscNumBuffers;
extern ID3D11Buffer* pscBuffer;
extern D3D11_BUFFER_DESC pscdesc;

extern struct propertiesModel currentParams;
extern std::unordered_set<propertiesModel> seenParams;
extern std::unordered_set<propertiesModel> wallhackParams;
extern int currentParamPosition;
extern std::mutex g_propertiesModels;

// Z-Buffering variables
extern ID3D11DepthStencilState* m_DepthStencilState;
extern ID3D11DepthStencilState* m_DepthStencilStateFalse;
extern ID3D11DepthStencilState* m_origDepthStencilState;
extern UINT pStencilRef;

extern void UpdateRedShader(float r, float g, float b, float a);
extern void UpdateBlueShader(float r, float g, float b, float a);

#endif