#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main()
{
    int server_socket; // socket网络描述符/套接字描述符
    //1.创建套接字描述符/买手机
    printf("开始创建tcp服务器\n");
    server_socket = socket(AF_INET, SOCK_STREAM, 0); //使用server_socket这个套接字描述符向网络发送数据。流式都是一样的步骤

    if (server_socket < 0)
    {
        perror("socket create failed:");
        return 0;
    }

    //2.告诉服务器我们的ip地址和端口号。所以要有对应用于保存的变量。 /买电话卡
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; //INADDR_ANY 宏定义，告诉系统自动绑定网卡IP地址
    server_addr.sin_port = htons(6666); //通过网络地址转换协议把主机字节顺序转换成网络字节顺序

    //3. 把ip地址和端口号绑定到 server_socket描述符上。 man bind /给手机插上电话卡
    if (bind(server_socket, &server_addr, sizeof(server_addr)) < 0)
    {
        perror("server bind error:");
        return 0;
    }

    //4. 调用listen开始监听程序 man listen。 带上手机才能接收到
    if (listen(server_socket, 10) < 0)
    {
        perror("server listen error:");
        return 0;
    }

    //5. 等待客户端连接过来 man accept
    //当我们程序调用这个函数时，如果没有客户端连接到服务器上那么这个函数将阻塞
    //直到客户端连接到服务器上才会解除阻塞，并且返回一个新的套接字描述符
    //后期和客户端的通讯都交给这个新的套接字描述符来负责。
    printf("服务器准备完成，等待客户端连接\n");
    accept_socket = accept(server_socket, NULL, NULL);
    printf("有客户端连接到服务器\n");
    while (1)
    {
        res = read(accpet_socket, buffer, sizeof(buffer));
    }


    printf("%s 向你问好!\n", "test");
    return 0;
}