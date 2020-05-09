#syslog配置方法

1.修改rsyslog的配置文件

	vim /etc/rsyslog.conf

在行尾部加入

	*.*  @127.0.0.1:514

注意不要打开rsyslog的udp模块

2.重启syslog

	service rsyslog restart

3.修改php 配置文件

	vim /etc/php.ini

4.修改php配置

	error_log = syslog

5.重启php-fpm