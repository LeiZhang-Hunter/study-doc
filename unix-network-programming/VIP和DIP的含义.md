#VIP和DIP的区别

1.MIP 是一对一的地址映射，工作在第三层。

 主要应用在公网ip与内网ip的一一映射，内网对互联网提供服务。

2.DIP是动态的地址池。

通常用在拥有大量的注册ip，但又拥有大量的非注册ip小多对大多。

3.VIP 是端口地址映射，面对的是只有一个注册的公网ip地址，有大量的内网地址需要上互联网
   主要是通过ip地址协议有65535个端口，所以理论上一个注册的公网ip地址可以带动60000个pc机上互联网

	eg:   10.28.1.2    80         176.99.68.5   80
         	10.28.1.3    123       176.99.68.5   123

有两个内网的ip要上公网，通过一公网ip地址的不同端口达到要求 （防火墙部署模式nat结合） 

MIP是静态一对一的双向地址映射。 
VIP是地址+端口的映射，将不同地址的不同端口，映射到规定地址的规定端口。
DIP分2种一种是PAT，另一种就是用地址池中的地址映射。和CISCO 的NAT相同。

1、配置由外网到内网的VIP：
在E3端口上配置VIP，可将一个外网IP和端口号对应到一个内网IP。通过这种方法可以将WEB服务器，邮件服务器或其他服务放到内网，而从外网只看到一个公网IP，增加安全性。

1. Network > Interfaces > Edit（对于ethernet1）：输入以下内容，然后单击Apply：
Zone Name: Trust
IP Address/Netmask: 10.1.1.1/24（当地内部网网关IP）

2. Network > Interfaces > Edit（对于ethernet3）：输入以下内容，然后单击OK：
Zone Name: Untrust
IP Address/Netmask: 210.1.1.1/24（当地电信所分配的IP地址）

VIP
3. Network > Interfaces > Edit（对于ethernet3）> VIP：输入以下地址，然后单击Add：
Virtual IP Address: 210.1.1.10（对外的服务器地址）

4. Network > Interfaces > Edit（对于ethernet3）> VIP > New VIP Service：输入以下内容，然后单击OK：
Virtual Port: 80
Map to Service: HTTP (80)：（此例为WEB服务器的配置，如果是其他的服务器，就加入其服务与对应的端口号）
Map to IP: 10.1.1.10（此为服务器本身IP，内网IP）

策略
5. Policies > (From: Untrust, To: Global) > New：输入以下内容，然后单击OK：
Source Address:
Address Book:（选择）, ANY
Destination Address:
Address Book:（选择）, VIP(210.1.1.10)：对外服务器地址
Service: HTTP（WEB服务器选项）
Action: Permit


