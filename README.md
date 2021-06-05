# 广域网聊天室的设计与实现

## client端

> 执行 ./client
>
> 请使用配置文件配置服务器的监听端口等信息
>
> Server_ip=192.168.1.40
>
> Server_port=8731
>
> My_Name=xxx
>
> Log_File=./chat.log

1.假设有A用户想给B用户发送私聊信息，可以发送：@B 这是来自A的私聊信息

2.所有公聊信息，服务器端转给Client端

3.请将Client收到的所有聊天信息，保存在本地的 Log.File中

4.使用tail -f Log_File查看文件，获取实时聊天信息

5.由服务器发送到本地的数据是哟个结构体，请client自行解析输出



```c
struct Message{
  	char from[20];
  	int flag;//若flag为1则是私聊信息，0为公聊信息，2则为服务器的通知信息或者客户端的连接信息, 3则为断开连接的请求
  	char Message[1024];
};
```

## Server端

> 执行：./server
>
> 请使用配置文件，将服务器监听端口，客户端监听端口等都写在配置文件中
>
> Server_Port=8731

1.Server将在Server_Port上进行监听，等待用户上线，并在该端口上接受用户输出信息

2.Server每收到一条信息后，需要将信息转发给其他所有在线用户

3.如果用户发送的是一条私聊信息，则此信息，只转发给目标用户

4.所有转发给用户的信息，都将使用结构体Messag进行封装

5.私聊信息中所指定的用户不存在或已经下线，需通过通知信息告知

6.请选用合理的数据结构，存储用户信息

7.支持100以上用户在线

8.在Client上线时，发送欢迎信息，告知当前所有人在线人数等

9.需考虑当两个用户在某一时刻同时上线的情况



## 1.先实现get_value获取配置文件的ip与port

```c
char *get_value(char *path, char *key) {
    FILE *fp = NULL;
    ssize_t nrd;
    char *line = NULL, *sub = NULL;
    extern char conf_ans[50];
    size_t linecap;
    if (path == NULL || key == NULL) {
        fprintf(stderr, "Error in argument!\n");
        return NULL;
    }
    if ((fp = fopen(path, "r")) == NULL) {
        perror("fopen");
        return NULL;
    }
    while ((nrd = getline(&line, &linecap, fp)) != -1) {
        if ((sub = strstr(line, key)) == NULL)
            continue;
        else {
            if (line[strlen(key)] == '=') {
                strncpy(conf_ans, sub + strlen(key) + 1, nrd - strlen(key) - 2);
                *(conf_ans + nrd - strlen(key) - 2) = '\0';
                break;
            }
        }
    }
    free(line);
    fclose(fp);
    if (sub == NULL) {
        return NULL;
    }
    return conf_ans;
}
```

## 2.实现颜色的输出color.h

```ABAP
#ifndef _COLOR_H
#define _COLOR_H

#define NONE "\033[0m"
#define BLACK "\033[0;30m"
#define L_BLACK "\033[1;30m"
#define RED "\033[0;31m"
#define L_RED "\033[1;31m"
#define GREEN "\033[0;32m"
#define L_GREEN "\033[1;32m"
#define YELLOW "\033[0;33m"
#define L_YELLOW "\033[1;33m"
#define BLUE "\033[0;34m"
#define L_BLUE "\033[1;34m"
#define L_PINK "\033[1;35m"
#define PINK "\033[0;35m"
#endif
```

> 宏定义使用的时候是在各个输出上面错误提示等加上颜色.
>
> <font color = "pink">中间夹的字符w会变颜色。  printf(YELLOW"w"NONE "%s is only \n", name); </font>

## 3.对 crtl + c信号的处理

> linux下的信号， crtl+c的信号是 2）

