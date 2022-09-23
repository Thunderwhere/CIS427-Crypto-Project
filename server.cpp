#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>

#define SERVER_PORT  9909
#define MAX_PENDING  5
#define MAX_LINE     256

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

int
main()
{
  struct sockaddr_in sin;
  char buf[MAX_LINE];
  socklen_t buf_len, addr_len;
  int s, new_s;

  std::string command = "";

  /* build address data structure */
  bzero((char *)&sin, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(SERVER_PORT);

  /* setup passive open */
  if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("simplex-talk: socket");
    exit(1);
  }
  if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
    perror("simplex-talk: bind");
    exit(1);
  }
  listen(s, MAX_PENDING);

 /* wait for connection, then receive and print text */
  while(1) {
    if ((new_s = accept(s, (struct sockaddr *)&sin, &addr_len)) < 0) {
      perror("simplex-talk: accept");
      exit(1);
    }

    char sellMessage[MAX_LINE] = "You used the Sell command";

    while (buf_len = recv(new_s, buf, sizeof(buf), 0)) {
        fputs(buf, stdout);

        command = buildCommand(buf);

        if (command == "BUY") {
            std::cout << "Buy command." << std::endl;
        }
        else if (command == "SELL") {
            std::cout << "Sell command." << std::endl;

        }
        else if (command == "LIST") {
            std::cout << "List command." << std::endl;
        }
        else if (command == "BALANCE") {
            std::cout << "Balance command." << std::endl;
        }
        else if (command == "SHUTDOWN") {
            std::cout << "Shutdown command." << std::endl;
            close(new_s);
            exit(EXIT_FAILURE);
        }
        else if (command == "QUIT") {
            std::cout << "Quit command." << std::endl;
        }
        else {
            std::cout << "Command not recognized" << std::endl;
        }
    }

    close(new_s);
  }
}
