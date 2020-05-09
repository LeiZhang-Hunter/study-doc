#netmask分配规则

原文地址:https://www.computerhope.com/jargon/n/netmask.htm

A netmask is a 32-bit mask used to divide an IP address into subnets and specify the network's available hosts. In a netmask, two bits are always automatically assigned. For example, in 255.255.225.0, "0" is the assigned network address. In 255.255.255.255, "255" is the assigned broadcast address. The 0 and 255 are always assigned and cannot be used.

Below is an example of a netmask and an example of its binary conversion.

	Netmask:	255.	255.	255.	255
	Binary:	11111111	11111111	11111111	11111111
	Netmask length	8	16	24	32
Counting out the bits in the binary conversion allows you to determine the netmask length. Above is an example of a 32-bit address. However, this address is a broadcast address and does not allow any hosts (computers or other network devices) to be connected to it.

A commonly used netmask is a 24-bit netmask, as seen below.
	
	Netmask:	255.	255.	255.	0
	Binary:	11111111	11111111	11111111	00000000
	Netmask length	8	16	24	--
Using a 24-bit netmask, the network would be capable of 2,097,150 networks or 254 different hosts with an IP range of 192.0.1.x to 223.255.254.x, which is usually more than enough addresses for one network.

A simple formula can be used to determine the capable amount of networks a netmask can support.

	2^(netmask length - # of used segments) - 2

For example, if we used a netmask length of 24, having a netmask of 255.255.255.0 with three used segments, subtract three from the netmask length, e.g., 24-3 = 21. With this number determined, plug it into the above formula to get 2^21 - 2 = 2,097,150 total number of networks. You are subtracting two from this number because of the broadcast and network addresses that are already being used.

Another example is a netmask length of 16, having a netmask of 255.255.0.0 with two used segments. Using the above formula, you would get 2^14 - 2 = 16,382 total number of networks.

To determine the amount of hosts a netmask is capable of supporting, use the following formula.

	2^(# of zeroes) - 2

For example, with a netmask length of 24, as shown in the above chart, there are eight zeroes. Therefore, using the formula above, this would be 2^8 - 2 = 254 total number of hosts. Again, two is subtracted from this number to account for the broadcast and network addresses.

Again, another example of a netmask length of 16, there would be 16 zeroes. The formula in this case would be 2^16 - 2 = 65,534 total number of hosts.

Below is a breakdown of each of the commonly used network classes.

	Class	Netmask length	# of networks	# of hosts	Netmask
	Class A	8	126	16,777,214	255.0.0.0
	Class B	16	16,382	65,534	255.255.0.0
	Class C	24	2,097,150	254	255.255.255.0