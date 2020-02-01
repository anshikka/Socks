#define _BSD_SOURCE

#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <zconf.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "../include/protocol.h"

FILE *logfp;
FILE *mapper;
FILE *commands;

typedef struct count_struct
{
    char letter;
    int count;
} letter_count;

void createLogFile(void)
{
    pid_t p = fork();
    if (p == 0)
        execl("/bin/rm", "rm", "-rf", "log", NULL);

    wait(NULL);
    mkdir("log", ACCESSPERMS);
    logfp = fopen("log/log_client.txt", "w");
}

void writeToLog(char *message)
{
    char messageBuf[256]; // write message to log up to 256 chars
    messageBuf[0] = '\0';
    strcat(messageBuf, message);
    fputs(messageBuf, logfp); // add message to log file
}

int setupTCP(char *ip, int port, int mapperId)
{

    // Create a TCP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    // Specify an address to connect to
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = inet_addr(ip);
    if (connect(sockfd, (struct sockaddr *)&address, sizeof(address)) == 0)
    {
        char connectStatus[128];
        sprintf(connectStatus, "[%d] open connection\n", mapperId);
        writeToLog(connectStatus);
        return sockfd;
    }
    else
    {
        printf("Connection failed! Tried ip: %s and port: %d\n", ip, port);
    }
    return -1;
}
// initialize struct with letters and counts = 0
void init_counts(letter_count counts[])
{
    for (int i = 0; i < 26; i++)
    {
        counts[i].letter = 65 + i;
        counts[i].count = 0;
    }
}

/* Remove newline character if found */
void remove_newline_character(char *token)
{
    int len = strlen(token) - 1;
    if (token[len] == '\n')
    {
        token[len] = '\0';
    }
}

// for each word, get the first letter and increase count
void process_first_letter(char letter, letter_count counts[])
{
    for (int i = 0; i < 26; i++)
    {
        if (counts[i].letter == toupper(letter))
        {
            counts[i].count += 1;
            break;
        }
    }
}

// line by line get words and get first letter
void process_text_file(char *path, letter_count counts[])
{
    FILE *text_file;
    remove_newline_character(path);
    text_file = fopen(path, "r");
    char word[MAX_CHAR];
    if (text_file == NULL)
    {
        printf("Could not open word text file: %s\n", path);
        _exit(-1);
    }
    else
    {
        // line by line
        while (fgets(word, MAX_CHAR, text_file) != NULL)
        {
            char f_letter = word[0];
            process_first_letter(f_letter, counts);
        }
    }
}

void convertCountsToRequest(letter_count counts[], int request[])
{
    for (int i = 0; i < 26; i++)
    {
        request[i + 2] = counts[i].count;
    }
}

// Fills Request/Response with Zeroes
void fillWithZeroes(int request[])
{
    for (int i = 0; i < 28; i++)
    {
        request[i] = 0;
    }
}

// Takes in a command and a mapperID and executes it
void handleReq(int mapperId, int sockfd, int command)
{
    int req[28]; // request
    int res[28]; // response
    letter_count counts[26]; // letter counts or current mapper
    int numMessages = 0;
    fillWithZeroes(req); // prepare request to be written
    switch (command)
    {
    case 1:
        // CHECKIN (fill with 0's)]
        req[0] = 1;
        req[1] = mapperId;
        write(sockfd, req, sizeof(req));
        read(sockfd, res, sizeof(res));
        char checkinStatus[128];
        sprintf(checkinStatus, "[%d] CHECKIN: %d %d\n", mapperId, res[1], res[2]);
        writeToLog(checkinStatus);
        break;

    case 2:
        // UPDATE_AZ_LIST (word count result of file)
        init_counts(counts);
        char mapper_file_buf[1024];
        char num_buf[3];
        char path[MAX_CHAR];
        sprintf(num_buf, "%d", mapperId);
        mapper_file_buf[0] = '\0';
        strcat(mapper_file_buf, "./MapperInput/Mapper_");
        strcat(mapper_file_buf, num_buf);
        strcat(mapper_file_buf, ".txt");
        mapper = fopen(mapper_file_buf, "r");
        if (mapper == NULL)
        {
            perror("Could not open mapper text file!\n");
            //_exit(-1);
        }
        else
        {
            while (fgets(path, MAX_CHAR, mapper) != NULL) // process each text file
            {
                init_counts(counts);
                fillWithZeroes(req);
                req[0] = 2;
                req[1] = mapperId;
                process_text_file(path, counts);
                convertCountsToRequest(counts, req); // convert struct to request
                write(sockfd, req, sizeof(req)); // send request for each file path
                read(sockfd, res, sizeof(res));
                numMessages++; // count number of messages sent to server
            }
        }
        char updateStatus[128];
        sprintf(updateStatus, "[%d] UPDATE AZLIST: %d\n", mapperId, numMessages);
        writeToLog(updateStatus); // write update status to log
        break;

    case 3:
        // GET_AZ_LIST
        req[0] = 3;
        req[1] = mapperId;
        write(sockfd, req, sizeof(req));
        read(sockfd, res, sizeof(res));
        char azList[1024];
        sprintf(azList, "[%d] GET_AZLIST: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n", mapperId, res[1], res[2], res[3], res[4], res[5], res[6], res[7], res[8], res[9], res[10], res[11], res[12], res[13], res[14], res[15], res[16], res[17], res[18], res[19], res[20], res[21], res[22], res[23], res[24], res[25], res[26], res[27]);
        writeToLog(azList); // write a retrieval of AZLIST to log
        break;

    case 4:
        // GET_MAPPER_UPDATES
        req[0] = 4;
        req[1] = mapperId;
        write(sockfd, req, sizeof(req));
        read(sockfd, res, sizeof(res));
        char mapperUpdateStatus[128];
        sprintf(mapperUpdateStatus, "[%d] GET_MAPPER_UPDATES: %d %d\n", mapperId, res[1], res[2]);
        writeToLog(mapperUpdateStatus); // write number of updates to log
        break;

    case 5:
        // GET_ALL_UPDATES
        req[0] = 5;
        req[1] = mapperId;
        write(sockfd, req, sizeof(req));
        read(sockfd, res, sizeof(res));
        char mapperAllUpdateStatus[128];
        sprintf(mapperAllUpdateStatus, "[%d] GET_ALL_UPDATES: %d %d\n", mapperId, res[1], res[2]);
        writeToLog(mapperAllUpdateStatus); // write number of total updates to log
        break;

    case 6:
        // CHECKOUT
        req[0] = 6;
        req[1] = mapperId;
        write(sockfd, req, sizeof(req));
        read(sockfd, res, sizeof(res));
        char checkoutStatus[128];
        sprintf(checkoutStatus, "[%d] CHECKOUT %d %d\n", mapperId, res[1], res[2]);
        close(sockfd);
        writeToLog(checkoutStatus); // write checkout status to log
        break;

    default:
        req[0] = command;
        req[1] = mapperId;
        write(sockfd, req, sizeof(req));
        read(sockfd, res, sizeof(res));
        sprintf(checkoutStatus, "[%d] UNKNOWN COMMAND %d %d\n", mapperId, res[1], res[0]); // Shows <mapper id> <NOK> <ReqType>
        close(sockfd);
        writeToLog(checkoutStatus); // write unkown status error status to log
        break;
    }
}

