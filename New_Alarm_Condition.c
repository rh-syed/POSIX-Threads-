/*
Assignment 3
By: 
Syed Rahman, 212206975
Nazia Zafor,
Matthew Cardinal,
Diana Lee,
*/

#define DEBUG

#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include "errors.h"

/*
 * Modified for Assignment 3 - Alarm Structure
 * Added MessageNum  to the alarm structure so that matching can occur
 * Added type to the alarm structure so that we can ensure no duplicate entries
 * Added processed to indicate the alarm has a thread attached to it
 * Added changed to indicate that an update to the alarm has occurred
 * The "alarm" structure now contains the time_t (time since the
 * Epoch, in seconds) for each alarm, so that they can be
 * sorted. Storing the requested number of seconds would not be
 * enough, since the "alarm thread" cannot tell how long it has
 * been on the list.
 */
typedef struct alarm_tag {
    struct alarm_tag    *link;
    int                 seconds;
    time_t              time;   /* seconds from EPOCH */
    char		type;   /* type = A<lert> or C<ancel> */
    int			messagenum; /* message(n) to match on */
    int			processed; /* Flag to show that a thread
                            * has been created for this alarm */
    int			changed;	/* Flag to show an update has occurred */
    char                message[128];
} alarm_t;
/*
 * For Assignment 3
 * Global Variables
 *
 */

char line[143]; /* This allows for a 128 character message portion */
int status;	/* status is used as a return code from the semaphore calls */
int insert;
int read_count = 0;
alarm_t *my_alarm, *left, *tmp_alarm;
alarm_t *thread_alarm;
alarm_t **last, *next;  /* alarm list links  */
alarm_t *alarm_list = NULL;
sem_t alarm_sem;	/* Semaphores */
sem_t alarmthread_sem;
sem_t mutex_sem;
time_t current_alarm = 0; /* Variables used for timer functions */
time_t mytime;
struct tm *time_now;
char buff[BUFSIZ];

/*
 * Added for assignment 3 - ThreadData structure
 * Used to pass information to the periodic display thread
 * as it's created
 */
typedef struct
{
    int id;
    int thread_seconds;
    int alarm_time;
    char thread_message[128];
} ThreadData;
ThreadData *td;

/*
 * Added For Assignment 3
 * Define function call stubs and threads
 * Functions are defined after the main section
 * threads are defined next
 *
 */
void ProcessMessage (void);
int ProcessCancel (void);
void *periodic_display_thread (void *arg);

