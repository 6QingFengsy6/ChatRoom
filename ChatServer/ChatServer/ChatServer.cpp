// ChatServer.cpp : 定义应用程序的入口点。

#include "stdafx.h"
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include "ChatServer.h"

#define MAX_LOADSTRING 100

// 自定义消息 - Socket事件通知
#define WM_SOCKET_NOTIFY  (WM_USER + 100)

// 最大客户端数量
#define MAX_CLIENTS 50

// 客户端Socket数组
SOCKET g_clientSockets[MAX_CLIENTS];
int g_clientCount = 0;
char g_clientNames[MAX_CLIENTS][64];  // 存储用户名

// 全局变量
SOCKET g_listenSocket = INVALID_SOCKET;  // 监听Socket
HWND g_hWnd = NULL;                       // 主窗口句柄
HINSTANCE hInst;								// 当前实例
TCHAR szTitle[MAX_LOADSTRING];					// 标题栏文本
TCHAR szWindowClass[MAX_LOADSTRING];			// 主窗口类名

// 验证问题
#define VERIFY_QUESTION "群主的学号是？"
#define VERIFY_ANSWER "1034230405"
// 客户端验证状态（是否已通过验证）
BOOL g_clientVerified[MAX_CLIENTS];

// 函数声明
void AddLogMessage(HWND hWnd, const char* msg);
BOOL InitWinsock();
BOOL StartServer(HWND hWnd, const char* ip, int port);
void CleanupServer();

// 此代码模块中包含的函数的前向声明:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MSG msg;
	HACCEL hAccelTable;

	// 初始化全局字符串
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_CHATSERVER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 执行应用程序初始化:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CHATSERVER));

	// 主消息循环:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}

//
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CHATSERVER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_CHATSERVER);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // 将实例句柄存储在全局变量中

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
              CW_USEDEFAULT, 0, 700, 450, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   // 保存全局窗口句柄
   g_hWnd = hWnd;

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

// 添加日志到列表框
void AddLogMessage(HWND hWnd, const char* msg)
{
    // 获取当前时间
    SYSTEMTIME st;
    GetLocalTime(&st);
    
    char timeBuf[64];
    sprintf(timeBuf, "[%02d:%02d:%02d] ", st.wHour, st.wMinute, st.wSecond);
    
    // 组合：时间 + 消息
    char fullMsg[512];
    sprintf(fullMsg, "%s%s", timeBuf, msg);
    
    // 将char转换为宽字符（解决乱码问题）
    WCHAR wfullMsg[512];
    MultiByteToWideChar(CP_ACP, 0, fullMsg, -1, wfullMsg, 512);
    
    // 添加到列表框（控件ID=3）
    SendMessage(GetDlgItem(hWnd, 3), LB_ADDSTRING, 0, (LPARAM)wfullMsg);
    // 自动滚动到底部
    SendMessage(GetDlgItem(hWnd, 3), LB_SETCURSEL, 
                SendMessage(GetDlgItem(hWnd, 3), LB_GETCOUNT, 0, 0) - 1, 0);
}

// 初始化Winsock
BOOL InitWinsock()
{
	// 清空客户端数组
    for (int i = 0; i < MAX_CLIENTS; i++) {
		g_clientSockets[i] = INVALID_SOCKET;
}
	g_clientCount = 0;
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        AddLogMessage(g_hWnd, "Winsock初始化失败！");
        return FALSE;
    }
    AddLogMessage(g_hWnd, "Winsock初始化成功");
    return TRUE;
}

