
#include <stdio.h>
#include <stdlib.h> /* for malloc, free, srand, rand */
#include <string.h>
/*******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/
/* possible events: */
#define  TIMER_INTERRUPT 0
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2
#define PKT_SIZE    sizeof(struct pkt)
#define  OFF             0
#define  ON              1
#define   A    0
#define   B    1
#define   N    8
#define BIDIRECTIONAL 1    /* change to 1 if you're doing extra credit */
/* and write a routine called B_output */

struct event {
    float evtime;           /* event time */
    int evtype;             /* event type code */
    int eventity;           /* entity where event occurs */
    struct pkt* pktptr;     /* ptr to packet (if any) assoc w/ this event */
    struct event* prev;
    struct event* next;
};
struct event* evlist = NULL;   /* the event list */


/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
    char data[20];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
    int seqnum;   
    int checksum;
    int acknum;
    char payload[20];
};

/* sender structure */
typedef struct Sender {
    int base;
    int next_seqnum;  
    struct pkt* window;
    struct pkt* finite_buffer;//finite length buffer

    int expectedseqnum;
   
    int acknum;
    int state;
}Sender;

Sender sender;
/* receiver structure, exactly same with sender*/
typedef struct Receiver {
    int base;
    int next_seqnum;
    struct pkt* window;
    struct pkt* finite_buffer;

    int expectedseqnum;
    int acknum;

    int state;
}Receiver;

Receiver receiver;

/* mychecksum, execute 16bit checksum*/
int mychecksum(struct pkt packet) {
    unsigned int sum=0;
    short *payload = (short*)(packet.payload);//change payload type to short (16bit)
   
    sum += (packet.seqnum + packet.acknum);

    if ( sum > 0xFFFF)//carry emerges
        sum += 1;
    for (int i = 0; i < 10; i++) {
        sum = (sum & 0xFFFF);
        sum += payload[i];//adding 16bits sequentially
        if (sum > 0xFFFF)// if carry emerges
            sum += 1;
    }  

    return ~(sum); 
   
}

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* Here I define some function prototypes to avoid warnings */
/* in my compiler. Also I declared these functions void and */
/* changed the implementation to match */
void init();
void generate_next_arrival();
void insertevent(struct event* p);
void starttimer(int AorB, float increment);

void stoptimer(int AorB);
void tolayer3(int AorB, struct pkt packet);
void tolayer5(int AorB, char* datasent);
/* called from layer 5, passed the data to be sent to other side */

void A_output(struct msg message)
{   
    
        if (sender.next_seqnum < sender.base + N) {//check window size
            (sender.finite_buffer)[sender.next_seqnum].seqnum = sender.next_seqnum;
           
            if (sender.state == 0) {//if called from above
                (sender.finite_buffer)[sender.next_seqnum].acknum = 999;
                printf("A: Send_packet without ACK (seq = %d) \n", sender.next_seqnum);
            }
            else {//if called from A_input
                (sender.finite_buffer)[sender.next_seqnum].acknum = sender.acknum;   
                printf("A: Send_packet with ACK (ACK = %d, seq = %d) \n", sender.acknum, sender.next_seqnum);                
            }
             

            for (int i = 0; i < 20; i++)
                (sender.finite_buffer)[sender.next_seqnum].payload[i] = message.data[i];

            (sender.finite_buffer)[sender.next_seqnum].checksum = mychecksum((sender.finite_buffer)[sender.next_seqnum]);
            tolayer3(A, sender.finite_buffer[sender.next_seqnum]);//sending
            if (sender.base == sender.next_seqnum)
                starttimer(A, 100.0);//start timer when no packets in window
            (sender.next_seqnum)++;
           
        }

        else
            printf("A window full, refusing data! \n");

        return;//refusing data
    }

   
    

      


