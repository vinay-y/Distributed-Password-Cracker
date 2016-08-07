#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <chrono>


using namespace std;

#define MAXDATASIZE 100

int main(int argc, char const *argv[])
{
    int sockfd;         // socket
    int numbytes,len;   // number of received bytes and length of string for input data
    short int port;     // port value
    struct sockaddr_in server_addr; // server address
    struct hostent *host;   // host for getting host by name
    char buf[MAXDATASIZE];  // char array for receiving data


    if (argc != 6) {
        cout<<"Usage: ./user <server ip/host-name> <server-port> <hash> <passwd-length> <binary-string>"<<endl;
        exit(1);
    }

    // get the input data in a string
    string outstring="user ";
    outstring.append(argv[3]);
    outstring.append(1,' ');
    outstring.append(argv[4]);
    outstring.append(1,' ');
    outstring.append(argv[5]);
    outstring.append(1,'\0');

    len=outstring.length();
    const char *cstr = outstring.c_str();
    cout<<"Data: "<<outstring<<endl;
    
    // get host by name
    if ((host=gethostbyname(argv[1])) == NULL) {
        perror("gethostbyname");
        exit(1);
    }


    const char * server;
    server = argv[1];
    port = atoi(argv[2]);
    cout<<"Connecting to: "<<server<<" "<<port<<endl;

    // create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port); // convert port to network byte order
    server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    bzero(&(server_addr.sin_zero), 8);

    // connect to server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }

    // start clock once connected
    chrono::steady_clock::time_point start = chrono::steady_clock::now();

    // send data
    cout<<"Sending data: ";
    if (send(sockfd, cstr, len, 0) == -1) {
        perror("send");
    }
    cout<<cstr<<endl;

    // receive data
    if ((numbytes=recv(sockfd, buf, MAXDATASIZE-1, 0)) <= 0) {
        if (numbytes == 0) {
            // connection closed
            printf("User: server hung up\n");
        } 
        else {
            perror("recv");
        }
    exit(1);
    }
    cout<<"Password is: "<<buf<<endl;
        
    chrono::steady_clock::time_point end1= chrono::steady_clock::now();
    std::cout<<"Time difference(ms) = ";
    std::cout << chrono::duration_cast<chrono::milliseconds>(end1 - start).count() <<endl;
    return 0;
}