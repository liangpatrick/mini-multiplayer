#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

// Dimensions for the drawn grid (should be GRIDSIZE * texture dimensions)
#define GRID_DRAW_WIDTH 640
#define GRID_DRAW_HEIGHT 640

#define WINDOW_WIDTH GRID_DRAW_WIDTH
#define WINDOW_HEIGHT (HEADER_HEIGHT + GRID_DRAW_HEIGHT)

// Header displays current score
#define HEADER_HEIGHT 50

// Number of cells vertically/horizontally in the grid
#define GRIDSIZE 10

typedef struct
{
    int x;
    int y;
} Position;

typedef enum
{
    TILE_GRASS,
    TILE_TOMATO,
    TILE_PLAYER
} TILETYPE;

TILETYPE grid[GRIDSIZE][GRIDSIZE];

// array of ALL players
Position *playPosArr;
size_t curr = 0;

int score;
int level;
int numTomatoes;
int clientFD;

bool shouldExit = false;

TTF_Font *font;

void initSDL()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "Error initializing SDL: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    int rv = IMG_Init(IMG_INIT_PNG);
    if ((rv & IMG_INIT_PNG) != IMG_INIT_PNG)
    {
        fprintf(stderr, "Error initializing IMG: %s\n", IMG_GetError());
        exit(EXIT_FAILURE);
    }

    if (TTF_Init() == -1)
    {
        fprintf(stderr, "Error initializing TTF: %s\n", TTF_GetError());
        exit(EXIT_FAILURE);
    }
}

void handleKeyDown(SDL_KeyboardEvent *event)
{
    // allows for read to happen
    bool keydown = false;
    // ignore repeat events if key is held down
    if (event->repeat)
        return;

    if (event->keysym.scancode == SDL_SCANCODE_Q || event->keysym.scancode == SDL_SCANCODE_ESCAPE)
    {
        shouldExit = true;
        write(clientFD, &playPosArr[curr].x, sizeof(int));
        write(clientFD, &playPosArr[curr].y, sizeof(int));
        write(clientFD, &shouldExit, sizeof(bool));
    }

    if (event->keysym.scancode == SDL_SCANCODE_UP || event->keysym.scancode == SDL_SCANCODE_W)
    {
        keydown = true;
        int temp = playPosArr[curr].y - 1;

        // printf("u:ppx: %d ppy: %d\n",playPosArr[curr].x, temp);
        write(clientFD, &playPosArr[curr].x, sizeof(int));
        write(clientFD, &temp, sizeof(int));
        write(clientFD, &shouldExit, sizeof(bool));
    }

    if (event->keysym.scancode == SDL_SCANCODE_DOWN || event->keysym.scancode == SDL_SCANCODE_S)
    {
        keydown = true;
        // moveTo(playerPosition.x, playerPosition.y + 1);
        int temp = playPosArr[curr].y + 1;

        // printf("d:ppx: %d ppy: %d\n",playPosArr[curr].x, temp);
        write(clientFD, &playPosArr[curr].x, sizeof(int));
        write(clientFD, &temp, sizeof(int));
        write(clientFD, &shouldExit, sizeof(bool));
    }

    if (event->keysym.scancode == SDL_SCANCODE_LEFT || event->keysym.scancode == SDL_SCANCODE_A)
    {
        keydown = true;
        // moveTo(playerPosition.x - 1, playerPosition.y);
        int temp = playPosArr[curr].x - 1;

        // printf("l:ppx: %d ppy: %d\n", temp,playPosArr[curr].y);
        write(clientFD, &temp, sizeof(int));
        write(clientFD, &playPosArr[curr].y, sizeof(int));
        write(clientFD, &shouldExit, sizeof(bool));
    }

    if (event->keysym.scancode == SDL_SCANCODE_RIGHT || event->keysym.scancode == SDL_SCANCODE_D)
    {
        keydown = true;
        // moveTo(playerPosition.x + 1, playerPosition.y);
        int temp = playPosArr[curr].x + 1;

        // printf("r:ppx: %d ppy: %d\n", temp,playPosArr[curr].y);
        write(clientFD, &temp, sizeof(int));
        write(clientFD, &playPosArr[curr].y, sizeof(int));
        write(clientFD, &shouldExit, sizeof(bool));
    }
    if (keydown)
    {
        read(clientFD, playPosArr, (sizeof(Position) * 10));
        // printf("m:ppx: %d ppy: %d\n",playPosArr[curr].x, playPosArr[curr].y);
    }
}

