/*****************************************************************************
 *                                                                           *
 *                      10101000100010001010101100101010101                  *
 *                      0101010101               0101010100                  *
 *                      0101010101               0101010100                  *
 *                      1010101010        10101  0101110101                  *
 *                      1010101010       01  10  0101011101                  *
 *                      1010111010       01      0101010101                  *
 *                       010101010  01  010 0101 010101010                   *
 *                       010101010  01  010  01  010101011                   *
 *                        01010101  01  010  01  01010101                    *
 *                        11010101  01  010  01  01010110                    *
 *                         1010101  01  010  01  0101000                     *
 *                         1110101  01  010  01  0101001                     *
 *                          110101      010      010001                      *
 *                           10101     001       01011                       *
 *                            1101  10101        0101                        *
 *                             110               011                         *
 *                              10               01                          *
 *                               01             10                           *
 *                                 00         10                             *
 *                                  01       01                              *
 *                                    01   01                                *
 *                                      010                                  *
 *                                       1                                   *
 *                                                                           *
 *                        Universidade Técnica de Lisboa                     *
 *                                                                           *
 *                          Instituto Superior Técnico                       *
 *                                                                           *
 *                                                                           *
 *                                                                           *
 *                    RIC - Redes Integradas de Comunicações                 *
 *                               ATM Application                             *
 *                                                                           *
 *                       Professor Paulo Rogério Pereira                     *
 *                                                                           *
 *                                                                           *
 *****************************************************************************
 * @filename: strs.c                                                         *
 * @description: ATM streaming server.                                       *
 * @language: C                                                              *
 * @compiled on: cc/gcc                                                      *
 * @last_update_at: 2007-11-01                                               *
 *****************************************************************************
 * @students that colaborated on this file:                                  *
 *  57442 - Daniel Silvestre - daniel.silvestre@tagus.ist.utl.pt             *
 *  57476 - Hugo Miguel Pinho Freire - hugo.freire@tagus.ist.utl.pt          *
 *****************************************************************************
 * @changelog for this file:                                                 *
 *	No changelog was kept for this file.                  									 *
 *****************************************************************************
 * @comments for this file:                                                  *
 *****************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>
#include <math.h> /* ceil() */
#include <signal.h> /* signal() */
#include <atm.h>
#include <atmsap.h>
#include "../include/str.h"

int buflen = DEFAULT_SDU_SIZE;	/* length of buffer */
int bitrate;
int sdu_size;
char *host;
char name[255];
char filename[255];
static struct timeval start_time;	/* Time at which timing started */
static struct timeval stop_time;	/* Time at which timing stopped */
char fmt = 'K';

/*****************************************************************************
 * @name						usage
 * @description  		prints to stderr the correct syntax usage
 * @param						void
 * @return          void
 */
void usage()
{
    fprintf(stderr,"usage: %s [itf.]vpi.vci filename rate [sdu_size] \n",name);
    exit(1);
}

/*****************************************************************************
 * @name						die_with_error
 * @description     perror's the given message, then exits with error 
 * @param						const char *message
 * @return          void
 */
void die_with_error(const char *message)
{
    perror(message);
    exit(1);
}

/*****************************************************************************
 * @name            nwrite
 * @description     writes data on the network 
 * @param           file descriptor, a buffer and his lenght
 * @return          on success returns int with number of bytes written, else -1 
 */
int nwrite(int s,char *buf,int l)
{
    int n;

    if ((n = write(s,buf,l)) < 0)
    {
        if(errno == EPIPE)
        {
            close(s);
            return -1;
        }
        else die_with_error("write() failed");
    }
    return n;
}

/*****************************************************************************
 * @name            nread
 * @description     reads data from the network 
 * @param           file descriptor, a bufer and his lenght
 * @return          on success, returns int with number of bytes red, else -1
 */
int nread(int s,char *buf,int l)
{
    int n;

    if((n = read(s,buf,l)) < 0)
    {
        if(errno == EPIPE)
        {
            close(s);
            return -1;
        }
        else die_with_error("read() failed");
    }
    return n;
    /*
    	int i, n;
    	char *buftmp = (char *) buf;
    	
    	for(i = 0;i < lenght;i += n){
    		if((n = read(s,buftmp + i,lenght - i)) <= 0)
    			die_with_error("recv() failed");
    	}
    	
    	return n;
    */
}

/*****************************************************************************
 * @name						fsize
 * @description     get the size for the given file
 * @param           path to file
 * @return          on success, returns the file size, otherwise returns -1
 */
long int fsize(const char *path)
{
    int i;
    struct stat buf;

    if((i=lstat(path, &buf)) < 0) return i;
    return buf.st_size;
}

