#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define PORT 5000
#define BUFF_SIZE 2048

int main(int argc, char* argv[])
{
	int sock, nsent;
	char buffer[BUFF_SIZE];
	memset(buffer, 0, sizeof(buffer));
	
	struct sockaddr_in s = {0};
	s.sin_family = AF_INET;
	s.sin_port = htons(PORT);
	s.sin_addr.s_addr = INADDR_ANY;
	
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	
	if(connect(sock, (struct sockaddr*)&s, sizeof(s)) < 0)
	{
		perror("connect");
		return 1;
	}
	
	int i = 0;
	while(argv[1][i] != '\0')
	{
		buffer[i] = argv[1][i];
		++i;
	}
	
	buffer[i] = '\0';
	
	if((nsent = send(sock, buffer, i, 0)) < 0)
	{
		perror("recv");
		return 1;
	}
	
	close(sock);
	
	return 0;
}
