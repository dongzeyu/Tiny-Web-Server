#include "tiny.h"


int main(int argc, char **argv)
{
    int listenfd, connfd;
    char hostname[MAXBUF], port[MAXBUF];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    if(argc != 2){
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    if(Signal(SIGCHLD, sigchild_handler) == SIG_ERR){
        unix_error("signal child handler error");
    }

    listenfd = Open_listenfd(argv[1]);
    while(1){
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
        Getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXBUF, port, MAXBUF, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(connfd);
        echo(connfd);
        Close(connfd);
    }
}

void sigchild_handler(int sig)
{
    int old_errno = errno;
    int status;
    pid_t pid;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0){

    }
    errno = old_errno;
}

void echo(int connfd)
{
    size_t n;
    char buf[MAXBUF];
    rio_t rio;

    Rio_readinitb(&rio, connfd);
    while((n = Rio_readlineb(&rio, buf, MAXBUF)) != 0){
        if(strcmp(buf, "\r\n") == 0)
            break;
        Rio_writen(connfd, buf, n);
    }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char buf[MAXBUF], body[MAXBUF];

    /*build http reponse body*/
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny web server</em>\r\n", body);
    /*build http response*/
    sprintf(buf, "HTTP/1.0, %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

int read_requesthdrs(rio_t *rp, char *method)
{
    char buf[MAXBUF];
    int len = 0;

    do{
        Rio_readlineb(rp, buf, MAXBUF);
        printf("%s", buf);
        if(strcasecmp(method, "POST") == 0 && strncasecmp(buf, "Content-Length:", 15) == 0)
            sscanf(buf, "Content-Length: %d", &len);
    }while(strcmp(buf, "\r\n"));

    return len;
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *ptr;

    if(!strstr(uri, "cgi-bin")){
        strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        if(uri[strlen(uri) - 1] == '/'){
            strcat(filename, "index.html");
        }
        return 1;
    }
    else{
        ptr = index(uri, '?');
        if(ptr){
            strcpy(cgiargs, ptr + 1);
            *ptr = '\0';
        }
        else{
            strcpy(cgiargs, "");
        }
        strcpy(filename, ".");
        strcat(filename, uri);
        return 0;
    }
}

void get_filetype(char *filename, char *filetype)
{
    if(strstr(filename, ".html")){
        strcpy(filetype, "text/html");
    }
    else if(strstr(filename, ".gif")){
        strcpy(filetype, "image/gif");
    }
    else if(strstr(filename, ".png")){
        strcpy(filetype, "image/png");
    }
    else if(strstr(filename, ".jpg")){
        strcpy(filetype, "image/jpeg");
    }
    else if(strstr(filename, ".mpeg")){
        strcpy(filetype, "video/mpeg");
    }
    else{
        strcpy(filetype, "text/plain");
    }
}

void serve_static(int fd, char *filename, int filesize, char *method)
{
    int srcfd;
    char *srcp, filetype[MAXBUF], buf[MAXBUF];

    /*send response headers*/
    get_filetype(filename, filetype);
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));
    printf("Response headers:\n");
    printf("%s", buf);

    if(strcasecmp(method, "HEAD") == 0){
        return;
    }

    /*send response body to client*/
    srcfd = Open(filename, O_RDONLY, 0);
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    Munmap(srcp, filesize);
}

void serve_dynamic(int fd, char *filename, char *cgiargs, char *method)
{
    char buf[MAXBUF], *emptylist[] = {NULL};

    /*return first part of HTTP respones*/
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "server: Tiny Web server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    if(Fork() == 0){
        setenv("QUERY_STRING", cgiargs, 1);
        setenv("REQUEST_METHOD", method, 1);
        Dup2(fd, STDOUT_FILENO);
        Execve(filename, emptylist, environ);
    }
}

void doit(int fd)
{
    int is_static;
    struct stat sbuf;
    char buf[MAXBUF], method[MAXBUF], uri[MAXBUF], version[MAXBUF];
    char filename[MAXBUF], cgiargs[MAXBUF];
    rio_t rio;

    /*read request line and headers*/
    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXBUF);
    printf("Request headers:\n");
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);
    if(!(strcasecmp(method, "GET") == 0 || strcasecmp(method, "POST") 
        ||strcasecmp(method, "HEAD") == 0)){
        clienterror(fd, method, "501", "Not implemented", 
        "Tiny does not implent this method");
        return ;
    }
    int param_len = read_requesthdrs(&rio, method);

    Rio_readnb(&rio, buf, param_len);

    /*parse uri from get request*/
    is_static = parse_uri(uri, filename, cgiargs);
    if(stat(filename, &sbuf) < 0){
        clienterror(fd, filename, "404", "Not found", "Server couldn't find this file");
        return ;
    }

    if(is_static){
        if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)){
            clienterror(fd, filename, "403", "Forbidden", "Server couldn't read the file");
            return;
        }
        serve_static(fd, filename, sbuf.st_size, method);
    }
    else{
        if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)){
            clienterror(fd, filename, "403", "Forbidden", "Server couldn't run the CGI program");
            return ;
        }
        if(strcasecmp(method, "GET") == 0)
            serve_dynamic(fd, filename, cgiargs, method);
        else
            serve_dynamic(fd, filename, buf, method);
    }

}

