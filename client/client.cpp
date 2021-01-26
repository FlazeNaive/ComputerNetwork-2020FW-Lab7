#include <iostream>
#include <winsock2.h>
#include <thread>
#include <signal.h>
#pragma comment(lib, "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
using namespace std;

const int MAXMES = 1024 ;

enum REQ_TYPE {DISCON, TIME, NAME, LINK, SEND};
string REQ_STR[] = { "DISCON", "TIME", "NAME", "LINK", "SEND" };

// MESSAGE's definition is in server.cpp
struct MESSAGE {
	int DES;
	REQ_TYPE OPT;
	char MES[MAXMES];
};

void printMenu() {
	cout << "-----------------------------------------------------" << endl ;
	cout << "Actions:\n" ;
	cout << "1: end connection" << endl ;
	cout << "2: get time" << endl ;
	cout << "3: get name" << endl ;
	cout << "4: get active links" << endl ;
	cout << "5: send message" << endl ;
	cout << "6: exit" << endl ;
}

int cnt_pack = 0 ;
// Here I didn't use lambda expression.. to make it easier to understand
void receiveT(SOCKET client) {
    char buf[sizeof(MESSAGE)];
    MESSAGE sen ;

    while ( 1 ) {
        // Message Receiving 

	int reRet = recv(client, buf, sizeof(buf), 0);
	if (reRet == -1) {
		cout << "Receiving failed:"<< errno << endl;
		exit(0);
	}
	++cnt_pack ;
	memcpy(&sen, buf, sizeof(buf)) ;
	 
        // Display received message
	cout << "-----------------------------------------------------" << endl ;
	cout << "Pack No. " << cnt_pack << endl ;
	cout << "Receive:\nType = " << REQ_STR[sen.OPT] << "\nDES = " << sen.DES << "\nMES = \n" << sen.MES << endl<< endl ;
	if(sen.OPT == DISCON) {
		cout << "Disconnecting..." << endl ;
		exit(0);
	}

	printMenu() ;
    }
}

sockaddr_in addr;
int conRet;
int client;

// To release the linkage while using Ctrl+C to exit
void sigint_handler(int sig) {
	if(sig == SIGINT){
		MESSAGE sen = { -1, DISCON, "" };
		cout << "Closing client and disconnecting..." << endl;

		char buf[sizeof(MESSAGE)] ;
		memcpy(buf, &sen, sizeof(MESSAGE)) ;
		int seRet = send(client, buf, sizeof(buf), 0);
		if (seRet == -1) {
			cout << "Sending failed:" << errno << endl;
		}
		seRet = send(client, buf, sizeof(buf), 0);
		if (seRet == -1) {
			cout << "Sending failed:" << errno << endl;
		}
	}
}

int main() {
	// initializing 
	signal(SIGINT, sigint_handler);

	WSADATA ws;
	WSAStartup(MAKEWORD(2, 2), &ws);

	client = socket(AF_INET, SOCK_STREAM, 0);
	if (client == -1) {
		cout << "Initializing failed:" << errno << endl;
		return 0 ;
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(5438);

	conRet = connect(client, (sockaddr*)& addr, sizeof(addr));
	if (conRet == -1) {
		cout << "Connecting failed:" << errno << endl;
		return 0 ;
	}

	cout << "Connected" << endl;

	// listen from client all the time
	thread msgListener(receiveT, client);
	msgListener.detach();

	printMenu() ;

	while(1) {
		// initialize the MESSAGE being sent
		MESSAGE sen ;
		sen = MESSAGE{
			-1, NAME, "" 
		} ;

		int opt ;
		string Text ;
		int des_num ;
		cin >> opt ;
		switch (opt) {
			case 1:
				sen.OPT = DISCON ;
				break ;
			case 2:
				sen.OPT = TIME ;
				break ;
			case 3:
				sen.OPT = NAME ;
				break ;
			case 4:
				sen.OPT = LINK ;
				break ;
			case 5:
				// only the type of sending message to others need to be treated differently
				sen.OPT = SEND ;
				cout << "To. " ;
				cin >> des_num ;
				sen.DES = des_num ;
				cout << "Input the text need to send:\n" ;
				cin >> Text ;
				strcpy(sen.MES, Text.c_str()) ;
				break ;
			case 6:
				cout << "Quiting..." << endl ;
				return 0 ;
			default:
				cout << "Wrong option!" << endl ;
				break ;
		}

		if ( opt < 1 || opt > 6 )
			continue ;

		cout << "Sending:\nType = " << REQ_STR[sen.OPT] << "\nDES = " << sen.DES << "\nMES = \n" << sen.MES << endl<< endl ;

		// sending the packet
		char buf[sizeof(MESSAGE)] ;
		memcpy(buf, &sen, sizeof(MESSAGE)) ;
		int seRet = send(client, buf, sizeof(buf), 0);
		if (seRet == -1) {
			cout << "Sending failed:" << errno << endl;
		}
	
	}
	return 0 ;
}
