#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>

#define BACKLOG 10
#define GRIDSIZE 10


int *connfdArr;

int server(int port);
void *thread(void *arg);

struct connection
{
    struct sockaddr_storage addr;
    socklen_t addr_len;
    int fd;
};

pthread_mutex_t lock;

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: %s [port]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    (void)server(atoi(argv[1]));
    return EXIT_SUCCESS;
}

typedef struct
{
    int x;
    int y;
} Position;

Position *posArr;

double rand01()
{
    return (double)rand() / (double)RAND_MAX;
}
typedef enum
{
    TILE_GRASS,
    TILE_TOMATO,
    TILE_PLAYER
} TILETYPE;

TILETYPE grid[GRIDSIZE][GRIDSIZE];

int score;
int level;
int numTomatoes;

// Position playerPosition;
void initGrid()
{
    for (int i = 0; i < GRIDSIZE; i++)
    {
        for (int j = 0; j < GRIDSIZE; j++)
        {
            double r = rand01();
            if (r < 0.1)
            {
                grid[i][j] = TILE_TOMATO;
                numTomatoes++;
            }
            else
                grid[i][j] = TILE_GRASS;
        }
    }

    // ensure grid isn't empty
    while (numTomatoes == 0)
        initGrid();
}

void moveTo(int x, int y, int cfd, int curr)
{
    // Prevent falling off the grid
    if (x < 0 || x >= GRIDSIZE || y < 0 || y >= GRIDSIZE)
    {
        printf("OOB\n");
        write(cfd, posArr, (sizeof(Position) * BACKLOG));

        return;
    }
    for (size_t i = 0; i < 10; i++)
        if (posArr[i].x != -1 && posArr[i].y != -1)
            if (posArr[i].x == x && posArr[i].y == y)
            {
                write(cfd, posArr, (sizeof(Position) * BACKLOG));

                return;
            }
    // Sanity check: player can only move to 4 adjacent squares
    if (!(abs(posArr[curr].x - x) == 1 && abs(posArr[curr].y - y) == 0) &&
        !(abs(posArr[curr].x - x) == 0 && abs(posArr[curr].y - y) == 1))
    {
        write(cfd, posArr, (sizeof(Position) * BACKLOG));
        fprintf(stderr, "Invalid move attempted from (%d, %d) to (%d, %d)\n", posArr[curr].x, posArr[curr].y, x, y);
        return;
    }

    posArr[curr].x = x;
    posArr[curr].y = y;

    if (grid[x][y] == TILE_TOMATO)
    {
        grid[x][y] = TILE_GRASS;
        score++;
        numTomatoes--;
        if (numTomatoes == 0)
        {
            level++;
            initGrid();
        }
    }
    printf("here\n");
    write(cfd, posArr, (sizeof(Position) * BACKLOG));
}

int server(int port)
{
    if (port < 1024 || port > 65535)
    {
        fprintf(stderr, "Invalid port number\n");
        return -1;
    }
    int socket_fd;
    struct sockaddr_in hint;
    struct connection *con;
    // pthread_t pthread;
    pthread_t tid;

    // IPv4
    memset(&hint, 0, sizeof hint);
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);

    // TCP
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    // bind
    if (bind(socket_fd, (struct sockaddr *)&hint, sizeof(hint)) == -1)
    {
        perror("bind");
        exit(1);
    }

    // listen
    if (listen(socket_fd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }
    // randomizes to initGrid can work properly
    srand(time(NULL));
    level = 1;
    initGrid();

    while (1)
    {

        con = malloc(sizeof(struct connection));
        con->addr_len = sizeof(struct sockaddr_storage);

        // accept connection
        con->fd = accept(socket_fd, (struct sockaddr *)&con->addr, &con->addr_len);
        if (con->fd == -1)
        {
            perror("accept");
            continue;
        }
        printf("Connected to (%d)\n", con->fd);

        // create new thread
        if (pthread_create(&tid, NULL, thread, con) != 0)
        {
            fprintf(stderr, "Unable to create thread: \n");
            close(con->fd);
            free(con);
            continue;
        }

        pthread_detach(tid);
    }
    // printf("Close\n");
    close(socket_fd);

    return 0;
}

void initConnfdArr(int *arr)
{
    for (size_t x = 0; x < BACKLOG; x++)
        arr[x] = -1;
}

void initPosArr(Position *pos)
{
    for (size_t x = 0; x < BACKLOG; x++)
    {
        Position p;
        p.x = -1;
        p.y = -1;
        pos[x] = p;
    }
}

