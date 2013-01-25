/*
* www.ibm.com/developerworks/cn/linux/l-cn-socketftp
* code:huangtao117@gmail.com
*/
#ifdef __MINGW32__
#include <winsock2.h>
#include <windows.h>
#endif

int main(int argv, char** argc)
{
	SOCKET control_sock;
	//struct hostent *hp;
	//struct sockaddr_in server;
	//memset(&server, 0, sizeof(struct sockaddr_in));
	
	/* 提示用户输入服务器 */
	printf("Please input server:");
	
	/* 初始化socket */
	//control_sock = socket(AF_INET, SOCK_STREAM, 0);
	//hp = gethostbyname(server_name);
	return 0;
}