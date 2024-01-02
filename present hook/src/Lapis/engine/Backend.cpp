#include "Backend.h"

#include <iostream>

#include "../../global.h"

#include "GlobalDefines.h"

#include "shaders/DefaultShaders.h"
#include "LapisEngine.h"
#include "../utils/hsl-to-rgb.hpp"

namespace Lapis
{
	namespace Backend
	{
        ///////////////////////
        /*    EXTERN VARS    */

        std::chrono::steady_clock::duration elapsedDuration{};
        std::chrono::steady_clock::duration deltaDuration{};
        std::chrono::steady_clock::duration initDuration{};
        std::unordered_map<std::string, std::shared_ptr<InternalMaterial>> builtinMaterials{};

        ///////////////////////
        /*  BACKEND GLOBALS  */

        HWND hwnd;
        Vec4 clientRect;

		//IDXGISwapChain* swapchain;
        IDXGIFactory* factory;
		ID3D11Device* device;
		ID3D11DeviceContext* deviceContext;
		ID3D11RenderTargetView* renderTargetView;
		ID3D11DepthStencilView* depthStencilView;
		ID3D11InputLayout* inputLayout;
		ID3D11Buffer* constantBuffer;
		ID3D11Buffer* vertexBuffer;
		ID3D11BlendState* blendState;
        ID3D11RasterizerState* rasterizerState;
        ID3D11DepthStencilState* depthStencilState;

		GlobalConstantBuffer gcb;

		int VertexCount = 0;
		int VertexVectorCapacity = 1000;
        std::vector<Vertex> LapisVertexVector{};
        std::vector<InternalLapisCommand> LapisCommandVector{};

        void InitBackendInternal(IDXGISwapChain* _swapchain)
        {
            DXGI_SWAP_CHAIN_DESC swapChainDesc;
            _swapchain->GetDesc(&swapChainDesc);
            hwnd = swapChainDesc.OutputWindow; // the g_hwnd
            std::cout << "Lapis Init got hwnd : " << hwnd << "\n";
            

            RECT rect;
            GetClientRect(hwnd, &rect);
            clientRect = rect;

            initDuration = std::chrono::high_resolution_clock::now().time_since_epoch();
            InitD3D11Internal(_swapchain);
        }
        void InitD3D11Internal(IDXGISwapChain* _swapchain)
        {
            //Grab Device, DeviceContext, Factory and Create RenderTargetView
            {
                ID3D11Device* _device = nullptr;
                ID3D11DeviceContext* _deviceContext = nullptr;
                if (FAILED(GetDeviceAndCtxFromSwapchain(_swapchain, &_device, &_deviceContext)))
                    return;


                // Get factory from device, set global backend vars, and add ref counts
                IDXGIDevice* pDXGIDevice = nullptr;
                IDXGIAdapter* pDXGIAdapter = nullptr;
                IDXGIFactory* pFactory = nullptr;

                if (_device->QueryInterface(IID_PPV_ARGS(&pDXGIDevice)) == S_OK)
                    if (pDXGIDevice->GetParent(IID_PPV_ARGS(&pDXGIAdapter)) == S_OK)
                        if (pDXGIAdapter->GetParent(IID_PPV_ARGS(&pFactory)) == S_OK) {
                            device = _device;
                            deviceContext = _deviceContext;
                            factory = pFactory;
                        }
                if (pDXGIDevice) pDXGIDevice->Release();
                if (pDXGIAdapter) pDXGIAdapter->Release();
                device->AddRef();
                deviceContext->AddRef();
                

                // Create RenderTargetView
                ID3D11Texture2D* backBuffer = nullptr;
                if (FAILED(_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer)))
                    return;
                device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
                backBuffer->Release();

                // Create DepthStencilView
                D3D11_TEXTURE2D_DESC depthBufferDesc;
                backBuffer->GetDesc(&depthBufferDesc); // copy from framebuffer properties
                depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
                depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
                ID3D11Texture2D* depthBuffer;
                device->CreateTexture2D(&depthBufferDesc, nullptr, &depthBuffer);
                device->CreateDepthStencilView(depthBuffer, nullptr, &depthStencilView);

                
            }

