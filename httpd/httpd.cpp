//https://blog.csdn.net/pzjdoytt/article/details/126775241?spm=1001.2014.3001.5501

#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>



//����ͨ����Ҫ������ͷ�ļ�����Ҫ���صĿ��ļ�
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
	ʵ������ĳ�ʼ��
	����ֵ���׽��֣��������˵��׽��֣�
	�˿�
	������	prot ��ʾ�˿�
			���*prot��ֵ��0����ô���Զ�����һ�����õĶ˿�
			Ϊ����tinyhttpd�������¾�

*/

int startup(unsigned short* port) {
	//����ͨ�ŵĳ�ʼ��
	WSADATA data;
	int ret = WSAStartup(MAKEWORD(1, 1),	//1.1�汾��Э��
		&data);
	if (ret) {	//ret != 0
		error_die("WSAStartup");
	}

	int server_socket = socket(PF_INET,	//�׽��ֵ�����
		SOCK_STREAM,	//������
		IPPROTO_TCP);
	if (server_socket == -1) {
		//��ӡ������ʾ������������
		error_die("�׽���");
	}

	//���ö˿ڸ���
	int opt = 1;
	ret = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,
		(const char*)&opt, sizeof(opt));
	if (ret == -1) {
		error_die("setsockopt");
	}

	//���÷������˵������ַ
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(*port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//���׽���
	if (bind(server_socket, (struct sockaddr*)&server_addr,
		sizeof(server_addr)) < 0) {
		error_die("bind");
	}

	//��̬����һ���˿�
	int namelen = sizeof(server_addr);
	if (*port == 0) {
		if (getsockname(server_socket,
			(struct sockaddr*)&server_addr,
			&namelen) < 0) {
			error_die("getsockname");
		}

		*port = server_addr.sin_port;
	}

	//������������
	if (listen(server_socket, 5) < 0) {
		error_die("listen");
	}

	return server_socket;
}

//��ָ���Ŀͻ����׽���sock����ȡһ�����ݣ����浽buff��
//����ʵ�ʶ�ȡ�����ֽ���
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
	//��ָ�����׽��֣�����һ����ʾ��û��ʵ�ֵĴ���ҳ��

}

void not_found(int client) {
	//to do.



}

void headers(int client) {
	//������Ӧ����ͷ��Ϣ
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
	//����ָ������Ϣ���ͻ���
	char buff[4096];
	int count = 0;
	printf("һ������%d���ֽڸ������\n", count);
	while (1) {
		int ret = fread(buff, sizeof(char), sizeof(buff), resource);
		if (ret <= 0) {
			break;
		}
		send(client, buff, ret, 0);
		count += ret;
	}
	printf("һ������%d���ֽڸ������\n", count);
}
void server_file(int client, const char* fileName) {
	char numchars = 1;
	char buff[1024];
	//���������ݰ���ʣ�������У�����
	while (numchars > 0 && strcmp(buff, "\n")) {
		numchars = get_line(client, buff, sizeof(buff));
		PRINTF(buff);
	}
	//���ļ�
	FILE* resource = fopen(fileName, "r");
	if (resource == NULL) {	//���ļ�ʧ��
		not_found(client);
	}
	else {
		//��ʽ������Դ�������
		headers(client);

		//�����������Դ��Ϣ
		cat(client, resource);

		printf("��Դ������ϣ�");
	}
	fclose(resource);
}

//�����û�������̺߳���
DWORD WINAPI accept_request(LPVOID arg) {
	char buff[1024];//1K

	int client = (SOCKET)arg;//�ͻ����׽���

	//��ȡһ������
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

	//�������ķ��������������Ƿ�֧��
	if (stricmp(method, "GET") && stricmp(method, "POST")) {
		//�����������һ��������ʾҳ��
		//to do
		unimplement(client);
		return 0;
	}

	//������Դ�ļ���·��
	// www.rock.com/abc/test.html
	//GET /abc/test.html HTTP/1.1\n
	char url[255];	//����������Դ������·��
	i = 0;
	//������Դ·��ǰ��Ŀո�
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
		//�������ʣ�����ݶ�ȡ���
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
	printf("httpd�����Ѿ����������ڼ��� %d �˿�...", port);

	struct sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);
	//to do

	while (1) {
		//����ʽ�ȴ��û�ͨ��������������
		int client_sock = accept(servor_sock,
			(struct sockaddr*)&client_addr,
			&client_addr_len);
		if (client_sock == -1) {
			error_die("accept");
		}

		//ʹ��client_sock���û����з���
		//����һ���µ��߳�
		DWORD threadId = 0;
		CreateThread(0, 0,
			accept_request,
			(void*)client_sock,
			0, &threadId);


	}

	// ��/����վ��������ԴĿ¼�µ� index.html

	closesocket(servor_sock);
	//system("pause");
	return 0;
}