void B_output(struct msg message)  /* need be completed only for extra credit */
{
    /* exactly same with A_output */
     if (receiver.next_seqnum < receiver.base + N) {
        (receiver.finite_buffer)[receiver.next_seqnum].seqnum = receiver.next_seqnum;

        if (receiver.state == 0) {
            (receiver.finite_buffer)[receiver.next_seqnum].acknum = 999;
            printf("B: Send_packet without ACK (seq = %d) \n", receiver.next_seqnum);
        }
        else {
            (receiver.finite_buffer)[receiver.next_seqnum].acknum = receiver.acknum;
            printf("B: Send_packet with ACK (ACK = %d, seq = %d) \n", receiver.acknum, receiver.next_seqnum);
           
        }


        for (int i = 0; i < 20; i++)
            (receiver.finite_buffer)[receiver.next_seqnum].payload[i] = message.data[i];

        (receiver.finite_buffer)[receiver.next_seqnum].checksum = mychecksum((receiver.finite_buffer)[receiver.next_seqnum]);
        tolayer3(B, receiver.finite_buffer[receiver.next_seqnum]);
        if (receiver.base == receiver.next_seqnum)
            starttimer(B, 100.0);
        (receiver.next_seqnum)++;

    }

    else
        printf("B window full, refusing data! \n");

    return;//refusing data

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    struct msg message;
    for (int i = 0; i < 20; i++)
        message.data[i] = 0;

    sender.state = 1;

    if ((packet.checksum == mychecksum(packet)))// if checksum right
    {                          
        printf("A side: packet(Ack = %d, seq = %d ) arrived!!\n", packet.acknum, packet.seqnum);
        if (packet.seqnum == sender.expectedseqnum)
        {
            sender.acknum = sender.expectedseqnum;//save expectedseqnum
            tolayer5(A, packet.payload);//extract data
            (sender.expectedseqnum)++;//increment expectedseqnum
        }
        else
            printf("A side: unexpected seq# arrived!!\n");

        
        
        if (packet.acknum != 999) {//not pure data            
            if (packet.acknum >= sender.base) {
                stoptimer(A);

                sender.base = (packet.acknum) + 1;
                sender.window = (sender.finite_buffer) + (PKT_SIZE * sender.base);                
                
                if (sender.base != sender.next_seqnum)
                    starttimer(A, 15.0);//if not received all, restart timer
               
            }

            else //if duplicate
                printf("A side: duplicate packet(Ack = %d, seq = %d) arrived!!\n", packet.acknum, packet.seqnum);       
                
            }      
         
    }

    else {        
        printf("A side: corrupted, resend please!!\n");
    }

    A_output(message); //send message

    sender.state = 0;
}
    
        


    
   
   


/* called when A's timer goes off */
void A_timerinterrupt()
{
    printf("A side: Timeout! base(%d��)���� seq#(%d��)���� ������!!\n", sender.base, sender.next_seqnum - 1);
    starttimer(A, 15.0);//give enough time to receive message
    for (int i = sender.base; i < sender.next_seqnum; i++) {
        (sender.finite_buffer)[i].acknum = sender.expectedseqnum -1;//same as acknum
        (sender.finite_buffer)[i].checksum = mychecksum((sender.finite_buffer)[i]);
        printf("A: Send_packet with ACK (ACK = %d, seq = %d)\n", (sender.finite_buffer)[i].acknum, i);
        tolayer3(A, (sender.finite_buffer)[i]);//send to B
    }
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. Y  ou can use it to do any initialization */
void A_init()
{
    sender.next_seqnum = 1;   
    sender.base = 1;
  
    sender.finite_buffer = (struct pkt*)malloc(PKT_SIZE * 50);//size is 50
    sender.window = (sender.finite_buffer) + PKT_SIZE;//SET window's starting point

    sender.expectedseqnum = 1;
    sender.acknum = 0;    
    
}


/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    /* exactly same with A_input */
    struct msg message;
    for (int i = 0; i < 20; i++)
        message.data[i] = 0;

    receiver.state = 1;

    if ((packet.checksum == mychecksum(packet)))
    {
        receiver.state = 1;
        printf("B side: packet(Ack = %d, seq = %d ) arrived!!\n", packet.acknum, packet.seqnum);

        if (packet.seqnum == receiver.expectedseqnum)
        {
            receiver.acknum = receiver.expectedseqnum;
            tolayer5(B, packet.payload);
            (receiver.expectedseqnum)++;
        }
        
        else
            printf("B side: unexpected seq# arrived!!\n");

        if (packet.acknum != 999) {

            if (packet.acknum >= receiver.base) {
                stoptimer(B);

                receiver.base = (packet.acknum) + 1;
                receiver.window = (receiver.finite_buffer) + (PKT_SIZE * receiver.base);


                if (receiver.base != receiver.next_seqnum)
                    starttimer(B, 15.0);

            }

            else
                printf("B side: duplicate packet(Ack = %d, seq = %d) arrived!!\n", packet.acknum, packet.seqnum);



        }           
    }

    else {       
        printf("B side: corrupted, resend please!!\n");
    }

    B_output(message);

    receiver.state = 0;
       
}

