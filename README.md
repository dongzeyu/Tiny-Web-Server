# Tiny-Web-Server

一个简单的web服务器，用来学习熟悉服务器的工作原理。

------

主要函数如下：
- main: 主函数调用`open_listenfd`函数打开一个监听套接字后，tiny执行典型的无限服务器循环，不断接收连接请求，执行事务，并关闭连接它的另一端。
- doit: 处理一个http事务。（目前只支持GET,POST,HEAD)
- clienterror: 检查一些明显的错误。（tiny缺乏实际服务器的许多错误处理特性）
- read_requesthdrs: 不使用请求报头的信息，tiny仅调用此函数来读取并忽略这些报头。
- parse_uri: 解析URI。tiny假设静态内容的准目录就是当前目录，动态内容的目录是./cgi-bin。
- serve_static: tiny提供集中常见的静态内容：html, 无格式的文本文件，jpg,mpeg等。
- serve_dynamic: 通过派生一个子进程并在子进程的上下文中运行一个CGI程序，提供各种类型的动态内容。

----

1. make生成可执行文件tiny
2. 执行./tiny port
3. 在浏览器访问： host:port
