/*********************************************************************************
  *FileName: myping.c
  *Author: Lan Qiao
  *Date: 04/19/2020
  *Description: Implementation of ping command in C.
                Support IPv4 ping operations.
**********************************************************************************/

#include "myping.h"

uint8_t buf[BUF_SIZE] = {0};  // buffer
struct timeval start, end;    // start/end time of a packet
struct timeval first, last;   // first/last sending time
char *pHost;                  // host
int nsend = 0;                // the number of packets sent
int nreceived = 0;            // the number of packets received
int sockfd;                   // socket file descriptor
double max = 0.0, min = 0.0, avg = 0.0, mdev = 0.0;  // variables for statistics

void handler() {
  gettimeofday(&last, NULL);
  statistics();
  exit(0);
}

void ping() {
  struct hostent *host;
  struct icmphdr sendicmp;
  in_addr_t inaddr;
  struct sockaddr_in to;
  struct sockaddr_in from;
  int fromlen = sizeof(struct sockaddr_in);
  int n;

  memset(&from, 0, sizeof(struct sockaddr_in));
  memset(&to, 0, sizeof(struct sockaddr_in));

  to.sin_family = AF_INET;

  if ((inaddr = inet_addr(pHost)) == INADDR_NONE) {
    // Domain name
    if ((host = gethostbyname(pHost)) == NULL) {
      printf("gethostbyname() error\n");
      exit(1);
    }
    to.sin_addr = *(struct in_addr *)host->h_addr_list[0];
  } else {
    // ip address
    to.sin_addr.s_addr = inaddr;
  }

  if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
    printf("socket() error\n");
    exit(1);
  }
  printf("ping %s (%s) %d bytes of data.\n", pHost, inet_ntoa(to.sin_addr),
         (int)ICMP_SIZE);

  // capture ctrl+c signal
  signal(SIGINT, handler);

  gettimeofday(&first, NULL);

  for (;;) {
    nsend++;
    memset(&sendicmp, 0, ICMP_SIZE);
    pack(&sendicmp, nsend);

    // get start time
    gettimeofday(&start, NULL);

    if (sendto(sockfd, &sendicmp, ICMP_SIZE, 0, (struct sockaddr *)&to,
               sizeof(to)) == -1) {
      printf("sendto() error\n");
      sleep(1);
      continue;
    }

    if ((n = recvfrom(sockfd, buf, BUF_SIZE, 0, (struct sockaddr *)&from,
                      &fromlen)) < 0) {
      printf("recvform() error\n");
      sleep(1);
      continue;
    }

    nreceived++;

    if (unpack(buf, n, inet_ntoa(from.sin_addr)) == -1) {
      printf("unpack() error\n");
    }

    sleep(1);
  }
}

void pack(struct icmphdr *icmp, int sequence) {
  icmp->type = ICMP_ECHO;
  icmp->code = 0;
  icmp->checksum = 0;
  icmp->un.echo.id = getpid();
  icmp->un.echo.sequence = sequence;
  gettimeofday(&start, NULL);
  icmp->checksum = checksum((uint16_t *)icmp, ICMP_SIZE);
}

int unpack(uint8_t *buf, int len, const uint8_t *addr) {
  int i, iphlen;
  struct iphdr *ip;
  struct icmphdr *icmp;
  double rtt;

  ip = (struct iphdr *)buf;

  // length of ip header
  iphlen = ip->ihl << 2;

  icmp = (struct icmphdr *)(buf + iphlen);

  // length of icmp
  len -= iphlen;

  if (len < 8) {
    printf("ICMP packets\'s length is less than 8\n");
    return -1;
  }

  if (icmp->type != ICMP_ECHOREPLY || icmp->un.echo.id != getpid()) {
    printf("ICMP packets not sent by pid\n");
    return -1;
  }

  // calculate round trip time
  gettimeofday(&end, NULL);
  rtt = timediff(&start, &end);

  // compute values for statistics
  if (min == 0.0 || min > rtt) min = rtt;
  if (max < rtt) max = rtt;
  avg += rtt;
  mdev += rtt * rtt;

  // print seq, ttl, rtt
  printf("%d bytes from %s : icmp_seq=%u ttl=%d rtt=%0.1lfms\n", len, addr,
         icmp->un.echo.sequence, ip->ttl, rtt);

  return 0;
}

uint16_t checksum(uint16_t *addr, int len) {
  uint32_t sum = 0;
  while (len > 1) {
    sum += *addr++;
    len -= 2;
  }

  if (len == 1) {
    sum += *(uint8_t *)addr;
  }

  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);

  return (uint16_t)~sum;
}

double timediff(struct timeval *begin, struct timeval *end) {
  // unit: ms
  double dif = (end->tv_sec - begin->tv_sec) * 1000.0 +
               (end->tv_usec - begin->tv_usec) / 1000.0;

  return dif;
}

void statistics() {
  // statistics
  printf("---  %s ping statistics ---\n", pHost);
  printf("%d packets transmitted, %d received, %d%% packet loss, time %dms\n",
         nsend, nreceived, (nsend - nreceived) / nsend * 100,
         (int)timediff(&first, &last));
  if (nreceived > 0) {
    avg /= nreceived;
    mdev = sqrt(mdev / nreceived - avg * avg);
    printf("rtt min/avg/max/mdev = %0.3lf/%0.3lf/%0.3lf/%0.3lf ms\n", min, avg,
           max, mdev);
  }
}

int main(int argc, uint8_t *argv[]) {
  // root is required
  if (getuid() != 0) {
    printf("Root privilege required\n");
    exit(1);
  }

  if (argc < 2) {
    printf("usage : %s [hostname/IP address]\n", argv[0]);
    exit(1);
  }

  pHost = argv[1];
  ping();

  return 0;
}