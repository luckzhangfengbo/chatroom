/*************************************************************************
	> File Name: client.c
	> Author: zhangfb
	> Mail: 1819067326
	> Created Time: 六  5/29 14:43:31 2021
 ************************************************************************/

#include  "../common/chatroom.h"
#include "../common/tcp_client.h"
#include "../common/common.h"

char *conf = "./client.conf";

int main() {
    int port, sockfd;
    struct Msg msg;
    char ip[20] = {0};
    port = atoi(get_value(conf,"SERVER_PORT"));//复制端口
    strcpy(ip, get_value(conf, "SERVER_IP"));//复制ip

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


    return 0;
}
