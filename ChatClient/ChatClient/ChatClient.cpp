// ChatClient.cpp
#include "stdafx.h"
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#define WM_SOCKET_NOTIFY (WM_USER + 100)

// 全局变量
SOCKET g_clientSocket = INVALID_SOCKET;
HWND g_hWnd = NULL;
char g_userName[64] = "";  // 存储用户名

// 函数声明
void AddLogMessage(HWND hWnd, const char* msg);
BOOL ConnectToServer(HWND hWnd, const char* ip, int port);
void Disconnect();
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// 入口函数
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // 注册窗口类
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"ChatClientClass";
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassEx(&wcex);

    // 创建窗口
    HWND hWnd = CreateWindow(L"ChatClientClass", L"聊天客户端", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, 580, 400, NULL, NULL, hInstance, NULL);
    if (!hWnd) return FALSE;

    g_hWnd = hWnd;
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

// 添加日志到列表框
void AddLogMessage(HWND hWnd, const char* msg)
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    char timeBuf[64];
    sprintf(timeBuf, "[%02d:%02d:%02d] ", st.wHour, st.wMinute, st.wSecond);
    char fullMsg[512];
    sprintf(fullMsg, "%s%s", timeBuf, msg);
    WCHAR wfullMsg[512];
    MultiByteToWideChar(CP_ACP, 0, fullMsg, -1, wfullMsg, 512);
    SendMessage(GetDlgItem(hWnd, 3), LB_ADDSTRING, 0, (LPARAM)wfullMsg);
    SendMessage(GetDlgItem(hWnd, 3), LB_SETCURSEL,
        SendMessage(GetDlgItem(hWnd, 3), LB_GETCOUNT, 0, 0) - 1, 0);
}

// 连接服务器
BOOL ConnectToServer(HWND hWnd, const char* ip, int port)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        AddLogMessage(hWnd, "Winsock初始化失败");
        return FALSE;
    }

    g_clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (g_clientSocket == INVALID_SOCKET) {
        AddLogMessage(hWnd, "创建Socket失败");
        WSACleanup();
        return FALSE;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip);

    // 设置为异步模式
    WSAAsyncSelect(g_clientSocket, hWnd, WM_SOCKET_NOTIFY, FD_CONNECT | FD_READ | FD_CLOSE);

    // 发起连接
    if (connect(g_clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK) {
            AddLogMessage(hWnd, "连接失败");
            closesocket(g_clientSocket);
            g_clientSocket = INVALID_SOCKET;
            WSACleanup();
            return FALSE;
        }
    }

    AddLogMessage(hWnd, "正在连接服务器...");
    return TRUE;
}

// 断开连接
void Disconnect()
{
    if (g_clientSocket != INVALID_SOCKET) {
        closesocket(g_clientSocket);
        g_clientSocket = INVALID_SOCKET;
    }
    WSACleanup();
}