/* EXTRA CREDIT */
void masterClient(char *ip, int port) {
    commands = fopen("commands.txt", "r");
    char masterCommand[2];
    int command;
    int sock;
    /* LOOP THROUGH COMMANDS FILE FOR EACH NUMBER */
    while (fgets(masterCommand, 2, commands) != NULL)
    {
        command = atoi(masterCommand);
        if (command != 0){
            sock = setupTCP(ip, port, -1);
            handleReq(-1, sock, command); // handle each command
            close(sock);
            char masterDisconnectStatus[128];
            sprintf(masterDisconnectStatus, "[%d] close connection\n", -1);
            writeToLog(masterDisconnectStatus); // write connection closure to log

        } 


    }

}

int main(int argc, char *argv[])
{
    int mappers;
    char folderName[100] = {'\0'};
    char *server_ip;
    int server_port;

    if (argc == 5)
    { // 4 arguments
        strcpy(folderName, argv[1]);
        mappers = atoi(argv[2]);
        server_ip = argv[3];
        server_port = atoi(argv[4]);
        if (mappers > MAX_MAPPER_PER_MASTER)
        {
            printf("Maximum number of mappers is %d.\n", MAX_MAPPER_PER_MASTER);
            printf("./client <Folder Name> <# of mappers> <server IP> <server Port>\n");
            exit(1);
        }
    }
    else
    {
        printf("Invalid or less number of arguments provided\n");
        printf("./client <Folder Name> <# of mappers> <server IP> <server Port>\n");
        exit(1);
    }

    // create log file
    createLogFile();

    // phase1 - File Path Partitioning
    traverseFS(mappers, folderName);

    // Phase2 - Mapper Clients's Deterministic Request Handling
    // spawn mapper processes
    pid_t mapper_pid[mappers];
    for (int i = 0; i < mappers; i++)
    {
        mapper_pid[i] = fork();
        if (mapper_pid[i] == 0) // parallel
        {
            // run commands 1-6 with a common socket
            int sock = setupTCP(server_ip, server_port, i + 1);
            handleReq(i + 1, sock, 1);
            handleReq(i + 1, sock, 2);
            handleReq(i + 1, sock, 3);
            handleReq(i + 1, sock, 4);
            handleReq(i + 1, sock, 5);
            handleReq(i + 1, sock, 6);
            char disconnectStatus[128];
            sprintf(disconnectStatus, "[%d] close connection\n", i + 1);
            writeToLog(disconnectStatus); // write socket closure to log
            exit(0); // terminate mapper process
        }
    }
    printf("FINISHED ALL CHILDREN\n");

    for (int k = 0; k < mappers; k++)
    {
        wait(NULL); // wait until all processes are complete
    }
    // Phase3 - Master Client's Dynamic Request Handling (Extra Credit)
    printf("RUNNING EXTRA CREDITS\n");
    masterClient(server_ip, server_port);

    fclose(logfp); // close log file
    return 0;
}
