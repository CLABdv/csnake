#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

//def = default but that word was taken
#define def 1
#define snake 2
#define apple 3
#define blank ' '
#define EVER ( ; ; )
struct handlesnakestruct
{
    long *time;
    int previnp;
    int inp;
    short *applepos;
    short *positions;
    short *age;
    int y;
    int x;
};

bool initsnake(short *, short *, int, int);
bool createapple(short*, short*, int, int);
bool runsnake(long *, short *, short *, short *, int, int);
void * handlesnake(void *);
void * checkinput(void *);
bool placesnake(void *, int *, int *);
void getcurryx(int, int *, int *, void *);


int main(int arc, char **argv)
{
    initscr();
    noecho();
    curs_set(0);
    keypad(stdscr, true);
    srand(time(NULL));
    cbreak();

    int x,y;
    //amount of time in nanoseconds the each frame sleeps
    long time = 35000000;
    

    getmaxyx(stdscr, y, x);
    //check size of terminal, if it's to small send error message
    if (y<15 || x < 15)
    {
        endwin();
        printf("terminal size to small. Try resizing.\n");
        return 1;
    }

// check if the terminal supports colours
    if (!has_colors()) 
    {
        endwin();
        printf("Your terminal does not support color\n");
        return 2;
    }
    start_color();

    if (can_change_color())
    {
        init_color(COLOR_GREEN, 200, 700, 300);
        init_color(COLOR_BLACK, 200, 0, 150);
        init_color(COLOR_WHITE, 800, 800, 800);
    }
    //init colours
	init_pair(def, COLOR_WHITE, COLOR_BLACK);
    init_pair(snake, COLOR_GREEN, COLOR_BLACK);
    init_pair(apple, COLOR_RED, COLOR_BLACK);
	attron(COLOR_PAIR(def));

    //  create borders (sloppily)
    int i, j;
    move(0,0);
    for (j=0; j<2; j++)
    {
        for (i=0; i<x; i++) addch('#');
        move(y-1, 0);
    }
    for (j=0; j<2; j++)
    {
        for(i=y; i>0; i--) mvaddch(i, j*(x-1), '#');
        move(y-1,x-1);
    }
    refresh();
    attroff(COLOR_PAIR(def));

    //holder of all possible positions
    // grid which is positioned as following:
    //
    // 0, 1, 2
    // 3, 4, 5
    // 6, 7, 8
    // 
    //if value on the position is one (1) the position is filled.
    short applepos;
    short * positions = calloc(y*x,sizeof(short));
    //to easily be able to see age of position
    short * age = calloc(y*x,sizeof(short));

    
    initsnake( age, positions, y, x);

    createapple(&applepos, positions, y, x);

    runsnake(&time, &applepos, positions, age, y, x);
    free(positions);
    free(age);

    endwin();
    return 0;
}

//frametime, time for frametime, age, positions, y, x
bool initsnake(short* age, short * positions, int y, int x)
{  
    //starting length of snake is 5. Only declared for clarity.
    short length = 5;
    
    //initialize snake colours 
    attron(COLOR_PAIR(snake));
    //starting positions of snake
    // and place snake start position
    int i;
    for (i=0; i<(length); i++)
    {
        positions[(y/2)*(x-2)+(x-2)/2-length+i-(x-2)] = true;
        age[(y/2)*(x-2)+(x-2)/2-length+i-(x-2)] = 4-i;
        mvaddch(y/2, x/2-length+i, '-');
    }    
    
    mvaddch(y/2, x/2-1, '<');

    refresh();
    attroff(COLOR_PAIR(snake));

    return 1;
}

//variable to store appleposition, snakepositions, y, x
bool createapple(short *applepos, short *positions, int y, int x)
{
    do
        *applepos = rand()%((y-2)*(x-2));
    while (positions[*applepos]);

    //create a bold and beautiful red apple 
    attron(COLOR_PAIR(apple));
    attron(A_BOLD);

    mvaddch((1+(*applepos-(*applepos%(x-2)))/(x-2)),1+(*applepos%(x-2)), '#');

    refresh();
    attroff(COLOR_PAIR(apple));
    attroff(A_BOLD);

    return 1;
}

//time/frame, appleposition, snakepositions, age, y, x
bool runsnake(long *time, short *applepos, short *positions, short *age, int y, int x)
{
    pthread_t HANDLESNAKE, CHECKINPUT;

    //create struct to pass to new threads
    struct handlesnakestruct snakestruct;
    snakestruct.time = time;
    snakestruct.applepos = applepos;
    snakestruct.positions=positions;
    snakestruct.age=age;
    snakestruct.y=y;
    snakestruct.x=x;
    snakestruct.previnp='d';

    //todo: do a tolower of input so that i dont need this
    //also optimise this or smth
    int startvalids[]={'W','S','D','w','s','d'};
    int i;
    for EVER
    {
        snakestruct.inp = getch();
        for (i=0; i<6; i++)
        {
            if (snakestruct.inp == startvalids[i])
                break;
        }
        if (i != 6)
            break;
    }

    pthread_create(&HANDLESNAKE, NULL, &handlesnake, (void *) &snakestruct);
    pthread_create(&CHECKINPUT, NULL, &checkinput, (void *) &snakestruct);

    pthread_join(HANDLESNAKE, NULL);
    pthread_join(CHECKINPUT, NULL);
    mvprintw(15,15,"cancel");
    return 1;
}

