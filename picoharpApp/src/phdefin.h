/* 
	PHLib programming library for PicoHarp 300
	PicoQuant GmbH, November 2007
*/


#define LIB_VERSION "2.2"

#define MAXDEVNUM	8

/* number of histogram channels */
#define HISTCHAN	65536
/* 128K event records */
#define TTREADMAX   131072
#define RANGES		8

#define MODE_HIST	0
#define MODE_T2		2
#define MODE_T3		3

#define FLAG_OVERFLOW     0x0040 
#define FLAG_FIFOFULL     0x0003

#define ZCMIN		0
/*mV*/
#define ZCMAX		20
/*mV*/
#define DISCRMIN	0
/*mV*/
#define DISCRMAX	800
/*mV*/

#define OFFSETMIN	0			
/*ps*/
#define OFFSETMAX	1000000000	
/*ps*/
#define ACQTMIN		1		
/*ms*/
#define ACQTMAX		36000000
/*ms  (10*60*60*1000ms = 10h) */

#define PHR800LVMIN -1600
/*mV*/
#define PHR800LVMAX  2400
/*mV*/