            // Create Shaders
            {
                InitDefaultShaders();
            }

            // Create InputLayout
            {
                D3D11_INPUT_ELEMENT_DESC ied[] =
                {
                    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(Vertex, pos),    D3D11_INPUT_PER_VERTEX_DATA, 0},
                    {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, color),  D3D11_INPUT_PER_VERTEX_DATA, 0},
                    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, offsetof(Vertex, uv),     D3D11_INPUT_PER_VERTEX_DATA, 0},
                    {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(Vertex, normal), D3D11_INPUT_PER_VERTEX_DATA, 0},
                };
                device->CreateInputLayout(ied, sizeof(ied) / sizeof(ied[0]), DEFAULTSHADER__UI_vertex, sizeof(DEFAULTSHADER__UI_vertex), &inputLayout);
            }

            // Create Constant Buffer
            {
                D3D11_BUFFER_DESC cbDesc{};
                cbDesc.ByteWidth = sizeof(GlobalConstantBuffer);
                cbDesc.Usage = D3D11_USAGE_DYNAMIC;
                cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                cbDesc.MiscFlags = 0;
                cbDesc.StructureByteStride = 0;
                device->CreateBuffer(&cbDesc, NULL, &constantBuffer);
            }

            // Create the blending setup
            {
                D3D11_BLEND_DESC desc;
                ZeroMemory(&desc, sizeof(desc));
                desc.AlphaToCoverageEnable = false;
                desc.RenderTarget[0].BlendEnable = true;
                desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
                desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
                desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
                desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
                desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
                desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
                desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
                device->CreateBlendState(&desc, &blendState);
            }

            // Create the rasterizer state
            {
                D3D11_RASTERIZER_DESC desc;
                ZeroMemory(&desc, sizeof(desc));
                desc.FillMode = D3D11_FILL_SOLID;
                desc.CullMode = D3D11_CULL_NONE;
                desc.ScissorEnable = true;
                desc.DepthClipEnable = true;
                device->CreateRasterizerState(&desc, &rasterizerState);
            }

            // Create depth-stencil State
            {
                D3D11_DEPTH_STENCIL_DESC desc;
                ZeroMemory(&desc, sizeof(desc));
                desc.DepthEnable = false;
                desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
                desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
                desc.StencilEnable = false;
                desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
                desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
                desc.BackFace = desc.FrontFace;
                device->CreateDepthStencilState(&desc, &depthStencilState);
            }
        }
        void SetupD3D11State()
        {
            deviceContext->OMSetRenderTargets(1, &renderTargetView, depthStencilView);

            // Setup viewport
            D3D11_VIEWPORT vp{};
            vp.Width = clientRect.width;
            vp.Height = clientRect.height;
            vp.MinDepth = 0.0f;
            vp.MaxDepth = 1.0f;
            vp.TopLeftX = vp.TopLeftY = 0;
            deviceContext->RSSetViewports(1, &vp);

            // Setup shader and vertex buffers
            unsigned int stride = sizeof(Vertex);
            unsigned int offset = 0;
            deviceContext->IASetInputLayout(inputLayout);
            deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
            //deviceContext->IASetIndexBuffer(bd->pIB, sizeof(Index) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
            deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            deviceContext->VSSetShader(builtinMaterials["UI"]->vertexShader, nullptr, 0);
            deviceContext->VSSetConstantBuffers(0, 1, &constantBuffer);
            deviceContext->PSSetShader(builtinMaterials["UI"]->pixelShader, nullptr, 0);
            deviceContext->PSSetConstantBuffers(0, 1, &constantBuffer);
            //deviceContext->PSSetSamplers(0, 1, &bd->pFontSampler);
            deviceContext->GSSetShader(nullptr, nullptr, 0);
            deviceContext->HSSetShader(nullptr, nullptr, 0); // In theory we should backup and restore this as well.. very infrequently used..
            deviceContext->DSSetShader(nullptr, nullptr, 0); // In theory we should backup and restore this as well.. very infrequently used..
            deviceContext->CSSetShader(nullptr, nullptr, 0); // In theory we should backup and restore this as well.. very infrequently used..

            // Setup blend state
            const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
            deviceContext->OMSetBlendState(blendState, blend_factor, 0xffffffff);
            deviceContext->OMSetDepthStencilState(depthStencilState, 0);
            deviceContext->RSSetState(rasterizerState);
        }
        void CleanD3D11()
        {
            safe_release(renderTargetView);
            safe_release(depthStencilView);

            safe_release(factory);
            safe_release(device);
            safe_release(deviceContext);

            builtinMaterials.clear();

            safe_release(inputLayout);
            safe_release(constantBuffer);
            safe_release(blendState);
            safe_release(rasterizerState);
            safe_release(depthStencilState);

            safe_release(vertexBuffer);
        }

        void DestroyViews()
        {
            deviceContext->OMSetRenderTargets(0, 0, 0);
            safe_release(renderTargetView);
            safe_release(depthStencilView);
            renderTargetView = nullptr;
            depthStencilView = nullptr;
        }
        void CreateViews(IDXGISwapChain* _swapchain)
        {
            ID3D11Texture2D* pBuffer;
            _swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBuffer);
            if (pBuffer) {
                device->CreateRenderTargetView(pBuffer, NULL, &renderTargetView);
            
                D3D11_TEXTURE2D_DESC depthBufferDesc;
                pBuffer->GetDesc(&depthBufferDesc); // copy from framebuffer properties
                depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
                depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
                ID3D11Texture2D* depthBuffer;
                device->CreateTexture2D(&depthBufferDesc, nullptr, &depthBuffer);
                if (depthBuffer)
                    device->CreateDepthStencilView(depthBuffer, nullptr, &depthStencilView);

                safe_release(depthBuffer);
            }
            safe_release(pBuffer);
        }


        void WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            std::cout << "Lapis::WndProcHandler()\n";

            switch (msg) {
            case WM_SIZE:
                clientRect.width = LOWORD(lParam);
                clientRect.height = HIWORD(lParam);
                std::cout << "resized clientRect: w" << clientRect.width << " h" << clientRect.height << "\n";
                break;
            default:
                break;
            }
        }


        void NewFrame()
        {
            static auto old = initDuration;

            auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
            deltaDuration = (now - old);
            elapsedDuration = (now - initDuration);
            old = now;

            using s = std::chrono::duration<float>;
            Lapis::deltaTime = std::chrono::duration_cast<s>(deltaDuration).count();// / 1000 / 1000;
            Lapis::elapsedTime += Lapis::deltaTime;
        }
        void RenderFrame()
        {
            if (doDebugPrint) std::cout << "\nLapis::RenderFrame()\n";
            static int VBufferSize = 0;
            if (VertexVectorCapacity > VBufferSize) {

                if (vertexBuffer)
                    vertexBuffer->Release();

                std::cout << "resizing back buffer\n";
                D3D11_BUFFER_DESC bd = {};

                bd.Usage = D3D11_USAGE_DYNAMIC;
                bd.ByteWidth = VertexVectorCapacity * sizeof(Vertex);
                bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;


                device->CreateBuffer(&bd, NULL, &vertexBuffer);       // create the buffer
                if (!vertexBuffer)
                    return;

                VBufferSize = VertexVectorCapacity;
            }

            RemapSubResource(vertexBuffer, LapisVertexVector.data(), sizeof(Vertex) * LapisVertexVector.size());

            if (doDebugPrint) std::cout << "backup dx\n";
            // Backup DX state that will be modified to restore it afterwards (unfortunately this is very ugly looking and verbose. Close your eyes!)
            struct BACKUP_DX11_STATE
            {
                UINT                        ScissorRectsCount, ViewportsCount;
                D3D11_RECT                  ScissorRects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
                D3D11_VIEWPORT              Viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
                ID3D11RasterizerState* RS;
                ID3D11BlendState* BlendState;
                FLOAT                       BlendFactor[4];
                UINT                        SampleMask;
                UINT                        StencilRef;
                ID3D11DepthStencilState* DepthStencilState;
                ID3D11ShaderResourceView* PSShaderResource;
                ID3D11SamplerState* PSSampler;
                ID3D11PixelShader* PS;
                ID3D11VertexShader* VS;
                ID3D11GeometryShader* GS;
                UINT                        PSInstancesCount, VSInstancesCount, GSInstancesCount;
                ID3D11ClassInstance* PSInstances[256], * VSInstances[256], * GSInstances[256];   // 256 is max according to PSSetShader documentation
                D3D11_PRIMITIVE_TOPOLOGY    PrimitiveTopology;
                ID3D11Buffer* IndexBuffer, * VertexBuffer, * VSConstantBuffer;
                UINT                        IndexBufferOffset, VertexBufferStride, VertexBufferOffset;
                DXGI_FORMAT                 IndexBufferFormat;
                ID3D11InputLayout* InputLayout;
            };
            BACKUP_DX11_STATE old = {};
            {
                old.ScissorRectsCount = old.ViewportsCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
                deviceContext->RSGetScissorRects(&old.ScissorRectsCount, old.ScissorRects);
                deviceContext->RSGetViewports(&old.ViewportsCount, old.Viewports);
                deviceContext->RSGetState(&old.RS);
                deviceContext->OMGetBlendState(&old.BlendState, old.BlendFactor, &old.SampleMask);
                deviceContext->OMGetDepthStencilState(&old.DepthStencilState, &old.StencilRef);
                deviceContext->PSGetShaderResources(0, 1, &old.PSShaderResource);
                deviceContext->PSGetSamplers(0, 1, &old.PSSampler);
                old.PSInstancesCount = old.VSInstancesCount = old.GSInstancesCount = 256;
                deviceContext->PSGetShader(&old.PS, old.PSInstances, &old.PSInstancesCount);
                deviceContext->VSGetShader(&old.VS, old.VSInstances, &old.VSInstancesCount);
                deviceContext->VSGetConstantBuffers(0, 1, &old.VSConstantBuffer);
                deviceContext->GSGetShader(&old.GS, old.GSInstances, &old.GSInstancesCount);

                deviceContext->IAGetPrimitiveTopology(&old.PrimitiveTopology);
                deviceContext->IAGetIndexBuffer(&old.IndexBuffer, &old.IndexBufferFormat, &old.IndexBufferOffset);
                deviceContext->IAGetVertexBuffers(0, 1, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset);
                deviceContext->IAGetInputLayout(&old.InputLayout);
            }


            SetupD3D11State();

            UpdateGlobalConstantBuffer();

            const D3D11_RECT r = { (LONG)clientRect.x, (LONG)clientRect.y, (LONG)clientRect.width, (LONG)clientRect.height };
            deviceContext->RSSetScissorRects(1, &r);
            debugPrint(clientRect.x);
            debugPrint(clientRect.y);
            debugPrint(clientRect.width);
            debugPrint(clientRect.height);



            for (auto& internalCommand : LapisCommandVector) {
                
                if (doDebugPrint) std::cout << "DrawCommand\n";
                DrawCommand(internalCommand);
                
            }
            
            if (doDebugPrint) std::cout << "restore dx\n";
            // Restore modified DX state
            {
                deviceContext->RSSetScissorRects(old.ScissorRectsCount, old.ScissorRects);
                deviceContext->RSSetViewports(old.ViewportsCount, old.Viewports);
                deviceContext->RSSetState(old.RS); if (old.RS) old.RS->Release();
                deviceContext->OMSetBlendState(old.BlendState, old.BlendFactor, old.SampleMask); if (old.BlendState) old.BlendState->Release();
                deviceContext->OMSetDepthStencilState(old.DepthStencilState, old.StencilRef); if (old.DepthStencilState) old.DepthStencilState->Release();
                deviceContext->PSSetShaderResources(0, 1, &old.PSShaderResource); if (old.PSShaderResource) old.PSShaderResource->Release();
                deviceContext->PSSetSamplers(0, 1, &old.PSSampler); if (old.PSSampler) old.PSSampler->Release();
                deviceContext->PSSetShader(old.PS, old.PSInstances, old.PSInstancesCount); if (old.PS) old.PS->Release();
                for (UINT i = 0; i < old.PSInstancesCount; i++) if (old.PSInstances[i]) old.PSInstances[i]->Release();
                deviceContext->VSSetShader(old.VS, old.VSInstances, old.VSInstancesCount); if (old.VS) old.VS->Release();
                deviceContext->VSSetConstantBuffers(0, 1, &old.VSConstantBuffer); if (old.VSConstantBuffer) old.VSConstantBuffer->Release();
                deviceContext->GSSetShader(old.GS, old.GSInstances, old.GSInstancesCount); if (old.GS) old.GS->Release();
                for (UINT i = 0; i < old.VSInstancesCount; i++) if (old.VSInstances[i]) old.VSInstances[i]->Release();
                deviceContext->IASetPrimitiveTopology(old.PrimitiveTopology);
                deviceContext->IASetIndexBuffer(old.IndexBuffer, old.IndexBufferFormat, old.IndexBufferOffset); if (old.IndexBuffer) old.IndexBuffer->Release();
                deviceContext->IASetVertexBuffers(0, 1, &old.VertexBuffer, &old.VertexBufferStride, &old.VertexBufferOffset); if (old.VertexBuffer) old.VertexBuffer->Release();
                deviceContext->IASetInputLayout(old.InputLayout); if (old.InputLayout) old.InputLayout->Release();
            }
        }
        void FlushFrame()
        {
            LapisVertexVector.clear();
            VertexCount = 0;

            LapisCommandVector.clear();
        }

        void BeginCommand(Topology topology, std::string materialName)
        {
            UNIMPLEMENTED(BeginCommand);
        }
        void EndCommand()
        {
            UNIMPLEMENTED(EndCommand);
        }

        void PushVertex(Vertex vert)
        {
            if (VertexCount + 1 > VertexVectorCapacity) {
                VertexVectorCapacity += 1000;
                LapisVertexVector.reserve(VertexVectorCapacity);
            }

            LapisVertexVector.push_back(vert);
            VertexCount += 1;
        }
        void PushCommand(LapisCommand lapisCommand)
        {
            LapisCommandVector.push_back(InternalLapisCommand(lapisCommand, VertexCount));
        }

        void UpdateGlobalConstantBuffer()
        {
            gcb.elapsedTime = elapsedTime;
            gcb.deltaTime = deltaTime;

            float L = 0, T = 0, R = clientRect.width, B = clientRect.height;
            DirectX::XMMATRIX m = {
                 2.0f / (R - L),   0.0f,           0.0f,       0.0f ,
                 0.0f,             2.0f / (T - B), 0.0f,       0.0f ,
                 0.0f,             0.0f,           0.5f,       0.0f ,
                 (R + L) / (L - R),(T + B) / (B - T),    0.5f,       1.0f
            };
            auto screen = m;

            auto dxscreen = DirectX::XMMatrixOrthographicLH(SCREEN_WIDTH, SCREEN_HEIGHT, -10, 1000);



            auto world = DirectX::XMMatrixIdentity();
#if USE_Z_UP
            world = world * Lapis::Helpers::XMMatrixRotationAxis(Vec3::right, 90 * DEG2RAD);
            world = world * DirectX::XMMatrixScaling(1, 1, -1);
#endif

            DirectX::XMVECTOR Eye = Helpers::XMVectorSet(0);
            DirectX::XMVECTOR At = Helpers::XMVectorSet(Vec3::forward);
            DirectX::XMVECTOR Up = Helpers::XMVectorSet(Vec3::up);
            auto view = DirectX::XMMatrixLookAtLH(Eye, At, Up);
            auto translateView = Helpers::XMMatrixTranslation(mainCamera.pos);
            auto rotateView = Helpers::XMMatrixRotationRollPitchYaw(-mainCamera.rotation);
            auto scaleView = Helpers::XMMatrixScaling(mainCamera.scale);
            view = view * translateView * rotateView;

            auto projection = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2, SCREEN_WIDTH / (FLOAT)SCREEN_HEIGHT, 0.01f, 100.0f);

            gcb.Screen = screen;
            gcb.World = DirectX::XMMatrixTranspose(world);
            gcb.View = DirectX::XMMatrixTranspose(view);
            gcb.Projection = DirectX::XMMatrixTranspose(projection);

            RemapSubResource(constantBuffer, &gcb, sizeof(gcb));

        }
        void DrawCommand(InternalLapisCommand internalLapisCommand)
        {
            auto model = DirectX::XMMatrixIdentity();
            auto scaleModel = Helpers::XMMatrixScaling(internalLapisCommand.transform.scale);
            auto rotateModel = Helpers::XMMatrixRotationRollPitchYaw(internalLapisCommand.transform.rot);
            auto translateModel = Helpers::XMMatrixTranslation(internalLapisCommand.transform.pos);

            model = model * scaleModel * rotateModel * translateModel;
            gcb.Model = DirectX::XMMatrixTranspose(model);

            RemapSubResource(constantBuffer, &gcb, sizeof(gcb));

            auto& material = internalLapisCommand.material;
            debugPrint(material->name);

            static ID3D11VertexShader* prevVertexShader = nullptr;
            if (prevVertexShader != material->vertexShader) {
                debugPrint(material->vertexShader);
                deviceContext->VSSetShader(material->vertexShader, 0, 0);
                prevVertexShader = material->vertexShader;
            }
            static ID3D11PixelShader* prevPixelShader = nullptr;
            if (prevPixelShader != material->pixelShader) {
                debugPrint(material->pixelShader);
                deviceContext->PSSetShader(material->pixelShader, 0, 0);
                prevPixelShader = material->pixelShader;
            }

            deviceContext->IASetPrimitiveTopology(internalLapisCommand.topology);
            debugPrint(internalLapisCommand.vertexCount);
            debugPrint(internalLapisCommand.startVertexLocation);
            deviceContext->Draw(internalLapisCommand.vertexCount, internalLapisCommand.startVertexLocation);
        }
        void InitDefaultShaders()
        {
#define CREATE_DEFAULT_SHADERS_SEPERATE(vsName, psName) \
        ID3D11VertexShader* vsName##_vertex; \
        device->CreateVertexShader(DEFAULTSHADER__##vsName##_vertex, sizeof(DEFAULTSHADER__##vsName##_vertex), NULL, &vsName##_vertex); \
        ID3D11PixelShader* psName##_pixel; \
        device->CreatePixelShader(DEFAULTSHADER__##psName##_pixel, sizeof(DEFAULTSHADER__##psName##_pixel), NULL, &psName##_pixel) \

#define CREATE_DEFAULT_SHADERS(name) CREATE_DEFAULT_SHADERS_SEPERATE(name, name);

#define CREATE_DEFAULT_MATERIAL_SEPERATE_SHADERS(name, vsName, psName) \
    { \
        CREATE_DEFAULT_SHADERS_SEPERATE(vsName, psName); \
            builtinMaterials.insert({ \
                    #name,std::make_unique<InternalMaterial>(#name, vsName##_vertex, psName##_pixel, nullptr)\
                }); \
    }

#define CREATE_DEFAULT_MATERIAL(name) CREATE_DEFAULT_MATERIAL_SEPERATE_SHADERS(name, name, name)


            CREATE_DEFAULT_MATERIAL(UI);
            CREATE_DEFAULT_MATERIAL_SEPERATE_SHADERS(UNLIT3D, UNLIT3D, UNLIT);
            CREATE_DEFAULT_MATERIAL_SEPERATE_SHADERS(CIRCLE, UI, CIRCLE);

#undef CREATE_DEFAULT_SHADER
        }
        void RemapSubResource(ID3D11Resource* resource, void* data, size_t size)
        {
            D3D11_MAPPED_SUBRESOURCE ms;
            deviceContext->Map(resource, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
            memcpy(ms.pData, data, size);
            deviceContext->Unmap(resource, NULL);
        }

        HRESULT GetDeviceAndCtxFromSwapchain(IDXGISwapChain* pSwapChain, ID3D11Device** ppDevice, ID3D11DeviceContext** ppContext)
        {
            HRESULT ret = pSwapChain->GetDevice(__uuidof(ID3D11Device), (PVOID*)ppDevice);

            if (SUCCEEDED(ret))
                (*ppDevice)->GetImmediateContext(ppContext);

            return ret;
        }
	}
}