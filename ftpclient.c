/*
* www.ibm.com/developerworks/cn/linux/l-cn-socketftp
* edit:huangtao117@gmail.com
*
* gcc -o ftpclient.exe ftpclient.c -lws2_32
*/
#ifdef _MSC_VER
#endif

#ifdef __GNUC__
#endif

#ifdef __MINGW32__
#endif

#include <stdio.h>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#endif

enum{
	s_getserver,
	s_login,
	s_filename,
	s_go
};

int main(int argv, char** argc)
{
	int state;
	int run;
	int ret;
	int i,j;
	int keeprecv;
	short port;
	char* lpchar;
	FILE* fp;
	char server_name[256];
	char username[256];
	char password[128];
	char dirname[256];
	char filename[256];
	char read_buf[1024];
	char send_buf[1024];
	char retcode[4];
	char line[256];
	int rcode;
	int linecount;
	int read_len;
	unsigned int file_total_size;
	unsigned int file_trans_size;
	SOCKET control_sock;
	SOCKET data_sock;
	struct hostent *hp;
	struct sockaddr_in server;
#ifdef _WIN32
	WSADATA wsaData;
#endif
	
	memset(&server, 0, sizeof(struct sockaddr_in));

#ifdef _WIN32
	ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
	
	state = s_getserver;
	run = 1;
	port = 21;
	read_len = 1023;
	while(run){
		switch(state)
		{
		case s_getserver:
			/* 提示用户输入服务器 */
			printf("please input server:");
			scanf("%s",server_name);
			if(strlen(server_name) == 0){
				printf("invalid server!please re-input:\n");
				continue;
			}
			if(stricmp(server_name, "q") == 0)
				run = 0;
			else
			{
				hp = gethostbyname(server_name);
				if(hp == NULL){
					printf("gethostbyname failed:%s!\n", server_name);
				}
				else{
					printf("\tofficial name: %s\n", hp->h_name);
					
					/* 初始化socket */
					control_sock = socket(AF_INET, SOCK_STREAM, 0);
					memcpy(&server.sin_addr, hp->h_addr, hp->h_length);
					server.sin_family = AF_INET;
					server.sin_port = htons(port);
					
					/* 连接到服务器端 */
					connect(control_sock, (struct sockaddr*)&server, sizeof(server));
					/* 客户端接收服务器端的一些欢迎信息 */
					keeprecv = 1;
					rcode = 0;
					while(keeprecv){
						ret = recv(control_sock, read_buf, read_len, 0);
						/*printf("ret=%d\n",ret);*/
						if(ret < 4)
							continue;
						i = j = 0;
						linecount = 1;
						memset(line, 0, 256);
						while(i < ret){
							if(i > 4 && read_buf[i-1] == '\n' && read_buf[i-2] == '\r'){
								j = 0;
								linecount++;
								memset(line, 0, 256);
							}
							if(read_buf[i] != '\n' && read_buf[i] != '\r')
								line[j] = read_buf[i];
							putchar(read_buf[i]);
							j++;
							i++;
						}
						if(rcode == 0){
							retcode[0] = read_buf[0];
							retcode[1] = read_buf[1];
							retcode[2] = read_buf[2];
							retcode[3] = '\0';
							rcode = atol(retcode);
						}
						else{
							retcode[0] = line[0];
							retcode[1] = line[1];
							retcode[2] = line[2];
							retcode[3] = '\0';
						}
						if(read_buf[3] == '-' && linecount == 1)
							keeprecv = 1;
						else{
							if(atol(retcode) == rcode && line[3] == ' ')
								keeprecv = 0;
						}
					}
					state = s_login;
				}
			}
			break;
		case s_login:
			/* 提示输入用户名 */
			printf("please input username:");
			scanf("%s", username);
			if(strlen(username) == 0){
				printf("invalid user name!please re-input:\n");
				continue;
			}
			if(stricmp(username, "q") == 0){
				run = 0;
				continue;
			}
			/* 命令 "USER username\r\n" */
			sprintf(send_buf, "USER %s\r\n", username);
			/* 客户端发送用户名到服务器端 */
			send(control_sock, send_buf, strlen(send_buf), 0);
			/* 客户端接收服务器的响应码和信息,正常为"331 User name okay, need password." */
			ret = recv(control_sock, read_buf, read_len, 0);
			
			printf("please input password:");
			scanf("%s", password);

			/* 命令 "PASS password\r\n" */
			sprintf(send_buf, "PASS %s\r\n", password);
			/* 客户端发送密码到服务器端 */
			send(control_sock, send_buf, strlen(send_buf), 0);
			/* 客户端接收服务器的响应码和信息,正常为"230 User logged in, proceed." */
			ret = recv(control_sock, read_buf, read_len, 0);
			if(strlen(read_buf) < 4){
				run = 0;
				continue;
			}
			retcode[0] = read_buf[0];
			retcode[1] = read_buf[1];
			retcode[2] = read_buf[2];
			retcode[3] = '\0';
			rcode = atol(retcode);
			if(rcode != 230){
				run = 0;
				continue;
			}
			
			/* 命令 "PASV\r\n" */
			sprintf(send_buf, "PASV\r\n");
			/* 客户端告诉服务器用被动模式 */
			send(control_sock, send_buf, strlen(send_buf), 0);
			/* 客户端接收服务器的响应码和新开的端口号 
			* 正常为 "227 Entering passive mode (h1,h2,h3,h4,p1,p2)" */
			ret = recv(control_sock, read_buf, read_len, 0);
			retcode[0] = read_buf[0];
			retcode[1] = read_buf[1];
			retcode[2] = read_buf[2];
			retcode[3] = '\0';
			rcode = atol(retcode);
			if(rcode != 227){
				run = 0;
				continue;
			}	
			lpchar = read_buf;
			while((*lpchar) != '(') lpchar++;
			for(i = 0; i < 6; i++){
				lpchar++;
				j = 0;
				memset(line, 0, 256);
				while((*lpchar) != ',' && (*lpchar) != ')'){
					line[j] = *lpchar;
					lpchar++;
					j++;
				}
				line[j] = '\0';
				switch(i)
				{
				case 0:
					server.sin_addr.S_un.S_un_b.s_b1 = atol(line);
					break;
				case 1:
					server.sin_addr.S_un.S_un_b.s_b2 = atol(line);
					break;
				case 2:
					server.sin_addr.S_un.S_un_b.s_b3 = atol(line);
					break;
				case 3:
					server.sin_addr.S_un.S_un_b.s_b4 = atol(line);
					break;
				case 4:
					port = 256 * atol(line);
					break;
				case 5:
					port += atol(line);
					break;
				}
			}
			server.sin_port = htons(port);
			server.sin_addr.s_addr = inet_addr("60.190.238.24");
			
			state = s_filename;			
			break;
		case s_filename:
			/* 提示用户输入目录和文件名 */
			printf("please input dirname:");
			scanf("%s", dirname);

			printf("please input filename:");
			scanf("%s", filename);
			
			state = s_go;
			break;
		case s_go:
			/* 连接服务器新开的数据端口 */
			data_sock = socket(AF_INET, SOCK_STREAM, 0);
			ret = connect(data_sock, (struct sockaddr *)&server, sizeof(server));
			if(ret != 0){
				printf("Connect data ip/port failed!\n");
				run = 0;
				continue;
			}
			if(strlen(dirname) > 0 && strcmp(dirname, ".") != 0){
				/* 命令 "CWD dirname\r\n" */
				sprintf(send_buf, "CWD %s\r\n", dirname);
				/* 客户端发送命令改变工作目录 */
				send(control_sock, send_buf, strlen(send_buf), 0);
				/* 客户端接收服务器的响应码和信息, 正常为 "250 Command okay." */
				memset(read_buf, 0, 1024);
				ret = recv(control_sock, read_buf, read_len, 0);
				printf("%s\n", read_buf);
				if(strlen(read_buf) < 4){
					run = 0;
					continue;
				}
				retcode[0] = read_buf[0];
				retcode[1] = read_buf[1];
				retcode[2] = read_buf[2];
				retcode[3] = '\0';
				rcode = atol(retcode);
				if(rcode != 250){
					printf("%s\n", read_buf);
					run = 0;
					continue;
				}
			}
			
			/* 命令 "SIZE filename\r\n" */
			sprintf(send_buf, "SIZE %s\r\n", filename);
			/* 客户端发送命令从服务器端得到下载的文件的大小 */
			send(control_sock, send_buf, strlen(send_buf), 0);
			/* 客户端接收服务器的响应码和信息, 正常为 "213 <size>" */
			ret = recv(control_sock, read_buf, read_len, 0);
			if(strlen(read_buf) < 4){
				run = 0;
				continue;
			}
			retcode[0] = read_buf[0];
			retcode[1] = read_buf[1];
			retcode[2] = read_buf[2];
			retcode[3] = '\0';
			rcode = atol(retcode);
			if(rcode != 213){
				printf("%s\n", read_buf);
				run = 0;
				continue;
			}
			lpchar = read_buf + 4;
			file_total_size = atol(lpchar);
			file_trans_size = 0;
			
			/* 命令 "RETR filename\r\n" */
			sprintf(send_buf, "RETR %s\r\n", filename);
			/* 客户端发送命令从服务器端下载文件 */
			send(control_sock, send_buf, strlen(send_buf), 0);
			/* 客户端接收服务器的响应码和信息, 正常为 "150 Opening data connection" */
			ret = recv(control_sock, read_buf, read_len, 0);
			if(strlen(read_buf) < 4){
				run = 0;
				continue;
			}
			retcode[0] = read_buf[0];
			retcode[1] = read_buf[1];
			retcode[2] = read_buf[2];
			retcode[3] = '\0';
			rcode = atol(retcode);
			if(rcode != 150){
				printf("%s\n", read_buf);
				run = 0;
				continue;
			}
			
			/* 客户端创建文件 */
			fp = fopen(filename,"wb+");
			if(fp == NULL){
				printf("Open file failed.\n");
				run = 0;
				continue;
			}
			for( ;; ){
				/* 客户端通过数据连接从服务器接收文件内容 */
				//memset(read_buf, 0, 1024);
				ret = recv(data_sock, read_buf, read_len, 0);
				file_trans_size += ret;
				/* 客户端写文件 */
				fwrite(read_buf, ret, 1, fp);
				if(file_trans_size >= file_total_size){
					break;
				}
			}
			/* 客户端关闭文件 */
			fclose(fp);
			run = 0;
			break;
		}
	}
	
	shutdown(data_sock, SD_RECEIVE);
	closesocket(data_sock);
	shutdown(control_sock, SD_BOTH);
	closesocket(control_sock);
	
#ifdef _WIN32
	WSACleanup();
#endif

	return 0;
}