/*
 * Modified for Assignment 3 - alarm_thread
 * Changes are:
 * 1. Removed timer functionality to match the specification
 * 2. Added alarm list processing to find cancels and unprocessed alarms
 * 3. Added unnamed semaphore processing to meet the specification
 *
 */
 
 
 void *alarm_thread (void *arg)
 {
	/*
     * The alarm thread's start routine.
     */
	 
	 alarm_t *tmp2_alarm;
	 int status, messageNum;
	 char msg[128];
	 /*
     * Modified for Assignment 3
     * Use the semaphore to control access to the alarm list
     * The while loop is designed to loop forever.  However,
     * it actually stops on the semaphore wait, and only continues
     * when the list is changed.  The semaphore is released by the
     * main function when the list is modified.  A semaphore wait
     * at the end of the While loop will cause the while to pause again.
     * The threads will be disintegrated when the process exits.
     */
	 
	 while(1){
		  sched_yield ();	/* share the CPU */
        /*
         * Wait on the thread semaphore.  The main function will provide the
         * required sem_post command that will wake up this thread when
         * the alarm list has changed.
         *
         */
		 sem_wait(&alarmthread_sem);
		 /*
         * Now ensure we have access to the list through the
         * alarm semaphore
         *
         */
		 sem_wait(&alarm_sem);
		 /*
         * Now use the semaphore to update read_count
         */
		 sem_wait(&mutex_sem);
        read_count++;
        sem_post(&mutex_sem);
        /*
         * Read the alarm list looking for a "Cancel Entry" or
         * any unprocessed alarm request
         *
         */
		 last = &alarm_list;
        tmp_alarm = *last;
        left = *last;
		
		while (tmp_alarm != NULL) 
		{
			/* test for a cancel request as they come first */
			if (strncmp(&tmp_alarm->type, "C", 1) == 0)
			{
				/* We have a Cancel, so remove it and the alarm */
				messageNum = tmp_alarm->messagenum;
				strcpy(msg, tmp_alarm->message);
				
				if (tmp_alarm == alarm_list) 
				{
                    alarm_list = tmp_alarm->link;
                    free(tmp_alarm);
                    tmp_alarm = alarm_list;
					
                }
				else 
				{
                    tmp2_alarm = tmp_alarm->link;
                    /* last->link = tmp_alarm->link; */
                    free(tmp_alarm);
                    tmp_alarm = tmp2_alarm;
                }
				/* Test next message to ensure it's an alarm with
                 * the correct message number
                 */
				 if (tmp_alarm->messagenum == messageNum && strncmp(&tmp_alarm->type, "A", 1) == 0) 
				 {
                    if (tmp_alarm == alarm_list) 
					{
                        alarm_list = tmp_alarm->link;
                        free(tmp_alarm);
                        tmp_alarm = alarm_list;
                    }
					else 
					{
                        /* Left points at the previous good alarm, so update link */
                        /*	left->link = tmp2_alarm->link; */
                        if (tmp2_alarm == NULL) 
						{
                            left->link = NULL;
                        }
                        else {
                            if (tmp2_alarm->link == NULL) 
							{
                                left->link = NULL;
                            }
                            else 
							{
                                left->link = tmp2_alarm->link;
                            }
                        }
                        free(tmp_alarm);
                        tmp_alarm = NULL;
                    }
				 }
				 /* Print out 'processed' message */
                time(&mytime);
                time_now = localtime(&mytime);
                strftime (buff, sizeof buff, "%H:%M:%S", time_now);
                printf ("Alarm processed at <%s>: %s \n",
                        buff, msg);
			
		}
		else {
                /* Test for an unprocessed alarm */
                if (tmp_alarm->processed == 0) {
                    /* Process the new alarm by creating a
                     * periodic display thread */
                    tmp_alarm->processed = 1; /* Indicate processed */
                    td->id = tmp_alarm->messagenum;
                    td->thread_seconds = tmp_alarm->seconds;
                    td->alarm_time = tmp_alarm->time;
                    strcpy(td->thread_message, tmp_alarm->message);
                    pthread_t periodic_thread;
                    status = pthread_create (
                                             &periodic_thread, NULL, periodic_display_thread, td);
                    if (status != 0)
                        err_abort (status, "Create alarm thread");
                    /* Print out 'processed' message */
                    time(&mytime);
                    time_now = localtime(&mytime);
                    strftime (buff, sizeof buff, "%H:%M:%S", time_now);
                    printf ("Alarm processed at <%s>: %d Message(%d) %s \n",
                            buff, tmp_alarm->seconds, tmp_alarm->messagenum,
                            tmp_alarm->message);
                }
            }
			 if (tmp_alarm != NULL) 
			 {
                left = *last;
                last = &tmp_alarm;
                tmp_alarm = tmp_alarm->link;
            }
		 
		 
	 }
	 
	  /*
         * Now use the semaphore to update read_count
         */
        sem_wait(&mutex_sem);
        read_count--;
        sem_post(&mutex_sem);
        /* Post the alarm semaphore for the next round */
        sem_post(&alarm_sem);
	
	}
 }
 /*
 * Added for Assignment 3 - periodic dispay thread
 * New thread to periodically check the alarm list and manage
 * an alarm with a specific message number.
 * The thread kills itself when the message is cancelled
 */
 void *periodic_display_thread (void *td_ptr)
{
    /* initialize by getting the time that we need to dispklay this thread */
    ThreadData *td1 = (ThreadData*)td_ptr;
    int messageNum = td1->id;
    int tsecs = td1->thread_seconds;
    time_t alarm_ring = td1->alarm_time;
    char tmsg[128];
    strcpy(tmsg, td1->thread_message);
    alarm_t *t_alarm;
    time_t now;
    struct tm *tm_now;
    char buff[BUFSIZ];
    int live = 1;
    int found;
	
    /*
     * Main loop runs until the message is cancelled
     *
     */
	 
    while(live) {
        /* Semaphore lock the list and loop through it to check the message is
         * still there and if it's been changed.
         */
        sem_wait(&alarm_sem);
        /*
         * Now use the semaphore to update read_count
         */
        sem_wait(&mutex_sem);
        read_count++;
        sem_post(&mutex_sem);
        /* Use a while loop to go through the list as we may be altering it */
        found = 0;
        last = &alarm_list;
        t_alarm = *last;
        left = *last;
        while (t_alarm != NULL) {
            /* Test to see if this is our alarm message - number & type */
            if(t_alarm->messagenum == messageNum && (strncmp(&t_alarm->type, "A", 1) == 0)) {
                /* We have our message, has it been changed? */
                found = 1;
                now = time(NULL);
                if (t_alarm->changed == 1) {
                    /* Yes, it has been changed so reset everything and print a message */
                    tm_now = localtime(&now);
                    strftime (buff, sizeof buff, "%H:%M:%S", tm_now);
                    printf ("Alarm replaced at <%s>: %d Message(%d) %s \n", buff,
                            t_alarm->seconds, t_alarm->messagenum, t_alarm->message);
                    /* reset the alarm, seconds, and message */
                    alarm_ring = t_alarm->time;
                    tsecs = t_alarm->seconds;
                    strcpy (tmsg, t_alarm->message);
                    t_alarm->changed = 0; /* mark the alarm as not-changed */
                    break;
                }
                if (now >= alarm_ring) {
                    /* yes, so let's display the right message */
                    tm_now = localtime(&now);
                    strftime (buff, sizeof buff, "%H:%M:%S", tm_now);
                    printf ("Alarm displayed at  <%s>: %d Message(%d) %s \n",
                            buff, tsecs, messageNum, tmsg);
                    /* Reset our alarm ring time */
                    alarm_ring = time(NULL) + tsecs;
                }
            }
            left = t_alarm;
            t_alarm = t_alarm->link;
        }  /* End of list-reading while loop */
        /*
         * Now use the semaphore to update read_count
         */
        sem_wait(&mutex_sem);
        read_count--;
        sem_post(&mutex_sem);
        /* Did we find our alarm message? If not, then it's been cancelled */
        if (!found) {
            tm_now = localtime(&now);
            strftime (buff, sizeof buff, "%H:%M:%S", tm_now);
            printf ("Display thread exiting at <%s>: %d Message(%d) %s \n",
                    buff, tsecs, messageNum, tmsg);
            live = 0;	/* let the thread know that we want to die */
            sem_post(&alarm_sem);  /* unlock the semaphore before we die */
            break;
        }
        /* We've finished with the list so release the semaphore */
        sem_post(&alarm_sem);
        /* We come through the list, so we sleep for a second */
        sleep(1);
    }
    /* The thread has come here to die so let's do it */
    pthread_exit(NULL);
}
/*
 * Main section of the program
 *
 */

