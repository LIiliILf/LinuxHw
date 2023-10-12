#include <cstdio>
#include <sys/types.h>          /* socket */
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>

void* thread_fun(void* arg){
    int acpt_socket = *((int*)arg);     //accpet传递过来后，需要把它取出来,强制转换成指针
    int res;
    char buffer[50] = { 0 };
    while (1){
        /*read函数就是接收客户端发来的数据，返回值表示实际从客户端那边收到的字节数。
        buffer就是收到客户端数据后把数据存放的地址，sizeof(buffer)就是希望读取的字节数。*/
        res = read(acpt_socket, buffer, sizeof(buffer));
        printf("client read %s\n", buffer);
        write(acpt_socket, buffer, res);
        memset(buffer, 0, sizeof(buffer));
        //服务器收到客户端数据后，原封不动把数据再回发给客户端。
    }
}
int main()
{
    char buffer[50] = { 0 };
    int res = 0;

    printf("开始创建tcp服务器\n");
    int server_socket; 
    int accept_socket;
    pthread_t thread_id;    //线程编号
    //1.使用socket创建一个TCP流式套接字
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("socket create error");
        return 0;
    }
    
    //2.告诉服务器ip地址和端口号。
    struct sockaddr_in server_addr;     //IPv4套接口地址数据结构
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; //自动绑定网卡ip
    server_addr.sin_port = htons(6666);//通过网络地址转换协议htons,把主机字节顺序转换成网络字节顺序
    int optvalue = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optvalue, sizeof(optvalue));   //端口复用
    
    //3.用bind()将ip地址和端口号绑定到server_socket描述符
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("server bind error:");
        return 0;
    }
    
    //4.调用listen监听程序 
    if (listen(server_socket, 10) < 0){
        perror("server listen error:");
        return 0;
    }
    
    //5. 等待客户端连接 调用accept()接收客户端的连接请求
    /*当我们程序调用这个函数时，如果没有客户端连接到服务器上那么这个函数将阻塞。
     直到有客户端连接到服务器上，解除阻塞并返回一个新的套接字描述符，那么后期和客户端的通讯都交给这个新的套接字描述符来负责*/
    printf("tcp服务器准备完成，等待客户端连接\n");

    /*多线程 把accept放到while里面然后创建线程
    创建成功后系统会执行thread_fun()里面的代码，会和主线程代码一起执行*/
    while (1){
        accept_socket = accept(server_socket, NULL, NULL);
        printf("有客户端连接到服务器\n");
        pthread_create(&thread_id, NULL, thread_fun, &accept_socket);
    }

		printf("%s 向你问好!\n", "Linux_netdisk_server");
	return 0;
}