// 启动服务器
BOOL StartServer(HWND hWnd, const char* ip, int port)
{
    // 1. 创建监听Socket
    g_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (g_listenSocket == INVALID_SOCKET) {
        AddLogMessage(hWnd, "创建Socket失败！");
        return FALSE;
    }
    
    // 允许端口重用（解决端口被占用问题）
    int reuse = 1;
    setsockopt(g_listenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
    
    // 2. 绑定地址和端口
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip);
    
    if (bind(g_listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        AddLogMessage(hWnd, "绑定端口失败！");
        closesocket(g_listenSocket);
        g_listenSocket = INVALID_SOCKET;
        return FALSE;
    }
    
    // 3. 开始监听
    if (listen(g_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        AddLogMessage(hWnd, "监听失败！");
        closesocket(g_listenSocket);
        g_listenSocket = INVALID_SOCKET;
        return FALSE;
    }
    
    // 设置为异步非阻塞模式
    if (WSAAsyncSelect(g_listenSocket, hWnd, WM_SOCKET_NOTIFY, FD_ACCEPT) == SOCKET_ERROR) {
        AddLogMessage(hWnd, "设置异步模式失败！");
        closesocket(g_listenSocket);
        g_listenSocket = INVALID_SOCKET;
        return FALSE;
    }
    
    char msg[256];
    sprintf(msg, "服务器启动成功，监听 %s:%d", ip, port);
    AddLogMessage(hWnd, msg);
    AddLogMessage(hWnd, "等待客户端连接...");
    
    return TRUE;
}

// 清理资源
void CleanupServer()
{
    if (g_listenSocket != INVALID_SOCKET) {
        closesocket(g_listenSocket);
        g_listenSocket = INVALID_SOCKET;
    }
    WSACleanup();
    AddLogMessage(g_hWnd, "服务器已关闭");
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的: 处理主窗口的消息。
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_SOCKET_NOTIFY:
    {
        int event = WSAGETSELECTEVENT(lParam);
        SOCKET eventSocket = (SOCKET)wParam;
        
        switch(event)
        {
        case FD_ACCEPT:  // 有新客户端连接
            {
                AddLogMessage(hWnd, "收到客户端连接请求...");
                SOCKET clientSocket = accept(g_listenSocket, NULL, NULL);

                if (clientSocket != INVALID_SOCKET) {
                    for (int i = 0; i < MAX_CLIENTS; i++) {
                        if (g_clientSockets[i] == INVALID_SOCKET) {
                            g_clientSockets[i] = clientSocket;
							strcpy(g_clientNames[i], "未命名");//初始化用户名
							g_clientVerified[i] = FALSE;//初始未验证
                            break;
                        }
                    }
                    g_clientCount++;
                    
                    WSAAsyncSelect(clientSocket, hWnd, WM_SOCKET_NOTIFY, FD_READ | FD_CLOSE);
                    
					// 发送验证问题给客户端
					char questionMsg[256];
					sprintf(questionMsg, "【验证】请回答: %s", VERIFY_QUESTION);
					send(clientSocket, questionMsg, strlen(questionMsg), 0);

                    char msg[256];
                    sprintf(msg, "客户端 %d 连接成功！", g_clientCount);
                    AddLogMessage(hWnd, msg);
                }
            }
            break;
            
        case FD_READ:
		{
			char buffer[1024];
			int ret = recv(eventSocket, buffer, sizeof(buffer) - 1, 0);
			if (ret > 0) {
				buffer[ret] = '\0';

				// 调试：弹窗显示收到的内容
				//char debugMsg[256];
				//sprintf(debugMsg, "服务器收到: %s", buffer);
				//MessageBoxA(hWnd, debugMsg, "调试", MB_OK);

				// 找到这个客户端的索引
				int clientIndex = -1;
				for (int i = 0; i < MAX_CLIENTS; i++) {
					if (g_clientSockets[i] == eventSocket) {
						clientIndex = i;
						break;
					}
				}
        
				// 检查是否是用户名消息
				if (strncmp(buffer, "USERNAME:", 9) == 0) {
					// 更新用户名
					strcpy(g_clientNames[clientIndex], buffer + 9);
					char msg[256];
					sprintf(msg, "用户 %s 已连接，等待验证", g_clientNames[clientIndex]);
					AddLogMessage(hWnd, msg);
				}
				// 检查是否是验证答案（未验证时收到的消息当作答案处理）
				else if (!g_clientVerified[clientIndex]) {
					// 验证答案
					if (strcmp(buffer, VERIFY_ANSWER) == 0) {
						g_clientVerified[clientIndex] = TRUE;
						char msg[256];
						sprintf(msg, "用户 %s 验证通过，加入聊天室", g_clientNames[clientIndex]);
						AddLogMessage(hWnd, msg);
                
						// 告诉客户端验证成功
						send(eventSocket, "【系统】验证通过，欢迎加入群聊", strlen("【系统】验证通过，欢迎加入群聊"), 0);
                
						// 通知其他用户有人加入
						char joinMsg[256];
						sprintf(joinMsg, "【系统】%s 加入了群聊", g_clientNames[clientIndex]);
						for (int i = 0; i < MAX_CLIENTS; i++) {
							if (g_clientSockets[i] != INVALID_SOCKET && g_clientSockets[i] != eventSocket) {
								send(g_clientSockets[i], joinMsg, strlen(joinMsg), 0);
							}
						}
					} else {
						// 验证失败
						char msg[256];
						sprintf(msg, "用户 %s 验证失败，断开连接", g_clientNames[clientIndex]);
						AddLogMessage(hWnd, msg);
						send(eventSocket, "【系统】验证失败，连接将断开", strlen("【系统】验证失败，连接将断开"), 0);
                
						// 断开连接
						closesocket(eventSocket);
						g_clientSockets[clientIndex] = INVALID_SOCKET;
						g_clientCount--;
					}
				}
				else {
					// 已验证，普通消息转发
					char msgWithName[1024];
					sprintf(msgWithName, "%s: %s", g_clientNames[clientIndex], buffer);
					AddLogMessage(hWnd, msgWithName);
            
					// 转发给所有其他客户端
					for (int i = 0; i < MAX_CLIENTS; i++) {
						if (g_clientSockets[i] != INVALID_SOCKET && g_clientSockets[i] != eventSocket) {
							send(g_clientSockets[i], msgWithName, strlen(msgWithName), 0);
						}
					}
				}
			}
			break;
		}
            
        case FD_CLOSE:  // 客户端断开连接
            {
                char msg[256];
                sprintf(msg, "客户端 %d 断开连接", eventSocket);
                AddLogMessage(hWnd, msg);
                
                // 从数组中移除
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (g_clientSockets[i] == eventSocket) {
                        g_clientSockets[i] = INVALID_SOCKET;
                        break;
                    }
                }
                g_clientCount--;
                closesocket(eventSocket);
            }
            break;
        }
    }
    break;   

		case WM_CREATE:
        // 创建控件
        CreateWindow(L"STATIC", L"服务器IP:", WS_CHILD | WS_VISIBLE,
                     10, 10, 70, 25, hWnd, NULL, NULL, NULL);
        CreateWindow(L"EDIT", L"127.0.0.1", WS_CHILD | WS_VISIBLE | WS_BORDER,
                     80, 10, 120, 25, hWnd, NULL, NULL, NULL);
        
        CreateWindow(L"STATIC", L"端口:", WS_CHILD | WS_VISIBLE,
                     220, 10, 40, 25, hWnd, NULL, NULL, NULL);
        CreateWindow(L"EDIT", L"8888", WS_CHILD | WS_VISIBLE | WS_BORDER,
                     270, 10, 80, 25, hWnd, NULL, NULL, NULL);
        
        // 启动按钮
        CreateWindow(L"BUTTON", L"启动服务器", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                     370, 10, 100, 25, hWnd, (HMENU)1, NULL, NULL);
        
        // 退出按钮
        CreateWindow(L"BUTTON", L"退出", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                     490, 10, 60, 25, hWnd, (HMENU)2, NULL, NULL);
        
        // 消息显示列表框
        CreateWindow(L"LISTBOX", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOINTEGRALHEIGHT | WS_VSCROLL,
                     10, 45, 660, 200, hWnd, (HMENU)3, NULL, NULL);

		// 群发标签
		CreateWindow(L"STATIC", L"公告/群发:", WS_CHILD | WS_VISIBLE,
					 10, 260, 80, 25, hWnd, NULL, NULL, NULL);

		// 公告输入框（添加 WS_TABSTOP 和 ES_MULTILINE 支持多行）
		CreateWindow(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_MULTILINE,
					 90, 260, 400, 60, hWnd, (HMENU)4, NULL, NULL);

		// 群发按钮
		CreateWindow(L"BUTTON", L"群发消息", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
					 500, 260, 100, 60, hWnd, (HMENU)5, NULL, NULL);
        break;

		case WM_CTLCOLORSTATIC:
		{
			HDC hdcStatic = (HDC)wParam;
			SetTextColor(hdcStatic, RGB(58, 52, 91));  // 紫色文字
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
			// 设置背景颜色
			RECT rect;
			GetClientRect(hWnd, &rect);
			HBRUSH hBrush = CreateSolidBrush(RGB(181, 172, 201)); // 浅紫色
			FillRect((HDC)wParam, &rect, hBrush);
			DeleteObject(hBrush);
			return 1;
		}

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case 1:  // 启动按钮
            {
                // 获取IP输入框内容
                char ip[32] = "127.0.0.1";
                char portStr[8] = "8888";
                
                // 获取IP编辑框内容（第二个控件）
                HWND hEditIP = GetWindow(hWnd, GW_CHILD);
                if (hEditIP) {
                    // 跳过第一个标签控件，找到编辑框
                    hEditIP = GetWindow(hEditIP, GW_HWNDNEXT);
                    if (hEditIP) {
                        GetWindowTextA(hEditIP, ip, 32);
                    }
                }
                
                // 获取端口编辑框内容
                HWND hEditPort = GetWindow(hWnd, GW_CHILD);
                for (int i = 0; i < 3; i++) {
                    if (hEditPort) hEditPort = GetWindow(hEditPort, GW_HWNDNEXT);
                }
                if (hEditPort) {
                    GetWindowTextA(hEditPort, portStr, 8);
                }
                
                int port = atoi(portStr);
                
                // 初始化Winsock并启动服务器
                if (InitWinsock()) {
                    StartServer(hWnd, ip, port);
                }
            }
            break;
		case 5:  // 群发消息按钮
			{
				char msg[512];
				GetWindowTextA(GetDlgItem(hWnd, 4), msg, 512);
				if (strlen(msg) > 0) {
					// 构造公告消息
					char announce[600];
					sprintf(announce, "[系统公告] %s", msg);
        
					// 在服务器列表框中显示
					AddLogMessage(hWnd, announce);
        
					// 转发给所有已连接的客户端
					for (int i = 0; i < MAX_CLIENTS; i++) {
						if (g_clientSockets[i] != INVALID_SOCKET) {
							send(g_clientSockets[i], announce, strlen(announce), 0);
						}
					}
        
					// 清空输入框
					SetWindowTextA(GetDlgItem(hWnd, 4), "");
				}
				break;
			}
        case 2:  // 退出按钮
            CleanupServer();
            DestroyWindow(hWnd);
            break;
        }
        break;
        
    case WM_DESTROY:
    // 关闭所有客户端连接
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_clientSockets[i] != INVALID_SOCKET) {
            closesocket(g_clientSockets[i]);
            g_clientSockets[i] = INVALID_SOCKET;
        }
    }
    CleanupServer();
    PostQuitMessage(0);
    break;
        
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}