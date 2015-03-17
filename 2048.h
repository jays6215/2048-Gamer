#ifndef GAME_H_
#define GAME_H_
#define NDEBUG       // turn off assert() for debugging
#define EN_PRINT     // turn on printing functions

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <sys/time.h>
#endif
#include <ctime>
#include <fstream>
#include <iostream>
#include <assert.h>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>

using namespace std;
#define TRUE                1
#define FALSE               0

#define GRID_LENGTH         4
#define GRID_SIZE           16
#define STAGE_NUM           7
#define FIRST_STAGE         2048

#define INITIAL_TILE_NUM    2
#define EMPTY               0
#define NORMAL_TILE         2
#define WILD_CARD_TILE      4

#define TILE_GEN_RATIO      9

#define DIR_NUM             4

#define ERROR_KEY           -1

typedef enum{
    LEFT = 0,
    DOWN,
    RIGHT,
    UP,
    INVALID
} dir_e;

void gotoXY(int xPos, int yPos);
double cpuTime();

class Grid{
    private:
        int   m_data[GRID_SIZE];
        int   m_nEmptyBlk;
        int   m_score;
        int   Tsa[5][5][4];
        int   m_dir;

        // inline
        int&  getEntry(int row, int col);
        int&  getFlipEntry(int row, int col);
        int&  getTransEntry(int row, int col);
        int&  getFlipTransEntry(int row, int col);
    public:
        // inline
        Grid();
        int   operator[](int index);
        int   operator()(int row, int col);
        void  clear();
        void  copy(Grid& grid);
        void  setBlock(int index, int val);
        int   getEmptyBlkNo();
        int   getScore();
        int   getTsaValue(int x, int y, int z);
        int   Rotate( int row, int col);

        int   getMaxTile();
        bool  shift(dir_e dir, int mod);
        void  print(int xPos = 0, int yPos = 0);
};

class Game{
    private:
        Grid  m_grid;
        bool  m_gameOver;
        int   m_nRound;
        int   m_maxScore;
        int   m_scoreSum;
        int   m_maxTile;
        int   m_passCnt[STAGE_NUM];
        int   m_moveCnt;
        double   m_startTime;
        double   m_endTime;

        // UCB vars.
        map<int, int> val_mp;

        // inline
        int   getRand();
        void  genNewTile();
        void  updateStats();
        void  setGameOver();
        void  dumpLog(const char* fileName);
    public:

        int next_status[ 4 ] ;
        // UCB vars.
        unsigned long long update_id ;
        // previous Networks
        int pre_sm_board[ 17 ] ;

        Game();
        ~Game();
        void  reset();

        // inline
        void  getCurrentGrid(Grid& currGrid);
        bool  insertDirection(dir_e dir, int setPrint, int Method, vector<int>fs[], int sb);
        bool  isGameOver();
        int   getScore();
        int   getMaxScore();
        int   getMaxTile();
        int   getIndexValue( int x, int y ) ;
        int   getNextStatus( int x, int y, int z ) ;
        unsigned long long getBoardValue( int index ) ;
        void  doVirtualMove( int line ) ;
        void  printGrid(int xPos, int yPos);
        int   getSmallBoardValue( int index , vector<int> &feature ) ;
};

/*************************************************
                   Grid inline
*************************************************/
// Grid()
// Description: initialize members
inline
Grid::Grid(){
    clear();
}

// operator[]
// Description: returns entry value specified by index
//              returns error key when given index is out of range
// Arguments:
//     index  -  index of entry
// Return Val: entry value specified by index, or ERROR_KEY if out of range
inline
int Grid::operator[](int index){
    if(index < 0 || index > GRID_SIZE - 1){
        assert(FALSE);
        return ERROR_KEY;
    }
    return m_data[index];
}

// operator()
// Description: returns entry value specified by (row, col) coordinates
//              returns error key when given coordinates are out of range
// Arguments:
//     row  -  row number of entry
//     col  -  column number of entry
// Return Val: entry value specified by (row, col) coordinates, or ERROR_KEY if out of range
inline
int Grid::operator()(int row, int col){
    int index = row * GRID_LENGTH + col;
    if(index < 0 || index > GRID_SIZE - 1){
        assert(FALSE);
        return ERROR_KEY;
    }
    return m_data[index];
}

// clear()
// Description: sets all grid entries to EMPTY
inline
void Grid::clear(){
    memset(m_data, EMPTY, sizeof(m_data));
    m_nEmptyBlk = GRID_SIZE;
    m_score = 0;
    memset(Tsa, 0 , sizeof(Tsa));
}

