#ifdef _WIN32
	#define _WIN32_WINNT _WIN32_WINNT_WIN7
	#include <winsock2.h> //for all socket programming
	#include <ws2tcpip.h> //for getaddrinfo, inet_pton, inet_ntop
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	void OSInit( void )
	{
		WSADATA wsaData;
		int WSAError = WSAStartup( MAKEWORD( 2, 0 ), &wsaData ); 
		if( WSAError != 0 )
		{
			fprintf( stderr, "WSAStartup errno = %d\n", WSAError );
			exit( -1 );
		}
	}
	void OSCleanup( void )
	{
		WSACleanup();
	}
	#define perror(string) fprintf( stderr, string ": WSA errno = %d\n", WSAGetLastError() )
#else
	#include <sys/socket.h> //for sockaddr, socket, socket
	#include <sys/types.h> //for size_t
	#include <netdb.h> //for getaddrinfo
	#include <netinet/in.h> //for sockaddr_in
	#include <arpa/inet.h> //for htons, htonl, inet_pton, inet_ntop
	#include <errno.h> //for errno
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	void OSInit( void ) {}
	void OSCleanup( void ) {}
#endif

#include <ws2tcpip.h>
#include <Windows.h>
#include <WinInet.h>

// gcc  try-nr10.c -l ws2_32 -o test.exe -lwininet


int initialization();
int connection( int internet_socket );
void execution( int internet_socket );
void cleanup( int internet_socket, int client_internet_socket );
void printAddressInfo(struct addrinfo *addressInfo);
void getPublicIPAddress();

char client_ip;
char server_ip;

int main( int argc, char * argv[] )
{
	//////////////////
	//Initialization//
	//////////////////
	 getPublicIPAddress();


	OSInit();

	int internet_socket = initialization();

	//////////////
	//Connection//
	//////////////

	int client_internet_socket = connection( internet_socket );

	/////////////
	//Execution//
	/////////////

	execution( client_internet_socket );


	////////////
	//Clean up//
	////////////

	cleanup( internet_socket, client_internet_socket );

	OSCleanup();

	return 0;
}

int initialization()
{
    //Step 1.1
    struct addrinfo internet_address_setup;
    struct addrinfo *internet_address_result;
    memset(&internet_address_setup, 0, sizeof internet_address_setup);
    internet_address_setup.ai_family = AF_UNSPEC;
    internet_address_setup.ai_socktype = SOCK_STREAM;
    internet_address_setup.ai_flags = AI_PASSIVE;

    // Replace "NULL" with the desired IP address
    const char *ipAddress = "::1";

    int getaddrinfo_return = getaddrinfo(ipAddress, "22", &internet_address_setup, &internet_address_result);
    printf("getaddrinfo: %s\n", gai_strerror(getaddrinfo_return));
    if (getaddrinfo_return != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(getaddrinfo_return));
        exit(1);
    }


	int internet_socket = -1;
	struct addrinfo * internet_address_result_iterator = internet_address_result;
	while( internet_address_result_iterator != NULL )
	{
		printf("%d\n", internet_address_result_iterator);
		//Step 1.2
		internet_socket = socket( internet_address_result_iterator->ai_family, internet_address_result_iterator->ai_socktype, internet_address_result_iterator->ai_protocol );
		if( internet_socket == -1 )
		{
			perror( "socket" );
		}
		else
		{
			//Step 1.3
			int bind_return = bind( internet_socket, internet_address_result_iterator->ai_addr, internet_address_result_iterator->ai_addrlen );
			if( bind_return == -1 )
			{
				perror( "bind" );
				close( internet_socket );
			}
			else
			{
				//Step 1.4
				int listen_return = listen( internet_socket, 1 );
				printf("listening on socket: %i\n", internet_socket);
				if( listen_return == -1 )
				{
					close( internet_socket );
					perror( "listen" );
					//printf("listen" );
				}
				else
				{
					break;
					
				}
			}
		}
		//printf("test");
		internet_address_result_iterator = internet_address_result_iterator->ai_next;
	}

	freeaddrinfo( internet_address_result );

	if( internet_socket == -1 )
	{
		fprintf( stderr, "socket: no valid socket address found\n" );
		exit( 2 );
	}

	printAddressInfo(internet_address_result);
	return internet_socket;
}

int connection( int internet_socket )
{
	//Step 2.1
	struct sockaddr_storage client_internet_address;
	socklen_t client_internet_address_length = sizeof client_internet_address;
	int client_socket = accept( internet_socket, (struct sockaddr *) &client_internet_address, &client_internet_address_length );
	if( client_socket == -1 )
	{
		perror( "accept" );
		close( internet_socket );
		exit( 3 );
	}
	printf("connection on: %i\n", client_internet_address);
	return client_socket;
}

void execution( int internet_socket )
{
	//Step 3.1
	int number_of_bytes_received = 0;
	char buffer[1000];
	number_of_bytes_received = recv( internet_socket, buffer, ( sizeof buffer ) - 1, 0 );
	if( number_of_bytes_received == -1 )
	{
		perror( "recv" );
	}
	else
	{
		buffer[number_of_bytes_received] = '\0';
		printf( "Received : %s\n", buffer );
	}

	//Step 3.2
	int number_of_bytes_send = 0;
	number_of_bytes_send = send( internet_socket, "Hello TCP world!", 16, 0 );
	if( number_of_bytes_send == -1 )
	{
		perror( "send" );
	}
}

void cleanup( int internet_socket, int client_internet_socket )
{
	//Step 4.2
	int shutdown_return = shutdown( client_internet_socket, SD_RECEIVE );
	if( shutdown_return == -1 )
	{
		perror( "shutdown" );
	}

	//Step 4.1
	close( client_internet_socket );
	close( internet_socket );
}

void printAddressInfo(struct addrinfo *addressInfo) {
      // Loop through the address list
    for (struct addrinfo *p = addressInfo; p != NULL; p = p->ai_next) {
        char ip[INET6_ADDRSTRLEN];
        void *addr;
        char *ipVersion;
        int port;

        if (p->ai_family == AF_INET) { // IPv4 address
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipVersion = "IPv4";
            port = ntohs(ipv4->sin_port);
        } else { // IPv6 address
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipVersion = "IPv6";
            port = ntohs(ipv6->sin6_port);
        }

        // Convert the IP address to a string
        const char *ipStr = NULL;
        DWORD ipStrLen = INET6_ADDRSTRLEN;
        int result = getnameinfo(p->ai_addr, p->ai_addrlen, ip, ipStrLen, NULL, 0, NI_NUMERICHOST);
        if (result == 0) {
            ipStr = ip;
        } else {
            fprintf(stderr, "getnameinfo failed: %s\n", gai_strerrorA(result));
            continue;
        }

        // Print the IP address and port
        printf("%s: %s\n", ipVersion, ipStr);
        printf("Port: %d\n", port);
    }
}

void getPublicIPAddress() {
    HINTERNET hInternet = NULL;
    HINTERNET hConnect = NULL;
    DWORD bytesRead = 0;
    char buffer[4096];

    // Initialize WinINet
    hInternet = InternetOpenA("Public IP Request", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hInternet == NULL) {
        printf("Failed to initialize WinINet.\n");
        return;
    }

    // Open a connection
    hConnect = InternetOpenUrlA(hInternet, "http://ipinfo.io/ip", NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (hConnect == NULL) {
        printf("Failed to open WinINet connection.\n");
        InternetCloseHandle(hInternet);
        return;
    }

    // Read the response
    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        printf("%.*s\n", bytesRead, buffer);
    }

    // Cleanup
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
}