/* called when B's timer goes off */
void B_timerinterrupt()
{
    /* exactly same with A_timerinterrupt */
    printf("B side: Timeout! base(%d��)���� seq#(%d��)���� ������!!\n", receiver.base, receiver.next_seqnum - 1);
    starttimer(B, 15.0);
    for (int i = receiver.base; i < receiver.next_seqnum; i++) {
        (receiver.finite_buffer)[i].acknum = (receiver.expectedseqnum-1);
        (receiver.finite_buffer)[i].checksum = mychecksum((receiver.finite_buffer)[i]);
        printf("B: Send_packet with ACK (ACK = %d, seq = %d)\n", (receiver.finite_buffer)[i].acknum, i);
        tolayer3(B, (receiver.finite_buffer)[i]);
    }

}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    /* same with A_init() */
    receiver.next_seqnum = 1;
    receiver.base = 1;
    receiver.finite_buffer = (struct pkt*)malloc(PKT_SIZE * 50);
    receiver.window = (receiver.finite_buffer) + PKT_SIZE;

    receiver.expectedseqnum = 1;
    receiver.acknum = 0;   
}


/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

 /* the event list */




int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */
int nsimmax = 0;           /* number of msgs to generate, then stop */
float time = (float)0.000;
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */
float lambda;              /* arrival rate of messages from layer 5 */
int   ntolayer3;           /* number sent into layer 3 */
int   nlost;               /* number lost in media */
int ncorrupt;              /* number corrupted by media*/

void main()
{
    struct event* eventptr;
    struct msg  msg2give;
    struct pkt  pkt2give;

    int i, j;
    /* char c; // Unreferenced local variable removed */

    init();
    A_init();
    B_init();

    while (1) {
        eventptr = evlist;            /* get next event to simulate */
        if (eventptr == NULL)
            goto terminate;
        evlist = evlist->next;        /* remove this event from event list */
        if (evlist != NULL)
            evlist->prev = NULL;
        if (TRACE >= 2) {
            printf("\nEVENT time: %f,", eventptr->evtime);
            printf("  type: %d", eventptr->evtype);
            if (eventptr->evtype == 0)
                printf(", timerinterrupt  ");
            else if (eventptr->evtype == 1)
                printf(", fromlayer5 ");
            else
                printf(", fromlayer3 ");
            printf(" entity: %d\n", eventptr->eventity);
        }
        time = eventptr->evtime;        /* update time to next event time */
        if (nsim == nsimmax)
            break;                        /* all done with simulation */
        if (eventptr->evtype == FROM_LAYER5) {
            generate_next_arrival();   /* set up future arrival */
            /* fill in msg to give with string of same letter */
            j = nsim % 26;
            for (i = 0; i < 20; i++)
                msg2give.data[i] = 97 + j;
            if (TRACE > 2) {
                printf("          MAINLOOP: data given to student: ");
                for (i = 0; i < 20; i++)
                    printf("%c", msg2give.data[i]);
                printf("\n");
            }
            nsim++;
            if (eventptr->eventity == A)
                A_output(msg2give);
            else
                B_output(msg2give);
        }
        else if (eventptr->evtype == FROM_LAYER3) {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i = 0; i < 20; i++)
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
            if (eventptr->eventity == A)      /* deliver packet by calling */
                A_input(pkt2give);            /* appropriate entity */
            else
                B_input(pkt2give);
            free(eventptr->pktptr);          /* free the memory for packet */
        }
        else if (eventptr->evtype == TIMER_INTERRUPT) {
            if (eventptr->eventity == A)
                A_timerinterrupt();
            else
                B_timerinterrupt();
        }
        else {
            printf("INTERNAL PANIC: unknown event type \n");
        }
        free(eventptr);
    }

