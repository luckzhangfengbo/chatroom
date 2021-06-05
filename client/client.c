/*************************************************************************
	> File Name: client.c
	> Author: zhangfb
	> Mail: 1819067326
	> Created Time: 六  5/29 14:43:31 2021
 ************************************************************************/

#include  "../common/chatroom.h"
#include "../common/tcp_client.h"
#include "../common/common.h"
#include "../common/color.h"
char *conf = "./client.conf";

int sockfd;
char logfile[50] = {0};

void logout(int signalnum) {
    close(sockfd);
    printf("recv a signal!");
    exit(1);
}

int main() {
    int port;
    struct Msg msg;
    char ip[20] = {0};
    port = atoi(get_value(conf,"SERVER_PORT"));//复制端口
    strcpy(ip, get_value(conf, "SERVER_IP"));//复制ip
    strcpy(logfile, get_value(conf, "LOG_FILE"));//复制log_file

    printf("ip = %s, port = %d\n", ip, port);


    //连接
    if ((sockfd = socket_connect(ip, port)) < 0) {
        perror("socket_connect");
        return 1;
    }

    strcpy(msg.from, get_value(conf, "MY_NAME"));//将名字拷贝过来
    msg.flag = 2;

    if (chat_send(msg, sockfd) < 0) {//上线
        return 2;
    }

    
    struct RecvMsg rmsg = chat_recv(sockfd);

    if (rmsg.retval < 0)  {
        fprintf(stderr, "ERROR!\n");
        return 1;
    }
    printf(GREEN"Server "NONE": %s", rmsg.msg.message);

    if (rmsg.msg.flag == 3) {
        close(sockfd);
        return 1;
    }
    pid_t pid;
    if ((pid = fork()) < 0) {//复制子进程
        perror("fork");
    }
    if (pid == 0) {
        sleep(2);
        signal(SIGINT, logout);
        system("clear");//清空屏幕
        char c = 'a';
        while (c != EOF) {
            printf(L_PINK"Please Input Message:"NONE"\n");
            scanf("%[^\n]s", msg.message);
            c = getchar();//干掉回车
            msg.flag = 0;
            chat_send(msg,sockfd);//发
            memset(msg.message, 0, sizeof(msg.message));
            system("clear");//清空屏幕 
        }
    } else {
        FILE *log_fp = fopen(logfile, "w");
        struct RecvMsg rmsg;
        while (1) {
            rmsg = chat_recv(sockfd);
            if (rmsg.msg.flag == 0) {
                fprintf(log_fp, L_BLUE"%s"NONE": %s\n", rmsg.msg.from, rmsg.msg.message);
            }
           // printf("%s : %s\n", rmsg.msg.from, rmsg.msg.message);
            fflush(log_fp);
        }
        wait(NULL);
        close(sockfd);
    }
    return 0;
}
