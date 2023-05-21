#ifdef _WIN32
	#define _WIN32_WINNT _WIN32_WINNT_WIN7
	#include <winsock2.h> //for all socket programming
	#include <ws2tcpip.h> //for getaddrinfo, inet_pton, inet_ntop
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	#include <Wininet.h>
	#include <windows.h>
	#pragma comment(lib, "ws2_32.lib")
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

// ******************* //
// additional librarys for windows// --> also need to work in linux --> RPI3
// ******************* //

#include <string.h>
//#include <pthread.h>
//#include <curl/curl.h>		// --> install library
//#include <jansson.h>	// --> install library
#include <stdlib.h>
//#include <arpa/inet.h>

// *************** //
// constant values //
// *************** //

#define API_URL_FORMAT "http://ip-api.com/json/%s"
#define BUFFER_SIZE 4096
#define filename "output.csv"

// **************** //
// global variables // 
// **************** //

char IP_LOOKUP[1000];
int fileBusy = 0;


// gcc -I"C:\curl\include" HTTP-Client.c -l ws2_32 -o net.exe -ljansson
// gcc  HTTP-Client.c -l ws2_32 -o net.exe -lwininet


// ******************** //
// initialize functions //
// ******************** //

int initialization();
int connection( int internet_socket );
void execution( int internet_socket , char* ipStr);
void cleanup( int internet_socket, int client_internet_socket );
static size_t writeMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
char* getLocationData(char* ipAddress);
void delay(int number_of_seconds);
DWORD WINAPI ClientThread(LPVOID lpParam);
void printIpAddress(const struct sockaddr *sa);
//char printIP(const struct sockaddr_storage* addr);

// ********************* //
// additional data types //
// ********************* //

typedef struct {
    char *data;
    size_t size;
} MemoryChunk;

int main( int argc, char * argv[] ) // should keep running for a long time ** at least multiple days ** 
{
	WSADATA wsaData;
	HANDLE threadHandle;
	// read lookup data and put it in ARRAY

	OSInit();

	// limit max request to 45/min

	int internet_socket = initialization();

	// make thread for every connection
	while(1)
	{
		char ipStr[INET6_ADDRSTRLEN];

		int client_internet_socket = connection( internet_socket ); // accept connection + print ip

		threadHandle = CreateThread(NULL, 0, ClientThread, (LPVOID)&client_internet_socket, 0, NULL);

		printf("test 1 \n");

		if (threadHandle == NULL) {
			printf("test 2 \n");
            printf("Failed to create client thread.\n");
            closesocket(client_internet_socket);
        } 
        else 
        {
        	printf("test 3 \n");
            // Detach the thread so it can continue executing independently
            CloseHandle(&client_internet_socket);
        }

		//execution( client_internet_socket , ipStr);

	}	

	OSCleanup();

	return 0;
}

int initialization()
{
	//Step 1.1
	struct addrinfo internet_address_setup;
	struct addrinfo * internet_address_result;
	memset( &internet_address_setup, 0, sizeof internet_address_setup );
	internet_address_setup.ai_family = AF_UNSPEC;
	internet_address_setup.ai_socktype = SOCK_STREAM;
	internet_address_setup.ai_flags = AI_PASSIVE;
	int getaddrinfo_return = getaddrinfo( NULL, "22", &internet_address_setup, &internet_address_result );
	if( getaddrinfo_return != 0 )
	{
		fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( getaddrinfo_return ) );
		exit( 1 );
	}
	int internet_socket = -1;
	struct addrinfo * internet_address_result_iterator = internet_address_result;


	while( internet_address_result_iterator != NULL )
	{
	    struct sockaddr_in clientAddr;
	    int clientAddrLen = sizeof(clientAddr);
	    getpeername(internet_socket, (struct sockaddr*)&clientAddr, &clientAddrLen);
	    char* clientIP = inet_ntoa(clientAddr.sin_addr);
	    printf("Server IP : %s\n", clientIP);


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

				printf("listening on socket: %i\n",internet_socket);

				if( listen_return == -1 )
				{
					close( internet_socket );
					perror( "listen" );
				}
				else
				{
					break;
				}
			}
		}
		internet_address_result_iterator = internet_address_result_iterator->ai_next;
	}

	freeaddrinfo( internet_address_result );

	if( internet_socket == -1 )
	{
		fprintf( stderr, "socket: no valid socket address found\n" );
		exit( 2 );
	}

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

	char ipStr[INET6_ADDRSTRLEN];
    int ipStrLength = sizeof(ipStr);
	int result = getnameinfo((struct sockaddr*)&client_internet_address, sizeof(client_internet_address),
                             ipStr, ipStrLength, NULL, 0, NI_NUMERICHOST);

	printf("accepted connection from %s\n", ipStr);

	return client_socket;
}

