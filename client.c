#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include "format.h"
#define False 0
#define True 1
#define PortNum 2908

int sock;
int CONNECTED = False;
pthread_t tid1, tid2;
pthread_mutex_t prt_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t prt_cond = PTHREAD_COND_INITIALIZER;
char *responsemsg;

void *recv_fun(void *arg)
{
    int len = 100;
    char *buff = malloc(len * sizeof(char));
    printf("Recv Start\n");

    while (CONNECTED)
    {
        int res;
        res = recv(sock, buff, len, 0);
        if (res < 0)
        {
            printf("recv error: %s(errno: %d)\n", strerror(errno), errno);
            exit(-1);
        }
        else if (res < len)
        {
            int msglen;
            int from;
            switch ((int)buff[0])
            {
            case 0:
                // response
                memcpy(&msglen, buff + 1, 4 * sizeof(char));
                responsemsg = malloc(msglen * sizeof(char));
                strcpy(responsemsg, buff + 1 + LENGTH);
                break;
            case 4:
                // message from other client
                memcpy(&msglen, buff + 1, 4 * sizeof(char));
                memcpy(&from, buff + 1 + LENGTH, 4 * sizeof(char));
                responsemsg = malloc(msglen * sizeof(char));
                strcpy(responsemsg, buff + 1 + 2 * LENGTH);
                char *tmp = malloc((50 + msglen) * sizeof(char));
                sprintf(tmp, "Get message from %d:\n", from);
                strcat(tmp, responsemsg);
                responsemsg = tmp;

                break;
            default:
                break;
            }
            pthread_cond_signal(&prt_cond);
        }
        else
        {
            // TODO: buffer长度不足一次信息，需要继续
        }
    }
    printf("thread quit\n");
    free(buff);
}

void *printer_fun(void *arg)
{
    pthread_mutex_lock(&prt_mutex);
    while (CONNECTED)
    {
        pthread_cond_wait(&prt_cond, &prt_mutex);
        printf("%s", responsemsg);
        free(responsemsg);
    }
}

void cli_connect(const char *ipaddr, int portid)
{
    printf("Server ip: %s\n", ipaddr);

    //向服务器（特定的IP和端口）发起请求
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));      //每个字节都用0填充
    serv_addr.sin_family = AF_INET;                //使用IPv4地址
    serv_addr.sin_addr.s_addr = inet_addr(ipaddr); //具体的IP地址
    serv_addr.sin_port = htons(portid);            //端口
    // printf("addr: %x\n", serv_addr.sin_addr.s_addr);

    printf("Connecting...\n");
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        printf("connect error: %s(errno: %d)\n", strerror(errno), errno);
    }
    else
    {
        printf("\nConnect to %s success!", ipaddr);
    }
    CONNECTED = True;
    pthread_create(&tid1, NULL, recv_fun, NULL);
    pthread_create(&tid2, NULL, printer_fun, NULL);
}

void cli_disconnect()
{
    if (!CONNECTED)
    {
        printf("\nERROR: Not connected!\n");
        return;
    }

    close(sock);
    CONNECTED = False;
    pthread_join(tid1, NULL);
}

void cli_time()
{
    if (!CONNECTED)
    {
        printf("Error: Not connected to any server!");
        return;
    }

    // 组装数据包
    int len = 3;
    char *buff = malloc(len * sizeof(char));
    buff[0] = REQTIME;
    int msglen = 0;
    memcpy(buff + 1, &msglen, 4 * sizeof(char));

    // 调用send
    int res;
    if ((res = send(sock, buff, len, 0)) < 0)
    {
        perror("send error");
        exit(1);
    }

    printf("send success!\n");
}

void cli_name()
{
    if (!CONNECTED)
    {
        printf("Error: Not connected to any server!");
        return;
    }

    // 组装数据包
    int len = 3;
    char *buff = malloc(len * sizeof(char));
    buff[0] = REQNAME;
    int msglen = 0;
    memcpy(buff + 1, &msglen, 4 * sizeof(char));

    // 调用send
    int res;
    if ((res = send(sock, buff, len, 0)) < 0)
    {
        perror("send error");
        exit(1);
    }

    printf("send success!\n");
}

void cli_list()
{
    if (!CONNECTED)
    {
        printf("Error: Not connected to any server!");
        return;
    }

    // 组装数据包
    int tollen = 5;
    char *buff = malloc(tollen * sizeof(char));
    buff[0] = REQLIST;
    int msglen = 0;
    memcpy(buff + 1, &msglen, 4 * sizeof(char));

    // 调用send
    int res;
    if ((res = send(sock, buff, tollen, 0)) < 0)
    {
        perror("send error");
        exit(1);
    }

    printf("send success!\n");
}

void cli_send()
{
    if (!CONNECTED)
    {
        printf("Error: Not connected to any server!");
        return;
    }
    // 先获取客户端列表
    // cli_list();
    // 此处停等
    printf("\nPlease choose the number of target client:\n");

    int target;
    scanf("%d", &target);

    printf("Please enter message below:(max 200)\n");
    char *msg = malloc(200 * sizeof(char));
    scanf("%s", msg);

    // 组装数据包
    int len = 5 + strlen(msg);
    char *buff = malloc(len * sizeof(char));
    buff[0] = MESSAGE;

    memcpy(buff + 1, &len, 4 * sizeof(char));
    memcpy(buff + 1 + LENGTH, &target, 4 * sizeof(char));
    strcpy(buff + 1 + 2 * LENGTH, msg);

    // 调用send
    int res;
    if ((res = send(sock, buff, len, 0)) < 0)
    {
        perror("send error");
        exit(1);
    }

    printf("send success!\n");
}

int main(void)
{
    sock = socket(AF_INET, SOCK_STREAM, 0);
    printf("----Welcome to Socket!----\n");
    printf("sock = %d\n", sock);

    char *ipaddr = malloc(20 * sizeof(char));
    char *cmd = malloc(20 * sizeof(char));
    CONNECTED = True;
    while (1)
    {
        printf("\nPlease Enter Command:\n");
        scanf("%s", cmd);
        if (cmd[0] == 'c')
        {
            int portid;
            printf("\nConnect:\nPlease enter the IP address of target server:\n");
            scanf("%s", ipaddr);
            printf("\nPlease enter the target port number:\n");
            scanf("%d", &portid);

            cli_connect(ipaddr, portid);
            //connect
        }
        else if (cmd[0] == 'd')
        {
            // disconnect
            cli_disconnect();
        }
        else if (cmd[0] == 't')
        {
            //time
            cli_time();
        }
        else if (cmd[0] == 'n')
        {
            //name
            cli_name();
        }
        else if (cmd[0] == 'l')
        {
            // list
            cli_list();
        }
        else if (cmd[0] == 's')
        {
            // send
            cli_send();
        }
        else if (cmd[0] == 'h')
        {
            // help
        }
        else if (cmd[0] == 'q')
        {
            printf("\nQuit\n");
            break;
        }

        printf("Command = %s\n", cmd);
    }
    free(cmd);
    free(ipaddr);
}