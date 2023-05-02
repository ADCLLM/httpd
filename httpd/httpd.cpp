//https://blog.csdn.net/pzjdoytt/article/details/126775241?spm=1001.2014.3001.5501

#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>



//网络通信需要包含的头文件、需要加载的库文件
#include <winsock.h>
#pragma comment(lib, "WS2_32.lib")

#define PRINTF(str) printf("[%s - %d]"#str"=%s\n", __func__, __LINE__, str);
/*
void PRINTF(const char* str) {
	printf("[%s - %d]%s", __LINE__, buff);

}*/
void error_die(const char* str) {
	perror(str);
	exit(1);
}



/*
	实现网络的初始化
	返回值：套接字（服务器端的套接字）
	端口
	参数：	prot 表示端口
			如果*prot的值是0，那么就自动分配一个可用的端口
			为了向tinyhttpd服务器致敬

*/

int startup(unsigned short* port) {
	//网络通信的初始化
	WSADATA data;
	int ret = WSAStartup(MAKEWORD(1, 1),	//1.1版本的协议
		&data);
	if (ret) {	//ret != 0
		error_die("WSAStartup");
	}

	int server_socket = socket(PF_INET,	//套接字的类型
		SOCK_STREAM,	//数据流
		IPPROTO_TCP);
	if (server_socket == -1) {
		//打印错误提示，并结束程序
		error_die("套接字");
	}

	//设置端口复用
	int opt = 1;
	ret = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,
		(const char*)&opt, sizeof(opt));
	if (ret == -1) {
		error_die("setsockopt");
	}

	//配置服务器端的网络地址
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(*port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//绑定套接字
	if (bind(server_socket, (struct sockaddr*)&server_addr,
		sizeof(server_addr)) < 0) {
		error_die("bind");
	}

	//动态分配一个端口
	int namelen = sizeof(server_addr);
	if (*port == 0) {
		if (getsockname(server_socket,
			(struct sockaddr*)&server_addr,
			&namelen) < 0) {
			error_die("getsockname");
		}

		*port = server_addr.sin_port;
	}

	//创建监听队列
	if (listen(server_socket, 5) < 0) {
		error_die("listen");
	}

	return server_socket;
}

//从指定的客户端套接字sock，读取一行数据，保存到buff中
//返回实际读取到的字节数
int get_line(int sock, char* buff, int size) {
	char c = 0;//'\0'
	int i = 0;


	//  \r\n
	while (i < size - 1 && c != '\n') {
		int n = recv(sock, &c, 1, 0);
		if (n > 0) {
			if (c == '\r') {
				n = recv(sock, &c, 1, MSG_PEEK);
				if (n > 0 && c == '\n') {
					recv(sock, &c, 1, 0);
				}
				else {
					c = '\n';
				}
			}
			buff[i++] = c;
		}
		else {
			//to do..
			c = '\n';
		}
	}

	buff[i] = 0;//  '\0'
	return i;
}

void unimplement(int client) {
	//向指定的套接字，发送一个提示还没有实现的错误页面

}

void not_found(int client) {
	//to do.



}

void headers(int client) {
	//发送响应包的头信息
	char buff[1024];
	strcpy(buff, "HTTP/1.0 200 OK\r\n");
	send(client, buff, strlen(buff), 0);

	strcpy(buff, "Server: RockHttpd/0.1\r\n");
	send(client, buff, strlen(buff), 0);

	strcpy(buff, "Content-type:text/html\n");
	send(client, buff, strlen(buff), 0);

	strcpy(buff, "\r\n");
	send(client, buff, strlen(buff), 0);
}

void cat(int client, FILE* resource) {
	//发送指定的信息给客户端
	char buff[4096];
	int count = 0;
	printf("一共发送%d个字节给浏览器\n", count);
	while (1) {
		int ret = fread(buff, sizeof(char), sizeof(buff), resource);
		if (ret <= 0) {
			break;
		}
		send(client, buff, ret, 0);
		count += ret;
	}
	printf("一共发送%d个字节给浏览器\n", count);
}
void server_file(int client, const char* fileName) {
	char numchars = 1;
	char buff[1024];
	//把请求数据包的剩余数据行，读完
	while (numchars > 0 && strcmp(buff, "\n")) {
		numchars = get_line(client, buff, sizeof(buff));
		PRINTF(buff);
	}
	//读文件
	FILE* resource = fopen(fileName, "r");
	if (resource == NULL) {	//读文件失败
		not_found(client);
	}
	else {
		//正式发送资源给浏览器
		headers(client);

		//发送请求的资源信息
		cat(client, resource);

		printf("资源发送完毕！");
	}
	fclose(resource);
}

//处理用户请求的线程函数
DWORD WINAPI accept_request(LPVOID arg) {
	char buff[1024];//1K

	int client = (SOCKET)arg;//客户端套接字

	//读取一行数据
	//0x015ffad8 " GET / HTTP/1.1\n"
	char numchars = get_line(client, buff, sizeof(buff));
	PRINTF(buff);//[accept_request-53]buff="EGT..."

	char method[255];
	int j = 0;
	int i = 0;
	while (!isspace(buff[j]) && i < sizeof(method) - 1) {
		method[i++] = buff[j++];
	}
	method[i] = 0;
	PRINTF(method);

	//检查请求的方法，本服务器是否支持
	if (stricmp(method, "GET") && stricmp(method, "POST")) {
		//向浏览器返回一个错误提示页面
		//to do
		unimplement(client);
		return 0;
	}

	//解析资源文件的路径
	// www.rock.com/abc/test.html
	//GET /abc/test.html HTTP/1.1\n
	char url[255];	//存放请求的资源的完整路径
	i = 0;
	//跳过资源路径前面的空格
	while (isspace(buff[j]) && j < sizeof(buff)) {
		j++;
	}

	while (!isspace(buff[j]) && i < sizeof(url) - 1 && j < sizeof(buff)) {
		url[i++] = buff[j++];
	}
	url[i] = 0;
	PRINTF(url);

	//www.rock.com
	//127.0.0.1
	//url / 
	//htdocs/

	char path[512] = "";
	sprintf(path, "htdocs%s", url);
	if (path[strlen(path) - 1] == '/') {
		strcat(path, "index.html");
	}
	PRINTF(path);

	struct stat status;
	if (stat(path, &status) == -1) {
		//请求包的剩余数据读取完毕
		while (numchars > 0 && strcmp(buff, "\n")) {
			numchars = get_line(client, buff, sizeof(buff));
		}
		not_found(client);
	}
	else {
		if ((status.st_mode & S_IFMT) == S_IFDIR) {
			strcat(path, "/index.html");
		}
		server_file(client, path);
	}
	closesocket(client);
	return 0;
}

int main(void) {
	unsigned short port = 80;
	int servor_sock = startup(&port);
	printf("httpd服务已经启动，正在监听 %d 端口...", port);

	struct sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);
	//to do

	while (1) {
		//阻塞式等待用户通过浏览器发起访问
		int client_sock = accept(servor_sock,
			(struct sockaddr*)&client_addr,
			&client_addr_len);
		if (client_sock == -1) {
			error_die("accept");
		}

		//使用client_sock对用户进行访问
		//创建一个新的线程
		DWORD threadId = 0;
		CreateThread(0, 0,
			accept_request,
			(void*)client_sock,
			0, &threadId);


	}

	// “/”网站服务器资源目录下的 index.html

	closesocket(servor_sock);
	//system("pause");
	return 0;
}