terminate:
    printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n",time,nsim);
}



void init()                         /* initialize the simulator */
{
    int i;
    float sum, avg;
    float jimsrand();


    printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
    printf("Enter the number of messages to simulate: ");
    scanf_s("%d", &nsimmax);
    printf("Enter  packet loss probability [enter 0.0 for no loss]:");
    scanf_s("%f", &lossprob);
    printf("Enter packet corruption probability [0.0 for no corruption]:");
    scanf_s("%f", &corruptprob);
    printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
    scanf_s("%f", &lambda);
    printf("Enter TRACE:");
    scanf_s("%d", &TRACE);

    srand(9999);              /* init random number generator */
    sum = (float)0.0;         /* test random number generator for students */
    for (i = 0; i < 1000; i++)
        sum = sum + jimsrand();    /* jimsrand() should be uniform in [0,1] */
    avg = sum / (float)1000.0;
    if (avg < 0.25 || avg > 0.75) {
        printf("It is likely that random number generation on your machine\n");
        printf("is different from what this emulator expects.  Please take\n");
        printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
        exit(0);
    }

    ntolayer3 = 0;
    nlost = 0;
    ncorrupt = 0;

    time = (float)0.0;                    /* initialize time to 0.0 */
    generate_next_arrival();     /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand()
{
    double mmm = RAND_MAX;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
    float x;                   /* individual students may need to change mmm */
    x = (float)(rand() / mmm);            /* x should be uniform in [0,1] */
    return(x);
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

void generate_next_arrival()
{
    double x, log(), ceil();
    struct event* evptr;
    /* char *malloc(); // malloc redefinition removed */
    /* float ttime; // Unreferenced local variable removed */
    /* int tempint; // Unreferenced local variable removed */

    if (TRACE > 2)
        printf("GENERATE NEXT ARRIVAL: creating new arrival\n");

    x = lambda * jimsrand() * 2;  /* x is uniform on [0,2*lambda] */
                              /* having mean of lambda        */
    evptr = (struct event*)malloc(sizeof(struct event));
    evptr->evtime = (float)(time + x);
    evptr->evtype = FROM_LAYER5;
    if (BIDIRECTIONAL && (jimsrand() > 0.5))
        evptr->eventity = B;
    else
        evptr->eventity = A;
    insertevent(evptr);
}


void insertevent(struct event* p)
{
    struct event* q, * qold;

    if (TRACE > 2) {
        printf("            INSERTEVENT: time is %lf\n", time);
        printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
    }
    q = evlist;     /* q points to header of list in which p struct inserted */
    if (q == NULL) {   /* list is empty */
        evlist = p;
        p->next = NULL;
        p->prev = NULL;
    }
    else {
        for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
            qold = q;
        if (q == NULL) {   /* end of list */
            qold->next = p;
            p->prev = qold;
            p->next = NULL;
        }
        else if (q == evlist) { /* front of list */
            p->next = evlist;
            p->prev = NULL;
            p->next->prev = p;
            evlist = p;
        }
        else {     /* middle of list */
            p->next = q;
            p->prev = q->prev;
            q->prev->next = p;
            q->prev = p;
        }
    }
}

void printevlist()
{
    struct event* q;
    /* int i; // Unreferenced local variable removed */
    printf("--------------\nEvent List Follows:\n");
    for (q = evlist; q != NULL; q = q->next) {
        printf("Event time: %f, type: %d entity:%d\n",q->evtime,q->evtype,q->eventity);
    }
    printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(int AorB) /* A or B is trying to stop timer */
{
    struct event* q;/* ,*qold; // Unreferenced local variable removed */

    if (TRACE > 2)
        printf("          STOP TIMER: stopping timer at %f\n", time);
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB)) {
            /* remove this event */
            if (q->next == NULL && q->prev == NULL)
                evlist = NULL;         /* remove first and only event on list */
            else if (q->next == NULL) /* end of list - there is one in front */
                q->prev->next = NULL;
            else if (q == evlist) { /* front of list - there must be event after */
                q->next->prev = NULL;
                evlist = q->next;
            }
            else {     /* middle of list */
                q->next->prev = q->prev;
                q->prev->next = q->next;
            }
            free(q);
            return;
        }
    printf("Warning: unable to cancel your timer. It wasn't running.\n");
}


void starttimer(int AorB, float increment) /* A or B is trying to stop timer */
{
    struct event* q;
    struct event* evptr;
    /* char *malloc(); // malloc redefinition removed */

    if (TRACE > 2)
        printf("          START TIMER: starting timer at %f\n", time);
    /* be nice: check to see if timer is already started, if so, then  warn */
   /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB)) {
            printf("Warning: attempt to start a timer that is already started\n");
            return;
        }

    /* create future event for when timer goes off */
    evptr = (struct event*)malloc(sizeof(struct event));
    evptr->evtime = (float)(time + increment);
    evptr->evtype = TIMER_INTERRUPT;
    evptr->eventity = AorB;
    insertevent(evptr);
}