/*****************************************************************************
 * @name						delay
 * @description     creates a delay with the given time
 * @param           time in microseconds
 * @return          void
 */
void delay(long int us)
{
    struct timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = us;
    (void)select(1,0,0,0,&tv);
}

/*****************************************************************************
 * @name            tvsub
 * @description     gets the diff between two times 
 * @param           two given times
 * @return					void
 */
static void tvsub(struct timeval *tdiff,struct timeval *t1,struct timeval *t0)
{

    tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
    tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
    if (tdiff->tv_usec < 0)
    {
        tdiff->tv_sec--;
        tdiff->tv_usec += 1000000;
    }
}

/*****************************************************************************
 * @name            outfmt
 * @description     gives the bytes in wanted bytes/bits unit   
 * @param           bytes
 */
char *outfmt(double b)
{
    static char obuf[50];

    switch (fmt)
    {
    case 'G':
        sprintf(obuf, "%f GB", b / 1024.0 / 1024.0 / 1024.0);
        break;
    default:
    case 'K':
        sprintf(obuf, "%f KB", b / 1024.0);
        break;
    case 'M':
        sprintf(obuf, "%f MB", b / 1024.0 / 1024.0);
        break;
    case 'g':
        sprintf(obuf, "%f Gbit", b * 8.0 / 1024.0 / 1024.0 / 1024.0);
        break;
    case 'k':
        sprintf(obuf, "%f Kbit", b * 8.0 / 1024.0);
        break;
    case 'm':
        sprintf(obuf, "%f Mbit", b * 8.0 / 1024.0 / 1024.0);
        break;
    }
    return obuf;
}

/*****************************************************************************
 * @name						main
 * @description  		gets clients requests and sends them the asked files
 * @param						argc, argv
 * @return					returns 0 on a successful exit
 */
