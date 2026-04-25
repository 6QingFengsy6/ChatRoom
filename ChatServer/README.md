# 网络群聊程序（WinSocket + Win32 API）

## 项目简介
基于 TCP 协议的 C/S 架构群聊系统。  
完全使用 **Win32 API + WinSocket** 实现，不依赖 MFC。

## 开发环境
- Visual C++ 2010 学习版
- Windows Socket 2

## 功能列表
- ✅ 服务器支持多客户端连接
- ✅ 客户端自定义用户名
- ✅ 问题式身份验证（答对才能进群）
- ✅ 群聊消息转发（显示昵称 + 时间）
- ✅ 客户端退出命令（Bye / Quit / 886）
- ✅ 服务器群发公告
- ✅ 窗口背景色美化

## 运行方法
1. 打开 `ChatServer.sln`，编译运行服务器
2. 点击「启动服务器」
3. 打开 `ChatClient.sln`，编译运行客户端（可运行多个）
4. 输入用户名 → 连接 → 回答问题 → 开始群聊

## 文件说明
| 文件 | 说明 |
|------|------|
| ChatServer.cpp | 服务器端主程序 |
| ChatClient.cpp | 客户端主程序 |


## 作者
6QingFengsy6
