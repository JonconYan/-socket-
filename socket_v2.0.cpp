#include<stdio.h>
#include<queue>
#include<iostream>
#include<Windows.h>
#include<string>
#include<sstream>
#include<fstream>
#include<thread>
#include<conio.h>
#pragma comment(lib,"ws2_32.lib")
using namespace std;
bool loop_yes = 1;
void send_string(SOCKET c_socket, string s);
void send_othertype(SOCKET c_socket, FILE* fp, string type);
void send_file(string filename, SOCKET c_socket);
void send_notfind(SOCKET c_socket, string a);
void main()
{
	//1.请求协议版本
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		printf("请求协议版本出现错误！\n");
		return ;
	}
	printf("请求协议版本成功！\n");


	//2.创建socket
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (SOCKET_ERROR == serverSocket)
	{
		printf("创建socket失败！\n");
		return ;
	}
	printf("创建socket成功！\n");

	//3.创建协议地址族
	sockaddr_in addr = { 0 };
	addr.sin_family = AF_INET;//协议版本 与socket申请的时候保持一致
	addr.sin_addr.S_un.S_addr = INADDR_ANY;
	addr.sin_port = htons(80);//波特号 0 - 65535
	//os内核和其他程序会使用到一些端口 尽量保证波特号在10000左右


	//4.绑定
	int r = ::bind(serverSocket, (sockaddr*)&addr, sizeof(addr));
	if (-1 == r)
	{
		printf("绑定失败！\n");
		closesocket(serverSocket);
		WSACleanup();
		return ;
	}
	printf("绑定成功！\n");


	//5.监听
	r = listen(serverSocket, 10);
	if (-1 == r)
	{
		printf("监听失败！\n");
		closesocket(serverSocket);
		WSACleanup();
		return;
	}
	printf("监听成功！\n");


	//6.等待连接 阻塞行为
	//要保存连接到服务器的客户的地址族
	fd_set readfd;
	FD_ZERO(&readfd);
	struct timeval tv_timeout;
	tv_timeout.tv_sec = 2;
	tv_timeout.tv_usec = 0;

	FD_SET(serverSocket, &readfd);
	SOCKET clientSocket;
	int len;
	sockaddr_in clientAddr = { 0 };

	vector <SOCKET> clients;//将所有正在连接的客户端放在容器中
	vector <SOCKET> clients_err;//所有应该被删除的客户端 
	//循环开始
	while (1)
	{
		if (kbhit()) {
			int k = getch();
			if (27 == k) { loop_yes = 0; cout << "服务器已经关闭\n"; }//按Esc键退出
			if (13 == k) { loop_yes = 1; cout << "服务器已经开启\n"; }
		}
		while (loop_yes)
		{
			for (int i = 0; i < clients_err.size(); i++)
			{
				for (int j = 0; j < clients.size(); j++)
				{
					if (clients[j] == clients_err[i])
					{
						clients.erase(clients.begin() + j);
						break;
					}
				}
			}
			clients_err.clear();
			FD_ZERO(&readfd);
			FD_SET(serverSocket, &readfd);
			for (std::vector<SOCKET>::iterator it = clients.begin(); it != clients.end(); it++)
			{
				u_long x = 1;
				ioctlsocket(*it, FIONBIO, &x);//设置非阻塞
				FD_SET(*it, &readfd);//每次遍历前更新一次
			}
			int sel = select(FD_SETSIZE, &readfd, NULL, NULL, &tv_timeout);
			if (sel < 1)
			{
				/*cout << "选择错误！\n";
				closesocket(serverSocket);
				WSACleanup();*/
				if (kbhit()) {
					int k = getch();
					if (27 == k) { loop_yes = 0; cout << "服务器已经关闭\n"; }//按Esc键退出
					if (13 == k) { loop_yes = 1; cout << "服务器已经开启\n"; }
				}
			}
			else if (FD_ISSET(serverSocket, &readfd))
			{
				len = sizeof(clientAddr);
				clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &len);
				clients.push_back(clientSocket);//将客户socket加入响应队列
				if (SOCKET_ERROR == clientSocket)
				{
					printf("服务器接收连接错误！\n");
					closesocket(serverSocket);
					WSACleanup();
					return;
				}
				printf("\n%d号客户连接到服务器，ip地址为：%s\n", clients.size(), inet_ntoa(clientAddr.sin_addr));
			}
			else {//有连接请求或socket数据发送
				//fd_set testfds = readfd;//防止readfd被修改
				for (std::vector<SOCKET>::iterator it = clients.begin(); it != clients.end(); it++)
				{

					if (FD_ISSET(*it, &readfd))//客户端socket有数据请求
					{
						//7.通信

						char buf[1024];
						setsockopt(*it, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv_timeout, sizeof(tv_timeout));//设置超时
						r = recv(*it, buf, 1023, NULL);
						if (r > 0)
						{

							//保存输入图像文件名和输出图像文件名
							buf[r] = '\0';
							cout << "\n接受到请求报文如下：\n";
							//cout << buf << "\n";
							stringstream ss;
							ss.str(buf);
							string get;//用来接报文头的第一个
							string fileName;
							//图像数据长度
							//文件指针
							ss >> get >> fileName;
							cout << "filename: " << fileName.substr(1) << endl;
							string s1, s2;
							ss >> get;
							for (int i = 0; i < 2; i++)
							{
								ss >> s1 >> s2;
								cout << s1 << ' ' << s2 << endl;
							}
							printf("\n现在开始对本轮次的%d号客户发送数据.................\n", it - clients.begin() + 1);
							send_file(fileName, *it);

						end:
							printf("\n发送数据成功！\n");
						}
						else
						{
							printf("\n接收数据超时！现在断开此用户的连接，剩余%d个用户正在连接\n", clients.size() - clients_err.size() - 1);
							closesocket(*it);
							clients_err.push_back(*it);
						}

					}
					//else cout << "this!!!!!!!!!!!!!!!\n";
				}






			}
		}
	}
}
void send_file(string filename, SOCKET c_socket)
{
	FILE* fp;
	string content_type;
	string workfolder("C:/Users/10618/source/repos/socket_server/socket_server/");//配置工作区
	workfolder = workfolder + filename.substr(1);
	char buf[1024];
	if (filename.size() <= 1)
	{
		send_string(c_socket, "You asked nothing!\n");
		return;
	}
	else if ((fp = fopen(workfolder.c_str(), "rb")) == NULL)
	{
		send_notfind(c_socket, "can't find this file!");
		return;
	}
	else if (filename.find("jpg") != filename.npos || filename.find("png") != filename.npos)
	{
		send_othertype(c_socket, fp, "image/jpeg");
		fclose(fp);
		return;
	}
	else if (filename.find("css") != filename.npos)
	{
		send_othertype(c_socket, fp, "text/css");
		fclose(fp);
		return;
	}
	else if (filename.find("js") != filename.npos)
	{
		send_othertype(c_socket, fp, "application/javascript");
		fclose(fp);
		return;
	}
	else if (filename.find("html") != filename.npos || filename.find("txt") != filename.npos)
	{
		send_othertype(c_socket, fp, "text/html");
		fclose(fp);
		return;
	}
	else
	{
		send_notfind(c_socket, "can't find this file!\n");
		fclose(fp);
		return;
	}
}
void send_string(SOCKET c_socket, string a)
{
	char buf[1024];
	string buf_string = a;
	int string_length = buf_string.size();
	int length = sprintf(buf, "HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: %d\r\n"
		"Connection: keep-alive\r\n\r\n", string_length);
	strcat(buf, buf_string.c_str());
	send(c_socket, buf, length + string_length, 0);
}
void send_othertype(SOCKET c_socket, FILE* fp, string type)
{
	//获取图像数据总长度
	fseek(fp, 0, SEEK_END);//将fp指向文件尾部
	int length = ftell(fp);//从头到fp指向的地方的统计
	fseek(fp, 0, SEEK_SET);//翻转fp
	//根据图像数据长度分配内存buffer
	char* imgbuffer = (char*)malloc((length + 2) * sizeof(char));
	//将图像数据读入buffer
	fread(imgbuffer, 1, length, fp);
	fclose(fp);
	//输入要保存的文件名


	char *buf_all = (char*)malloc(length * sizeof(char) + 1024);
	int head_length;
	head_length = sprintf(buf_all, "HTTP/1.1 200 OK\r\n"
		"Content-Type: %s\r\n"
		"Content-Length: %d\r\n"
		"Connection: keep-alive\r\n\r\n", type.c_str(), length);
	//strcat(buf_all, imgbuffer);
	memcpy(buf_all + head_length, imgbuffer, length);
	int a = send(c_socket, buf_all, length + head_length, 0);
	if (a == SOCKET_ERROR)
	{
		printf("发送数据失败！\n");
	}
}
void send_notfind(SOCKET c_socket, string a)
{
	char buf[1024];
	string buf_string = a;
	int string_length = buf_string.size();
	int length = sprintf(buf, "HTTP / 1.1 404 Not Found"
		"Content-Type: text/html\r\n"
		"Content-Length: %d\r\n"
		"Connection: keep-alive\r\n\r\n", string_length);
	strcat(buf, buf_string.c_str());
	send(c_socket, buf, length + string_length, 0);
}