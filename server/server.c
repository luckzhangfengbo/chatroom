/*************************************************************************
	> File Name: server.c
	> Author: zhangfb
	> Mail: 1819067326
	> Created Time: 六  5/29 14:21:46 2021
 ************************************************************************/

#include "../common/head.h"
#include "../common/common.h"
#include "../common/tcp_server.h"
#include "../common/chatroom.h"
#include "../common/color.h"

struct User {
    char name[20];
    int online;//是否在线
    pthread_t tid;//保存线程id
    int fd;
};


char *conf = "./server.conf";

struct User *client;
int sum = 0;
#define MAX_CLIENT 512



void send_all(struct Msg msg) {
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (client[i].online) {
            chat_send(msg, client[i].fd);
        }
    }
}



void *work(void *arg) {
    //printf("client login!\n");
    int sub = *(int *)arg;//下标
    int client_fd = client[sub].fd;
    struct RecvMsg rmsg;
    printf(GREEN"Login "NONE" : %s\n", client[sub].name);
    while (1) {
        rmsg = chat_recv(client_fd);
        if (rmsg.retval < 0) {
            printf(PINK"Logout: "NONE" %s \n", client[sub].name);
            close(client_fd);
            client[sub].online = 0;//下线
            sum--;
            return NULL;
        }
        printf(BLUE"%s"NONE" : %s\n",rmsg.msg.from, rmsg.msg.message);
        if(rmsg.msg.flag == 0) {
            send_all(rmsg.msg);
        } else {
            printf("这是一个私聊信息");
        }
    }
    return NULL;
}


int find_sub() {
    for (int i = 0;  i < MAX_CLIENT; i++) {
        //不在线就返回下标
        if (!client[i].online) return i;
    }
    return -1;
}

bool check_online(char *name) {
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (client[i].online && !strcmp(name, client[i].name)) {
            printf("D: %s is online\n", name);
            return true;
        }
    }
    return false;
}


int main() {
    int port, server_listen, fd;
    struct RecvMsg recvmsg;
    struct Msg msg;
    port = atoi(get_value(conf, "SERVER_PORT"));
    printf("%d\n", port);
    client = (struct User *)calloc(MAX_CLIENT, sizeof(struct User));
    //监听
    if ((server_listen = socket_create(port)) < 0) {
        perror("socket_create");
        return 1;
    }
    //等待客户端连接
    while(1) {
        if ((fd = accept(server_listen, NULL, NULL)) < 0) {
            perror("accept");
            continue;
        }

        recvmsg = chat_recv(fd);
        if (recvmsg.retval < 0) {
            close(fd);
            continue;
        }

        if (check_online(recvmsg.msg.from)) {
            msg.flag = 3;
            strcpy(msg.message, "You have Already login System!");
            chat_send(msg, fd);
            close(fd);
            continue;

            //拒绝连接
        } 
        msg.flag = 2;
        strcpy(msg.message, "Welcome to this chat room!");
        chat_send(msg,fd);
        
        int sub;
        sum++;
        sub = find_sub();
        client[sub].online = 1;
        client[sub].fd = fd;
        strcpy(client[sub].name, recvmsg.msg.from);

        pthread_create(&client[sub].tid, NULL, work, (void *)&sub);
        //创建子线程去实现
    }

    return 0;
}
