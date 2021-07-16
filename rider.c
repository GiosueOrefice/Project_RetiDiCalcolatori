#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<errno.h>
#include<time.h>

void FullRead(int fd, void * buf, size_t count); //funzione per la gestione del valore di ritorno nella fullRead
void FullWrite(int fd,const void * buf, size_t count); //funzione per la gestione del valore di ritorno nella fullWrite
ssize_t fullRead(int fd, void * buf, size_t count);
ssize_t fullWrite(int fd, const void * buf, size_t count);

int main(int argc, char ** argv) {
  int sockfd; //descrittore per comunicare con il ristorante
  int n, id_cliente, ordineDisponibile, checkConsegna;
  int id_rider, sync = 0, consegnaEffettuata = 0;
  int scelta;
  struct sockaddr_in servaddr;

  if (argc != 3) {
    fprintf(stderr, "usage: %s <IPaddress> and <port>\n", argv[0]);
    exit(1);
  }
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    fprintf(stderr, "socket error\n");
    exit(1);
  }
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(atoi(argv[2]));
  if (inet_pton(AF_INET, argv[1], & servaddr.sin_addr) < 0) {
    fprintf(stderr, "inet_pton error for %s\n", argv[1]);
    exit(1);
  }
  if (connect(sockfd, (struct sockaddr * ) & servaddr, sizeof(servaddr)) < 0) {
    fprintf(stderr, "connect error\n");
    exit(1);
  }

  //ID RIDER
  srand(time(NULL));
  id_rider = 100 + rand() % 200;
  printf("RIDER CON ID: %d\n", id_rider);

  while (1) {
    ordineDisponibile = 0;
    while (ordineDisponibile == 0) {
      sync = 0;
      FullWrite(sockfd, & sync, sizeof(int)); //invia 0 per sincronizzarsi con il ristorante
      FullRead(sockfd, & ordineDisponibile, sizeof(int)); //Attende un nuovo ordine disponibile per la consegna
    }
    do {
      printf("NUOVO ORDINE DISPONIBILE\n[1] per prenderlo in carico\n");
      scanf("%d", & scelta);
      if (scelta != 1) printf("Opzione non disponibile. Sceglierne una tra quelle elencate.\n");
    } while (scelta != 1);

    sync = 1;
    FullWrite(sockfd, & sync, sizeof(int)); //presa in carico di un ordine
    FullRead(sockfd, & checkConsegna, sizeof(int)); //riceve 1 se un'ordine è ancora disponibile, 0 in caso contrario
    if (checkConsegna == 0) {
      printf("ORDINE GIA PRESO IN CARICO DA UN ALTRO RIDER E NON CI SONO ALTRI ORDINI DA CONSEGNARE\n");
      continue;
    }

    FullWrite(sockfd, & id_rider, sizeof(int));
    FullRead(sockfd, & id_cliente, sizeof(int));

    consegnaEffettuata = 1;
    printf("Consegna effettuata al cliente %d.\n", id_cliente);
    FullWrite(sockfd, & consegnaEffettuata, sizeof(int));
  }
  exit(0);
} //fine main

ssize_t fullRead(int fd, void * buf, size_t count) {
  size_t nleft;
  ssize_t nread;
  nleft = count;
  while (nleft > 0) {
    if ((nread = read(fd, buf, nleft)) < 0) {

      if (errno == EINTR)
        continue;
      else
        exit(nread);
    } else if (nread == 0) {
      break;
    } else {
      nleft -= nread;
      buf += nread;
    }
  }
  buf = 0;
  return nleft;
}

ssize_t fullWrite(int fd,
  const void * buf, size_t count) {
  size_t nleft;
  ssize_t nwritten;
  nleft = count;
  while (nleft > 0) {
    /* repeat until no left */
    if ((nwritten = write(fd, buf, nleft)) < 0) {
      if (errno == EINTR)
        continue;
      else exit(nwritten);

    }

    nleft -= nwritten;
    buf += nwritten;
  }
  return nleft;
}
void FullRead(int fd, void * buf, size_t count) {
  if ((fullRead(fd, buf, count)) == -1)
    perror("Errore nella read.\n");
}

void FullWrite(int fd,
  const void * buf, size_t count) {
  if ((fullWrite(fd, buf, count)) == -1)
    perror("Errore nella write.\n");
}