#include "framework.h"
#include <d2d1.h>


#include "Config.h"
#include "MathFunc.h"
#include "DXGIh.h"
#include "DX12.h"
#include "Vertex.h"


#pragma comment(lib,"D2d1.lib")

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


ComPtr<ID2D1HwndRenderTarget> target;
Microsoft::WRL::ComPtr<ID2D1Factory> factory;

namespace GDITools {
	ComPtr<ID2D1SolidColorBrush> brush;
}

void InitApplication(HWND hWnd) {
	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, factory.GetAddressOf());
	D2D1_RENDER_TARGET_PROPERTIES prop;
	//UINT dpi = GetDpiForWindow(hWnd);
	prop.dpiX = 96;
	prop.dpiY = 96;
	prop.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
	prop.pixelFormat = { DXGI_FORMAT_B8G8R8A8_UNORM ,D2D1_ALPHA_MODE_PREMULTIPLIED };
	prop.usage = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;
	prop.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
	D2D1_HWND_RENDER_TARGET_PROPERTIES wndProp;
	wndProp.hwnd = hWnd;
	RECT r;
	GetClientRect(hWnd, &r);
	wndProp.pixelSize = { (unsigned)r.right,(unsigned)r.bottom };
	wndProp.presentOptions = D2D1_PRESENT_OPTIONS_RETAIN_CONTENTS;
	if (factory->
		CreateHwndRenderTarget(&prop, &wndProp, target.GetAddressOf()) != S_OK) {
		
	}

}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	
	try {
		D3D12_NS::EnableDebug();
		D3D12_NS::InitApplication();
		D3D12_NS::GPUAdapter adapter{};
		GINS::WindowResource mainWindow(&adapter, 800, 600, GINS::TestDrawFunction);

		GINS::InitTest(&adapter,&mainWindow);
		//int* data = new int[1000];
		//D3D12_VERTEX_BUFFER_VIEW bv;
		//MemoryNS::CreateDefaultBuffer(adapter.GetDevice(), adapter.GetCommandList(), 1000 * sizeof(int),sizeof(int), data,&bv);
		D3D12_NS::StartMainLoop({ &mainWindow});
	}
	catch (std::exception e) {

	}
	
	/*
	WNDCLASSEX wc;
	wc.cbClsExtra = 0;
	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hIconSm = LoadIconW(NULL, IDI_APPLICATION);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = L"Class";
	wc.hInstance = hInstance;
	wc.lpfnWndProc = WndProc;
	RegisterClassEx(&wc);
	HWND hMain = CreateWindow(L"Class", L"Window", WS_OVERLAPPEDWINDOW, 0, 0, 400, 400, NULL, NULL, hInstance, 0);
	ShowWindow(hMain,SW_SHOW);
	
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	*/
	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	PAINTSTRUCT ps;
	HDC hdc;
	static int x = 200, y = 200;
	switch (uMsg) {
	case WM_CREATE:
		InitApplication(hWnd);
		break;
	case WM_PAINT:
		BeginPaint(hWnd, &ps);
		target->BeginDraw();
		target->CreateSolidColorBrush({ (float)(rand()%10000)/10000,(float)(rand() % 10000) / 10000,0.8, (float)(rand() % 10000) / 10000 }, GDITools::brush.GetAddressOf());
		target->FillRectangle({ 0,0,100,100 }, GDITools::brush.Get());
		target->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
		target->DrawLine({ 100,100 }, { (float)x,(float)y }, GDITools::brush.Get());
		target->EndDraw();
		EndPaint(hWnd, &ps);
		GDITools::brush.ReleaseAndGetAddressOf();
		break;

	case WM_MOUSEMOVE:
		x = LOWORD(lParam);
		y = HIWORD(lParam);
		InvalidateRect(hWnd, NULL, FALSE);
		break;

	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}