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

#define MAX_CLIENT 512


void *work(void *arg) {
    printf("client login!\n");
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
            //拒绝连接
        } else {
            int sub;
            sub = find_sub();
            client[sub].online = 1;
            client[sub].fd = fd;
            strcpy(client[sub].name, recvmsg.msg.from);

        pthread_create(&client[sub].tid, NULL, work, NULL);
        //创建子线程去实现
        }



    }

    return 0;
}
