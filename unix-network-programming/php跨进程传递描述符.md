php 字节流 域套接字 跨进程传递描述符代码案例

服务端:

	<?php
	ini_set("display_errors",true);
	//
	$socket = socket_create(AF_UNIX,SOCK_STREAM,0);
	$dir = "/root/fpm";
	unlink($dir);
	socket_bind($socket,$dir);
	socket_listen($socket,100);
	
	while(1)
	{
	    $connfd = socket_accept($socket);
	    var_dump($connfd);
	    $data = ["controllen" => socket_cmsg_space(SOL_SOCKET, SCM_RIGHTS, 3)];
	    $msg = socket_recvmsg($connfd,$data,0);
	    var_dump($data);
	}?>


客户端



	<?php

	ini_set("display_errors", true);
	$socket = socket_create(AF_UNIX, SOCK_STREAM, 0);
	$dir = "/root/fpm";
	$result = socket_connect($socket, $dir);
	
	var_dump($result);
	$fp = fopen("/root/common.h", "r");
	var_dump($fp);
	$r = socket_sendmsg($socket, [
	        "iov" => [" "],
	        "control" =>
	        [
	            [
	                "level" => SOL_SOCKET,
	                "type" => SCM_RIGHTS,
	                "data" => [$fp]
	            ]
	        ]
	    ]
	    , 0);?>

php 数据报流 域套接字 跨进程传递描述符代码案例

	<?php
	$s = socket_create(AF_UNIX, SOCK_DGRAM, 0) or die("err");
	unlink($dir);
	$br = socket_bind($s, $dir) or die("err");
	
	$dir = "/root/fpm";
	
	while(1)
	{
	    $data = ["name" => [], "buffer_size" => 2000, "controllen" => socket_cmsg_space(SOL_SOCKET, SCM_RIGHTS, 3)];
	    $result = socket_recvmsg($s, $data, 0);
	    if($result)
	    {
	        var_dump($data);
	    }
	}

客户端:

	<?php
	$sends1 = socket_create(AF_UNIX, SOCK_DGRAM, 0) or die("err");
	var_dump($sends1);
	$r = socket_sendmsg($sends1, ["name" => ["path" => $dir], "iov" => ["test ", "thing", "\n"], "control" => [["level" => SOL_SOCKET, "type" => SCM_RIGHTS, "data" => [$sends1, STDIN, STDOUT, STDERR]]]], 0);
	var_dump($r);