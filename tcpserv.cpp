#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

const int MAXINBUF  = 10;
const int MAXQUEUE = 5;
const char join[10] = {'#','J','O','I','N','\0','\0','\0','\0','\0'};
const char leave[10] = {'#','L','E','A','V','E','\0','\0','\0','\0'};
const char list[10] = {'#','L','I','S','T','\0','\0','\0','\0','\0'};
const char log[10] = {'#','L','O','G','\0','\0','\0','\0','\0','\0'};
const char ok[3] = {'#','O','K'};
const char AM[15] = {'#','A','L','R','E','A','D','Y',' ','M','E','M','B','E','R'};
const char NM[11] = {'#','N','O','T',' ','M','E','M','B','E','R'};

struct clients{
  time_t joined;
  unsigned long address;
  std::string ad;
};

int main(){
  // create a server socket
  int servSocket;

  std::ofstream fout;
  std::ifstream fin;
  fout.open("log.txt");
  fout.close();
  fout.close();

  //open server socket
  servSocket=socket(PF_INET,SOCK_STREAM,0);
  if(servSocket<0 ){
  	printf("socket open failed.\n"); 
  	return(1); 
  }

  // set address of server to use any available IP interface
  struct sockaddr_in servAddress;
  servAddress.sin_family=AF_INET;
	servAddress.sin_addr.s_addr=INADDR_ANY;
	servAddress.sin_port=htons(30000);

	//attempt bind
	if(bind(servSocket, (struct sockaddr *)&servAddress, sizeof(struct sockaddr_in))){
		printf("Bind address failed.\n"); 
		return(1); 
	}
  
  //attempt listen
  if(listen(servSocket,MAXQUEUE) < 0){ //sets listening queue to size 5
  	printf("socket listen failed.\n"); 
  	return(1); 
  }

  // start accept clients
  struct sockaddr_in client;
  int clientlen=sizeof(struct sockaddr_in);
  //initiaizing variables for use in loop
  std::vector<clients> cli;
  int cliSocket;
  char buffer[MAXINBUF];
  std::string str, //str is used as a 1 time use string and is strictly temporary. 
              buff; //buff is a string object of the buffer value
  time_t now;

  while(true){
  fout.open("log.txt", std::ios::app);
  	// receive a connection request
  	cliSocket = accept(servSocket,(struct sockaddr *)&client, (socklen_t*)&clientlen);
  	if (cliSocket<0){
  		printf("client connection failed.\n");
  	}
    recv(cliSocket, buffer, sizeof(buffer), 0);
    buff = buffer;

    if(buffer == (str = join)){
      now = time(0);
      str = ctime(&now);
      str.pop_back();
      fout << str << ": Received a “#JOIN” action from agent \"" << inet_ntoa(client.sin_addr) << "\"" << std::endl;
      bool newmember = true;
      for(int i=0; i < cli.size() ;i++){
        if(client.sin_addr.s_addr == cli[i].address){
          newmember=false;
        }
      }
      if(newmember){
        clients temp;
        temp.joined = time(0);
        temp.address = client.sin_addr.s_addr;
        temp.ad = inet_ntoa(client.sin_addr);
        cli.push_back(temp);
        send(cliSocket,ok,sizeof(ok),0);
        now = time(0);
        str = ctime(&now);
        str.pop_back();
        fout << str << ": Responded to agent " << inet_ntoa(client.sin_addr) << " with \"$OK\"" << std::endl;
      }
      else{
        send(cliSocket,AM,sizeof(AM),0);
        now = time(0);
        str = ctime(&now);
        str.pop_back();
        fout << str << ": Responded to agent " << inet_ntoa(client.sin_addr) << " with \"$ALREADY  MEMBER\"" << std::endl;
      }
    }
    else if(buffer == (str = leave)){
      now = time(0);
      str = ctime(&now);
      str.pop_back();
      fout << str << ": Received a “#LEAVE” action from agent \"" << inet_ntoa(client.sin_addr) << "\"" << std::endl;
      bool ismember = false;
      int index;
      for(int i=0; i < cli.size() ;i++){
        if(client.sin_addr.s_addr == cli[i].address){
          index = i;
          ismember=true;
        }
      }
      if(ismember){
        cli.erase(cli.begin() + index);
        send(cliSocket,ok,sizeof(ok),0);
        now = time(0);
        str = ctime(&now);
        str.pop_back();
        fout << str << ": Responded to agent " << inet_ntoa(client.sin_addr) << " with \"$OK\"" << std::endl;
      }
      else{
        send(cliSocket,NM,sizeof(NM),0);
        now = time(0);
        str = ctime(&now);
        str.pop_back();
        fout << str << ": Responded to agent " << inet_ntoa(client.sin_addr) << " with \"$NOT MEMBER\"" << std::endl;
      }
    }
    else if(buffer == (str = list)){
      now = time(0);
      str = ctime(&now);
      str.pop_back();
      fout << str << ": Received a “#LIST” action from agent \"" << inet_ntoa(client.sin_addr) << "\"" << std::endl;
      bool ismember = false;
      for(int i=0; i < cli.size() ;i++){
        if(client.sin_addr.s_addr == cli[i].address){
          ismember=true;
        }
      }
      if(ismember){
        std::string tosend = "<";
        for(int i=0; i < cli.size(); i++){
          tosend += cli[i].ad;
          tosend += ",";
          tosend += std::to_string(difftime(time(0),cli[i].joined));
          tosend += ">";
          send(cliSocket,tosend.c_str(),tosend.size(),0);
          tosend = "<";
          now = time(0);
          str = ctime(&now);
          str.pop_back();
          fout << str << ": Responded to agent " << inet_ntoa(client.sin_addr) << " with list of active agents" << std::endl;
        }
      }
    }
    else if(buffer == (str = log)){
      now = time(0);
      str = ctime(&now);
      str.pop_back();
      fout << str << ": Received a “#LOG” action from agent \"" << inet_ntoa(client.sin_addr) << "\"" << std::endl;
      bool ismember = false;
      std::string tosend;
      for(int i=0; i < cli.size() ;i++){
        if(client.sin_addr.s_addr == cli[i].address){
          ismember=true;
        }
      }
      if(ismember){
        std::string temp;
        now = time(0);
        str = ctime(&now);
        str.pop_back();
        fout << str << ": Responded to agent " << inet_ntoa(client.sin_addr) << " with current log" << std::endl;
        fout.close();
        fin.open("log.txt");
        while(getline(fin,temp)){
          temp += "\n";
          send(cliSocket,temp.c_str(),temp.size(),0);
        }
        fin.close();
      }
    }
    else{
      std::cout << "nonvalid input received" << std::endl;
    }
    for(int i = 0; i < sizeof(buffer);i++){
      buffer[i] = '\0';
    }
    fout.close();
  }
}