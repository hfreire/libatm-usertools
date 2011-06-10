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
 * @filename: strc.c                                                         *
 * @description: ATM streaming client.                                       *
 * @language: C                                                              *
 * @compiled on: cc/gcc                                                      *
 * @last update at: 2007-11-01                                               *
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
#include <signal.h>
#include <errno.h>
#include <math.h>
#include <atm.h>
#include <atmsap.h>
#include "../include/str.h"

int buflen = DEFAULT_SDU_SIZE;	/* length of buffer */
int sdu_size;
int bitrate;
char name[255];
char filename[255];

/*****************************************************************************
 * @name						usage
 * @description  		prints to stderr the correct syntax usage
 * @param						
 * @return
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
 * @name            main
 * @description     sends requests for files 
 * @param           argc, argv
 * @return          returns 0 on a successful exit
 */
int main(int argc,char **argv)
{

    struct sockaddr_atmsvc catm;
    struct atm_sap sap;
    struct atm_qos qos;
    int s, i, m, n;
    long l; /* file size */
    char *buf;	/* ptr to dynamic buffer */

    memcpy(name,argv[0],strlen(argv[0]));
    /* verify syntax */
    if (argc != 4 && argc != 5) usage();

    memcpy(filename,argv[2],strlen(argv[2]));
    bitrate = atoi(argv[3]);
    if (argc == 5) sdu_size = atoi(argv[4]);
    else sdu_size = DEFAULT_SDU_SIZE;

    memset(&catm,0,sizeof(catm));

    if (text2atm(argv[1],(struct sockaddr *) &catm,sizeof(catm),
                 T2A_PVC | T2A_SVC  | T2A_UNSPEC | T2A_WILDCARD | T2A_NAME) < 0) usage();
    /* create a socket */
    if ((s = socket(catm.sas_family,SOCK_DGRAM,0)) < 0)
        die_with_error("socket() failed");

    memset(&qos,0,sizeof(qos));
    /* configure qos */
    qos.aal = ATM_AAL5;
    qos.rxtp.max_sdu = qos.txtp.max_sdu = MAX_SDU_SIZE;
    qos.rxtp.traffic_class = qos.txtp.traffic_class = ATM_UBR;
    /* set options on socket */
    if (setsockopt(s,SOL_ATM,SO_ATMQOS,&qos,sizeof(qos)) < 0)
        die_with_error("setsockopt() failed");
    /* configure bhli for svc */
    if (catm.sas_family == AF_ATMSVC)
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
    if(bind(s,(struct sockaddr *) &catm,
            catm.sas_family == AF_ATMPVC ? sizeof(struct sockaddr_atmpvc):
            sizeof(struct sockaddr_atmsvc)) < 0)
        die_with_error("bind() failed");
    /* svc connect */
    if (catm.sas_family == AF_ATMSVC)
    {
        if (connect(s,(struct sockaddr *) &catm,sizeof(catm)) < 0)
            die_with_error("connect() failed");
    }
    /* adjust the buffer for default sdu size */
    if ((buf = (char *)malloc(buflen)) == (char *)NULL)
        die_with_error("malloc() failed");
    /* send the filename */
    memcpy(buf,filename,strlen(filename));
    if((m = nwrite(s,buf,buflen)) < 0)
        die_with_error("nwrite() failed");
    /* send the bitrate */
    sprintf(buf,"%i",bitrate);
    if((m = nwrite(s,buf,buflen)) < 0)
        die_with_error("nwrite() failed");
    /* send the sdu size */
    sprintf(buf,"%i",sdu_size);
    if((m = nwrite(s,buf,buflen)) < 0)
        die_with_error("nwrite() failed");
    /* read the file size */
    if((m = nread(s,buf,buflen)) < 0)
        die_with_error("nread() failed");
    /* parse size from string */
    l = atol(buf);
    /* if file not found exit */
    if(atol(buf) < 0) exit(0);
    /* adjust the buffer for sdu size */
    if(buflen != sdu_size)
    {
        free(buf);
        buflen = sdu_size;
        if ((buf = (char *)malloc(buflen)) == (char *)NULL)
            die_with_error("malloc() failed");
    }
    /* receive the file */
    for(i = 0,n = buflen;i < l;i += buflen)
    {
        if((m = nread(s,buf,buflen)) < 0)
            die_with_error("nread() failed");
        if(i+buflen>l) n = l-i;
        fwrite(buf,n,1,stdout);
    }
    /* close socket */
    close(s);
    free(buf);
    return 0;
}



