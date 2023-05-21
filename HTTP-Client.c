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

#define API_URL_FORMAT "http://ip-api.com/json/%s"
#define filename "output.csv"
#define MAX_JSON_LENGTH 2048
#define MAX_STRING_LENGTH 256
#define MAX_ARRAY_SIZE 14

int initialization();
int connection( int internet_socket );
void cleanup( int internet_socket, int client_internet_socket );
void printAddressInfo(struct addrinfo *addressInfo);
void getPublicIPAddress();
DWORD WINAPI ClientThread(LPVOID lpParam);
char* getLocationData(char* ipAddress);
void formatBytes(int bytes);
int writeToFile(char buffer[1000]);
void delay(int number_of_seconds);
void formatJsonToArray(const char *json, char **array);


char client_ip;
char server_ip;
int fileBusy = 0;

int main( int argc, char * argv[] )
{
	WSADATA wsaData;
	HANDLE threadHandle;
	getPublicIPAddress();


	OSInit();

	int internet_socket = initialization();
	int threadcounter = 0;

	while(1){
		int client_internet_socket = connection( internet_socket );
		threadcounter++;
		printf("thread count: %i\n",threadcounter);
		threadHandle = CreateThread(NULL, 0, ClientThread, (LPVOID)&client_internet_socket, 0, NULL);
		//printf("test 1 \n");

		if (threadHandle == NULL) {
			//printf("test 2 \n");
            printf("Failed to create client thread.\n");
            closesocket(client_internet_socket);
            threadcounter--;
        } 
        else 
        {
        	//printf("test 3 \n");
            // Detach the thread so it can continue executing independently
            CloseHandle(&client_internet_socket);
            threadcounter--;
        }
	}
	

	//execution( client_internet_socket );



	//cleanup( internet_socket, client_internet_socket );

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
	//printf("connection on: %i\n", client_internet_address);
	return client_socket;
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


DWORD WINAPI ClientThread(LPVOID lpParam) {

	//printf("thread created\n");
    SOCKET clientSocket = *(SOCKET*)lpParam;
    //printf("clientSocket: %i\n", clientSocket);
    char buffer[1024];
    int bytesRead;

    struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    getpeername(clientSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
    char* clientIP = inet_ntoa(clientAddr.sin_addr);
    printf("Client connected: %s\n", clientIP);

	const char *json = getLocationData(clientIP);
    char *array[MAX_ARRAY_SIZE];
    formatJsonToArray(json, array);

    printf("%s\n", array[0]);
    printf("%s\n", array[1]);
    printf("%s\n", array[2]);

   	int number_of_bytes_send = 10;
	int total_bytes_send = 0;
	int counter = 0;

	printf("sending Data: ");

	while(number_of_bytes_send > 0) 												// loop until error
	{
		number_of_bytes_send = send( clientSocket, clientIP, 16, 0 );				// send data
		total_bytes_send += number_of_bytes_send;									// add current bytes send to total bytes send
		if(counter > 16384)
		{
			printf(".");
			counter = 0;
		}
		else
		{
			counter++;
		}
	}

	printf("\n");
	formatBytes(total_bytes_send);
	printf("\n");

	//printf("wait for file to be not busy\n");
	//printf(array[0]);
	while(fileBusy == 1) 															// wait for file to be not busy
	{
		delay(1); 																	// delay 1 second before trying again
	}

	char temp[1000];
	char text[1000];
	sprintf(temp, "%i", total_bytes_send);

	if(strcmp(array[0], "\"status\":\"fail\"") == 0)
	{
		strcat(text,clientIP);
		strcat(text, ";");
		strcat(text, "fail;");
		strcat(text, temp);
		strcat(text, "\n");
		writeToFile(text);
	}
	else
	{
		printf("adding data to output array\n");
		for (int i = 0; i < MAX_ARRAY_SIZE && array[i] != NULL; i++) {
			if(array[i] == NULL)
			{
				break;
			}
	        strcat(text, array[i]);
			strcat(text, ";");
	    }
		strcat(text, temp);
		strcat(text, "\n");
		printf("printing to file\n");
		writeToFile(text);
		printf("file printed\n");
	}	

	printf("closing socket\n");
   	closesocket(clientSocket);
	  
    return 0;
}

char* getLocationData(char* ipAddress) 
{
    HINTERNET hInternet, hConnect;
    DWORD bytesRead;
    char buffer[2048];

    // Format the API URL with the provided IP address
    char apiUrl[2048];
    snprintf(apiUrl, 2048, API_URL_FORMAT, ipAddress);

    hInternet = InternetOpenA("IPAPI Client", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hInternet == NULL) {
        printf("Failed to initialize WinINet.\n");
        return NULL;
    }

    hConnect = InternetOpenUrlA(hInternet, apiUrl, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (hConnect == NULL) {
        printf("Failed to open URL: %d\n", GetLastError());
        InternetCloseHandle(hInternet);
        return NULL;
    }

    char* data = NULL;
    size_t dataSize = 0;

    while (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';

        // Resize the data buffer and append the received data
        char* newData = realloc(data, dataSize + bytesRead + 1);
        if (newData == NULL) {
            printf("Failed to allocate memory.\n");
            free(data);
            InternetCloseHandle(hConnect);
            InternetCloseHandle(hInternet);
            return NULL;
        }

        data = newData;
        memcpy(data + dataSize, buffer, bytesRead);
        dataSize += bytesRead;
        data[dataSize] = '\0';
    }

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    return data;
}

void formatBytes(int bytes) {
    double size = bytes;
    const char* units[] = {"bytes", "KB", "MB", "GB"};

    int unitIndex = 0;
    while (size >= 1024 && unitIndex < 3) {
        size /= 1024;
        unitIndex++;
    }

    printf("Amount of bytes send: %.2f %s\n", size, units[unitIndex]);
}

int writeToFile(char buffer[1000])
{
	fileBusy = 1; 																// file = busy
	printf("file status: busy\n");
	FILE *fp;
	fp = fopen (filename, "a");													// open file in append mode
	fprintf(fp, buffer);														// append buffer to file
    fclose(fp);																	// close file
    fileBusy = 0; 																// file = not busy
    printf("file status: not busy\n");
}

void delay(int number_of_seconds)
{   
    sleep(number_of_seconds);								
}

void formatJsonToArray(const char *json, char **array) {
    char tempJson[MAX_JSON_LENGTH];
    strcpy(tempJson, json);

    // Remove leading and trailing curly braces
    char *start = strchr(tempJson, '{');
    char *end = strrchr(tempJson, '}');
    if (start && end) {
        start++;
        *end = '\0';
    }

    // Split key-value pairs into array elements
    int index = 0;
    char *pair = strtok(start, ",");
    while (pair != NULL && index < MAX_ARRAY_SIZE) {
        array[index] = pair;
        pair = strtok(NULL, ",");
        index++;
    }
}