![image-20210521161647857](https://tva1.sinaimg.cn/large/008i3skNly1gqq4d2cd3pj30lj08w7gx.jpg)



```c
client.c

//退出的函数
void logout(int signalnum) {
    close(sockfd);//先关闭连接
    printf("您已退出.\n");
    exit(1);
}

 signal(SIGINT, logout);//对crtl+c信号的处理


```

> Bug.   
>
> 1.退出的时候名字错了，  server端的*sub发生的改变， int sub = *(int *)arg。 <font color = "red">一直处于动态变化</font>
>
> 2.按crtl + D会疯狂回车



## 4.服务端大致框架的实现

> 1.MAX_CLIENT = 512客户的数量是512个
>
> 2.atoi将端口本地字节序变成网络字节序
>
> 3.之后用server_listen监听端口
>
> 4.chatroom.h定义MSG结构体, 是存储的信息， RercvMsg是收的信息的结构体， SendMsg是发的信息的结构体
>
> 5.服务端定义User结构体存储用户信息
>
> 6.online判断用户是否在线
>
> 7.所有转发给用户的信息，都将使用结构体Messag进行封装

```c
#ifndef _CHATROOM_H
#define _CHATROOM_H

#include "head.h"

struct Msg {
    char from[20];
    int flag;
    char message[512];
};



struct RecvMsg {
    struct Msg msg;
    int retval;
};


int chat_send(struct Msg msg, int fd) {
    if (send(fd, (void *)&msg, sizeof(msg), 0) <= 0) {
        return -1;
    }
    return 0;
}

struct RecvMsg chat_recv(int fd) {
    struct RecvMsg tmp;
    memset(&tmp, 0, sizeof(tmp));
    if (recv(fd, &tmp.msg, sizeof(struct Msg), 0) <= 0) {
        tmp.retval = -1;
    }
    return tmp;
}


#define MAX_CLIENT 512
#endif
```



```c
/*************************************************************************
	> File Name: server.c
	> Author: zhangfb
	> Mail: 1819067326@qq.com
	> Created Time: Sat 22 May 2021 07:12:55 AM CST
 ************************************************************************/

#include "../common/head.h"
#include "../common/common.h"
#include "../common/tcp_server.h"
#include "../common/chatroom.h"
#include "../common/color.h"

struct User {
    char name[20];
    int online;
    pthread_t tid;
    int fd;
};



char *conf = "./server.conf";


struct User *client;

bool check_online(char *name) {
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (client[i].online && !strcmp(name, client[i].name)) {
            return true;
        }
    }
    return false;
}

int main() {
    int port, server_listen, fd;
    struct RecvMsg recvmsg;;
    port = atoi(get_value(conf, "SERVER_PORT"));
    client = (struct User *)calloc(MAX_CLIENT, sizeof(struct User));
    if ((server_listen = socket_create(port)) < 0) {
        perror("socket_create");
        return 1;
    }
    while (1) {
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
```

## 5.服务端：完成主要框架，可运行测试

```c
/*************************************************************************
	> File Name: server.c
	> Author: zhangfb
	> Mail: 1819067326@qq.com
	> Created Time: Sat 22 May 2021 07:12:55 AM CST
 ************************************************************************/

#include "../common/head.h"
#include "../common/common.h"
#include "../common/tcp_server.h"
#include "../common/chatroom.h"
#include "../common/color.h"

struct User {
    char name[20];
    int online;
    pthread_t tid;
    int fd;
};



char *conf = "./server.conf";


struct User *client;


void *work(void *arg) {
    printf("client login!\n");
    return NULL;
}


int find_sub() {
    for (int i = 0;  i < MAX_CLIENT; i++) {
        if (!client[i].online) return i;
    }
    return -1;
}


bool check_online(char *name) {
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (client[i].online && !strcmp(name, client[i].name)) {
            return true;
        }
    }
    return false;
}

int main() {
    int port, server_listen, fd;
    struct RecvMsg recvmsg;;
    port = atoi(get_value(conf, "SERVER_PORT"));
    client = (struct User *)calloc(MAX_CLIENT, sizeof(struct User));
    if ((server_listen = socket_create(port)) < 0) {
        perror("socket_create");
        return 1;
    }
    while (1) {
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
```

![image-20210522181500579](https://tva1.sinaimg.cn/large/008i3skNly1gqrdebdsjej30fy04kwin.jpg)

## 6.客户端：完成客户端的上线连接

> 将客户端配置文件里面的内容传到客户端， 上线连接进行提醒，
>
> 客户端只是实现了登陆的功能， 没有实现下线的功能， 

```c
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
```





```c
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
```



## 7.线程处理函数work的实现

```c
//线程处理函数
void *work(void *arg) {
    //printf("client login!\n");
    int *sub = (int *)arg;//下标
    int client_fd = client[*sub].fd;
    struct RecvMsg rmsg;
    while(1) {
        rmsg = chat_recv(client_fd);
        if (rmsg.retval < 0) {
            printf(PINK"logout :"NONE" %s \n",client[*sub].name);
            close(client_fd);
            client[*sub].online = 0;
            return NULL;
        }
        printf(BLUE"%s "NONE" : %s\n", rmsg.msg.from, rmsg.msg.message);
    }

    return NULL;
}
```

## 8.客户端：接受服务端对登录操作的反馈信息



```c
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


    struct RecvMsg rmsg = chat_recv(sockfd);

    if (rmsg.retval < 0)  {
        fprintf(stderr, "ERROR!\n");
        return 1;
    }
    printf(GREEN"Server "NONE": %s", rmsg.msg.message);

    if (rmsg.msg.flag == 3) {
        close(sockfd);
    }
    return 0;
}
```



```c
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
    //printf("client login!\n");
    int *sub = (int *)arg;//下标
    int client_fd = client[*sub].fd;
    struct RecvMsg rmsg;
    printf(GREEN"Login "NONE" : %s\n", client[*sub].name);
    while (1) {

        rmsg = chat_recv(client_fd);
        if (rmsg.retval < 0) {
            printf(PINK"Logout: "NONE" %s \n", client[*sub].name);
            close(client_fd);
            client[*sub].online = 0;//下线
            return NULL;
        }
        printf(BLUE"%s"NONE" : %s\n",rmsg.msg.from, rmsg.msg.message);
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
        sub = find_sub();
        client[sub].online = 1;
        client[sub].fd = fd;
        strcpy(client[sub].name, recvmsg.msg.from);

        pthread_create(&client[sub].tid, NULL, work, (void *)&sub);
        //创建子线程去实现

    }

    return 0;
}
```



## 9.客户端：在子进程中发送数据

> 1.fork（）复制 复制子进程， 完善程序
>
> <font color = "red">2.存在一个bug， crtl+c结束的时候不会下线</font>

```c
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
  /*
  就加了下面这一段用子进程去向服务端发信息， 父进程等待子进程结束
    pid_t pid;
    if ((pid = fork()) < 0) {//复制子进程
        perror("fork");
    }
    if (pid == 0) {
        system("clear");//清空屏幕
        while (1) {
            printf(L_PINK"Please Input Message:"NONE"\n");
            scanf("%[^\n]s", msg.message);
            getchar();//干掉回车
            chat_send(msg,sockfd);//发
            memset(msg.message, 0, sizeof(msg.message));
            system("clear");//清空屏幕
        }
    } else {
        wait(NULL);
    }
*/
    return 0;
}
```

## 10.对 crtl + c信号的处理

> linux下的信号， crtl+c的信号是 2）

![image-20210521161647857](https://tva1.sinaimg.cn/large/008i3skNly1gqq4d2cd3pj30lj08w7gx.jpg)



```c
client.c

//退出的函数
void logout(int signalnum) {
    close(sockfd);//先关闭连接
    printf("您已退出.\n");
    exit(1);
}

 signal(SIGINT, logout);//对crtl+c信号的处理


```

> Bug.   
>
> 1.退出的时候名字错了，  server端的*sub发生的改变， int sub = *(int *)arg。 <font color = "red">一直处于动态变化</font>
>
> 2.按crtl + D会疯狂回车

## 11.作业：将公聊信息转发给所有人

 

```c
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
              //这样去实现其实还有更好的方法， 用freopen函数去实现这个功能， 在12中会体现到
            }
           // printf("%s : %s\n", rmsg.msg.from, rmsg.msg.message);
            fflush(log_fp);
        }
        wait(NULL);
        close(sockfd);
    }
    return 0;
}
```



![image-20210605143544372](https://tva1.sinaimg.cn/large/008i3skNly1gr7dqlsb0bj30h40920yi.jpg)



```c
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
```



## 12.freopen的使用



```c
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
struct RecvMsg rmsg;
int sockfd;
char logfile[50] = {0};

void logout(int signalnum) {
    close(sockfd);
    printf("您已退出!");
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
    signal(SIGINT, logout);
    pid_t pid;
    if ((pid = fork()) < 0) {//复制子进程
        perror("fork");
    }
    if (pid == 0) {
        sleep(2);
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
        //FILE *log_fp = fopen(logfile, "w");
        freopen(logfile, "a+", stdout);
        while (1) {
            rmsg = chat_recv(sockfd);
            if (rmsg.msg.flag == 0) {
                printf(L_BLUE"%s"NONE": %s\n", rmsg.msg.from, rmsg.msg.message);
            fflush(stdout);
            }
        }
        wait(NULL);
        close(sockfd);
    }
    return 0;
}
```



## 13.服务端：对私聊信息的处理

```c
/*************************************************************************
	> File Name: client.c
	> Author: zhangfb
	> Mail: 1819067326
	> Created Time: 六  5/29 14:43:31 2021
 ************************************************************************/
#include "../common/chatroom.h"
#include "../common/tcp_client.h"
#include "../common/common.h"
#include "../common/color.h"

char *conf = "./client.conf";
int sockfd;

char logfile[50] = {0};
void logout(int signalnum) {
    close(sockfd);
    printf("您已退出.\n");
    exit(1);
}


int main() {
    int port;
    struct Msg msg;
    char ip[20] = {0};
    port = atoi(get_value(conf, "SERVER_PORT"));
    strcpy(ip, get_value(conf, "SERVER_IP"));
    strcpy(logfile, get_value(conf, "LOG_FILE"));


    if ((sockfd = socket_connect(ip, port)) < 0) {
        perror("socket_connect");
        return 1;
    }
    //strcpy(msg.from, get_value(conf, "MY_NAME"));
    struct passwd *pwd;
    pwd = getpwuid(getuid());

    strcpy(msg.from, pwd->pw_name);
    msg.flag = 2;
    if (chat_send(msg, sockfd) < 0) {
        return 2;
    }

    struct RecvMsg rmsg = chat_recv(sockfd);

    if (rmsg.retval < 0) {
        fprintf(stderr, "Error!\n");
        return 1;
    }


    if (rmsg.msg.flag == 3) {
        close(sockfd);
        return 1;
    }

    signal(SIGINT, logout);
    pid_t pid;
    if ((pid = fork()) < 0){
        perror("fork");
    }
    if (pid == 0) {
        sleep(2);
        char c = 'a';
        while (c != EOF) {
            system("clear");
            printf(L_PINK"Please Input Message:"NONE"\n");
            memset(msg.message, 0, sizeof(msg.message));
            scanf("%[^\n]s", msg.message);
            c = getchar();
            msg.flag = 0;
            if (msg.message[0] == '@') {
                msg.flag = 1;
            } else if (msg.message[0] == '#') {
                msg.flag = 4;
            }
            if (!strlen(msg.message)) continue;
            chat_send(msg, sockfd);
        }
        close(sockfd);
    } else {
        freopen(logfile, "w", stdout);
        printf(L_GREEN"Server "NONE": %s\n", rmsg.msg.message);
        fflush(stdout);
        while (1) {
            rmsg = chat_recv(sockfd);
            if (rmsg.retval < 0) {
                printf("Error in Server!\n");
                break;
            }
            if (rmsg.msg.flag == 0) {
                printf(L_BLUE"%s"NONE": %s\n", rmsg.msg.from, rmsg.msg.message);
            } else if (rmsg.msg.flag == 2) {
                printf(YELLOW"通知信息: " NONE "%s\n", rmsg.msg.message);
            } else if (rmsg.msg.flag == 1){
                printf(L_BLUE"%s"L_GREEN"*"NONE": %s\n", rmsg.msg.from, rmsg.msg.message);
            } else {
                printf("Error!\n");
            }
            fflush(stdout);
        }
        wait(NULL);
        close(sockfd);
    }
    return 0;
}

```







```c
/*************************************************************************
	> File Name: server.c
	> Author: zhangfb
	> Mail: 1819067326
	> Created Time: 六  5/29 14:21:46 2021
 ************************************************************************/
#include "../common/common.h"
#include "../common/tcp_server.h"
#include "../common/chatroom.h"
#include "../common/color.h"

struct User{
    char name[20];
    int online;
    pthread_t tid;
    int fd;
};


char *conf = "./server.conf";

struct User *client;
int sum = 0;


void get_online(char *message) {
    int cnt = 0;
    char tmp[25] = {0};
    sprintf(message, "当前有 ");
    for (int i = 0; i < MAX_CLIENT; i++) {
        if(client[i].online) {
            if (cnt != 0 ) sprintf(tmp, ",%s", client[i].name);
            else sprintf(tmp, "%s", client[i].name);
            strcat(message, tmp);
            cnt++;
            if (cnt >= 50) break;
        }
    }
    sprintf(tmp, " 等%d个用户在线", sum);
    strcat(message, tmp);
}

void send_all(struct Msg msg) {
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (client[i].online)
            chat_send(msg, client[i].fd);
    }
}
void send_all_ex(struct Msg msg, int sub) {
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (sub == i) continue;
        if (client[i].online)
            chat_send(msg, client[i].fd);
    }
}

int check_name(char *name) {
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (client[i].online && !strcmp(client[i].name, name))
            return i;
    }
    return -1;
}

void *work(void *arg){
    int sub = *(int *)arg;
    int client_fd = client[sub].fd;
    struct RecvMsg rmsg;
    printf(GREEN"Login "NONE" : %s\n", client[sub].name);
    rmsg.msg.flag = 2;
    sprintf(rmsg.msg.message, "你的好友 %s 上线了，和他打个招呼吧😁", client[sub].name);
    send_all_ex(rmsg.msg, sub);
    while (1) {
        rmsg = chat_recv(client_fd);
        if (rmsg.retval < 0) {
            printf(PINK"Logout: "NONE" %s \n", client[sub].name);
            sprintf(rmsg.msg.message, "好友 %s 已下线.", client[sub].name);
            close(client_fd);
            client[sub].online = 0;
            sum--;
            rmsg.msg.flag = 2;
            send_all(rmsg.msg);
            return NULL;
        }

        if (rmsg.msg.flag == 0) {
            printf(BLUE"%s"NONE" : %s\n",rmsg.msg.from, rmsg.msg.message);
            if (!strlen(rmsg.msg.message)) continue;
            send_all(rmsg.msg);
        } else if (rmsg.msg.flag == 1) {
            if (rmsg.msg.message[0] == '@') {
                char to[20] = {0};
                int i = 1;
                for (; i <= 20; i++) {
                    if (rmsg.msg.message[i] == ' ')
                        break;
                }
                strncpy(to, rmsg.msg.message + 1, i - 1);
                int ind;
                if ((ind = check_name(to)) < 0) {
                    sprintf(rmsg.msg.message, "%s is not online.", to);
                    rmsg.msg.flag = 2;
                    chat_send(rmsg.msg, client_fd);
                    continue;
                } else if (!strlen(rmsg.msg.message + i)) {
                    sprintf(rmsg.msg.message, "私聊消息不能为空");
                    rmsg.msg.flag = 2;
                    chat_send(rmsg.msg, client_fd);
                    continue;
                }
                printf(L_PINK"Note"NONE": %s 给 %s 发送了一条私密信息\n", rmsg.msg.from, to);
                chat_send(rmsg.msg, client[ind].fd);
            }
        } else if (rmsg.msg.flag == 4 && rmsg.msg.message[0] == '#') {
            printf(L_PINK"Note"NONE": %s查询了在线人数\n", rmsg.msg.from);
            get_online(rmsg.msg.message);
            rmsg.msg.flag = 2;
            chat_send(rmsg.msg, client_fd);
        }
    }
    return NULL;
}


int find_sub() {
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (!client[i].online) return i;
    }
    return -1;
}

bool check_online(char *name) {
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (client[i].online && !strcmp(name, client[i].name)) {
            printf(YELLOW"W"NONE": %s is online\n", name);
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
    client = (struct User *)calloc(MAX_CLIENT, sizeof(struct User));

    if ((server_listen = socket_create(port)) < 0) {
        perror("socket_create");
        return 1;
    }
    while (1) {
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
            strcpy(msg.message, "You have Already Login System!");
            chat_send(msg, fd);
            close(fd);
            continue;
            //拒绝连接：
        }
        msg.flag = 2;
        strcpy(msg.message, "Welcome to this chat room!");
        chat_send(msg, fd);

        int sub, ret;
        sum++;
        sub = find_sub();
        client[sub].online = 1;
        client[sub].fd =fd;
        strcpy(client[sub].name, recvmsg.msg.from);
        ret = pthread_create(&client[sub].tid, NULL, work, (void *)&sub);
        if (ret != 0) {
            fprintf(stderr, "pthread_create");
        }
    }
    return 0;
}
```

