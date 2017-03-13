#include <stdio.h>//printf
#include <string.h>//字符串处理
#include <fcntl.h>//open系统调用
#include <netdb.h>//查询DNS
#include <arpa/inet.h>//ip地址处理
#include <stdlib.h>//exit函数
#include <sys/stat.h>//stat系统调用获取文件大小
#include <sys/time.h>//获取下载时间
#include "httpCommon.h"
#include"progress_bar.h"
void parse_url(const char *url, char *host, int *port, char *file_name,const char **papszRelativeUrl)
{
    /*通过url解析出域名, 端口, 以及文件名*/
    int j = 0;
    int start = 0;
    *port = 80;
   const char *patterns[] = {"http://", "https://", NULL};

    for (int i = 0; patterns[i]; i++)//分离下载地址中的http协议
        if (strncmp(url, patterns[i], strlen(patterns[i])) == 0)
            start = strlen(patterns[i]);

    //解析域名, 这里处理时域名后面的端口号会保留
	int i = start;
    for (i = start; url[i] != '/' && url[i] != '\0'; i++, j++)
        host[j] = url[i];
    host[j] = '\0';
	(*papszRelativeUrl) = url + i;
	printf("请求相对URL地址为：%s\n",*papszRelativeUrl);
    //解析端口号, 如果没有, 那么设置端口为80
    char *pos = strstr(host, ":");
    if (pos)
        sscanf(pos, ":%d", port);

    //删除域名端口号
    for (int i = 0; i < (int)strlen(host); i++)
    {
        if (host[i] == ':')
        {
            host[i] = '\0';
            break;
        }
    }

    //获取下载文件名
    j = 0;
    for (int i = start; url[i] != '\0'; i++)
    {
        if (url[i] == '/')
        {
            if (i !=  int(strlen(url)) - 1)
                j = 0;
            continue;
        }
        else
            file_name[j++] = url[i];
    }
    file_name[j] = '\0';
}

unsigned long get_file_size(const char *filename)
{
    //通过系统调用直接得到文件的大小
    struct stat buf;
    if (stat(filename, &buf) < 0)
        return 0;
    return (unsigned long) buf.st_size;
}