inline
int Grid::getTsaValue(int x, int y, int z)
{
    return Tsa[ x ][ y ][ z ];
}


// copy()
// Description: copies contents of given grid into this grid
// Arguments:
//     grid  -  given grid
inline
void Grid::copy(Grid& grid){
    if(this == &grid)  return;
    memcpy(m_data, grid.m_data, sizeof(m_data));
    m_nEmptyBlk = grid.m_nEmptyBlk;
    m_score = grid.m_score;
}

// setBlock()
// Description: sets value of specified block
// Arguments:
//     index  -  index of block
//	   val    -  specified value that will be assigned to block
inline
void Grid::setBlock(int index, int val){
    if(index < 0 || index >= GRID_SIZE){
        assert(FALSE);
        return;
    }
    if(m_data[index] == EMPTY && val != EMPTY)
        m_nEmptyBlk--;
    else if(m_data[index] != EMPTY && val == EMPTY)
        m_nEmptyBlk++;
    m_data[index] = val;
}

// getEmptyBlkNo()
// Description: return number of empty blocks in grid
// Return Val: number of empty blocks in given grid
inline
int Grid::getEmptyBlkNo(){
    return m_nEmptyBlk;
}

// getScore()
// Description: return current score in grid
// Return Val: current score in grid
inline
int Grid::getScore(){
    return m_score;
}

inline
int Grid::Rotate(int row, int col)
{
    if( m_dir == 0 )
        return row * GRID_LENGTH + col ;
    if( m_dir == 1 )
        return (GRID_LENGTH - 1 - col) * GRID_LENGTH + row ;
    if( m_dir == 2 )
        return row * GRID_LENGTH + (GRID_LENGTH - 1 - col) ;
    return col * GRID_LENGTH + row ;
}

// getEntry()
// Description: get entry of original grid
// Arguments:
//     row  -  row number of entry
//     col  -  col number of entry
// Return Val: entry of original grid
inline
int& Grid::getEntry(int row, int col){
    m_dir = 0;
    return m_data[row*GRID_LENGTH + col];
}

// getFlipEntry()
// Description: get entry of horizontally flipped grid
// Arguments:
//     row  -  row number of entry
//     col  -  col number of entry
// Return Val: entry of horizontally flipped grid
inline
int& Grid::getFlipEntry(int row, int col){
    m_dir = 2;
    return m_data[row*GRID_LENGTH + (GRID_LENGTH - 1 - col)];
}

// getTransEntry()
// Description: get entry of transposed grid
// Arguments:
//     row  -  row number of entry
//     col  -  col number of entry
// Return Val: entry of transposed grid
inline
int& Grid::getTransEntry(int row, int col){
    m_dir = 3;
    return m_data[col*GRID_LENGTH + row];
}

// getFlipTransEntry()
// Description: get entry of horizontally flipped, transposed grid
// Arguments:
//     row  -  row number of entry
//     col  -  col number of entry
// Return Val: entry of horizontally flipped, transposed grid
inline
int& Grid::getFlipTransEntry(int row, int col){
    m_dir = 1;
    return m_data[(GRID_LENGTH - 1 - col)*GRID_LENGTH + row];
}

/*************************************************
                   Game inline
*************************************************/


// getRand()
// Description: return pseudo random integer value between "0 to RAND_MAX"
// Return Val: pseudo random integer value between "0 to RAND_MAX"
inline
int Game::getRand(){
    return rand();
}

// isGameOver()
// Description: check if game is over (cannot shift in any direction)
// Return Val: returns TRUE if game is over, FALSE if not
inline
bool Game::isGameOver(){
    return m_gameOver;
}

// getScore()
// Description: gets the current score of the game
// Return Val: current score of game
inline
int Game::getScore(){
    return m_grid.getScore();
}

// getScore()
// Description: gets the highest score ever achieved
// Return Val: highest score
inline
int Game::getMaxScore(){
    int currentScore = getScore();
    if(currentScore > m_maxScore)
        return currentScore;
    else
        return m_maxScore;
}

inline
int Game::getMaxTile(){
        return m_grid.getMaxTile() ;
}

inline
int Game::getIndexValue( int x , int y ){
    return val_mp[ m_grid(x, y) ];
}

