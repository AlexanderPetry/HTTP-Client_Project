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


#include <string.h>
#include <curl.h> 		// --> install library
#include <jansson.h>	// --> install library

#define API_URL "http://ip-api.com/json/"

#define filename "output.csv"

int fileBusy = 0;


int initialization();
int connection( int internet_socket );
void execution( int internet_socket );
void cleanup( int internet_socket, int client_internet_socket );
static size_t writeMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
char** getLocationData(char* ipAddress);
void delay(int number_of_seconds);

typedef struct {
    char *data;
    size_t size;
} MemoryChunk;

int main( int argc, char * argv[] )
{

	OSInit();

	// for every connection
		int internet_socket = initialization();

		int client_internet_socket = connection( internet_socket );

		execution( client_internet_socket );

		cleanup( internet_socket, client_internet_socket );


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
	return client_socket;
}

void execution( int internet_socket )
{
	// get IP adress

	// ***************************** //
	// keep sending data to attacker //
	// ***************************** //

	int number_of_bytes_send = 0;
	int total_bytes_send = 0;

	while(number_of_bytes_send > 0) 											// loop until error
	{
		number_of_bytes_send = send( internet_socket, message, 16, 0 );			// send data
		total_bytes_send += number_of_bytes_send;								// add current bytes send to total bytes send
	}

	// *********************** //
	// Get info via geolocatio //
	// *********************** //

	char outputText[];

	char** locationData = getLocationData(client_internet_address);				// get location data
    if (locationData == NULL) 													// check if locationData is correct
    {
        fprintf(stderr, "Failed to retrieve location data\n");
    }

    // ********************************* //
	// print info to buffer then to file //
	// ********************************* //

	while(fileBusy == 1) 														// wait for file to be not busy
	{
		delay(1); 																// delay 1 second before trying again
	}

	writeToFile(outputText); 													// print buffer in output file
}

int writeToFile(char* buffer[1000])
{
	fileBusy = 1; 					// file = busy
	FILE *fp;
	fp = fopen (filename, "a");		// open file in append mode
	fprintf(fp, buffer);			// append buffer to file
    fclose(fp);						// close file
    fileBusy = 0; 					// file = not busy
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

char** getLocationData(char* ipAddress) 
{
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize CURL\n");
        return NULL;
    }

    char url[256];
    snprintf(url, sizeof(url), "%s%s", API_URL, ipAddress);

    MemoryChunk chunk = {NULL, 0};
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "Failed to perform HTTP request: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        return NULL;
    }

    curl_easy_cleanup(curl);

    json_error_t error;
    json_t *root = json_loads(chunk.data, 0, &error);
    if (!root) {
        fprintf(stderr, "Failed to parse JSON: %s\n", error.text);
        free(chunk.data);
        return NULL;
    }

    const char *status = json_string_value(json_object_get(root, "status"));
    if (strcmp(status, "success") != 0) {
        fprintf(stderr, "Failed to retrieve location information\n");
        json_decref(root);
        free(chunk.data);
        return NULL;
    }

    char** locationData = (char**) malloc(4 * sizeof(char*));
    locationData[0] = strdup(json_string_value(json_object_get(root, "country")));
    locationData[1] = strdup(json_string_value(json_object_get(root, "regionName")));
    locationData[2] = strdup(json_string_value(json_object_get(root, "city")));
    locationData[3] = strdup(json_string_value(json_object_get(root, "zip")));

    json_decref(root);
    free(chunk.data);

    return locationData;
}

void delay(int number_of_seconds)
{
    // Converting time into milli_seconds
    int milli_seconds = 1000 * number_of_seconds;
 
    // Storing start time
    clock_t start_time = clock();
 
    // looping till required time is not achieved
    while (clock() < start_time + milli_seconds)
        ;
}