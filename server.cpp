#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sqlite3.h> 

#define SERVER_PORT  1879
#define MAX_PENDING  5
#define MAX_LINE     256

static int callback(void *data, int argc, char **argv, char **azColName) {
        int i;
        fprintf(stderr, "%s: ", (const char*)data);
        
        for(i = 0; i<argc; i++) {
                printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
        }
        printf("\n");
        return 0;
}

int main()
{
        struct sockaddr_in sin;
        char buf[MAX_LINE];
        socklen_t buf_len, addr_len;
        int s, new_s;

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

        /* check if at least one user is in db, if not create one*/
        sqlite3 *db;
        char *zErrMsg = 0;
        int rc;
        char *sql;
        const char* data = "Callback function called";

        /* Open database */
        rc = sqlite3_open("cis427_crypto.sqlite", &db);
        
        if(rc) {
                fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
                return(0);
        } 
        else {
                fprintf(stderr, "Opened database successfully\n");
        }

        /* Create SQL statement */
        /* Explanation:
                if(users is empty)
                        results in an error
        */
        sql = "INSERT INTO users VALUES (1, 'cis427@gmail.com', 'John', 'Smith', 'J_Smith', 'password', 100)"; // check if table is empty, i.e. no entries

        /* Execute SQL statement */
        rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);

        if( rc != SQLITE_OK ){
                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                sqlite3_free(zErrMsg);
        }
        else {
                fprintf(stdout, "User Added Successfully\n");
        }
        sqlite3_close(db);

        /* wait for connection, then receive and print text */
        while(1) {
                if ((new_s = accept(s, (struct sockaddr *)&sin, &addr_len)) < 0) {
                        perror("simplex-talk: accept");
                        exit(1);
                }
                while (buf_len = recv(new_s, buf, sizeof(buf), 0)) {
                        printf("s: ");
                        fputs(buf, stdout);
                }
                close(new_s);
        }
}
