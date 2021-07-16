#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<errno.h>
#include<string.h>
#include<signal.h>
#include<time.h>

#define MAXPRODOTTI 25

struct ordine {
  int quantita[MAXPRODOTTI];
};
typedef struct ordine Ordine;

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

void FullRead(int fd, void * buf, size_t count); //funzione per la gestione del valore di ritorno nella fullRead
void FullWrite(int fd, const void * buf, size_t count); //funzione per la gestione del valore di ritorno nella fullWrite
ssize_t fullRead(int fd, void * buf, size_t count);
ssize_t fullWrite(int fd, const void * buf, size_t count);
void handler(int sig); //funzione per la gestione del segnale SIGINT
void creaOrdine(Ordine *ordine, int numProdotti);
void stampaRistoranti(int sockfd, int n);
int scegliRistorante(int n);
void stampaMenu(int sockfd,int numProdotti);
int serv;

int main(int argc, char ** argv) {
  int j, i;
  int sockfd, n, sync, idRider; //sockfd descrittore per comunicare con il rider
  int numProdotti;
  int id;
  int scelta = 1, scelta2, scelta3, sceltaR;
  Ristorante ristorante;
  Ordine ordine;
  struct sockaddr_in servaddr;
  
  if (argc != 2) {
    fprintf(stderr, "usage: %s <IPaddress>\n", argv[0]);
    exit(1);
  }
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    fprintf(stderr, "socket error\n");
    exit(1);
  }
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(1024);
  if (inet_pton(AF_INET, argv[1], & servaddr.sin_addr) < 0) {
    fprintf(stderr, "inet_pton error for %s\n", argv[1]);
    exit(1);
  }
  if (connect(sockfd, (struct sockaddr * ) & servaddr, sizeof(servaddr)) < 0) {
    fprintf(stderr, "connect error\n");
    exit(1);
  }
  //ID CLIENT
  srand(time(NULL));
  id = 1 + rand() % 100;

  serv = sockfd;
  signal(SIGINT, handler); //gestione segnale SIGINT

  do {
    do {
      system("clear");
      printf("BENVENUTO CLIENTE, IL TUO ID E': %d\n", id);
      sync = 1;

      FullWrite(sockfd, & sync, sizeof(int)); //invia numero per la sincronizzazione con il server
      FullRead(sockfd, & n, sizeof(n)); //riceve il numero dei ristoranti connessi

      if(n>0){
      	stampaRistoranti(sockfd,n);
      	sceltaR = scegliRistorante(n);
      }

      else {
      	do {
          printf("Attualmente non è connesso nessun ristorante.\n[0]AGGIORNA LISTA RISTORANTI. \n");
          scanf("%d", & sceltaR);
          if (sceltaR != 0) printf("Opzione non disponibile. Sceglierne una tra quelle elencate.\n");
        } while (sceltaR != 0);
      } 
    } while (sceltaR == 0);

    sync = 2;
    FullWrite(sockfd, & sync, sizeof(int));
    FullWrite(sockfd, & sceltaR, sizeof(n));
    FullRead(sockfd, & numProdotti, sizeof(int));
    stampaMenu(sockfd,numProdotti);

    do {
      printf("\n\n[1] CREA ORDINE.\n[2] ESCI.\nScelta... ");
      scanf("%d", & scelta3);
      if (scelta3 > 2 || scelta3 < 1) printf("Opzione non disponibile. Sceglierne una tra quelle elencate.\n");
    } while (scelta3 > 2 || scelta3 < 1);
    //CREARE ORDINE
    if (scelta3 == 1) 
      	creaOrdine(&ordine,numProdotti);
      
    else if (scelta3 == 2) {
      sync = 6;
      FullWrite(sockfd, & sync, sizeof(int));
      exit(0);
    }
    sync = 4;
    FullWrite(sockfd, & sync, sizeof(int));
    FullWrite(sockfd, & ordine, sizeof(Ordine));

    FullRead(sockfd, & idRider, sizeof(int));
    FullWrite(sockfd, & id, sizeof(int));
    printf("Consegna effettuata dal rider %d.\n\n", idRider);

    do {
      printf("[1] PER EFFETTUARE UN ALTRO ORDINE.\n[2] PER USCIRE.\nScelta: ");
      scanf("%d", & scelta2);
      if (scelta2 < 1 || scelta2 > 2) printf("Opzione non disponibile. Sceglierne una tra quelle elencate.\n");
    } while (scelta2 < 1 || scelta2 > 2);
  } while (scelta2 == 1);
  if (scelta2 == 2) {
    sync = 6;
    FullWrite(sockfd, & sync, sizeof(int));
    exit(0);
  }
  exit(0);
} //fine Main

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

void handler(int sig) {
  int sync = 6;
  FullWrite(serv, & sync, sizeof(int));
  exit(0);
}

 
void creaOrdine(Ordine *ordine, int numProdotti) {
	int j,n,scelta;

	printf("CREA IL TUO ORDINE\n");
    for (j = 0; j < MAXPRODOTTI; j++)
        ordine->quantita[j] = -1;
    scelta = 1;
	 while (scelta == 1) {
	    printf("Indice Prodotto: ");
	    scanf("%d", & j);
	    while (j > numProdotti || j<1) {
	        printf("Indice prodotto non presente nel menu. Scegline un altro tra 1 e %d.\nIndice prodotto: ", numProdotti);
	        scanf("%d", & j);
        }
        printf("Indicare la quantita' del prodotto scelto: ");
        scanf("%d", & n);
	    ordine->quantita[j - 1] = n;
	    do {
	        printf("[1] AGGIUNGI UN ALTRO PRODOTTO.\n[2] INVIA ORDINE.\n");
	        scanf("%d", & scelta);
	        if (scelta < 1 || scelta > 2) printf("Opzione non disponibile. Sceglierne una tra quelle elencate.\n");
	    } while (scelta < 1 || scelta > 2);
	}
}

void stampaRistoranti(int sockfd, int n) {
	Ristorante ristorante;
	int i;
	if (n > 0) {
        printf("\n  QUESTA E' LA LISTA DEI RISTORANTI CONNESSI\n");
        printf("------------------------------------------------\n");
        printf("   Nome ristorante\t  Tipologia ristorante\n");

        for (i = 0; i < n; i++) { //Riceve i ristoranti e stampa
          FullRead(sockfd, & ristorante, sizeof(Ristorante));
          printf("%d)    %s\t\t\t%s\n", i + 1, ristorante.nome, ristorante.tipo);
        }

        printf("\n");
	}
}

int scegliRistorante(int n){
	int sceltaR;
	 do {
          printf("Quale ristorante vuoi scegliere?(0 per aggiornare la lista dei ristoranti): ");
          scanf("%d", & sceltaR);
          if (sceltaR > n || sceltaR < 0) printf("Opzione non disponibile. Sceglierne una tra quelle elencate.\n");
        } while (sceltaR > n || sceltaR < 0); 

    return sceltaR;      
}

void stampaMenu(int sockfd, int numProdotti){
	int j;
	Prodotto prodotto;

    printf("\n\t  MENU\n---------------------------\n");
    printf("    Nome\tPrezzo\n");
    for (j = 0; j < numProdotti; j++) {
      FullRead(sockfd, & prodotto, sizeof(Prodotto));
      printf("%d)  %s \t %d€\n", j + 1, prodotto.nome, prodotto.prezzo);
    }
}