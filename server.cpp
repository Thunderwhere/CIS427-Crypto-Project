// Standard C++ headers
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


// Server Port/Socket/Addr related headers
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>


// SQLite3 related headers/definitions
#include "sqlite3.h"
#define SERVER_PORT  9909
#define MAX_PENDING  5
#define MAX_LINE     256


// Functions
std::string buildCommand(char*);
bool extractInfo(char*, std::string*, std::string);
static int callback(void*, int, char**, char**);




/////////////////
// Main Fuction//
/////////////////

int main(int argc, char* argv[]) {
    
    // Database Variables
    sqlite3* db;
    char* zErrMsg = 0;
    int rc;
    const char* sql;
    
    // Open Database and Connect to Database
    rc = sqlite3_open("cis427_crypto.sqlite", &db);

    // Check if Database was opened successfully
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return(0);
    } else {
        fprintf(stderr, "Opened database successfully\n");
    }

    // Create sql users table creation command
    sql = "create table if not exists users(\
        ID int NOT NULL,\
        first_name varchar(255),\
        last_name varchar(255),\
        user_name varchar(255) NOT NULL,\
        password varchar(255),\
        usd_balance DOUBLE NOT NULL,\
        PRIMARY KEY(ID)\
    );";

    // Execute users table creation
    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);

    // Create sql cryptos table creation command
    sql = "create table if not exists cryptos (\
        ID int PRIMARY KEY NOT NULL,\
        crypto_name varchar(10) NOT NULL,\
        crypro_balance DOUBLE,\
        user_id int,\
        FOREIGN KEY(user_id) REFERENCES Users (ID)\
    );";

    // Execute cryptos table creation
    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);


    //CHECK IF USER ALREADY EXISTS
    //IF EXISTS DO NOTHING
    //ELSE CREATE NEW USER


    // Server Variables
    struct sockaddr_in srv;
    char buf[MAX_LINE];
    socklen_t buf_len, addr_len;
    int nRet;
    int nClient;
    int nSocket;
    std::string infoArr[4];
    std::string command = "";


    // Setup passive open // Initialize the socket
    nSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (nSocket < 0) {
        std::cout << "Socket not Opened\n";
        sqlite3_close(db);
        std::cout << "Closed DB" << std::endl;
        exit(EXIT_FAILURE);
    }
    else {
        std::cout << "Socket Opened: " << nSocket << std::endl;
    }


    // Build address data structure
    srv.sin_family = AF_INET;
    srv.sin_port = htons(SERVER_PORT);
    srv.sin_addr.s_addr = INADDR_ANY;
    memset(&(srv.sin_zero), 0, 8);
    

    // Set Socket Options
    int nOptVal = 0;                          
    int nOptLen = sizeof(nOptVal);            
    nRet = setsockopt(nSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&nOptVal, nOptLen);
    if (!nRet) {
        std::cout << "The setsockopt call successful\n";
    }
    else {
        std::cout << "Failed setsockopt call\n";
        sqlite3_close(db);
        std::cout << "Closed DB" << std::endl;
        close(nSocket);
        std::cout << "Closed socket: " << nSocket << std::endl;
        exit(EXIT_FAILURE);
    }


    //Bind the socket to the local port
    nRet = (bind(nSocket, (struct sockaddr*)&srv, sizeof(srv)));
    if (nRet < 0) {
        std::cout << "Failed to bind to local port\n";
        sqlite3_close(db);
        std::cout << "Closed DB" << std::endl;
        close(nSocket);
        std::cout << "Closed socket: " << nSocket << std::endl;
        exit(EXIT_FAILURE);
    }
    else {
        std::cout << "Successfully bound to local port\n";
    }
    
        
    //Listen to the request from client
    nRet = listen(nSocket, MAX_PENDING);
    if (nRet < 0) {
        std::cout << "Failed to start listen to local port\n";
        sqlite3_close(db);
        std::cout << "Closed DB" << std::endl;
        close(nSocket);
        std::cout << "Closed socket: " << nSocket << std::endl;
        exit(EXIT_FAILURE);
    }
    else {
        std::cout << "Started listening to local port\n";
    }


    /* wait for connection, then receive and print text */
    while (1) {

        if ((nClient = accept(nSocket, (struct sockaddr*)&srv, &addr_len)) < 0) {
            perror("simplex-talk: accept");
            exit(1);
        }


        //char sellMessage[MAX_LINE] = "You used the Sell command";

        while (buf_len = recv(nClient, buf, sizeof(buf), 0)) {
            fputs(buf, stdout);

            command = buildCommand(buf);

            if (command == "BUY") {
                if (extractInfo(buf, infoArr, command))
                    std::cout << "Buy command. Crypto type: " << infoArr[0] << " Amount of Crypto: " << infoArr[1] << " Price per Unit: " << infoArr[2] << " User: " << infoArr[3] << std::endl;
                else
                    std::cout << "Invalid command: Missing information" << std::endl;
            }
            else if (command == "SELL") {
                if (extractInfo(buf, infoArr, command))
                    std::cout << "Sell command. Crypto type: " << infoArr[0] << " Amount of Crypto: " << infoArr[1] << " Price per Unit: " << infoArr[2] << " User: " << infoArr[3] << std::endl;
                else
                    std::cout << "Invalid command: Missing information" << std::endl;
            }
            else if (command == "LIST") {
                std::cout << "List command." << std::endl;
            }
            else if (command == "BALANCE") {
                std::cout << "Balance command." << std::endl;
            }
            else if (command == "SHUTDOWN") {
                std::cout << "Shutdown command." << std::endl;
                sqlite3_close(db);
                std::cout << "Closed DB" << std::endl;
                close(nClient);
                std::cout << "Closed Client Connection: " << nClient << std::endl;
                close(nSocket);
                std::cout << "Closed Server socket: " << nSocket << std::endl;
                exit(EXIT_FAILURE);
            }
            else if (command == "QUIT") {
                std::cout << "Quit command." << std::endl;
                close(nClient);
                break;
            }
            else {
                std::cout << "Command not recognized" << std::endl;
            }
                      
        }

    }
    close(nClient);
}




std::string buildCommand(char line[]) {
    std::string command = "";
    std::cout << "entered function" << std::endl;
    size_t len = strlen(line);
    for (size_t i = 0; i < len; i++) {
        std::cout << "looping" << std::endl;
        if (line[i] == '\n')
            continue;
        if (line[i] == ' ')
            break;
        command += line[i];
    }
    std::cout << "out of function loop" << std::endl;
    return command;
}




bool extractInfo(char line[], std::string info[], std::string command) {
    std::cout << "entered info loop" << std::endl;
    int l = command.length();
    int spaceLocation = l + 1;

    for (int i = 0; i < 4; i++) {
        info[i] = "";
        for (int j = spaceLocation; j < strlen(line); j++) {
            if (line[j] == ' ')
                break;
            if (line[j] == '\n')
                break;
            info[i] += line[j];
        }
        if (info[i] == "") {
            std::fill_n(info, 4, 0);
            return false;
        }

        spaceLocation += info[i].length() + 1;
        std::cout << info[i] << std::endl;
    }
    return true;
}




static int callback(void* NotUsed, int argc, char** argv, char** azColName) {
    int i;
    for (i = 0; i < argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}
