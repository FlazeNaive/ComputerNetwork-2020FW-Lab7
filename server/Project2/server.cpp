#include<iostream>
#include<map>
#include<vector>
#include<algorithm>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<string>
#include<thread>
#pragma comment(lib, "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
using namespace std;

const int PORT = 5438;
const int MAXMES = 1024;

enum REQ_TYPE {DISCON, TIME, NAME, LINK, SEND};
string REQ_STR[] = { "DISCON", "TIME", "NAME", "LINK", "SEND" };

/* ----------------------------------------------------------
 * MESSAGE PACKAGE:
 * DES - socket id for destination
 * OPT - type of packet shown above
 * MES[MAXMES] - message, maximum length is 1023
------------------------------------------------------------*/
struct MESSAGE {
	int DES;
	REQ_TYPE OPT;
	char MES[MAXMES];
};
/* ----------------------------------------------------------
 * CLIENT, used for record active linkages
 * port and ip are for port and ip respectively
 * 
 * Clients_info[ socket_id ] = { port, ip }
------------------------------------------------------------*/
struct CLIENT {
	int port;
	string ip;
};
map<int, CLIENT> Clients_info;


int main() {
	ios::sync_with_stdio(false);
	cout << "Initializing..." << endl;

	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0)
	{
		return 0;
	}

	int slisten = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (!~slisten ) {
		cout << "Invalid socket!!" << endl;
		return 0;
	}

	// setting ip
	struct sockaddr_in sin;
	string ip;
	ip = "127.0.0.1";
	inet_pton(AF_INET, ip.c_str(), (void*)&sin.sin_addr.S_un.S_addr);

	// setting the properties of socket
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);
	memset(&(sin.sin_zero), '\0', 8);

	// bind the linkage
	if (bind(slisten, (LPSOCKADDR)&sin, sizeof(sin)) == -1) {
		cout << "Bind ip error!\n";
		return 0;
	}

	if (listen(slisten, 10) == SOCKET_ERROR) {
		cout << "Error in listening!\n";
		return 0;
	}
	cout << "Server initialization done.\n";

	// Clients maps socket_id to sin
	// Flags are used to store the status of link (whether disconnected)
	int addr_size = sizeof(sin);
	map<int, struct sockaddr_in> Clients;
	map<int, bool> Flags;

	while (true) {
		int client = accept(slisten, (sockaddr*)&sin, &addr_size);
		if (client == -1) {
			cout << "Receive error: " << errno << endl;
			break;
		}

		// lambda expression used when using thread
		thread t([&](sockaddr_in addr, int client) {
			char TMP_CLI[255];
			// Get ip from hex
			inet_ntop(AF_INET, (void*)&sin.sin_addr, TMP_CLI, 16);
			Clients[client] = sin;
			Clients_info[client].ip = TMP_CLI;
			Clients_info[client].port = sin.sin_port;

			cout << "CLIENT:\nID =  " << client << "\nip = " << Clients_info[client].ip << "\nport = " << Clients_info[client].port << endl << endl;

			// Initialize Flas as TRUE
			Flags[client] = 1;

			while (Flags[client]) {

				// Receive the packet
				char buf[sizeof(MESSAGE)];
				int reRet = recv(client, buf, sizeof(buf), 0);
				if (reRet == -1) {
					Flags[client] = 0;
					Clients_info.erase(client);
					cout << "receive failed:" << errno << endl;
					break;
				}
				cout << "Connection Request from Client: " << to_string(client) << " Recevied" << endl;

				// interpret the packet
				MESSAGE rec, sen;
				memcpy(&rec, buf, sizeof(MESSAGE));
				memset(sen.MES, 0, sizeof(sen.MES));
				cout << "Received:\nType = " << REQ_STR[rec.OPT] << "\nDES = " << rec.DES << "\nMES = " << rec.MES << endl << endl;
				sen.DES = client;
				sen.OPT = rec.OPT;

				time_t cc;
				string tmp_time;
				char name[255];
				unsigned long size = 255;
				string tmp_link;
				int ret;
				string tmp_text;

				// Process it according to its type
				switch (rec.OPT) {
				case DISCON:
					Flags[client] = 0;
					Clients_info.erase(client);
					cout << "Disconnected link with " << client << endl;
					break;
				case TIME:
					cout << "Send time to " << client << endl;
					cc = time(0);
					tmp_time = ctime(&cc);
					strcpy(rec.MES, tmp_time.c_str());
					/*
					for (int j = 0; j < 100; ++j) {
						cc = time(0);
						tmp_time = ctime(&cc);
						strcpy(rec.MES, tmp_time.c_str());
						if (rec.OPT >= DISCON && rec.OPT <= SEND) {
							sen.DES = client;
							cout << "Replying..." << endl;
							tmp_text = rec.MES;
							tmp_text = "Reply from server:\n" + tmp_text + "\nYour request is processed\n";
							strcpy(sen.MES, tmp_text.c_str());
							ret = send(client, (char*)&sen, sizeof(sen), 0);
							if (ret < 0)
								cout << "Send Failed" << endl;
							else
								cout << "Done." << endl;

						}
						Sleep(10);
					}
					*/
					break;
				case NAME:
					cout << "Send name to " << client << endl;
					gethostname(name, size);
					strcpy(rec.MES, name);
					break;
				case LINK:
					cout << "Send current active links to " << client << endl;
					tmp_link = "\tID\tIP\tPort\n";
					for (auto i : Clients_info)
						tmp_link = tmp_link + "\t" + to_string(i.first) + "\t" + i.second.ip + "\t" + to_string(i.second.port) + "\n";
					strcpy(rec.MES, tmp_link.c_str());

					break;
				case SEND:
					cout << "Send message from " << client << " to " << rec.DES << endl;
					tmp_text = rec.MES;
					tmp_text = "message from [" + to_string(client) + "]: \n" + tmp_text;
					memset(sen.MES, 0, sizeof(sen.MES));
					strcpy(sen.MES, tmp_text.c_str());
					sen.DES = rec.DES;

					cout << "Start sending..." << endl;
					ret = send(sen.DES, (char*)&sen, sizeof(sen), 0);
					if (ret < 0) {
						cout << "send failed" << endl;
						strcpy(rec.MES, "Sending Failed");
					}

					break;
				default:
					cout << "Wrong requirement type!" << endl;
				}
				cout << "Send:\nType = " << REQ_STR[sen.OPT] << "\nDES = " << sen.DES << "\nMES = " << sen.MES << endl<< endl ;

				// Always reply the sender after processing the packet
				if (rec.OPT >= DISCON && rec.OPT <= SEND) {
					sen.DES = client;
					cout << "Replying..." << endl;
					tmp_text = rec.MES;
					tmp_text = "Reply from server:\n" + tmp_text + "\nYour request is processed\n";
					strcpy(sen.MES, tmp_text.c_str());
					ret = send(client, (char*)&sen, sizeof(sen), 0);
					if (ret < 0)
						cout << "Send Failed" << endl;
					else
						cout << "Done." << endl;

				}
			}

		}, sin, client);
		t.detach();
	}


}
