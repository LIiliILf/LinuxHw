#include <cstdio>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>

int main()
{
    int client_socket;
    struct sockaddr_in server_addr; //用来填写要连接的服务器的ip地址和端口号
    char buffer[50] = { 0 };
    int res;
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0){
        perror("client socket failed:");
        return 0;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("192.168.85.16"); //直接指定或者如果s和c在同一台电脑中可以用127.0.0.1
    server_addr.sin_port = htons(6666);

    //客户端使用connect()连接服务器 man connect
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {   //& server_addr 取地址，然后强制转换
        perror("connect error");
        return 0;
    }
    printf("client连接server成功\n");

    //客户端用write()往服务器发送数据，用户在客户端终端连续输入字符串，回车表示发送数据。
    while (fgets(buffer, sizeof(buffer), stdin) != NULL){
        res = write(client_socket, buffer, sizeof(buffer));//把buffer里面所有的数据发送给服务器,用write往服务器发送数据。 man write
        printf("send bytes = %d\n", res);
        memset(buffer, 0, sizeof(buffer));
        res = read(client_socket, buffer, sizeof(buffer));
        printf("receive from server info:%s\n", buffer);
        memset(buffer, 0, sizeof(buffer));
    }

    printf("%s 向你问好!\n", "Linux_netdisk_client");
    return 0;
}