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
#define SERVER_PORT  42069
#define MAX_PENDING  5
#define MAX_LINE     256


// Functions
std::string buildCommand(char*);
bool extractInfo(char*, std::string*, std::string);
static int callback(void*, int, char**, char**);

std::string resultant;  // any better solutions than using a global var?


/////////////////
// Main Fuction//
/////////////////

int main(int argc, char* argv[]) {
    
    // Database Variables
    sqlite3* db;
    char* zErrMsg = 0;
    int rc;
    const char* sql;
    const char* data = "Callback function called";  //Might not need in the end, as does nothing rn

    
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
        FOREIGN KEY(user_id) REFERENCES users(ID)\
    );";

    // Execute cryptos table creation
    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);


    //CHECK IF USER ALREADY EXISTS
    //IF EXISTS DO NOTHING
    //ELSE CREATE NEW USER
    //Something about this does not work properly. Returns "SQL error: UNIQUE constraint failed: users.ID"
    //Actually I think you know this, because the code changed only to insert... Or I'm seeing things.
    sql = "INSERT INTO users VALUES (1, 'cis427@gmail.com', 'John', 'Smith', 'J_Smith', 'password', 100);";
    rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);

    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else {
        fprintf(stdout, "New User Added Successfully\n");
    }

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
        else {
            std::cout << "Client connected on socket: " << nClient << std::endl;
            send(nClient, "Got the connection done successfully", 37, 0);
        }

        while ((buf_len = (recv(nClient, buf, sizeof(buf), 0)))) {
            
            std::cout << "s: Recieved: " << buf;    // This needs to be sent from the server
            command = buildCommand(buf);
            /* 
                THE BUY COMMAND:
                    1. Check if the command was used properly
                        - Return an error if not used properly
                        - Otherwise, continue
                    2. Check if the client-selected user exists in the users table
                        - Return an error if the user does not exist
                        - Otherwise, continue
                    3. Calculate the crypto transaction price
                    4. Get the user's usd balance
                    5. Check if the user can afford the transaction
                        - If they cannot, return an error
                        - Otherwise, continue
                    6. Update the user's balance in the users table
                    7. Check the cryptos table if there already exists a record of the selected crypto
                        - If there exists a record, update the record
                        - Otherwise, create a new record
                    8. The command completed successfully, return 200 OK, the new usd_balance and new crypto_balance
            */
            if (command == "BUY") {
                // Checks if the client used the command properly
                if(!extractInfo(buf, infoArr, command)) {
                    send(nClient, "403 message format error: Missing information", sizeof(buf), 0);
                }
                else {
                    //std::cout << "Recieved: " << buf << std::endl;   // Might need to move up top to like line 191/192
                    // Check if selected user exists in users table 
                    std::string selectedUsr = infoArr[3];
                    std::string sql = "SELECT IIF(EXISTS(SELECT 1 FROM users WHERE users.ID=" + selectedUsr + "), 'PRESENT', 'NOT_PRESENT') result;";
                    rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);

                    if( rc != SQLITE_OK ) {
                        fprintf(stderr, "SQL error: %s\n", zErrMsg);
                        sqlite3_free(zErrMsg);
                        send(nClient, "403 message format error", 25, 0);
                    }
                    else if (resultant == "PRESENT") {
                        // USER EXISTS
                        fprintf(stdout, "User Exists in Users Table.\n");   // Might need to remove

                        // Calculate crypto price
                        double cryptoPrice = std::stod(infoArr[1]) * std::stod(infoArr[2]);

                        // Get the usd balance of the user
                        sql = "SELECT usd_balance FROM users WHERE users.ID=" + selectedUsr;
                        rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);
                        std::string usd_balance = resultant;

                         if( rc != SQLITE_OK ) {
                            fprintf(stderr, "SQL error: %s\n", zErrMsg);
                            sqlite3_free(zErrMsg);
                            send(nClient, "403 message format error", 25, 0);
                        }
                        else if (stod(usd_balance) >= cryptoPrice) {    // User has enough in balance to make the purchase
                            // Update usd_balance with new balance
                            double difference = stod(usd_balance) - cryptoPrice;
                            std::string sql = "UPDATE users SET usd_balance=" + std::to_string(difference) + " WHERE ID =" + selectedUsr + ";";
                            rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);

                            if( rc != SQLITE_OK ) {
                                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                sqlite3_free(zErrMsg);
                                send(nClient, "403 message format error", 25, 0);
                            }

                            // Add new record or update record to crypto table
                            // Checks if record already exists in cryptos
                            sql = "SELECT IIF(EXISTS(SELECT 1 FROM cryptos WHERE cryptos.crypto_name='" + infoArr[0] + "' AND cryptos.user_id='" + selectedUsr + "'), 'RECORD_PRESENT', 'RECORD_NOT_PRESENT') result;";
                            rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);

                            if( rc != SQLITE_OK ) {
                                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                sqlite3_free(zErrMsg);
                                send(nClient, "403 message format error", 25, 0);
                            }
                            else if (resultant == "RECORD_PRESENT"){
                                // A record exists, so update the record
                                sql = "UPDATE cryptos SET crypto_balance= crypto_balance +" + infoArr[1] + " WHERE cryptos.crypto_name='" + infoArr[0] + "' AND cryptos.user_id='" + selectedUsr + "';";
                                rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);
                                if( rc != SQLITE_OK ) {
                                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                    sqlite3_free(zErrMsg);
                                    send(nClient, "403 message format error", 25, 0);
                                }
                            }
                            else {
                                // A record does not exist, so add a record
                                sql = "INSERT INTO cryptos(crypto_name, crypto_balance, user_id) VALUES ('" + infoArr[0] + "', '" + infoArr[1] + "', '" + selectedUsr + "');";
                                rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);
                                if( rc != SQLITE_OK ) {
                                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                    sqlite3_free(zErrMsg);
                                    send(nClient, "403 message format error", 25, 0);
                                }
                            }

                            // Get the new usd_balance
                            sql = "SELECT usd_balance FROM users WHERE users.ID=" + selectedUsr;
                            rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);
                            usd_balance = resultant;
                            if( rc != SQLITE_OK ) {
                                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                sqlite3_free(zErrMsg);
                                send(nClient, "403 message format error", 25, 0);
                            }

                            // Get the new crypto_balance
                            sql = "SELECT crypto_balance FROM cryptos WHERE cryptos.crypto_name='" + infoArr[0] + "' AND cryptos.user_id='" + selectedUsr + "';";
                            rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);
                            if( rc != SQLITE_OK ) {
                                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                sqlite3_free(zErrMsg);
                                send(nClient, "403 message format error", 25, 0);
                            }
                            std::string crypto_balance = resultant;

                            // The command completed successfully, return 200 OK, the new usd_balance and new crypto_balance
                            std::string tempStr = "200 OK\n   BOUGHT: New balance: " + crypto_balance + " " + infoArr[0] + ". USD balance $" + usd_balance;
                            send(nClient, tempStr.c_str(), sizeof(buf), 0);
                        }
                        else {
                            //std::cout << "Not enough balance." << std::endl;
                            send(nClient, "403 message format error: not enough USD", sizeof(buf), 0);
                        }
                    }
                    else {
                        // USER DOES NOT EXIST
                        fprintf(stdout, "User Does Not Exist in Users Table!\n");
                        std::string tempStr = "403 message format error: user " + selectedUsr + " does not exist";
                        send(nClient, tempStr.c_str(), sizeof(buf), 0);
                    }
                }
            }
             /* 
                THE SELL COMMAND:
                    1. Check if the command was used properly
                        - Return an error if not used properly. Otherwise continue
                    2. Check if the client-selected user exists in the users table
                        - Return an error if the user does not exist. Otherwise, continue
                    3. Check if the user owns the selected coin
                        - Return an error if they do not own the coin. Otherwise, continue
                    4. Check if the user has enough of the selected coin to sell
                        - Return an error if they don't. Otherwise, continue
                    5. Update the users table
                        a. Increase the user's usd balance
                    6. Update the cryptos table
                        a. Decrease the user's crypto balance
                    7. If this stage is reached, the sell command has completed successfully
                        - return 200 ok, the new cryptos balance, and the new usd balance
            */
            else if (command == "SELL") {
                // Check if the client used the command properly
                if(!extractInfo(buf, infoArr, command)) {
                    std::cout << "Invalid command: Missing information" << std::endl;   // Might need to remove
                    send(nClient, "403 message format error: Missing information", sizeof(buf), 0);
                }
                else {
                    std::cout << "Recieved: " << buf << std::endl;
                    std::string selectedUsr = infoArr[3];
                    // Check if the selected user exists in users table 
                    std::string sql = "SELECT IIF(EXISTS(SELECT 1 FROM users WHERE users.ID=" + selectedUsr + "), 'PRESENT', 'NOT_PRESENT') result;";

                    /* Execute SQL statement */
                    rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);

                    if( rc != SQLITE_OK ) {
                        fprintf(stderr, "SQL error: %s\n", zErrMsg);
                        sqlite3_free(zErrMsg);
                        send(nClient, "403 message format error", sizeof(buf), 0);
                    }
                    else if (resultant == "PRESENT") {
                        // Check if the user owns the selected coin
                        sql = "SELECT IIF(EXISTS(SELECT 1 FROM cryptos WHERE cryptos.crypto_name='" + infoArr[0] + "' AND cryptos.user_id='" + selectedUsr + "'), 'RECORD_PRESENT', 'RECORD_NOT_PRESENT') result;";
                        rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);

                        if( rc != SQLITE_OK ) {
                            fprintf(stderr, "SQL error: %s\n", zErrMsg);
                            sqlite3_free(zErrMsg);
                            send(nClient, "403 message format error", sizeof(buf), 0);
                        }
                        else if (resultant == "RECORD_NOT_PRESENT") {
                            // Return: user doesn't own the selected coin
                            send(nClient, "403 message format error: User does not own this coin.", sizeof(buf), 0);
                        }
                        else {
                            // Check if the user has enough of the selected coin to sell
                            double numCoinsToSell = std::stod(infoArr[1]);
                            // Get the number of coins the user owns of the selected coin
                            sql = "SELECT crypto_balance FROM cryptos WHERE cryptos.crypto_name='" + infoArr[0] + "' AND cryptos.user_id='" + selectedUsr + "';";
                            rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);

                            if( rc != SQLITE_OK ) {
                                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                sqlite3_free(zErrMsg);
                                send(nClient, "403 message format error", sizeof(buf), 0);
                            }

                            double crypto_balance = std::stod(resultant);
                            // Not enough coins in balance to sell
                            if (crypto_balance < numCoinsToSell) {
                                send(nClient, "403 message format error: Attempting to sell more coins than the user has.", sizeof(buf), 0);
                            }
                            else {
                                // Get dollar amount to sell
                                double cryptoPrice = std::stod(infoArr[1]) * std::stod(infoArr[2]);

                                /* Update users table */
                                // Add new amount to user's balance
                                sql = "UPDATE users SET usd_balance= usd_balance +" + std::to_string(cryptoPrice) + " WHERE users.ID='" + selectedUsr + "';";
                                rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);

                                if( rc != SQLITE_OK ) {
                                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                    sqlite3_free(zErrMsg);
                                    send(nClient, "403 message format error", sizeof(buf), 0);
                                }

                                /* Update cryptos table */
                                // Remove the sold coins from cryptos
                                sql = "UPDATE cryptos SET crypto_balance= crypto_balance -" + std::to_string(numCoinsToSell) + " WHERE cryptos.crypto_name='" + infoArr[0] + "' AND cryptos.user_id='" + selectedUsr + "';";
                                rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);

                                if( rc != SQLITE_OK ) {
                                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                                    sqlite3_free(zErrMsg);
                                    send(nClient, "403 message format error", sizeof(buf), 0);
                                }

                                // Question: if the new balance is zero, should we delete the record in cryptos?

                                // Get new usd_balance
                                sql = "SELECT usd_balance FROM users WHERE users.ID=" + selectedUsr;
                                rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);
                                std::string usd_balance = resultant;

                                // Get new crypto_balance
                                sql = "SELECT crypto_balance FROM cryptos WHERE cryptos.crypto_name='" + infoArr[0] + "' AND cryptos.user_id='" + selectedUsr + "';";
                                rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);
                                std::string crypto_balance = resultant;

                                // Sell command completed successfully
                                std::string tempStr = "200 OK\n   SOLD: New balance: " + crypto_balance + " " +  infoArr[0] + ". USD $" + usd_balance;
                                send(nClient, tempStr.c_str(), sizeof(buf), 0);
                            }
                        }
                    }
                    else {
                        fprintf(stdout, "User Does Not Exist  in Users Table.\nCannot Process Sell Command.\n");   // Might need to remove
                        send(nClient, "403 message format error: user does not exist.", sizeof(buf), 0);
                    }
                }
            }
            else if (command == "LIST") {
                std::cout << "List command." << std::endl;
                resultant = "";
                // List all records in cryptos table for user_id = 1
                std::string sql = "SELECT * FROM cryptos WHERE cryptos.user_id=1";

                /* Execute SQL statement */
                rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);

                if( rc != SQLITE_OK ) {
                    // user does not exist
                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                    sqlite3_free(zErrMsg);
                    send(nClient, "403 message format error", sizeof(buf), 0);
                }

                std::string sendStr = "200 OK\n   The list of records in the Crypto database for user 1:\n   " + resultant;
                send(nClient, sendStr.c_str(), sizeof(buf), 0);
            }
            else if (command == "BALANCE") {
                std::cout << "Balance command." << std::endl;
                // check if user exists
                std::string sql = "SELECT IIF(EXISTS(SELECT 1 FROM users WHERE users.ID=1), 'PRESENT', 'NOT_PRESENT') result;";

                /* Execute SQL statement */
                rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);

                if( rc != SQLITE_OK ) {
                    // user does not exist
                    fprintf(stderr, "SQL error: %s\n", zErrMsg);
                    sqlite3_free(zErrMsg);
                    send(nClient, "403 message format error", sizeof(buf), 0);
                }
                else if (resultant == "PRESENT") {
                    // outputs balance for user 1
                    sql = "SELECT usd_balance FROM users WHERE users.ID=1";
                    rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);
                    std::string usd_balance = resultant;

                    // get full user name
                    sql = "SELECT first_name FROM users WHERE users.ID=1";
                    rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);
                    std::string user_name = resultant;

                    sql = "SELECT last_name FROM users WHERE users.ID=1";
                    rc = sqlite3_exec(db, sql.c_str(), callback, (void*)data, &zErrMsg);
                    user_name += " " + resultant;

                    std::string tempStr = "200 OK\n   Balance for user " + user_name + ": $" + usd_balance;
                    send(nClient, tempStr.c_str(), sizeof(buf), 0);
                }
                else {
                    send(nClient, "User does not exist.", sizeof(buf), 0);
                }
            }
            else if (command == "SHUTDOWN") {
                send(nClient, "200 OK", 7, 0);
                std::cout << "Shutdown command." << std::endl;
                sqlite3_close(db);
                std::cout << "Closed DB" << std::endl;
                close(nClient);
                std::cout << "Closed Client Connection: " << nClient << std::endl;
                close(nSocket);
                std::cout << "Closed Server socket: " << nSocket << std::endl;
                exit(EXIT_SUCCESS);
            }
            else if (command == "QUIT") {
                // Quit might only need to be handled client side?
                send(nClient, "200 OK", 7, 0);
                std::cout << "Quit command." << std::endl;
                close(nClient);
                break;
            }
            else {
                std::cout << "Command not recognized" << std::endl;
                send(nClient, "400 invalid command", 20, 0);
            }           
        }
    }
    close(nClient);
}

