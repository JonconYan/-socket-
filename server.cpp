#include<stdio.h>
#include<queue>
#include<iostream>
#include<Windows.h>
#include<string>
#include<sstream>
#include<fstream>
#pragma comment(lib,"ws2_32.lib")
using namespace std;
void send_string(SOCKET c_socket, string s);
void send_othertype(SOCKET c_socket, FILE* fp, string type);
void send_file(string filename, SOCKET c_socket);
void send_notfind(SOCKET c_socket, string a);
int main()
{
	//1.����Э��汾
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		printf("����Э��汾���ִ���\n");
		return -1;
	}
	printf("����Э��汾�ɹ���\n");


	//2.����socket
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (SOCKET_ERROR == serverSocket)
	{
		printf("����socketʧ�ܣ�\n");
		return -2;
	}
	printf("����socket�ɹ���\n");

	//3.����Э���ַ��
	sockaddr_in addr = { 0 };
	addr.sin_family = AF_INET;//Э��汾 ��socket�����ʱ�򱣳�һ��
	addr.sin_addr.S_un.S_addr = INADDR_ANY;
	addr.sin_port = htons(80);//���غ� 0 - 65535
	//os�ں˺����������ʹ�õ�һЩ�˿� ������֤���غ���10000����


	//4.��
	int r = bind(serverSocket, (sockaddr*)&addr, sizeof(addr));
	if (-1 == r)
	{
		printf("��ʧ�ܣ�\n");
		closesocket(serverSocket);
		WSACleanup();
		return -3;
	}
	printf("�󶨳ɹ���\n");


	//5.����
	r = listen(serverSocket, 10);
	if (-1 == r)
	{
		printf("����ʧ�ܣ�\n");
		closesocket(serverSocket);
		WSACleanup();
		return -4;
	}
	printf("�����ɹ���\n");


	//6.�ȴ����� ������Ϊ
	//Ҫ�������ӵ��������Ŀͻ��ĵ�ַ��
	fd_set readfd;
	FD_ZERO(&readfd);
	struct timeval tv_timeout;
	tv_timeout.tv_sec = 2;
	tv_timeout.tv_usec = 0;

	FD_SET(serverSocket, &readfd);
	SOCKET clientSocket;
	int len;
	sockaddr_in clientAddr = { 0 };

	vector <SOCKET> clients;//�������������ӵĿͻ��˷���������
	vector <SOCKET> clients_err;//����Ӧ�ñ�ɾ���Ŀͻ��� 
	//ѭ����ʼ
	while (1)
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
			ioctlsocket(*it, FIONBIO, &x);//���÷�����
			FD_SET(*it, &readfd);//ÿ�α���ǰ����һ��
		}
		int sel = select(FD_SETSIZE, &readfd, NULL, NULL , NULL);
		if (sel < 1)
		{
			cout << "ѡ�����\n";
			closesocket(serverSocket);
			WSACleanup();
			return 0;
		}
		else if (FD_ISSET(serverSocket, &readfd))
		{
			len = sizeof(clientAddr);
			clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &len);
			clients.push_back(clientSocket);//���ͻ�socket������Ӧ����
			if (SOCKET_ERROR == clientSocket)
			{
				printf("�������������Ӵ���\n");
				closesocket(serverSocket);
				WSACleanup();
				return -5;
			}
			printf("\n%d�ſͻ����ӵ���������ip��ַΪ��%s\n", clients.size(),inet_ntoa(clientAddr.sin_addr));
		}
		else {//�����������socket���ݷ���
			//fd_set testfds = readfd;//��ֹreadfd���޸�
			for (std::vector<SOCKET>::iterator it = clients.begin(); it != clients.end(); it++)
			{
	
					if(FD_ISSET(*it, &readfd))//�ͻ���socket����������
					{
						//7.ͨ��

						char buf[1024];
						setsockopt(*it, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv_timeout, sizeof(tv_timeout));//���ó�ʱ
						r = recv(*it, buf, 1023, NULL);
						if (r > 0)
						{

							//��������ͼ���ļ��������ͼ���ļ���
							buf[r] = '\0';
							cout << "\n���ܵ����������£�\n";
							cout << buf <<"\n";
							stringstream ss;
							ss.str(buf);
							string get;//�����ӱ���ͷ�ĵ�һ��
							string fileName;
							//ͼ�����ݳ���
							//�ļ�ָ��
							ss >> get >> fileName;
							printf("\n���ڿ�ʼ�Ա��ִε�%d�ſͻ���������.................\n",it-clients.begin()+1);
							send_file(fileName, *it);

						end:
							printf("\n�������ݳɹ���\n");
						}
						else
						{
							printf("\n�������ݳ�ʱ�����ڶϿ����û������ӣ�ʣ��%d���û���������\n",clients.size()-clients_err.size()-1);
							closesocket(*it);
							clients_err.push_back(*it);
						}
					
					}
					//else cout << "this!!!!!!!!!!!!!!!\n";
			}






		}
	}

	return 0;
}
void send_file(string filename, SOCKET c_socket)
{
	FILE* fp;
	string content_type;
	string workfolder("C:/Users/10618/source/repos/socket_server/socket_server/");//���ù�����
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
	//��ȡͼ�������ܳ���
	fseek(fp, 0, SEEK_END);//��fpָ���ļ�β��
	int length = ftell(fp);//��ͷ��fpָ��ĵط���ͳ��
	fseek(fp, 0, SEEK_SET);//��תfp
	//����ͼ�����ݳ��ȷ����ڴ�buffer
	char* imgbuffer = (char*)malloc((length + 2) * sizeof(char));
	//��ͼ�����ݶ���buffer
	fread(imgbuffer, 1, length, fp);
	fclose(fp);
	//����Ҫ������ļ���


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
		printf("��������ʧ�ܣ�\n");
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