void download(int client_socket, char *file_name, long content_length)
{
    /*下载文件函数*/
    long hasrecieve = 0;//记录已经下载的长度
    struct timeval t_start, t_end;//记录一次读取的时间起点和终点, 计算速度
    int mem_size = 8192;//缓冲区大小8K
    int buf_len = mem_size;//理想状态每次读取8K大小的字节流
    int len;

    //创建文件描述符
    int fd = open(file_name, O_CREAT | O_WRONLY, S_IRWXG | S_IRWXO | S_IRWXU);
    if (fd < 0)
    {
        printf("文件创建失败!\n");
        exit(0);
    }

    char *buf = (char *) malloc(mem_size * sizeof(char));

    //从套接字流中读取文件流
    long diff = 0;
    int prelen = 0;
    double speed;

    while (hasrecieve < content_length)
    {
        gettimeofday(&t_start, NULL ); //获取开始时间
        len = read(client_socket, buf, buf_len);
        write(fd, buf, len);
        gettimeofday(&t_end, NULL ); //获取结束时间

        hasrecieve += len;//更新已经下载的长度

        //计算速度
        if (t_end.tv_usec - t_start.tv_usec >= 0 &&  t_end.tv_sec - t_start.tv_sec >= 0)
            diff += 1000000 * ( t_end.tv_sec - t_start.tv_sec ) + (t_end.tv_usec - t_start.tv_usec);//us

        if (diff >= 1000000)//当一个时间段大于1s=1000000us时, 计算一次速度
        {
            speed = (double)(hasrecieve - prelen) / (double)diff * (1000000.0 / 1024.0);
            prelen = hasrecieve;//清零下载量
            diff = 0;//清零时间段长度
        }

        progress_bar(hasrecieve, content_length, speed);

        if (hasrecieve == content_length)
            break;
    }
}
int ExcuteMain(const char * pszDownloadLink,const char *pszOutFileName = NULL)
{ 
	char url[2048] = "127.0.0.1";//设置默认地址为本机,
    char host[64] = {0};//远程主机地址
    char ip_addr[16] = {0};//远程主机IP地址
    int port = 80;//远程主机端口, http默认80端口
    char file_name[256] = {0};//下载文件名
    puts("1: 正在解析下载地址...");
    const char * pszRelativeUrl = NULL;
	strcpy(url,pszDownloadLink);
	parse_url(url, host, &port, file_name,&pszRelativeUrl);//从url中分析出主机名, 端口号, 文件名

    if (pszOutFileName !=NULL)
    {
        printf("\t您已经将下载文件名指定为: %s\n", pszOutFileName);
        strcpy(file_name, pszOutFileName);
    }

    puts("2: 正在获取远程服务器IP地址...");
    get_ip_addr(host, ip_addr);//调用函数同访问DNS服务器获取远程主机的IP
    if (strlen(ip_addr) == 0)
    {
        printf("错误: 无法获取到远程服务器的IP地址, 请检查下载地址的有效性\n");
        return 0;
    }

    puts("\n>>>>下载地址解析成功<<<<");
    printf("\t下载地址: %s\n", url);
    printf("\t远程主机: %s\n", host);
    printf("\tIP 地 址: %s\n", ip_addr);
    printf("\t主机PORT: %d\n", port);
    printf("\t 文件名 : %s\n\n", file_name);

    //设置http请求头信息
    char header[2048] = {0};
    sprintf(header, \
            "GET %s HTTP/1.1\r\n"\
            "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"\
            "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537(KHTML, like Gecko) Chrome/47.0.2526Safari/537.36\r\n"\
            "Host: %s\r\n"\
            "Connection: keep-alive\r\n"\
            "\r\n"\
        ,pszRelativeUrl, host);

    puts("3: 创建网络套接字...");
    int client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket < 0)
    {
        printf("套接字创建失败: %d\n", client_socket);
        exit(-1);
    }

    //创建IP地址结构体
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip_addr);
    addr.sin_port = htons(port);

    //连接远程主机
    puts("4: 正在连接远程主机...");
    int res = connect(client_socket, (struct sockaddr *) &addr, sizeof(addr));
    if (res == -1)
    {
        printf("连接远程主机失败, error: %d\n", res);
        exit(-1);
    }

    puts("5: 正在发送http下载请求...");
    write(client_socket, header, strlen(header));//write系统调用, 将请求header发送给服务器
	
     puts("6: 正在解析http响应头...");
   	 std::string strHeaders;
	 GetHeader(strHeaders,client_socket);

    struct HTTP_RES_HEADER resp = parse_header(strHeaders.c_str());

    printf("\n>>>>http响应头解析成功:<<<<\n");

    printf("\tHTTP响应代码: %d\n", resp.status_code);
    if (resp.status_code != 200)
    {
		if(resp.status_code ==302)
		{
			printf("下载链接需要重定向到%s\n",resp.szLocation);
			return ExcuteMain(resp.szLocation,pszOutFileName);
		}
        printf("文件无法下载, 远程主机返回: %d\n", resp.status_code);
        return 0;
    }
    printf("\tHTTP文档类型: %s\n", resp.content_type);
    printf("\tHTTP主体长度: %lld字节\n\n", resp.content_length);


    printf("7: 开始文件下载...\n");
    download(client_socket, file_name, resp.content_length);
    printf("8: 关闭套接字\n");

    if (resp.content_length == get_file_size(file_name))
        printf("\n文件%s下载成功! ^_^\n\n", file_name);
    else
    {
        remove(file_name);
        printf("\n文件下载中有字节缺失, 下载失败, 请重试!\n\n");
    }
	shutdown(client_socket, 2);//关闭套接字的接收和发送
	printf("ExcuteMain successfully return 0\n");
	exit(0);
    return 0;
}
int main(int argc, char const *argv[])
{
	char url[256];
    if (argc == 1)
    {
        printf("您必须给定一个http地址才能开始工作\n");
        exit(0);
    }
    else
        strcpy(url, argv[1]);

	return ExcuteMain(url,argc ==3 ? argv[2]:NULL);
}