int main (int argc, char *argv[])
{
    int status;
    int tempno;
    int valid_entry;
    char astring[10];
    char bstring[20];
    char buf1[20];
    char astr1[8]="Message(";
    char astr2[1]=")";
    char astr3[7]="Cancel:";
    td = (ThreadData*)malloc(sizeof(ThreadData));
    if (td == NULL)
        errno_abort ("Allocate threaddata");
    /*
     * Added for Assignment 3 - Semaphore initialization
     * Create and Initialize the alarm list semaphore
     * The main function and the threads will use it to control access
     *  to the alarm list
     *
     */
    sem_init (&alarm_sem, 0, 1);
    /*
     * Create and initialize the alarm thread semaphore.
     * It's used to signal that the alarm list has changed and the alarm
     * thread can process it.
     *
     */
    sem_init(&alarmthread_sem, 0, 1);
    /*
     * Create and initialize the mutex (read_count) semaphore.
     * It's used to manage access to the read_count variable
     *
     */
    sem_init(&mutex_sem, 0, 1);
    /*
     * setup and create the thread
     *
     */
    pthread_t thread;
    status = pthread_create (
                             &thread, NULL, alarm_thread, NULL);
    if (status != 0)
        err_abort (status, "Create alarm thread");
    /*
     * Changed for Assignment 3
     * Loop waiting for input or the user hitting ctrl+d
     * Validate two types of user input and, when valid, add
     * to the alarm list and kick the alarm thread to process it.
     *
     */
    while (1) {
        status = 0;
        valid_entry = 0;
        printf ("Alarm> ");
        if (fgets (line, sizeof (line), stdin) == NULL) exit (0);
        if (strlen (line) <= 1) continue;
        my_alarm = (alarm_t*)malloc (sizeof (alarm_t));
        if (alarm == NULL)
            errno_abort ("Allocate alarm");
        /*
         * Changed for Assignment 3 to validate two message types
         * The first type = "Time Message(n) message".
         * The second type = "Cancel: Message(n)"
         * Where n is any integer (currently a single digit!)
         * Parse input line into either of the two types
         * with a maximum of 128 characters including the end of line character
         * Each parameter is seperated by whitespace.
         */
        if (isdigit(line[0])) { /* Is the first character a digit? */
            /* Yes, so process as a type 1 message */
            if (sscanf(line, "%d %s %128[^\n]", &my_alarm->seconds, astring, my_alarm->message) < 3) {
                printf ("Bad command \n");
                free (my_alarm);
            }
            else {
                /* Check that format of the astring is valid */
                if (strncmp(astr1, astring, 8) == 0 && isdigit(astring[8])) {
                    if (isdigit(astring[9]) && strncmp(&astring[10], astr2, 1) == 0) {
                        /* we have 2 digits in a valid row */
                        tempno = atoi(&astring[8]);
                        tempno = tempno * 10;
                        tempno = tempno + atoi(&astring[9]);
                        my_alarm->messagenum = tempno;
                        valid_entry = 1;
                    }
                    if (strncmp(&astring[9], astr2, 1) == 0) {
                        /* we have 1 digit in a valid row */
                        my_alarm->messagenum = atoi(&astring[8]);
                        valid_entry = 1;
                    }
                    if (valid_entry) {
                        /* Add the time, proccessed flag,and message
                         * number into the alarm */
                        my_alarm->time = time (NULL) + my_alarm->seconds;
                        my_alarm->processed = 0;
                        my_alarm->messagenum = atoi(&astring[8]);
                        my_alarm->changed = 0;
                        strcpy(&my_alarm->type, "A");
                        /* Now process the message as required */
                        ProcessMessage();
                        /* We have a valid message so print out 'received' message */
                        time(&mytime);
                        time_now = localtime(&mytime);
                        strftime (buff, sizeof buff, "%H:%M:%S", time_now);
                        printf ("Alarm Request Received at <%s>: %d Message(%d) %s \n",
                                buff, my_alarm->seconds, my_alarm->messagenum,
                                my_alarm->message);
                        /* Use the semaphore to kick the alarm thread */
                        sem_post(&alarmthread_sem);
                    }
                    else {
                        printf ("Bad command \n");
                        free (my_alarm);
                    }
                }
                else {
                    printf ("Bad command \n");
                    free (my_alarm);
                }
            }
        }
        else {
            /* First character is not a digit so it should */
            /* be a Cancel command with a valid message number in it */
            if (sscanf(line, "%s %s", &astring, bstring) < 2) {
                printf ("Bad command \n");
                free (my_alarm);
            }
            else {
                if (strncmp(astring, astr3, 7) == 0 && strncmp(bstring, astr1, 8) == 0
                    && isdigit(bstring[8])) {
                    /* We have most of a 'cancel' message */
                    if (isdigit(bstring[9]) && strncmp(&bstring[10], astr2, 1) == 0) {
                        /* we have 2 digits in a valid row */
                        tempno = atoi(&bstring[8]);
                        my_alarm->messagenum = tempno;
                        valid_entry = 1;
                    }
                    if (strncmp(&bstring[9], astr2, 1) == 0) {
                        /* we have 1 digit in a valid row */
                        my_alarm->messagenum = atoi(&bstring[8]);
                        valid_entry = 1;
                    }
                    if (valid_entry) {
                        /* Now process the message as required and test if it existed */
                        /* Set message number and type for the insert */
                        buf1[0]='\0';
                        strcat(buf1,astring);
                        strcat(buf1, " ");
                        strcat(buf1,bstring);
                        strcpy(my_alarm->message, buf1);
                        strcpy(&my_alarm->type, "C");
                        my_alarm->seconds = 0;
                        my_alarm->time = time (NULL);
                        status = ProcessCancel();
                        if (status == 1) {
                            printf ("Message Number Does Not Exist! \n");
                            free (my_alarm);
                        }
                        else {
                            /* We have a valid Cancel message
                             * so print out 'received' message */
                            time(&mytime);
                            time_now = localtime(&mytime);
                            strftime (buff, sizeof buff, "%H:%M:%S", time_now);
                            printf ("Alarm Request Received at <%s>: %s \n", buff,
                                    buf1);
                            /* Use the semaphore to kick the alarm thread */
                            sem_post(&alarmthread_sem);
                        }
                    }
                    else {
                        printf ("Bad command \n");
                        free (my_alarm);
                    }
                }
                else {
                    printf ("Bad command \n");
                    free (my_alarm);
                }
            }
        }
        sched_yield ();	/* share the CPU */
    } /* End of while loop */
}
void ProcessMessage (void) {
    /* 
     * Acquire the semaphore
     *
     */
    sem_wait(&alarm_sem);
    /*
     * Insert the new alarm into the list of alarms,
     * sorted by message number (messagenum).
     */
    int insert = 0;
    last = &alarm_list;
    next = *last;
    while (next != NULL) {
        /* Test for a match and if true update the record */
        if (next->messagenum == my_alarm->messagenum) {
            strcpy(next->message, my_alarm->message);
            next->seconds = my_alarm->seconds;
            next->time = time (NULL) + my_alarm->seconds;
            next->changed = 1;
            insert = 1;
            break;
        }
        /* No match, but if next message number is larger then insert */
        if (next->messagenum >= my_alarm->messagenum) {
            *last = my_alarm;
            my_alarm->link = next;
            break;
        }
        last = &next->link;
        next = next->link;
    }
    /*
     * Make sure we only insert if we didn't get a match
     * If we reached the end of the list, insert the new alarm
     * there.  ("next" is NULL, and "last" points to the link
     * field of the last item, or to the list header.)
     */
    if (!insert  && next == NULL) {
        *last = my_alarm;
        my_alarm->link = NULL;
    }
    insert = 0;  /* reset insert */
    /*
     * Release the semaphore
     *
     */
    sem_post(&alarm_sem);
}
/*
 * Added for Assignment 3
 * Function ProcessCancel
 * The function tries to match a cancel request with an alarm request in the list
 * It also checks for duplicate cancel requests
 * It returns a zero if successful and a 1 if it can't find a match
 */
 int ProcessCancel (void) {
    /* 
     * Acquire the semaphore
     *
     */
    sem_wait(&alarm_sem);
    /* Insert the new alarm into the list of alarms,
     * sorted by message number (messagenum).
     */
    insert = 0;
    last = &alarm_list;
    next = *last;
    while (next != NULL) {
        /* Test for a match and test to see if there's a cancel already */
        if (next->messagenum == my_alarm->messagenum) {
            /* test to see if it's a cancel and if it is then ignore this one */
            if (strncmp(&next->type, "C", 1) != 0) {
                *last = my_alarm;
                my_alarm->link = next;
                break;
            }
            else {
                /* cancel already exists */
                break;
            }
        }
        last = &next->link;
        next = next->link;
    }
    /*
     * If we reached the end of the list, then the Message Number does not exist
     * so return 1 to show an error.
     * 
     */
    if (next == NULL) 
        insert = 1;
    
    /*
     * Release the semaphore and return
     *
     */
    sem_post(&alarm_sem);
    return insert;
}
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 