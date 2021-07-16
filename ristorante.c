#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<errno.h>
#include<string.h>
#include <signal.h>

#define MAXPRODOTTI 25

struct prodotto {
  char nome[4096];
  int prezzo;
};
typedef struct prodotto Prodotto;

struct ristorante {
  char nome[4096];
  char tipo[4096];

};
typedef struct ristorante Ristorante;

struct ordine {

  int quantita[MAXPRODOTTI];

};
typedef struct ordine Ordine;

struct ListaOrdini {
  Ordine ordine;
  int statoOrdine;
  struct ListaOrdini * nxt;
};
typedef struct ListaOrdini listOrdini;

listOrdini * creaListOrdini();
void aggiungere_fine(listOrdini ** fine, Ordine ordine);
void aggiungi_in_testa(listOrdini ** head, Ordine ordine);
void setStatoOrdine(listOrdini ** punt);

void FullRead(int fd, void * buf, size_t count);
void FullWrite(int fd, const void * buf, size_t count);
ssize_t fullRead(int fd, void * buf, size_t count);
ssize_t fullWrite(int fd, const void * buf, size_t count);
void handler(int sig);
int numOrdini = 0;
int numOrdiniTotale = 0;
int serv;
listOrdini * punt;
listOrdini * head, * fine;

int main(int argc, char ** argv) {
  int j, i;
  int sockfd, list_fd;
  int n, sync, numProdotti;
  int fd_client;
  int sync_rider, sync_server;
  int idRider, idClient;
  struct sockaddr_in servaddr;
  int conf;

  Ristorante datiRistorante;
  Ordine ordine;

  head = creaListOrdini();
  punt = head;
  fine = punt;

  numProdotti = 3;
  Prodotto prodotti[numProdotti];

  strcpy(datiRistorante.nome, "Sorbillo");
  strcpy(datiRistorante.tipo, "Pizzeria");

  strcpy(prodotti[0].nome, "Margherita");
  strcpy(prodotti[1].nome, "Marinara");
  strcpy(prodotti[2].nome, "Napoletana");
  prodotti[0].prezzo = 4;
  prodotti[1].prezzo = 3;
  prodotti[2].prezzo = 7;

  if ((list_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(0);
  }

  struct sockaddr_in serv_add;
  serv_add.sin_family = AF_INET;
  serv_add.sin_port = htons(1025); // port 1025
  serv_add.sin_addr.s_addr = htonl(INADDR_ANY);

  int enable = 1;
  if (setsockopt(list_fd, SOL_SOCKET, SO_REUSEADDR, & enable, sizeof(int)) < 0) {
    perror("setsockopt");
    exit(1);
  }
  if (setsockopt(list_fd, SOL_SOCKET, SO_REUSEPORT, & enable, sizeof(int)) < 0) {
    perror("setsockopt");
    exit(1);
  }

  if (bind(list_fd, (struct sockaddr * ) & serv_add, sizeof(serv_add)) < 0) {
    perror("bind");
    exit(1);
  }

  if (listen(list_fd, 1024) < 0) {
    perror("listen");
    exit(1);
  }

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    fprintf(stderr, "socket error\n");
    exit(1);
  }
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(1024);
  if (inet_pton(AF_INET, "127.0.0.1", & servaddr.sin_addr) < 0) {
    fprintf(stderr, "inet_pton error for %s\n", argv[1]);
    exit(1);
  }
  if (connect(sockfd, (struct sockaddr * ) & servaddr, sizeof(servaddr)) < 0) {
    fprintf(stderr, "connect error\n");
    exit(1);
  }

  serv = sockfd;
  sync_server = 0;
  FullWrite(sockfd, & sync_server, sizeof(int)); 

  FullWrite(sockfd, & datiRistorante, sizeof(Ristorante)); //invio dei dati al server

  int scelta;

  fd_set rset;
  fd_set allset;
  int client[FD_SETSIZE];

  int maxd = list_fd;
  int maxi = -1;

  if (sockfd > list_fd)
    maxd = sockfd;

  for (i = 1; i < FD_SETSIZE; i++)
    client[i] = -1;

  client[0] = list_fd;
  client[1] = sockfd;

  struct sockaddr_in cliaddr;
  socklen_t cliaddr_len;

  int connd;
  int sockd;
  int ready;

  FD_ZERO( & allset);
  FD_SET(list_fd, & allset);
  FD_SET(sockfd, & allset);

  signal(SIGINT, handler);
  while (1) {
    // setta  l’insieme  dei  descrittori  da  controllare  
    rset = allset;
    /* 
      chiama la select: 
        - esce quando un descrittore è pronto
        - restituisce il numero di descrittori pronti
    */

    if ((ready = select(maxd + 1, & rset, NULL, NULL, NULL)) < 0)
      perror("errore nella select");

    // controlla se il socket di ascolto è leggibile 
    if (FD_ISSET(list_fd, & rset)) {

      cliaddr_len = sizeof(cliaddr);

      // invoca la accept
      if ((connd = accept(list_fd, (struct sockaddr * ) & cliaddr, & cliaddr_len)) < 0)
        perror("errore nella accept");

      // inserisce il socket di connessione in un posto libero di client
      for (i = 0; i < FD_SETSIZE; i++)

        if (client[i] < 0) {
          client[i] = connd;
          break;
        }

      // se non ci sono posti segnala errore
      if (i == FD_SETSIZE)
        perror("troppi client");

      FD_SET(connd, & allset);

      // registra il socket ed aggiorna maxd
      if (connd > maxd)
        maxd = connd;

      if (i > maxi)
        maxi = i;

      if (--ready <= 0)
        continue;
    }

    // controlla tutti i socket di ascolto se sono leggibili
    for (i = 1; i <= maxi; i++) {

      if ((sockd = client[i]) < 0)
        continue;

      if (FD_ISSET(sockd, & rset)) {

        FullRead(sockd, & sync, sizeof(int));

        switch (sync) {

        case -1: { //Il ristorante invia i prodotti al Server
          sync_server = 3;
          FullWrite(sockfd, & sync_server, sizeof(int));
          FullWrite(sockfd, & numProdotti, sizeof(int));

          //for (i = 0; i < numProdotti; i++)
            FullWrite(sockfd,prodotti, sizeof(prodotti));
          break;
        }

        case -2: { //IL ristorante riceve l'ordine del cliente dal server
          FullRead(sockfd, & ordine, sizeof(Ordine));
          printf("Nuovo ordine disponibile.\n");
          for (i = 0; i < numProdotti; i++) {
            if (ordine.quantita[i] != -1)
              printf("Nome Prodotto: %s  Quantità Prodotto: %d\n", prodotti[i].nome, ordine.quantita[i]);
          }
          aggiungere_fine( & fine, ordine);

          break;
        }

        case 0: { //Il rider si mette in attesa di un'ordine disponibile
          if (numOrdini > 0) //se non ci sono ordini 
            conf = 1;
          else
            conf = 0;

          FullWrite(sockd, & conf, sizeof(int));
          break;
        }

        case 1: { //Dopo che il rider ha accettato di consegnare, controlla se non è stato preso in carico da un altro
          if (punt -> statoOrdine == 0) {
            conf = 1;
            FullWrite(sockd, & conf, sizeof(int));
            sync_server = 5;
            FullRead(sockd, & idRider, sizeof(int));
            FullWrite(sockfd, & sync_server, sizeof(int));
            FullWrite(sockfd, & idRider, sizeof(int));
            int consegna;
            FullRead(sockfd, & idClient, sizeof(int));
            FullWrite(sockd, & idClient, sizeof(int));
            FullRead(sockd, & consegna, sizeof(int));
            FullWrite(sockfd, & consegna, sizeof(int));
            printf("Consegna effettuata dal rider: %d al cliente %d.\n", idRider, idClient);
            setStatoOrdine( & punt);
          } else { //ordine già preso in carico da un altro rider
            conf = 0;
            FullWrite(sockd, & conf, sizeof(int));
          }
          break;
        }
        default:
          break;
        }

        sync = -1;
        // se la coda finisce chiudi

        if (--ready <= 0)
          break;
      }

    }
    if (numOrdini > 0 && punt -> statoOrdine == 1)
      punt = punt -> nxt;
  }

}

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

listOrdini * creaListOrdini() {
  listOrdini * x;
  x = NULL;
  return x;
}

void aggiungere_fine(listOrdini ** punt, Ordine ordine) {
  if (numOrdiniTotale == 0)
    aggiungi_in_testa(punt, ordine);
  else {
    listOrdini * c;
    c = malloc(sizeof(listOrdini));
    c -> ordine = ordine;
    c -> statoOrdine = 0;

    ( * punt) -> nxt = c;

    c -> nxt = NULL;
    numOrdini++;
    numOrdiniTotale++;
    fine = c;
  }
}

void aggiungi_in_testa(listOrdini ** head, Ordine ordine) {
  listOrdini * x;
  x = malloc(sizeof(listOrdini));
  x -> ordine = ordine;
  x -> statoOrdine = 0;
  x -> nxt = * head;
  * head = x;
  numOrdini++;
  numOrdiniTotale++;
  punt = fine;
}

void setStatoOrdine(listOrdini ** punt) {
  ( * punt) -> statoOrdine = 1;
  numOrdini--;
}

void handler(int sig) {

  int sync_server = 7;
  FullWrite(serv, & sync_server, sizeof(int));
  exit(0);
}