void processInputs()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            shouldExit = true;
            break;

        case SDL_KEYDOWN:
            handleKeyDown(&event.key);
            break;

        default:
            break;
        }
    }
}

void drawGrid(SDL_Renderer *renderer, SDL_Texture *grassTexture, SDL_Texture *tomatoTexture, SDL_Texture *playerTexture)
{
    SDL_Rect dest;
    for (int i = 0; i < GRIDSIZE; i++)
    {
        for (int j = 0; j < GRIDSIZE; j++)
        {
            dest.x = 64 * i;
            dest.y = 64 * j + HEADER_HEIGHT;

            SDL_Texture *texture = (grid[i][j] == TILE_GRASS) ? grassTexture : tomatoTexture;
            SDL_QueryTexture(texture, NULL, NULL, &dest.w, &dest.h);
            SDL_RenderCopy(renderer, texture, NULL, &dest);
        }
    }
    // renders all players that exist
    for (size_t x = 0; x < 10; x++)
    {
        if (playPosArr[x].x != -1 && playPosArr[x].y != -1)
        {
            dest.x = 64 * playPosArr[x].x;
            dest.y = 64 * playPosArr[x].y + HEADER_HEIGHT;
            SDL_QueryTexture(playerTexture, NULL, NULL, &dest.w, &dest.h);
            SDL_RenderCopy(renderer, playerTexture, NULL, &dest);
        }
    }
}

void drawUI(SDL_Renderer *renderer)
{
    // largest score/level supported is 2147483647
    char scoreStr[18];
    char levelStr[18];
    sprintf(scoreStr, "Score: %d", score);
    sprintf(levelStr, "Level: %d", level);

    SDL_Color white = {255, 255, 255};
    SDL_Surface *scoreSurface = TTF_RenderText_Solid(font, scoreStr, white);
    SDL_Texture *scoreTexture = SDL_CreateTextureFromSurface(renderer, scoreSurface);

    SDL_Surface *levelSurface = TTF_RenderText_Solid(font, levelStr, white);
    SDL_Texture *levelTexture = SDL_CreateTextureFromSurface(renderer, levelSurface);

    SDL_Rect scoreDest;
    TTF_SizeText(font, scoreStr, &scoreDest.w, &scoreDest.h);
    scoreDest.x = 0;
    scoreDest.y = 0;

    SDL_Rect levelDest;
    TTF_SizeText(font, levelStr, &levelDest.w, &levelDest.h);
    levelDest.x = GRID_DRAW_WIDTH - levelDest.w;
    levelDest.y = 0;

    SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreDest);
    SDL_RenderCopy(renderer, levelTexture, NULL, &levelDest);

    SDL_FreeSurface(scoreSurface);
    SDL_DestroyTexture(scoreTexture);

    SDL_FreeSurface(levelSurface);
    SDL_DestroyTexture(levelTexture);
}

void unix_error(char *msg) /* Unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}

int open_clientfd(char *hostname, char *port)
{
    int clientfd, rc;
    struct addrinfo hints, *listp, *p;

    /* Get a list of potential server addresses */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM; /* Open a connection */
    hints.ai_flags = AI_NUMERICSERV; /* ... using a numeric port arg. */
    hints.ai_flags |= AI_ADDRCONFIG; /* Recommended for connections */
    if ((rc = getaddrinfo(hostname, port, &hints, &listp)) != 0)
    {
        fprintf(stderr, "getaddrinfo failed (%s:%s): %s\n", hostname, port, gai_strerror(rc));
        return -2;
    }

    /* Walk the list for one that we can successfully connect to */
    for (p = listp; p; p = p->ai_next)
    {
        /* Create a socket descriptor */
        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue; /* Socket failed, try the next */

        /* Connect to the server */
        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1)
            break; /* Success */
        if (close(clientfd) < 0)
        { /* Connect failed, try another */ //line:netp:openclientfd:closefd
            fprintf(stderr, "open_clientfd: close failed: %s\n", strerror(errno));
            return -1;
        }
    }

    /* Clean up */
    freeaddrinfo(listp);
    if (!p) /* All connects failed */
        return -1;
    else /* The last connect succeeded */
        return clientfd;
}
int Open_clientfd(char *hostname, char *port)
{
    int rc;

    if ((rc = open_clientfd(hostname, port)) < 0)
        unix_error("Open_clientfd error");
    return rc;
}
void Close(int fd)
{
    int rc;

    if ((rc = close(fd)) < 0)
        unix_error("Close error");
}

