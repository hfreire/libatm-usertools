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
 * @description: ATM streaming server using PVC and AAL5 emulation over ALL0.*
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

#include "../include/str.h"
#include "../include/crc32h.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <math.h> /* ceil() */
#include <signal.h> /* signal() */
#include <atm.h>
#include <atmsap.h>

int buflen = DEFAULT_SDU_SIZE;	/* length of buffer */
int bitrate;
int sdu_size;
char *host;
char name[255];
char filename[255];
static struct timeval start_time;	/* Time at which timing started */
static struct timeval stop_time;	/* Time at which timing stopped */
char fmt = 'K';
struct sockaddr_atmpvc satm;

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
 * @name						crcerr
 * @description  		prints crc error
 * @param				    none		
 * @return          void
 */
void crcerr()
{
    fprintf(stderr,"strs0: crc errors were detected.\n");
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
 * @name            mkheader
 * @description     make header for cell 
 * @param           integer telling if last cell
 */
unsigned long mkheader(int last)
{
    unsigned long header[1];
    int pti, gfc, clp, vci;
    short vpi;

    gfc = clp = 0;
    /* set vpi and vci */
    vpi = satm.sap_addr.vpi;
    vci = satm.sap_addr.vci;
    /* if last cell, turn pti on */
    if(last) pti = 1;
    else pti = 0;
    /* apply masks */
    *header = ((gfc << ATM_HDR_GFC_SHIFT) & ATM_HDR_GFC_MASK) |
              ((vpi << ATM_HDR_VPI_SHIFT) & ATM_HDR_VPI_MASK) |
              ((vci << ATM_HDR_VCI_SHIFT) & ATM_HDR_VCI_MASK) |
              ((pti << ATM_HDR_PTI_SHIFT) & ATM_HDR_PTI_MASK) | clp;
    return *header;
}

/*****************************************************************************
 * @name            mkcell
 * @description     makes a cell from a pdu
 * @param           cell number in pdu, buffer and integer telling if last cell
 */
char* mkcell(int n,char *buf, int last)
{
    int i,j;
    char *cell;
    unsigned long header;

    /* alloc memory for cell */
    if ((cell = (char *)malloc(ATM_AAL0_SDU * sizeof(char))) == (char *)NULL)
        die_with_error("malloc() failed");
    /* create a header */
    header = mkheader(last);

    memset(cell,0,sizeof(cell));
    memcpy(cell,&header,sizeof(header));
    /* insert buffer in cell */
    for(i = n * ATM_CELL_PAYLOAD, j = 0; i < n * ATM_CELL_PAYLOAD + ATM_CELL_PAYLOAD;i++,j++)
    {
        cell[j+4] = buf[i];
    }
    return cell;
}

/*****************************************************************************
 * @name            cwrite
 * @description     cell write 
 * @param           socket, buffer and buffer lenght
 */
int cwrite(int s,char* buf,int l)
{
    int padding, n, num;
    unsigned char *pdu;
    int i,size;
    unsigned long header[2];
    unsigned int crc, lenght;
    char *cell;
    int pdu_size;

    padding = n = num = pdu_size = 0;

    /* get padding */
    if((l + PDU_CAB) <= ATM_CELL_PAYLOAD)
        padding = 40 - l;
    else
    {
        if(((l + PDU_CAB) % ATM_CELL_PAYLOAD) == 0)
            padding = 0;
        else
        {
            n = (l + PDU_CAB) / ATM_CELL_PAYLOAD + 1;
            padding = n * ATM_CELL_PAYLOAD - (l + PDU_CAB);
        }
    }
    /* alloc memory for pdu */
    pdu_size = (l + padding + PDU_CAB);
    pdu = malloc((pdu_size * sizeof(unsigned char)));

    memset(pdu,0,sizeof(pdu));
    /* insert data */
    memcpy(pdu,buf,l);

    /* insert padding */
//  num = strlen(pdu);
//  for(i = num; i < (num + padding) ;i++)
//    pdu[i] = 'L';

    /* insert lenght on pdu */
    lenght = l & 0x0000ffff;
    *header = htonl(lenght | ( 0 << 2));
    *(header+1) = 0;
    memcpy(pdu+l+padding,&header,sizeof(header));
    /* insert crc on pdu */
    crc = ~update_crc(0xffffffff,(char *) pdu,(l+padding+4));
    *(header+1) = htonl(crc);
    memcpy(pdu+padding+l,&header,sizeof(header));
    /* make cells and send them*/
    for(n = 0,i = 0;i<(pdu_size)/ATM_CELL_PAYLOAD;i++,n += size)
    {
        cell = mkcell(i,(char *) pdu, i == (((pdu_size)/ATM_CELL_PAYLOAD)-1) ? 1:0);
        if((size = write(s,cell,ATM_AAL0_SDU)) < 0)
            die_with_error("write() failed");
        free(cell);
    }
    return n;
}

/*****************************************************************************
 * @name            cread
 * @description     cell read 
 * @param           socket, buffer and buffer lenght
 * @return          returns the number of bytes red
 */
int cread(int s,char *buf, int lenght)
{
    int size, lcell, ncell, sdu_s, n;
    unsigned long aal0_cab[1];
    unsigned long pdu_cab[2];
    char *tbuf;
    unsigned char *cell;
    unsigned long crc, crc_aux;

    lcell = ncell = n = 0;
    /* alloc memory  for temporary buffers */
    tbuf = malloc(ATM_CELL_PAYLOAD*sizeof(char));
    cell = malloc(ATM_AAL0_SDU*sizeof(unsigned char));
    /* read cells */
    while(!lcell)
    {
        if ((size = read(s,cell,ATM_AAL0_SDU)) < 0)
            die_with_error("read() failed");
        n += size;
        memcpy(tbuf+(ncell*ATM_CELL_PAYLOAD),cell+4,ATM_CELL_PAYLOAD);
        memcpy(&aal0_cab,cell,sizeof(unsigned long));
        /* read pti from cell */
        lcell = GET(aal0_cab,PTI);
        if(!lcell)
        {
            ncell++;
            /* realloc memory if necessary */
            tbuf = realloc(tbuf,ncell*ATM_CELL_PAYLOAD+ATM_CELL_PAYLOAD);
        }
    }
    /* read header from last cell */
    memcpy(pdu_cab,cell+ATM_CELL_PAYLOAD-4,sizeof(unsigned long));
    memcpy((pdu_cab+1),cell+ATM_CELL_PAYLOAD,sizeof(unsigned long));
    /* get lenght and crc */
    sdu_s= ntohl(GET2(pdu_cab,0xffff0000,0));
    crc = ntohl(*(pdu_cab+1));
    /* verify crc */
    crc_aux = ~update_crc(0xffffffff,tbuf,(ncell+1)*ATM_CELL_PAYLOAD-4);
    if(crc_aux!=crc) return -1;
    /* copy to given buffer */
    memcpy(buf,tbuf,sdu_s);
    /* free temporary buffers */
    free(tbuf);
    free(cell);
    return n;
}

/*****************************************************************************
 * @name						main
 * @description  		gets clients requests and sends them the asked files
 * @param						argc, argv
 * @return					returns 0 on a successful exit
 */
int main(int argc,char **argv)
{

    struct atm_qos qos;
    int s, i, m, n;
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
                T2A_PVC | T2A_UNSPEC | T2A_WILDCARD) < 0) usage();
    /* create socket */
    if((s = socket(AF_ATMPVC,SOCK_DGRAM,ATM_AAL0)) < 0)
        die_with_error("socket() failed");

    memset(&qos,0,sizeof(qos));
    /* configure qos */
    qos.aal = ATM_AAL0;
    qos.rxtp.traffic_class = qos.txtp.traffic_class = ATM_UBR;
    qos.rxtp.max_sdu = qos.txtp.max_sdu = ATM_AAL0_SDU;
    /* set options on socket */
    if(setsockopt(s,SOL_ATM,SO_ATMQOS,&qos,sizeof(qos)) < 0)
        die_with_error("setsockopt() failed");
    /* ignore sigpipe */
    signal(SIGPIPE,SIG_IGN);
    /* generate crc table */
    gen_crc_table();
    /* pvc/svc bind */
    if(bind(s,(struct sockaddr *) &satm, sizeof(satm)) < 0)
        die_with_error("bind() failed");
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
        /* receive filename */
        if((cread(s,buf,buflen)) < 0) crcerr();
        memset(filename,'\0',sizeof(filename));
        memcpy(filename,buf,strlen(buf));
        /* receive bitrate */
        if((cread(s,buf,buflen)) < 0) crcerr();
        bitrate = atoi(buf);
        /* receive sdu size */
        if((cread(s,buf,buflen)) < 0) crcerr();
        sdu_size = atoi(buf);
        /* output info about request */
        fprintf(stderr,"strs: %s was requested at %i Bytes/sec with a SDU size of %i Bytes.\n",
                filename,bitrate,sdu_size);
        /* verify if filename exists, if not warn the client */
        if((l = fsize(filename)) < 0)
        {
            memcpy(buf,FILE_NOT_FOUND,strlen(FILE_NOT_FOUND));
            cwrite(s,buf,buflen);
            fprintf(stderr,"strs: %s was not found.\n",filename);
        }
        else
        {

            /* send the file size */
            fprintf(stderr,"strs: %s has %ld Bytes.\n",filename,l);
            sprintf(buf,"%ld",l);
            cwrite(s,buf,buflen);
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
            for(nbytes = 0,i = 0,n = buflen;i < l;i += buflen)
            {
                if(i + buflen > l) n = l - i;
                fread(buf,n,1,fd);
                gettimeofday(&ts, (struct timezone *)0);
                m = cwrite(s,buf,buflen);
                gettimeofday(&tf, (struct timezone *)0);
                /* delay for cbr */
                delay(d - tf.tv_usec + ts.tv_usec);
                nbytes += m;
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
    }
}
