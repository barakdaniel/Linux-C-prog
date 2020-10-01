#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <pthread.h>
#include <libcli.h>
#include <signal.h>
#include <strings.h>
#include <stdlib.h>
#include <semaphore.h>
#include <execinfo.h>


#define PORT 5000
#define TELNET 8000
#define BT_BUF_SIZE 1024

char ip[21];
char path[50];
int listenOnTelnet = 1;
int backTracing = 0;
char telnetBuffer[BT_BUF_SIZE];
sem_t semaphore;
int s;


/////////////////// Cygnus Functions ///////////////////

void backTrace()
{
	int j = 0, nptrs = 0;
	void *buffer[BT_BUF_SIZE];
	char **strings;
	char iToChar[12];
	
	memset(telnetBuffer, 0, sizeof(telnetBuffer));
	memset(buffer, 0, sizeof(buffer));	
	

	nptrs = backtrace(buffer, BT_BUF_SIZE);
	printf("backtrace() returned %d addresses\n", nptrs);

	/* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
	would produce similar output to the following: */

	strings = backtrace_symbols(buffer, nptrs);
	if (strings == NULL) {
		perror("backtrace_symbols");
		exit(EXIT_FAILURE);
	}	
	
	for (j = 0; j < nptrs; j++)
	{
		sprintf( iToChar, "%d: ", j+1 );
		strcat(telnetBuffer, iToChar);
		strcat(telnetBuffer, strings[j]);
		strcat(telnetBuffer, "\n");
	}
	

	free(strings);
}

void  __attribute__ ((no_instrument_function))  __cyg_profile_func_enter (void *this_fn, void *call_site)
{
	if(backTracing)
	{
		backTracing = 0;
        	backTrace();
        	sem_post(&semaphore);
        }
}

void  __attribute__ ((no_instrument_function))  __cyg_profile_func_exit (void *this_fn, void *call_site) { }


/////////////////// BackTrace - Telnet Functions ///////////////////

int cmd_backtrace(struct cli_def *cli, char *command, char *argv[], int argc)
{
	backTracing = 1;
	sem_wait(&semaphore);
	cli_print(cli, telnetBuffer);
	return CLI_OK;
}


