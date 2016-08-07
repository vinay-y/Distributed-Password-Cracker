
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <map>
#include <set>
#include <cstring>
#include <queue>
#include <string>

using namespace std;

int main(int argc, char const *argv[])
{
    short int port;

    if (argc != 2) {
        cout<<"Usage: ./server <server-port>"<<endl;
        exit(0);
    }

    port = atoi(argv[1]);

    fd_set all_fds;      // Set of file descriptors of all sockets that are open
    fd_set read_fds;     // temporary file descriptor set for receiving through select()
    struct sockaddr_in server_addr;     // server address
    struct sockaddr_in client_addr;     // client address
    
    int fdmax;           // maximum file descriptor number in all_fds set
    int listener;        // server's listening socket descriptor
    int newfd;           // socket descriptor for handling new connections

    char buf[256];       // char array for receiving data
    int nbytes;          // number of bytes of received data
    
    socklen_t addrlen;   // used while handling new connections
    int i, j;            // iterators

    set<int> users;      // set of all active users 
    set<int> workers;    // set of all active workers

    FD_ZERO(&all_fds);
    FD_ZERO(&read_fds);  // clear the all_fds and the read_fds sets


    // create listener socket
    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    int yes=1;           // used in setsockopt()
    // Remove "address already in use" error message for bind
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    // bind to the port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port); // convert port to network byte order
    memset(&(server_addr.sin_zero), '\0', 8);
    if (bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(1);
    }

    // listen on listener socket
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(1);
    }
    // add the listener to the all_fds set
    FD_SET(listener, &all_fds);
    
    // keep track of the biggest file descriptor
    fdmax = listener;   // so far, it’s this one
    
    // map each worker to the user for whom it is working, 0 if not working
    map<int, int> worker2user;

    // queue of jobs with the job giver's fd and job description as pair
    queue<pair<int,string> > jobs;

    // queue of the free workers waiting for a job
    queue<int> free_workers;
    cout<<"Starting server..."<<endl;
    // main select() loop
    for(;;) {

        read_fds = all_fds; // get all fd's in read_fds
        
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(1);
        } 
        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) { 
                // handle new connections
                    addrlen = sizeof(client_addr);
                    if ((newfd = accept(listener, (struct sockaddr *)&client_addr, &addrlen)) == -1) {
                        perror("accept");
                    } 
                    else {
                        FD_SET(newfd, &all_fds); // add to all_fds set
                        if (newfd > fdmax) { // keep track of the maximum
                            fdmax = newfd;
                        }
                        cout<<"Server: New connection from IP "<<inet_ntoa(client_addr.sin_addr)<<" on socket "<<newfd<<endl;
                    }
                    
                } 
                else {
                    // handle data from a client
                    if ((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                        // connection closed
                            cout<<"Server: Socket "<<i<<" hung up"<<endl;
                            // remove from workers if a worker
                            if (workers.find(i)!=workers.end()) {
                                workers.erase(i);                                
                            }
                            //remove from users if a user
                            if (users.find(i)!=users.end()) {
                                users.erase(i);                                
                            }
                        } 
                        else {
                            perror("recv");
                        }
                        close(i); // close connection for socket i
                        FD_CLR(i, &all_fds); // remove i from all_fds set
                    }
                    else {
                        // Received data from i
                        cout<<"Received from socket "<<i<<" : "<<buf<<endl;

                        // getting the identity of the client
                        char identity[5];
                        memcpy(identity, buf, 4);
                        identity[4] = '\0';

                        // a new worker
                        if (strcmp(identity, "work")==0) {
                            cout<<"Worker Connected on socket "<<i<<endl;
                            workers.insert(i);
                            free_workers.push(i);
                            worker2user[i] = 0;
                        }
                        // a new user
                        else if (strcmp(identity, "user")==0) {
                            cout<<"User Connected on socket "<<i<<endl;
                            users.insert(i);

                            // separating various parts in user's message
                            char hash[20], pass_len[3];
                            bool bin_str[3];
                            int j = 0;
                            for (j=5; ;j++) {
                                if (buf[j]!=' ') {
                                    hash[j-5]=buf[j];
                                }
                                else {
                                    hash[j-5]='\0';
                                    j++;
                                    break;
                                }
                            }
                            for (int k=0;;j++,k++) {
                                if (buf[j]!=' ') {
                                    pass_len[k]=buf[j];
                                }
                                else {
                                    pass_len[k]='\0';
                                    j++;
                                    break;
                                }
                            }
                            for (int k=0;;j++,k++) {
                                if (buf[j]!='\0') {
                                    bin_str[k]=buf[j]-'0';
                                }
                                else {
                                    break;
                                }
                            }

                            // determine length of the search space of characters
                            int length = 0;
                            if (bin_str[0]) {
                                length+=26;  
                            }
                            if (bin_str[1]) {
                                length+=26;
                            } 
                            if (bin_str[2]) {
                                length+=10;
                            }

                            // processed data
                            char data[100];
                            memcpy(data, &buf[5], strlen(buf)-4);
                            cout<<"Processed user "<<i<<" data is: "<<data<<endl;

                            // creating jobs by splitting work
                            string to_q, to_q_cpy(data);    // to_q goes to queue
                            int start = 0, end = 0, split = 0;
                            if (!workers.empty()) {
                                int num_workers = workers.size();
                                end = (length)/(num_workers) - 1;
                                split = end + 1;
                                int k = 0;
                                for (k=0; k<(num_workers-1); k++) {
                                    to_q = to_q_cpy;
                                    to_q.append(1,' ');
                                    to_q.append(to_string(start));
                                    to_q.append(1,' ');
                                    to_q.append(to_string(end));
                                    cout<<"Job "<<k<<": "<<to_q<<endl;
                                    jobs.push(make_pair(i, to_q));
                                    start = start + split;
                                    end = end + split;
                                }
                                end = length - 1;
                                to_q = to_q_cpy;
                                to_q.append(1,' ');
                                to_q.append(to_string(start));
                                to_q.append(1,' ');
                                to_q.append(to_string(end));
                                cout<<"Job "<<k<<": "<<to_q<<endl;
                                jobs.push(make_pair(i, to_q));
                            }


                        }
                        // got a reply
                        else if (strcmp(identity, "ans ")==0) {
                            //free the worker
                            cout<<"Got a reply from worker "<<i<<endl;
                            int user = worker2user[i];
                            worker2user[i] = 0;
                            free_workers.push(i);

                            if (buf[4]!='n') {
                                // send password to user
                                char data[strlen(buf)-3];
                                memcpy(data, &buf[4], strlen(buf)-3);
                                data[strlen(buf)-4]='\0';
                                cout<<"Sending "<<data<<"to user "<<i<<endl;
                                if (send(user, data, strlen(buf)-3, 0) == -1) {
                                    perror("send");
                                }
                                users.erase(user);
                                cout<<"Sent the password to user "<<user<<endl;

                                // stop other workers
                                for (set<int>::iterator it = workers.begin(); it!=workers.end(); it++) {
                                    if (worker2user[*it]==user) {
                                        if (send(*it, " quit\0", 6, 0) == -1) {
                                            perror("send");
                                        }
                                        worker2user[*it] = 0;
                                        free_workers.push(*it);
                                    }
                                }

                                // remove other jobs of same user
                                while(jobs.front().first==user) {
                                    jobs.pop();
                                }
                            }
                        }
                    }
                }
            }
        }

        // if there are jobs and free workers assign jobs
        while (!jobs.empty() && !free_workers.empty()) {

            int worker = 0;
            pair<int, string> work;
            worker = free_workers.front();
            free_workers.pop();

            // worker is still connected and is actually free
            if (workers.find(worker)!=workers.end() && worker2user[worker]==0) {
                work = jobs.front();
                jobs.pop();
                worker2user[worker] = work.first;
                string tostr = work.second;
                tostr.append(1, '\0');
                const char *cstr = tostr.c_str();
                cout<<cstr<<endl;
                int len = tostr.length();
                if (send(worker, cstr, len, 0) == -1) {
                    perror("send");
                }
                cout<<"Giving worker "<<worker<<" job of user "<<work.first<<endl;

            }
        }


    }

    cout<<"Bye Bye"<<endl;  //never executed!
    return 0;
}