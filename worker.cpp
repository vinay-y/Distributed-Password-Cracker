

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <string>
#include <cstring>
#include <vector>
#include <crypt.h>
using namespace std;

#define MAXDATASIZE 100

//[a-zA-Z0-9]

int main(int argc, char const *argv[])
{
    int sockfd;         // socket
    int numbytes;       // number of received bytes 
    short int port;     // port value
    struct sockaddr_in server_addr; // server address
    struct hostent *host;   // host for getting host by name
    char buf[MAXDATASIZE];  // char array for receiving data

    if (argc != 3) {
        cout<<"Usage: ./worker <server ip/host-name> <server-port>"<<endl;
        exit(1);
    }

    string caps= "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    string small="abcdefghijklmnopqrstuvwxyz";
    string numbers="0123456789";

    // get host by name
    if ((host=gethostbyname(argv[1])) == NULL) {
        perror("gethostbyname");
        exit(1);
    }

    const char * server;
    server = argv[1];
    port = atoi(argv[2]);
    cout<<"Connecting to: "<<server<<" "<<port<<endl;

    // get host by name
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port); // convert port to network byte order
    server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    bzero(&(server_addr.sin_zero), 8);

    // connect to server
    if (connect(sockfd, (struct sockaddr *)&server_addr,
        sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }

    // send message indicating identity
    if (send(sockfd, "worker\n", 7, 0) == -1) {
        perror("send");
    }

    pid_t pid = 0;  // stores pid of child

    // main loop
    while(true) {

        // receive data
        if ((numbytes=recv(sockfd, buf, MAXDATASIZE-1, 0)) <= 0) {
            if (numbytes == 0) {
                // connection closed
                cout<<"Worker: server hung up"<<endl;
                kill(pid,9);
                cout<<"killed child process"<<endl;
            } 
            else {
                perror("recv");
            }
            exit(1); // exit if server hung up
        }
        cout<<"Received data: "<<buf<<endl;
        char * bufcpy;
        memcpy(bufcpy, buf, 6);

        // kill working processes if command is to stop(quit) else fork
        if (strcmp(bufcpy, " quit\0")==0) {
            kill(pid,9);
            cout<<"killed child process"<<endl;
        }
        else {
        pid = fork();

        if (pid > 0) {
            // parent process : do nothing continue listening
        }
        else if (pid == 0){
            //child process
            // if command is to stop; stop!
            if (strcmp(bufcpy, " quit\0")==0) {
                exit(0);
            }


            cout<<"Working..."<<endl;
            const char * me; // final message to be sent
            int len = 0; // length of final messaage to be sent

            // split message
            string hash1,len1str,bin1,num1str,num2str;
            int s[5] = {0};   // location of spaces
            int t = 0;

            // compute s values
            for(int m=0; m < strlen(buf); m++)
            {
                if (buf[m] == ' ') {
                    s[t] = m;
                    t++;
                }
                s[4] = m+1;
            }

            string buf1(buf);
            // split into parts
            hash1 = buf1.substr(0,s[0]);
            len1str = buf1.substr(s[0]+1,s[1]-s[0]-1);
            bin1 = buf1.substr(s[1]+1,s[2]-s[1]-1);
            num1str = buf1.substr(s[2]+1,s[3]-s[2]-1);
            num2str = buf1.substr(s[3]+1,s[4]-s[3]-1);
            int num1, num2, len1;
            // convert strings to int
            num1 = stoi(num1str);
            num2 = stoi(num2str);
            len1 = stoi(len1str);
            string sal=hash1.substr(0,2);
            const char * salt = sal.c_str();
            cout<<"Salt is : "<<salt<<endl;

            // shortloop for first letter possibilities, main loop for the rest
            string mainloop = "", shortloop = "";
            if(bin1[0] == '1') {
                mainloop += small;
            }
            if(bin1[1] == '1') {
                mainloop += caps;
            }
            if(bin1[2] == '1') {
                mainloop += numbers;
            }

            shortloop = mainloop.substr(num1,num2-num1+1);

            cout<<"First letter takes values: "<<shortloop<<endl;
            vector<int> v;
            v.resize(len1);
            string current = ""; // stores string being tested
            current += shortloop[v[0]];
            for(int i = 1; i < len1; i++) {
                current += mainloop[v[i]];
            }

            bool flag = 0;
            while(true) {

                for (int i = 0; i < mainloop.length(); i++)
                {
                    
                    const char * curr = current.c_str();
                    string currhash(crypt(curr,salt));
                    if(currhash.compare(hash1)==0){
                        // Found the pass; send it to server
                        current.append(1,'\0');
                        current = "ans " + current;
                        
                        const char *mes = current.c_str();

                        me = mes;
                        len = current.length();

                        flag = 1;
                        break;
                    }
                    v[len1-1]++;
                    current[len1-1]=mainloop[v[len1-1]];
                }
                
                if (flag) {
                    break;
                }
                v[len1-1]--;
                current[len1-1]=mainloop[v[len1-1]];
                int j;

                // get cycle back from first character to last
                for(j = len1-1;j>0;j--){
                    if (v[j]!=mainloop.length()-1)
                    {
                        v[j]++;
                        current[j]=mainloop[v[j]];
                        break;
                    }
                    else{
                        v[j]=0;
                        current[j]=mainloop[0];
                    }
                
                }

                if (j==0) {
                    if(v[0]!=shortloop.length()-1)
                    {
                        v[0]++;
                        current[0]=shortloop[v[0]];
                    }
                    else {
                        /*Not found, message return*/
                        char mes[] = "ans n\n";
                        me = mes;
                        len = strlen(mes)+1;
                        break;
                    }
                }
            
            }
            
            // send data
            cout<<"Sending data: "<<me<<endl;
            if (send(sockfd, me, len, 0) == -1) {
                perror("send");
            }
            exit(0);
        }
        else {
            cout<<"Fork failed"<<endl;
            exit(1);
        }
    }
    }
    return 0;
}