void telnetBackTrace()
{
	struct sockaddr_in servaddr;
	struct cli_command *c;
	struct cli_def *cli;
	int on = 1, x;

	// Must be called first to setup data structures
	cli = cli_init();

	// Set the hostname (shown in the the prompt)
	cli_set_hostname(cli, "Notify");

	// Set the greeting
	cli_set_banner(cli, "Welcome to the CLI test program.");

	// Enable 2 username / password combinations
	cli_allow_user(cli, "TeamKakashi", "1");

	// Set up a few simple one-level commands
	cli_register_command(cli, NULL, "backtrace", cmd_backtrace, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
	
	// Create a socket
	s = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	// Listen on port 12345
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(12345);
	bind(s, (struct sockaddr *)&servaddr, sizeof(servaddr));

	// Wait for a connection
	listen(s, 50);

	while (listenOnTelnet && (x = accept(s, NULL, 0)))
	{
		// Pass the connection off to libcli
		cli_loop(cli, x);
		close(x);
	}

	// Free data structures
	cli_done(cli);
	
	
}

void sendToUDP(char* name, char* access, char* time)
{
	int sock, nsent;
	char toSend[2048];
	memset(toSend, 0, sizeof(toSend));
	
	struct sockaddr_in s = {0};
	s.sin_family = AF_INET;
	s.sin_port = htons(PORT);
	
	if(inet_pton(AF_INET, ip, &s.sin_addr.s_addr)<=0)  
   	{ 
        	perror("\nInvalid address/ Address not supported"); 
        	exit(1); 
   	} 
	
	
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(connect(sock, (struct sockaddr*)&s, sizeof(s)) < 0)
	{
		perror("connect");
		exit(1);
	}
	
	strcpy(toSend, "\nFILE ACCESSED: ");
	strcat(toSend, name);	
	strcat(toSend, "\nACCESS: ");
	strcat(toSend, access);
	strcat(toSend, "\nTIME OF ACCESS: ");
	strcat(toSend, time);
	strcat(toSend, "\n");
	strcat(toSend, "\0");
	
	if((nsent = send(sock, toSend, strlen(toSend), 0)) < 0)
	{
		perror("recv");
		exit(1);
	}
	
	close(sock);

	exit(0);
}




void * socketThread(void *arg)
{
	int serverSocket, newSocket;
	struct sockaddr_in serverAddr;
	struct sockaddr_storage serverStorage;
	socklen_t addr_size;
	
	//Create the socket. 
	serverSocket = socket(PF_INET, SOCK_STREAM, 0);

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(TELNET);
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
	
	//Bind the address struct to the socket 
	bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

	if(listen(serverSocket,50)==0) 
	{}
	else
	{
	    perror("telnet");
	}
	//Accept call creates a new socket for the incoming connection
	addr_size = sizeof serverStorage;
        newSocket = accept(serverSocket, (struct sockaddr *) &serverStorage, &addr_size);
}







static void handle_events(int fd, int *wd, int htmlFd)
{
	char buf[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));
	const struct inotify_event *event;
	int i;
	ssize_t len;
	char *ptr;
    	char timeBuffer[32];
    	char operationBuffer[16];
    	char nameBuffer[1024];


	/* Loop while events can be read from inotify file descriptor. */

	for (;;) {

		/* Read some events. */

		len = read(fd, buf, sizeof buf);
		if (len == -1 && errno != EAGAIN) {
			perror("read");
			exit(EXIT_FAILURE);
		}

		if (len <= 0)
			break;

		/* Loop over all events in the buffer */

		for (ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len) 
		{
			memset(nameBuffer, 0, 1024);
    			memset(operationBuffer, 0, 16);
			time_t timer;
    			struct tm* tm_info;

			event = (const struct inotify_event *) ptr;

			if (!(event->mask & IN_OPEN))
			{
			/* Print event time */

			memset(timeBuffer, 0, sizeof (timeBuffer));
			timer = time(NULL);
			tm_info = localtime(&timer);
 	               strftime(timeBuffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
			write(htmlFd, timeBuffer, strlen(timeBuffer));
			write(htmlFd, ": ", strlen(": "));
			
			/* Print event type */
			
			//	write(htmlFd, "FILE ACCESSED: ", strlen("FILE ACCESSED: "));
			if (event->mask & IN_CLOSE_NOWRITE)
				strcpy(operationBuffer, "NOWRITE ");
			if (event->mask & IN_CLOSE_WRITE)
				strcpy(operationBuffer, "WRITE ");

			write(htmlFd, operationBuffer, strlen(operationBuffer));
			/* Print the name of the watched directory */
	
			if (*wd == event->wd) {
				strcat(nameBuffer, path);
			}
			

			/* Print the name of the file */

			if (event->len)
			{
				write(htmlFd, event->name, strlen(event->name));
				strcat(nameBuffer, event->name);
			}

			/* Print type of filesystem object */

			if(event->mask & IN_ISDIR)
				write(htmlFd, " [directory]<br>", strlen(" [directory]<br>"));
			else
				write(htmlFd, " [file]<br>", strlen(" [file]<br>"));
				
				
			pid_t pid;
			pid = fork();
			if(pid == -1)
				perror("fork");
			if(pid == 0)
				sendToUDP(nameBuffer, operationBuffer, timeBuffer);
			}
	
		}

	}
}

int main(int argc, char *argv[])
{
	int htmlFd;
	char buf;
	int fd, poll_num;
	int wdes;
	nfds_t nfds;
	struct pollfd fds[2];
	int opt;	
	
	sem_init(&semaphore, 0, 0);
	
	if(argc != 5)
	{
		perror("Wrong number of arguments");
	}
	
	int bt_thread;
	pthread_t tid;

	if( pthread_create(&tid, NULL, telnetBackTrace, &bt_thread) != 0 )
      			perror("Failed to create thread");
		
       while ((opt = getopt(argc, argv, "i:d:")) != -1)
       {
               switch (opt) 
		{
 			case 'i':
 			{
				strcpy(ip, optarg);
				break;
			}	
 			case 'd':
 			{
				strcpy(path, optarg);
				break;
			}	
			default:
				printf("Bad arguments was caught\n");
				break;
 		}
 	}

	htmlFd = open("/var/www/html/index.html", O_WRONLY);
	if(htmlFd == -1)
		perror("open");

	if (argc < 3) {
		printf("Usage: %s PATH [PATH ...]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	printf("Press ENTER key to terminate.\n");

	/* Create the file descriptor for accessing the inotify API */

	fd = inotify_init1(IN_NONBLOCK);
	if (fd == -1) {
		perror("inotify_init1");
		exit(EXIT_FAILURE);
	}


	/* Mark directories for events
	   - file was opened
	   - file was closed */

	wdes = inotify_add_watch(fd, path, IN_OPEN | IN_CLOSE);
	if (wdes == -1) {
		fprintf(stderr, "Cannot watch '%s'\n", path);
		perror("inotify_add_watch");
		exit(EXIT_FAILURE);
	}

	/* Prepare for polling */

	nfds = 2;

	/* Console input */

	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;

	/* Inotify input */

	fds[1].fd = fd;
	fds[1].events = POLLIN;

	/* Wait for events and/or terminal input */

	write(htmlFd, "<html><body>", strlen("<html><body>"));


	printf("Listening for events.\n");
	
	while (1) {
        	
		poll_num = poll(fds, nfds, -1);
		if (poll_num == -1) {
			if (errno == EINTR)
				continue;
			perror("poll");
			exit(EXIT_FAILURE);
		}

		if (poll_num > 0) {

			if (fds[0].revents & POLLIN) {		// CHECK IF STDIN IS RELEVANT?

				/* Console input is available. Empty stdin and quit */

				while (read(STDIN_FILENO, &buf, 1) > 0 && buf != '\n')
					continue;
				break;
			}

			if (fds[1].revents & POLLIN) {

				/* Inotify events are available */

				handle_events(fd, &wdes, htmlFd);
			}
		}
	}

	listenOnTelnet = 0;
	printf("listenOnTelnet = %d\n", listenOnTelnet);
	printf("Listening for events stopped.\n");

	//if(lseek(htmlFd, 0, EOF) < 0)
	//	perror("seek");
	write(htmlFd, "</body></html>", strlen("</body></html>"));
	/* Close inotify file descriptor */

	close(htmlFd);
	close(fd);
	pthread_join(tid, NULL);
	exit(EXIT_SUCCESS);
}
