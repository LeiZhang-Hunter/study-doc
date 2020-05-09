#syslog总结

####1.搭建syslog服务器

1)修改配置文件 vim /etc/rsyslog.conf

	#去掉这一部分数据可以使rsyslog.conf创建udp套接字监听514端口
	#$ModLoad imudp
	#$UDPServerRun 514

	#去掉这一部分数据可以使rsyslog.conf创建tcp套接字监听514端口
	#$ModLoad imtcp
	#$InputTCPServerRun 514



2）重启syslog的守护进程

service rsyslog restart

3)查看进程是否开启

lsof -i:514

	rsyslogd 25923 root    3u  IPv4  46086      0t0  TCP *:shell (LISTEN)
	rsyslogd 25923 root    4u  IPv6  46087      0t0  TCP *:shell (LISTEN)

我们可以使用lsof 命令查看rsyslog监控的514端口是否开启

rsyslog-mysql

####2.搭建syslog服务器的客户端

vim /etc/rsyslog.conf

	//TCP
	#*.* @@remote-host:514	

	//UDP
	#*.* @remote-host:514	


####3.php测试

使用logger测试

	logger -p local0.info "hello world123132444444445555555555"

出现在/var/log/messages中

使用php代码进行测试

	<?php
	openlog("/var/log/messages", LOG_PID | LOG_PERROR, LOG_CRON);
	$access = date("Y/m/d H:i:s");
	syslog(LOG_WARNING, "Unauthorized client: $access 192.168.59.126 test");
	closelog();

发现消息出现在服务器的

	/var/log/cron