//this always checks for an inputpthread_cancel(CHECKINPUT);
void *checkinput(void * inputs)
{
    struct handlesnakestruct *inps= inputs;
    pthread_setcancelstate(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    //positions will be null if a collision has occured
    while (inps->positions != NULL)
    {
        inps -> inp = getch();
        
    }
}

void * handlesnake(void * inputs)
{
    struct handlesnakestruct *inps = inputs;
    //struct to be able to use nanosleep
    struct timespec slp;
    slp.tv_sec=0;
    int i, length, area, head, printy, printx;
    area = ((inps -> x) - 2) * ((inps -> y) - 2);
    length = 5;

    

    slp.tv_nsec=*inps->time;
    for EVER
    {
        for (i=0; i < area; i++)
        {
            if (inps->positions[i])
            {
                inps->age[i]++;
                if (inps->age[i] > length)
                {
                    inps->positions[i] = false;
                    inps->age[i] = 0;
                    getcurryx(i, &printy, &printx, (void *) inps);
                    mvaddch(printy, printx, blank);
                }
                else if (inps ->age[i] == 1)
                {
                    head = i;
                }
            } 
        }

        if (!placesnake((void *) inps, &head, &length))
        {
            attroff(COLOR_PAIR(snake));
            slp.tv_nsec=200000000;
            attron(A_BOLD);
            attron(COLOR_PAIR(apple));
            //15 is length of printed string
            mvprintw(0,(inps->x/2)-(15/2),"-- You lose! --");
            attroff(A_BOLD);
            attroff(COLOR_PAIR(apple));
            refresh();
            nanosleep(&slp,NULL);
            inps->positions = NULL;
            return NULL;
        }
        attroff(COLOR_PAIR(snake));
        inps ->previnp = inps->inp;
        refresh();
        nanosleep(&slp, NULL);
    }
}


bool placesnake(void *inputs, int *head, int *length)
{
    attron(COLOR_PAIR(snake));
    struct handlesnakestruct *inps = inputs;
    int printy, printx;
    getcurryx(*head, &printy, &printx, (void *) inps);
    switch (inps->inp)
    {
        case 'd':
        case 'D':
        if (inps->previnp != 'a' && inps->previnp != 'A')
        {
            if ((printx+1)>(inps->x-2))
                return false;
            ++*head;
            inps->positions[*head] = true;

            mvaddch(printy, printx+1, '<');
            switch (inps->previnp)
            {
                case 's':
                case 'S':
                mvaddch(printy, printx, '\\');
                break;
                case 'w':
                case 'W':
                mvaddch(printy, printx, '/');
                break;
                default:
                mvaddch(printy, printx, '-');
                break;
            }
        }
        else
        {
            inps->inp = 'a';
            if (!placesnake((void*) inps,head,length))
                return false;
        }
        break;

        case 'w':
        case 'W':
        if (inps->previnp != 's' && inps->previnp != 'S')
        {
            if (printy-1 < 1)
                    return false;
            (*head)-=(inps->x-2);
            inps->positions[*head]=true;
            mvaddch(printy-1,printx, 'V');
            switch (inps -> previnp)
            {
                case 'a':
                case 'A':
                mvaddch(printy, printx, '\\');
                break;
                case 'd':
                case 'D':
                mvaddch(printy, printx, '/');
                break;
                default:
                mvaddch(printy, printx, '|');
                break;
            }
        }
        else
        {
            inps->inp = 's';
            if (!placesnake((void*) inps,head,length))
                return false;

        }
        break;

        case 's':
        case 'S':
        if (inps->previnp != 'w' && inps->previnp != 'W')
        {
            if ((printy+1)>(inps->y-2))
                return false;
            (*head)+=(inps->x-2);
            inps->positions[*head] = true;

            mvaddch(printy+1, printx, '^');
            switch (inps->previnp)
            {
                case 'd':
                case 'D':
                mvaddch(printy,printx,'\\');
                break;
                case 'a':
                case 'A':
                mvaddch(printy,printx,'/');
                break;
                default:
                mvaddch(printy, printx, '|');
                break;
            }
        }
        else
        {
            inps->inp = 'w';
            if (!placesnake((void*) inps,head,length))
                return false;
        }
        break;

        case 'a':
        case 'A':
        if (inps->previnp != 'd' && inps->previnp != 'D')
        {
            if ((printx-1)<1)
                return false;
            --*head;
            inps->positions[*head] = true;
            mvaddch(printy, printx-1, '>');
            switch (inps->previnp)
            {
                case 's':
                case 'S':
                mvaddch(printy, printx, '/');
                break;
                case 'w':
                case 'W':
                mvaddch(printy, printx, '\\');
                break;
                default:
                mvaddch(printy, printx, '-');
                break;
            }
        }
        else
        {
            inps->inp = 'd';
            if (!placesnake((void*) inps,head,length))
                return false;
        }
        break;

        default:
        inps->inp=inps->previnp;
        if (!placesnake((void*) inps,head,length))
            return false;
        break;
    }
    if (*inps->applepos == *head)
    {
        ++*length;
        createapple(inps->applepos,inps->positions,inps->y,inps->x);
    }
    else if (inps->age[*head] > 0)
    {
        return false;
    }
    

    return true;
}

//get the y and x coordinate of position
//aka get row and column
void getcurryx(int pos, int *printy, int *printx, void *inputs)
{
    struct handlesnakestruct *inps = inputs;
    (*printx) = 1+(pos%(inps->x-2));
    (*printy) = 1+(pos-(pos%(inps->x-2)))/(inps->x-2);
}