std::string buildCommand(char line[]) {
    std::string command = "";
    //std::cout << "entered function" << std::endl;
    size_t len = strlen(line);
    for (size_t i = 0; i < len; i++) {
        //std::cout << "looping" << std::endl;
        if (line[i] == '\n')
            continue;
        if (line[i] == ' ')
            break;
        command += line[i];
    }
    //std::cout << "out of function loop" << std::endl;
    return command;
}

bool extractInfo(char line[], std::string info[], std::string command) {
    //std::cout << "entered info loop" << std::endl;
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
            if (i > 0) {
                if (((int)line[j] > 57 || (int)line[j] < 46) && (int)line[j] != 47)
                    return false;
            }
        }
        if (info[i] == "") {
            std::fill_n(info, 4, 0);
            return false;
        }

        spaceLocation += info[i].length() + 1;
    }
    /*if ((int)info[1].substr(0, 1)[0] > 57 || (int)info[2].substr(0, 1)[0] > 57 || (int)info[3].substr(0, 1)[0] > 57)
        return false;
    if ((int)info[1].substr(0, 1)[0] < 48 || (int)info[2].substr(0, 1)[0] < 48 || (int)info[3].substr(0, 1)[0] < 48)
        return false;*/
    return true;
}

static int callback(void* NotUsed, int argc, char** argv, char** azColName) {
    if (argc == 1) {
        resultant = argv[0];
        return 0;
    }

    // mainly for the LIST command
    for (int i = 0; i < argc; i++) {
        //std::cout << argv[i] << std::endl;
        if (resultant == "")
            resultant = argv[i];
        else
            resultant = resultant + " " + argv[i];
            
        // new line btwn every record
        if (i == 3)
            resultant += "\n  ";
    }

    return 0;
}