/************************** TOLAYER3 ***************/
void tolayer3(int AorB, struct pkt packet)  /* A or B is trying to stop timer */
{
    struct pkt* mypktptr;
    struct event* evptr, * q;
    /* char *malloc(); // malloc redefinition removed */
    float lastime, x, jimsrand();
    int i;


    ntolayer3++;

    /* simulate losses: */
    if (jimsrand() < lossprob) {
        nlost++;
        if (TRACE > 0)
            printf("          TOLAYER3: packet being lost\n");
        return;
    }

    /* make a copy of the packet student just gave me since he/she may decide */
    /* to do something with the packet after we return back to him/her */
    mypktptr = (struct pkt*)malloc(sizeof(struct pkt));
    mypktptr->seqnum = packet.seqnum;
    mypktptr->acknum = packet.acknum;
    mypktptr->checksum = packet.checksum;
    for (i = 0; i < 20; i++)
        mypktptr->payload[i] = packet.payload[i];
    if (TRACE > 2) {
        printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
            mypktptr->acknum, mypktptr->checksum);
        for (i = 0; i < 20; i++)
            printf("%c", mypktptr->payload[i]);
        printf("\n");
    }

    /* create future event for arrival of packet at the other side */
    evptr = (struct event*)malloc(sizeof(struct event));
    evptr->evtype = FROM_LAYER3;   /* packet will pop out from layer3 */
    evptr->eventity = (AorB + 1) % 2; /* event occurs at other entity */
    evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
  /* finally, compute the arrival time of packet at the other end.
     medium can not reorder, so make sure packet arrives between 1 and 10
     time units after the latest arrival time of packets
     currently in the medium on their way to the destination */
    lastime = time;
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == FROM_LAYER3 && q->eventity == evptr->eventity))
            lastime = q->evtime;
    evptr->evtime = lastime + 1 + 9 * jimsrand();



    /* simulate corruption: */
    if (jimsrand() < corruptprob) {
        ncorrupt++;
        if ((x = jimsrand()) < .75)
            mypktptr->payload[0] = 'Z';   /* corrupt payload */
        else if (x < .875)
            mypktptr->seqnum = 999999;
        else
            mypktptr->acknum = 999999;
        if (TRACE > 0)
            printf("          TOLAYER3: packet being corrupted\n");
    }

    if (TRACE > 2)
        printf("          TOLAYER3: scheduling arrival on other side\n");
    insertevent(evptr);
}

void tolayer5(int AorB, char* datasent)
{
    int i;
    if (TRACE > 2) {
        printf("          TOLAYER5: data received: ");
        for (i = 0; i < 20; i++)
            printf("%c", datasent[i]);
        printf("\n");
    }

}