void execution( int internet_socket , char *ipStr)
{
	// read firts message and get IP adress
	char buffer1[1000];
	int number_of_bytes_received = recv( internet_socket, buffer1, ( sizeof buffer1 ) - 1, 0 );
	printf("ipstr test 2 : %s\n", ipStr);

	struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    getpeername(internet_socket, (struct sockaddr*)&clientAddr, &clientAddrLen);
    char* serverIP = inet_ntoa(clientAddr.sin_addr);
	 printf("Server IP : %s\n", serverIP);

	int number_of_bytes_send = send( internet_socket, serverIP, strlen(ipStr), 0 );

	// ************************ //
	// Get info via geolocation //
	// ************************ //

	char outputText[1000];

	char* locationData = getLocationData(ipStr);									// get location data

    if (locationData == NULL) 														// check if locationData is correct
    {
        fprintf(stderr, "Failed to retrieve location data\n");						// send error
    }

    	printf("locationdata test 1 : %s\n", locationData);

    /*
	// ***************************** //
	// keep sending data to attacker //
	// ***************************** //

	int number_of_bytes_send = 0;
	int total_bytes_send = 0;
	char message[1000] = "(";

	strcat(message, locationData[6]);
	strcat(message, ";");
	strcat(message, locationData[7]);
	strcat(message, ")");


	while(number_of_bytes_send > 0) 												// loop until error
	{
		number_of_bytes_send = send( internet_socket, message, 16, 0 );				// send data

		total_bytes_send += number_of_bytes_send;									// add current bytes send to total bytes send
	}

	
    // ********************************* //
	// print info to buffer then to file //
	// ********************************* //

	outputText = dataConverter(locationData, total_bytes_send, seconds);			// convert data to readable format

	while(fileBusy == 1) 															// wait for file to be not busy
	{
		delay(1); 																	// delay 1 second before trying again
	}
	writeToFile(outputText); 	
	*/																				// print buffer in output file
}
/*
int writeToFile(char* buffer[1000])
{
	fileBusy = 1; 																// file = busy
	FILE *fp;
	fp = fopen (filename, "a");													// open file in append mode
	fprintf(fp, buffer);														// append buffer to file
    fclose(fp);																	// close file
    fileBusy = 0; 																// file = not busy
}
*/


static size_t writeMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) 
{
    size_t realSize = size * nmemb;
    MemoryChunk *chunk = (MemoryChunk *)userp;
    char *p = realloc(chunk->data, chunk->size + realSize + 1);
    if (p == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 0;
    }
    chunk->data = p;
    memcpy(&(chunk->data[chunk->size]), contents, realSize);
    chunk->size += realSize;
    chunk->data[chunk->size] = '\0';
    return realSize;
}

char* getLocationData(char* ipAddress) 
{
    HINTERNET hInternet, hConnect;
    DWORD bytesRead;
    char buffer[BUFFER_SIZE];

    // Format the API URL with the provided IP address
    char apiUrl[BUFFER_SIZE];
    snprintf(apiUrl, BUFFER_SIZE, API_URL_FORMAT, ipAddress);

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

void delay(int number_of_seconds)
{   /*
    int milli_seconds = 1000 * number_of_seconds;									// Converting time into milli_seconds 

    clock_t start_time = clock();													// Storing start time

    while (clock() < start_time + milli_seconds){;}			*/;						// looping till required time is not achieved
}

char dataConverter(char** locationData, int dataSent, time_t time) 					// **TODO** add readAttempt variable to func **TODO**
{
		/*																			// format: Time;IP;DataAmount;Country;region;City;ZIP;Org;RepeatAttempt -> CSV file
	char temp[100];
	char output[100];

	strftime(temp, 1000, "%Y-%m-%d %H:%M:%S", localtime(&time)); 					// convert int seconds to "Y-M-D-H-M-S"
	char data_time = strcat(time_string, ";");										// convert "Y-M-D-H-M-S" to "Y-M-D-H-M-S;"

	char data_ip = strcat(locationData[4], ";"); 									// convert "IP" to "IP;"

	sprintf(temp, "%i", checkRepeatAttempt(locationData[4]));						// convert int repeatAttempt to "bool"
	char data_repeat = strcat(temp, ";\n");											// convert "bool" to "bool;"

	strcat(output, data_time);														// add time to output string
	strcat(output, data_ip);														// add ip to output string
	// ...
	strcat(output, data_repeat);	*/;												// add repeatAttempt to output string + \n to go to next line
}

int checkRepeatAttempt(char ip)
{/*
	int ip_size = 0;																// **TODO** fill in ip size **TODO**
	int cntr = 0;																	// counter to count each IP in lookup table

	while(true)
	{
		temp[100]; 																	// temporary holds each value of lookup table

		if(IP_LOOKUP[cntr] == NULL)													// if lookup is empty return "no match found" = 0
		{
			return 0;
			break;
		}

		for (int i = 0 ; i != ip_size ; i++)										// loop over every character
		{
        	temp = strcat(temp, IP_LOOKUP[cntr * ip_size + i]) 						// add character to temp string
    	}

    	if(strcmp(temp,ip) == 0) 													// if item in list == ip return "match found" = 1
    	{
    		return 1;
    		break;
    	}
		

	}*/
	printf("test");
}

DWORD WINAPI ClientThread(LPVOID lpParam) {
	int b_once = 0;

	if(b_once == 0)
	{
		printf("thread created\n");
	    SOCKET clientSocket = *(SOCKET*)lpParam;
	    printf("clientSocket: %i\n", clientSocket);
	    char buffer[1024];
	    int bytesRead;

	    struct sockaddr_in clientAddr;
	    int clientAddrLen = sizeof(clientAddr);
	    getpeername(clientSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
	    char* clientIP = inet_ntoa(clientAddr.sin_addr);
	    printf("Client connected: %s\n", clientIP);

	   	execution( clientSocket , clientIP);


	   	closesocket(clientSocket);
	   	b_once = 1;
 	}
    return 0;
}