// function so that one particular read can always read so everyone can update
int fd_set_blocking(int fd, int blocking)
{
    /* Save the current flags */
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return 0;

    if (blocking)
        flags &= ~O_NONBLOCK;
    else
        flags |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flags) != -1;
}

int main(int argc, char *argv[])
{
    int nread;
    bool start = false;

    playPosArr = malloc(sizeof(Position) * 10);
    for (size_t x = 0; x < 10; x++)
    {
        Position p;
        p.x = -1;
        p.y = -1;
        playPosArr[x] = p;
    }

    // clientFD is declared as a global variable up top
    char *host, *port;

    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }
    host = argv[1];
    port = argv[2];

    clientFD = Open_clientfd(host, port);
   
    // read all initial variables
    read(clientFD, grid, sizeof(int) * GRIDSIZE * GRIDSIZE);
    read(clientFD, &level, sizeof(int)); // level = 1;
    read(clientFD, &score, sizeof(int));
    read(clientFD, playPosArr, sizeof(Position) * 10);
    read(clientFD, &curr, sizeof(size_t));
 
    // too lazy to figure out if this is still needed
    srand(time(NULL));

    initSDL();

    font = TTF_OpenFont("resources/Burbank-Big-Condensed-Bold-Font.otf", HEADER_HEIGHT);
    if (font == NULL)
    {
        fprintf(stderr, "Error loading font: %s\n", TTF_GetError());
        exit(EXIT_FAILURE);
    }

    SDL_Window *window = SDL_CreateWindow("Client", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);

    if (window == NULL)
    {
        fprintf(stderr, "Error creating app window: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

    if (renderer == NULL)
    {
        fprintf(stderr, "Error creating renderer: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    SDL_Texture *grassTexture = IMG_LoadTexture(renderer, "resources/grass.png");
    SDL_Texture *tomatoTexture = IMG_LoadTexture(renderer, "resources/tomato.png");
    SDL_Texture *playerTexture = IMG_LoadTexture(renderer, "resources/player.png");

    // main game loop
    while (!shouldExit)
    {
        // printf("hello");
        SDL_SetRenderDrawColor(renderer, 0, 105, 6, 255);
        SDL_RenderClear(renderer);

        processInputs();

        // exit protocol
        if (shouldExit)
        {
            read(clientFD, playPosArr, sizeof(Position) * 10);
            continue;
        }
        // unblocks the socket and checks if successful
        int suc;
        if ((suc = fd_set_blocking(clientFD, 0)) == 1)
        {
            nread = read(clientFD, &start, sizeof(bool));
        }
        // blocks the socket, won't affect if suc != 1
        fd_set_blocking(clientFD, 1);
        // if read is successful, has if statement cause read blocks
        if ((nread) > 0)
        {
            // printf("here\n");

            read(clientFD, playPosArr, sizeof(Position) * 10);
            read(clientFD, grid, sizeof(int) * GRIDSIZE * GRIDSIZE);
            read(clientFD, &level, sizeof(int));
            read(clientFD, &score, sizeof(int));
        }

        drawGrid(renderer, grassTexture, tomatoTexture, playerTexture);
        drawUI(renderer);

        SDL_RenderPresent(renderer);

        SDL_Delay(16); // 16 ms delay to limit display to 60 fps
    }
    close(clientFD); //line:netp:echoclient:close

    // clean up everything
    SDL_DestroyTexture(grassTexture);
    SDL_DestroyTexture(tomatoTexture);
    SDL_DestroyTexture(playerTexture);

    TTF_CloseFont(font);
    TTF_Quit();

    IMG_Quit();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
