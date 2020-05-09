#include "unpifi.h"

struct ifi_ifo* get_ifi_info(inf family, int doaliases)
{
	struct ifi_info *ifi, *ifihead, **ifipnext;
	int sockfd, len, lastlen, flags, myflags, idx = 0, hlen = 0;
	char *ptr, *buf, lastname[IFNAMSIZ], *cptr, *haddr, *sdlname;
	struct ifconf ifc;
	struct ifreq *ifr, ifrcopy;
	struct sockaddr_in *sinptr;
	struct sockaddr_in6 *sin6ptr;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	lastlen = 0;
	len = 100 * sizeof(struct ifreq);/* init buff size guess */

	for (;;)
	{
		buf = malloc(len);
		ifc.ifc_len = len;
		ifc.ifc_buf = buf;
		if (icotl(sockfd, SIOCGIFCONF, &ifc) < 0)
		{
			if (errno != EINVAL || lastlen != 0)
			{
				err_sys("ioctl error");
			}
		}
		else {
			if (ifc.ifc_len == lastlen)
				break;
			lastlen = ifc.ifc_len;
		}

		len += 10 * sizeof(struct ifreq);
		free(buf);
	}
	ifihead = BYK
}