inline
int Game::getSmallBoardValue( int index , vector<int> &ids )
{
    int ret = 0, tmp = 1 , rag = 18 ;
    int fz = ids.size();
    if( fz == 4 )
    {
        if( index == 4 )
            ret = ( val_mp[ m_grid( ids[3]/4, ids[3]%4 ) ] * rag * rag * rag
                  + val_mp[ m_grid( ids[2]/4, ids[2]%4 ) ] * rag * rag
                  + val_mp[ m_grid( ids[1]/4, ids[1]%4 ) ] * rag
                  + val_mp[ m_grid( ids[0]/4, ids[0]%4 ) ] ) ;
        else
            ret = ( val_mp[ getNextStatus(index, ids[3]/4, ids[3]%4) ] * rag * rag * rag
                  + val_mp[ getNextStatus(index, ids[2]/4, ids[2]%4) ] * rag * rag
                  + val_mp[ getNextStatus(index, ids[1]/4, ids[1]%4) ] * rag
                  + val_mp[ getNextStatus(index, ids[0]/4, ids[0]%4) ] ) ;
        return ret ;
    }
    for( int i = 0 ; i < fz ; i++ )
    {
        int j = ids[i] ;
        if( index == 4 )
            ret += tmp * val_mp[ m_grid( j/4 , j%4 ) ] ;
        else
            ret += tmp * val_mp[ getNextStatus(index, j/4, j%4) ] ;
        tmp *= rag ;
    }
    return ret ;
}

inline
unsigned long long Game::getBoardValue( int index )
{
    unsigned long long ret = 0, tmp = 1 ;
    for( int i = 0 ; i < GRID_SIZE ; i++ )
    {
        if( index == 4 )
            ret += tmp * val_mp[ m_grid( i/4 , i%4 ) ] ;
        else
            ret += tmp * val_mp[ getNextStatus(index, i/4, i%4)] ;
        tmp *= 16 ;
    }
    return ret ;
}

inline
int Game::getNextStatus( int x, int y, int z )
{
    return  m_grid.getTsaValue(x, y, z);
}

inline
void Game::doVirtualMove( int setPrint )
{
    next_status[0] = m_grid.shift(LEFT,0);
    next_status[1] = m_grid.shift(DOWN,0);
    next_status[2] = m_grid.shift(RIGHT,0);
    next_status[3] = m_grid.shift(UP,0);
    if( setPrint )
    {
        for( int i = 0 ; i < GRID_LENGTH ; i++ )
            for( int d = 0 ; d < 4 ; d++ )
            {
                for( int j = 0 ; j < GRID_LENGTH ; j++ )
                    printf( "%4d " , getNextStatus(d, i, j) ) ;
                printf( "%c" , "|\n"[d+1==4] ) ;
            }
        for( int d = 0 ; d < 4 ; d++ )
            printf( "%c Reward:   %7d %c", "xo"[next_status[d]], getNextStatus(d,4,0), "|\n"[d+1==4] ) ;
        for( int d = 0 ; d < 4 ; d++ )
            printf( "  EmpBlk:   %7d %c", getNextStatus(d,4,1), "|\n"[d+1==4] ) ;
        for( int d = 0 ; d < 4 ; d++ )
            printf( "%19llu %c", getBoardValue(d),"|\n"[d+1==4] ) ;
        printf( "+--------------Left-+---------------Down-+--------------Right-+---------------Up-+\n" ) ;
    }
}

// insertDirection()
// Description: shift tiles in given direction and generate new tile randomly
//              if cannot be shifted, will not generate new tile
// Arguments:
//     dir  -  shift direction
//
// Return Val: returns TRUE if shift was successful and tile was generated, FALSE if not
inline
bool Game::insertDirection(dir_e dir, int setPrint, int method , vector<int>fs[], int sb )
{

    doVirtualMove( setPrint ) ;
    /* record board St' in dir */
    update_id = getBoardValue((int)dir);
    for( int i = 0 ; i < 17 ; i++ )
        pre_sm_board[i] = getSmallBoardValue((int)dir, fs[i]) ;
    if(!m_grid.shift(dir,1))
        return FALSE;
    genNewTile();
    doVirtualMove( setPrint ) ;
    m_moveCnt++;
    setGameOver();
    return TRUE;
}

// getCurrentGrid()
// Description: copy contents of game grid into given grid
// Arguments:
//     currGrid  -  game grid will be copied into this grid
inline
void Game::getCurrentGrid(Grid& currGrid){
    currGrid.copy(m_grid);
}

// printGrid()
// Description: prints the game grid at the given (x,y) coordinates
// Arguements:
//      xPos  -  x coordinate
//      yPos  -  y coordinate
inline
void Game::printGrid(int xPos, int yPos){
    m_grid.print(xPos, yPos);
}

#endif