int main(int argc,char **argv)
{

    struct sockaddr_atmsvc satm, catm;
    struct atm_sap sap;
    struct sockaddr_in frominet;
    struct atm_qos qos;
    unsigned int fromlen, peerlen;
    int c, s, i, m, n, y;
    long l, d;
    FILE *fd;
    char *buf, *cwd;
    double realt, nbytes, mbps;
    struct timeval td, ts, tf;

    memcpy(name,argv[0],strlen(argv[0]));
    /* verify syntax */
    if(argc != 2) usage();

    memset(&satm,0,sizeof(satm));

    if(text2atm(argv[1],(struct sockaddr *) &satm,sizeof(satm),
                T2A_PVC | T2A_SVC  | T2A_UNSPEC | T2A_WILDCARD | T2A_NAME) < 0) usage();
    /* create socket */
    if((s = socket(satm.sas_family,SOCK_DGRAM,0)) < 0)
        die_with_error("socket() failed");

    memset(&qos,0,sizeof(qos));
    /* configure qos */
    qos.aal = ATM_AAL5;
    qos.rxtp.traffic_class = qos.txtp.traffic_class = ATM_UBR;
    qos.rxtp.max_sdu = qos.txtp.max_sdu = MAX_SDU_SIZE;
    /* set options on socket */
    if(setsockopt(s,SOL_ATM,SO_ATMQOS,&qos,sizeof(qos)) < 0)
        die_with_error("setsockopt() failed");
    /* configure bhli for svc */
    if (satm.sas_family == AF_ATMSVC)
    {
        memset(&sap,0,sizeof(sap));
        sap.bhli.hl_type = ATM_HL_USER;
        sap.bhli.hl_length = strlen(STR_BHLI);
        memcpy(sap.bhli.hl_info,STR_BHLI,strlen(STR_BHLI));
        if (setsockopt(s,SOL_ATM,SO_ATMSAP,&sap,sizeof(sap)) < 0)
            die_with_error("setsockopt()");
    }
    /* ignore sigpipe */
    signal(SIGPIPE,SIG_IGN);
    /* pvc/svc bind */
    if(bind(s,(struct sockaddr *) &satm,
            satm.sas_family == AF_ATMPVC ?
            sizeof(struct sockaddr_atmpvc):sizeof(struct sockaddr_atmsvc)) < 0)
        die_with_error("bind() failed");

    if(satm.sas_family == AF_ATMSVC)
    {
        if ((host = (char *) malloc(MAX_ATM_ADDR_LEN+1)) == (char *)NULL)
            die_with_error("malloc() failed");
        listen(s,0);
    }
    /* change to directory containg files */
    buf = getcwd(NULL,0);
    cwd = (char *) malloc(strlen(buf)+strlen(DEFAULT_FILE_DIR));
    sprintf(cwd,"%s%s",buf,DEFAULT_FILE_DIR);
    chdir(cwd);
    free(buf);
    free(cwd);
    /* adjust the buffer for default sdu size*/
    if ((buf = (char *)malloc(buflen)) == (char *)NULL)
        die_with_error("malloc() failed");
    /* main cycle */
    while (1)
    {
        if(satm.sas_family == AF_ATMSVC)
        {
            fromlen = sizeof(frominet);
            /* accept clients */
            if ((c = accept(s,(struct sockaddr *) &frominet,&fromlen) ) < 0)
                die_with_error("accept() failed");
            peerlen = sizeof(catm);
            /* get client host */
            if (getpeername(c,(struct sockaddr *) &catm,&peerlen) < 0)
                die_with_error("getpeername() failed");
            if (atm2text(host,MAX_ATM_ADDR_LEN+1,(struct sockaddr *) &catm,
                         A2T_NAME | A2T_PRETTY) < 0)
                strcpy(host,"<invalid>");
            /* output client host */
            fprintf(stderr,"strs: accept from %s.\n",host);
            /* save server socket for later use */
            y = s;
            s = c;
        }
        /* receive filename */
        if((m = nread(s,buf,buflen)) < 0) break;
        memset(filename,'\0',sizeof(filename));
        memcpy(filename,buf,strlen(buf));
        /* receive bitrate */
        if((m = nread(s,buf,buflen)) < 0) break;
        bitrate = atoi(buf);
        /* receive sdu size */
        if((m = nread(s,buf,buflen)) < 0) break;
        sdu_size = atoi(buf);
        /* output info about request */
        fprintf(stderr,"strs: %s was requested at %i Bytes/sec with a SDU size of %i Bytes.\n",
                filename,bitrate,sdu_size);
        /* verify if filename exists, if not warn the client */
        if((l = fsize(filename)) < 0)
        {
            memcpy(buf,FILE_NOT_FOUND,strlen(FILE_NOT_FOUND));
            if((m = nwrite(s,buf,buflen)) < 0) break;
            fprintf(stderr,"strs: %s was not found.\n",filename);
        }
        else
        {
            /* send the file size */
            fprintf(stderr,"strs: %s has %ld Bytes.\n",filename,l);
            sprintf(buf,"%ld",l);
            if((m = nwrite(s,buf,buflen)) < 0) break;
            /* open the requested file */
            if((fd = fopen(filename,"rb")) == NULL)
                die_with_error("fopen() failed");
            /* adjust the buffer */
            if(buflen != sdu_size)
            {
                free(buf);
                buflen = sdu_size;
                if ((buf = (char *)malloc(buflen)) == (char *)NULL)
                    die_with_error("malloc() failed");
            }
            /* get the delay */
            d = ((buflen / (double) bitrate)*1e06);
            /* start the timer */
            gettimeofday(&start_time, (struct timezone *)0);

            /* send the file */
            for(nbytes = 0, i = 0,n = buflen;i < l;i += buflen)
            {
                if(i + buflen > l) n = l - i;
                fread(buf,n,1,fd);
                gettimeofday(&ts, (struct timezone *)0);
                if((m = nwrite(s,buf,buflen)) < 0)
                {
                    fprintf(stderr,"strs: client %s died.\n",host);
                    i = -1;
                    break;
                }
                gettimeofday(&tf, (struct timezone *)0);
                /* delay for cbr */
                delay(d - tf.tv_usec + ts.tv_usec);
                nbytes += m;
            }
            /* transmission was aborted, reset values and continue */
            if(i < 0)
            {
                s = y;
                continue;
            }
            /* stop the timer */
            gettimeofday(&stop_time, (struct timezone *)0);
            /* get real time */
            tvsub(&td, &stop_time, &start_time);
            realt = (double)td.tv_sec + ((double)td.tv_usec /
                                         (double)1000000.0);
            /* safeback if really fast */
            if(realt <= 0.0)
                realt = 0.001;
            /* get mbps */
            mbps = (double)(nbytes*8)/realt/1000000.0;
            /* output the stats regarding last transfer */
            fprintf(stderr,"strs: %.0f bytes in %f real seconds = %s/sec (%f Mb/sec).\n",
                    nbytes,realt,outfmt((double)nbytes/realt),mbps);
            /* close file descriptor */
            fclose(fd);
            /* reset buffer to default sdu size */
            if(buflen != DEFAULT_SDU_SIZE)
            {
                free(buf);
                buflen = DEFAULT_SDU_SIZE;
                if ((buf = (char *)malloc(buflen)) == (char *)NULL)
                    die_with_error("malloc() failed");
            }
        }
        /* close client socket */
        if(satm.sas_family == AF_ATMSVC)
        {
            close(c);
            s = y;
        }
    }
    return 0;
}