// 窗口过程函数
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
		// 用户名标签
		CreateWindow(L"STATIC", L"用户名:", WS_CHILD | WS_VISIBLE, 
			10, 10, 50, 25, hWnd, NULL, NULL, NULL);

		// 用户名输入框（ID=5）
		CreateWindow(L"EDIT", L"用户", WS_CHILD | WS_VISIBLE | WS_BORDER, 
			65, 10, 80, 25, hWnd, (HMENU)5, NULL, NULL);
    
		// 服务器IP
		CreateWindow(L"STATIC", L"服务器IP:", WS_CHILD | WS_VISIBLE, 
			155, 10, 70, 25, hWnd, NULL, NULL, NULL);

		// IP输入框（ID=6）
		CreateWindow(L"EDIT", L"127.0.0.1", WS_CHILD | WS_VISIBLE | WS_BORDER, 
			225, 10, 100, 25, hWnd, (HMENU)6, NULL, NULL);
    
		// 端口
		CreateWindow(L"STATIC", L"端口:", WS_CHILD | WS_VISIBLE, 
			335, 10, 40, 25, hWnd, NULL, NULL, NULL);

		// 端口输入框（ID=7）
		CreateWindow(L"EDIT", L"8888", WS_CHILD | WS_VISIBLE | WS_BORDER, 
			375, 10, 60, 25, hWnd, (HMENU)7, NULL, NULL);
    
		// 连接按钮（ID=1）
		CreateWindow(L"BUTTON", L"连接服务器", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 
			445, 10, 100, 25, hWnd, (HMENU)1, NULL, NULL);
 
        // 消息输入框（ID=2）
        CreateWindow(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 
			10, 300, 400, 50, hWnd, (HMENU)2, NULL, NULL);

        // 发送按钮（ID=4）
        CreateWindow(L"BUTTON", L"发送", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 
			420, 300, 50, 50, hWnd, (HMENU)4, NULL, NULL);

        // 消息显示列表框（ID=3）
        CreateWindow(L"LISTBOX", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL, 
			10, 45, 480, 240, hWnd, (HMENU)3, NULL, NULL);
        break;

		case WM_CTLCOLORSTATIC:
		{
			HDC hdcStatic = (HDC)wParam;
			SetTextColor(hdcStatic, RGB(58, 52, 91));  // 标签文字：紫色
			SetBkMode(hdcStatic, TRANSPARENT);
			return (LRESULT)GetStockObject(NULL_BRUSH);
		}

		case WM_CTLCOLORBTN:
		{
			HDC hdcBtn = (HDC)wParam;
			SetBkMode(hdcBtn, TRANSPARENT);
			return (LRESULT)GetStockObject(NULL_BRUSH);
		}

		case WM_ERASEBKGND:
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			// 设置背景颜色
			HBRUSH hBrush = CreateSolidBrush(RGB(253, 232, 237));//浅粉色
			FillRect((HDC)wParam, &rect, hBrush);
			DeleteObject(hBrush);
			return 1;
		}

    case WM_SOCKET_NOTIFY:
    {
        int event = WSAGETSELECTEVENT(lParam);
        switch (event)
        {
        case FD_CONNECT:
		{
			int error = WSAGETSELECTERROR(lParam);
			if (error == 0) {
				AddLogMessage(hWnd, "已连接到服务器！");

			// 调试：弹窗显示要发送的用户名
        char debugMsg[256];
        sprintf(debugMsg, "正在发送用户名: %s", g_userName);
        MessageBoxA(hWnd, debugMsg, "调试", MB_OK);

				// 发送用户名给服务器
				char nameMsg[128];
				sprintf(nameMsg, "USERNAME:%s", g_userName);
				send(g_clientSocket, nameMsg, strlen(nameMsg), 0);
			} else {
				AddLogMessage(hWnd, "连接服务器失败！");
				Disconnect();
			}
			break;
		}
        case FD_READ:
		{
			char buffer[1024];
			int ret = recv(g_clientSocket, buffer, sizeof(buffer) - 1, 0);
			if (ret > 0) {
				buffer[ret] = '\0';
        
				// 检查是否是验证消息
				if (strncmp(buffer, "【验证】", 8) == 0) {
					// 显示问题
					AddLogMessage(hWnd, buffer);
					AddLogMessage(hWnd, "请在输入框中输入答案并点击发送");
            
					// 设置一个标志，表示当前处于验证状态
					// 简单处理：验证期间发送的内容都当作答案
				} else if (strncmp(buffer, "【系统】验证通过", 11) == 0) {
					AddLogMessage(hWnd, buffer);
					AddLogMessage(hWnd, "验证通过，现在可以发送聊天消息了");
				} else if (strncmp(buffer, "【系统】验证失败", 11) == 0) {
					AddLogMessage(hWnd, buffer);
					AddLogMessage(hWnd, "验证失败，程序将退出");
					Disconnect();
					DestroyWindow(hWnd);
				} else {
					AddLogMessage(hWnd, buffer);
				}
			}
			break;
		}
        case FD_CLOSE:
            AddLogMessage(hWnd, "服务器断开连接");
            Disconnect();
            break;
        }
        break;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case 1: // 连接按钮
		{
			// 获取用户名
			GetWindowTextA(GetDlgItem(hWnd, 5), g_userName, 64);
			if (strlen(g_userName) == 0) {
				strcpy(g_userName, "用户");
			}

			// 获取IP（控件ID=6）
			char ip[32] = "127.0.0.1";
			GetWindowTextA(GetDlgItem(hWnd, 6), ip, 32);
    
			// 获取端口（控件ID=7）
			char portStr[8] = "8888";
			GetWindowTextA(GetDlgItem(hWnd, 7), portStr, 8);
    
			int port = atoi(portStr);
			ConnectToServer(hWnd, ip, port);
			break;
		}

        case 4: // 发送按钮
		{
			if (g_clientSocket == INVALID_SOCKET) {
				AddLogMessage(hWnd, "未连接服务器");
				break;
			}
			char msg[256];
			GetWindowTextA(GetDlgItem(hWnd, 2), msg, 256);
			if (strlen(msg) > 0) {
				// 直接发送消息（服务器会根据是否已验证来决定是否转发）
				send(g_clientSocket, msg, strlen(msg), 0);
        
				// 自己显示
				char selfMsg[256];
				sprintf(selfMsg, "[我] %s", msg);
				AddLogMessage(hWnd, selfMsg);
        
				// 清空输入框
				SetWindowTextA(GetDlgItem(hWnd, 2), "");
			}
			break;
		}
        }
        break;

    case WM_DESTROY:
        Disconnect();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}