void *thread(void *arg)
{

    char host[100], port[10];
    struct connection *c = (struct connection *)arg;
    int err;
    // find out the name and port of the remote host
    err = getnameinfo((struct sockaddr *)&c->addr, c->addr_len, host, 100, port, 10, NI_NUMERICSERV);
    
    if (err != 0)
    {
        fprintf(stderr, "getnameinfo: %s", gai_strerror(err));
        close(c->fd);
        return NULL;
    }
    printf("[%s:%s] connection\n", host, port);

    // storing cfd in an array on the heap, so no issues when sharing
    if (connfdArr == NULL)
    {
        pthread_mutex_lock(&lock);
        connfdArr = malloc(BACKLOG * sizeof(int));
        printf("Entered initConnfdArr\n");
        initConnfdArr(connfdArr);
        printf("Exited initConnfdArr\n");
        pthread_mutex_unlock(&lock);
    }

    // do the same shit as above ^^^^, as in initialize if needed, then store the shit
    if (posArr == NULL)
    {
        pthread_mutex_lock(&lock);
        posArr = malloc(sizeof(Position) * BACKLOG);
        printf("Entered initPosArr\n");
        initPosArr(posArr);
        printf("Exited initPosArr\n");
        pthread_mutex_unlock(&lock);
    }
    Position pp;
    size_t curr = 0;
    pthread_mutex_lock(&lock);
    for (size_t x = 0; x < BACKLOG; x++)
    {
        if (connfdArr[x] == -1)
        {
            connfdArr[x] = c->fd;
            printf("new CFD: %d\n", connfdArr[x]);
            break;
        }
    }

    for (size_t x = 0; x < BACKLOG; x++)
    {
        if (posArr[x].x == -1 && posArr[x].y == -1)
        {
            pp.x = pp.y = GRIDSIZE / 2;
            posArr[x] = pp;
            // printf("1:ppx: %d ppy: %d\n", posArr[curr].x, posArr[curr].y);
            curr = x;
            printf("curr posArr: %ld\n", curr);
            break;
        }
    }

    if (grid[posArr[curr].x][posArr[curr].y] == TILE_TOMATO)
    {
        grid[posArr[curr].x][posArr[curr].y] = TILE_GRASS;
        numTomatoes--;
    }
    pthread_mutex_unlock(&lock);

    // sending prelimanary info: grid and level and score
    write(c->fd, grid, sizeof(int) * GRIDSIZE * GRIDSIZE);
    write(c->fd, &level, sizeof(int));
    write(c->fd, &score, sizeof(int));
    write(c->fd, posArr, sizeof(Position) * BACKLOG);
    write(c->fd, &curr, sizeof(size_t));

    // now waits to accept movement/input from client
    while (1)
    {
        int tempx = -2, tempy = -2;
        // printf("1:ppx: %d ppy: %d\n", posArr[curr].x, posArr[curr].y);
        // printf("1: tempx: %d ppy: %d\n", tempy, );
        bool exit = false;
        read(c->fd, &tempx, sizeof(int));
        read(c->fd, &tempy, sizeof(int));
        read(c->fd, &exit, sizeof(bool));

        // exit protocol
        if (exit)
        {
            posArr[curr].x = posArr[curr].y = -1;

            write(connfdArr[curr], posArr, sizeof(Position) * BACKLOG);
            connfdArr[curr] = -1;
            break;
        }

        if (tempx != -2 || -2 != tempy)
        {
            pthread_mutex_lock(&lock);
            printf("Entered moveTo\n");
            moveTo(tempx, tempy, c->fd, curr);
            printf("Exited moveTo\n");

            for (size_t x = 0; x < BACKLOG; x++)
            {
                if (!(connfdArr[x] == -1))
                {
                    bool temp = true;
                    printf("cfd: %d, x: %ld\n", connfdArr[x], x);
                    write(connfdArr[x], &temp, sizeof(bool));

                    write(connfdArr[x], posArr, sizeof(Position) * BACKLOG);
                    write(connfdArr[x], grid, sizeof(int) * GRIDSIZE * GRIDSIZE);
                    write(connfdArr[x], &level, sizeof(int));
                    write(connfdArr[x], &score, sizeof(int));
                }
                else
                    break;
            }
            pthread_mutex_unlock(&lock);
        }
    }
    
    close(c->fd);
    free(c);

    return NULL;
}