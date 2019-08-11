#include "messager_unix.h"

#include <signal.h>
#if defined(__FreeBSD__)
    typedef union sigval sigval_t;
#endif /* __FreeBSD__ */
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

static void send_port_number_to_process(pid_t pid, int port)
{
    sigval_t val;
    val.sival_int = port;
    sigqueue(pid, SIGUSR1, val);
}

typedef struct {
    int si_socket;
    int si_port;
} serverinfo_t;

serverinfo_t create_listen_socket()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    memset(&addr, 0, sizeof(addr_len));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (0 != bind(sock, (struct sockaddr*) &addr, addr_len)) {
    exit(1);
    }
    if (0 != listen(sock, 1)) {
    exit(1);
    }
    getsockname(sock, (struct sockaddr*) &addr, &addr_len);
    serverinfo_t result;
    result.si_socket = sock;
    result.si_port = ntohs(addr.sin_port);
    return result;
}

static void serve_data(const char * data, size_t data_len, int socket)
{
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    int fd = accept(socket, (struct sockaddr*) &addr, &addr_len);
    if (-1 != fd) {
        int status = send(fd, data, data_len, 0);
        if (status == -1) {
            exit(2);
        }
    }
}
void MessagerUnix::sendMessage(pid_t receiver, const QString &message)
{
    QByteArray utf8 = message.toUtf8();
    size_t buffer_len = utf8.size() + 1;
    char * buffer = (char*) malloc(buffer_len * sizeof(char));
    strcpy(buffer, utf8.constData());
    serverinfo_t si;
    si = create_listen_socket();
    send_port_number_to_process(receiver, si.si_port);
    serve_data(buffer, buffer_len, si.si_socket);
    free(buffer);
}
