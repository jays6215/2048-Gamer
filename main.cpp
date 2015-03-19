#include "2048.h"
#include <vector>
#include <map>
#include <ctime>
#include <cmath>
#include <cfloat>
#ifdef _WIN32
#include <conio.h>
#elif defined(__linux__)
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <unordered_map>
using namespace std;
int getch(void){
    struct termios oldattr, newattr;
    int ch;
    tcgetattr( STDIN_FILENO, &oldattr );
    newattr = oldattr;
    newattr.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newattr );
    ch = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldattr );
    return ch;
}
#endif
/* direction return */
const char* dirToStr(dir_e dir){
    if(dir == LEFT)   return "LEFT";
    if(dir == DOWN)   return "DOWN";
    if(dir == RIGHT)  return "RIGHT";
    if(dir == UP)     return "UP";
    return "INVALID";
}
dir_e mapDir( int dir )
{
    /* Return the direction */
    if(dir == 0)  return LEFT;
    if(dir == 1)  return DOWN;
    if(dir == 2)  return RIGHT;
    if(dir == 3)  return UP;
    return INVALID ;
}

// Exmaple1. Original game played with AWSD keys
dir_e getDirFromKeyboard(){
    int dir;
    dir = getch();
    if(dir == 'A' || dir == 'a')  return LEFT;
    if(dir == 'S' || dir == 's')  return DOWN;
    if(dir == 'D' || dir == 'd')  return RIGHT;
    if(dir == 'W' || dir == 'w')  return UP;
    return INVALID;
}

// Exmaple2. Random direction inserting bot
dir_e getRandDir(){
    int dir = rand()%4;
    if(dir == 0)  return LEFT;
    if(dir == 1)  return DOWN;
    if(dir == 2)  return RIGHT;
    if(dir == 3)  return UP;
    return INVALID;
}

/* Useful method */
void stop( )
{
    char cc ;
    do{
        cc =  getch();
    }while( cc != 'a' ) ;
}
void ClearWindow()
{
    #ifdef _WIN32
    system("cls");
    #elif defined(__linux__)
    system("clear");
    #endif
}

/* Parameters */
int      ExpTs = 30 ;
int    TrainTs = 500000 ;
bool  setPrint = 0 ;   // Print board or not
bool   setTest = 0 ;   // 0=Train, 1=Test
int     method = 1 ;   // 0 - 1 0=Random, 1=UCB, 2=Select
int   choose_m = 0 ;   // 1 - 4
int   update_m = 0 ;   // 1 - 3
int  small_big = 0 ;   // Small=0, Big=1
int cumulative = 0 ;   // 0:=, 1:+=
double   alpha = 0.5 ; // TD-Learning alpha
double    beta = 1.0 ; // UCB-c
const char testf[ 2 ][ 10 ] = { "[train]", "[test]" } ;

/* Define */
#define FEATURESIZE         17
#define FEAKINDS        200000
#define INF         1000000000

/* Recorder */
map <int,int> logtwo ;
int tile_cnt[ 18 ] ;

/* Lookup Tables */
struct STATUS
{
    double sum_score ;
    int access ;
};
/* Feature recoredrs */
unordered_map<unsigned long long, STATUS> boards;
vector <int>    fea[ FEATURESIZE ] ;
STATUS small_boards[ FEATURESIZE ][ FEAKINDS ] ; // 17 * 18 ^ 4

STATUS getBigBoardItem( unsigned long long tar )
{
    STATUS ret ;
    ret.sum_score = 0.0 ;
    ret.access = 0 ;
    if( boards.find( tar ) != boards.end() )
    {
        ret.sum_score = boards[ tar ].sum_score ;
        ret.access = boards[ tar ].access  ;
    }
    return ret ;
}

double getSmBoardAvgValue( int fid, int tar )
{
    STATUS ret = small_boards[ fid ][ tar ] ;
    if( ret.access == 0 )
        return 0.0 ;
    return ret.sum_score ;
    //return ret.sum_score / ret.access ;
}
double getBigBoardAvgValue( unsigned long long tar )
{
    STATUS ret ;
    ret.sum_score = 0.0 ;
    ret.access = 0 ;
    if( boards.find( tar ) != boards.end() )
    {
        ret.sum_score = boards[ tar ].sum_score ;
        ret.access = boards[ tar ].access  ;
    }
    if( ret.access == 0 )
        return 0.0 ;
    return ret.sum_score ;
    //return ret.sum_score / ret.access ;
}
void Initail()
{
    // Recorder clear
    logtwo.clear();
    memset( tile_cnt, 0, sizeof(tile_cnt) ) ;
    for( int i = 0 , j = 1 ; i < 18 ; i++ )
    {
        if( j == 1 )
            logtwo[ 0 ] = i ;
        else
            logtwo[ j ] = i ;
        j *= 2 ;
    }
    /* LUT clear */
    boards.clear( ) ;
    for( int i = 0 ; i < FEATURESIZE ; i++ )
        for( int j = 0 ; j < FEAKINDS ; j++ )
        {
            small_boards[i][j].sum_score = 0.0 ;
            small_boards[i][j].access = 1 ;
        }
    if( setTest )
    {
        char cname[ 220 ] ;
        FILE *Ftrain ;
        sprintf( cname, "%s[%02d][Feature][%c][M%dC%dU%d][alpha%3.0f][beta%.0f].dat"
                , testf[1-setTest], ExpTs, "AC"[cumulative], method, choose_m, update_m, alpha*10000 , beta*1000 ) ;
        Ftrain = fopen( cname, "r" ) ;
        for( int i = 0 ; i < FEATURESIZE ; i++ )
            for( int j = 0 ; j < FEAKINDS ; j++ )
                fscanf( Ftrain, "%lf %d", &small_boards[i][j].sum_score, &small_boards[i][j].access ) ;
        fclose( Ftrain ) ;
        printf( "Read %s OK!" , cname ) ;
        #if 0
        for( int i = 0 ; i < FEATURESIZE ; i++ )
        {
            for( int j = 0 ; j < FEAKINDS ; j++ )
                if( small_boards[i][j].sum_score != 0 )
                    printf( "%.13f %d " , small_boards[i][j].sum_score, small_boards[i][j].access ) ;
            printf( "\n" ) ;
        }
        #endif
    }
    for( int i = 0 ; i < FEATURESIZE ; i++ )
        fea[ i ].clear( ) ;
    #if 1
    /* Basic Contruct LUTs */
    /* 4 col */
    for( int i = 0 ; i < 4 ; i++ )
        for( int j = 0 ; j < 4 ; j++ )
            fea[ i ].push_back( 4 * j + i ) ;
    /* 4 row */
    for( int i = 0 ; i < 4 ; i++ )
        for( int j = 0 ; j < 4 ; j++ )
            fea[ i+4 ].push_back( 4 * i + j ) ;
    /* 9 squre */
    int s_shift[ 4 ] = {0, 1, 4, 5} ;
    for( int i = 0 ; i < 9 ; i++ )
        for( int j = 0 ; j < 4 ; j++ )
            fea[ i+8 ].push_back( 4 * (i/3) + (i%3) + s_shift[j] ) ;
    #else
    fea[ 0].push_back( 3) , fea[ 0].push_back( 7) , fea[ 0].push_back(11) , fea[ 0].push_back(15) ;
    fea[ 1].push_back( 0) , fea[ 1].push_back( 1) , fea[ 1].push_back( 2) , fea[ 1].push_back( 3) ;
    fea[ 2].push_back(12) , fea[ 2].push_back( 8) , fea[ 2].push_back( 4) , fea[ 2].push_back( 0) ;
    fea[ 3].push_back(15) , fea[ 3].push_back(14) , fea[ 3].push_back(13) , fea[ 3].push_back(12) ;
    fea[ 4].push_back(15) , fea[ 4].push_back(11) , fea[ 4].push_back( 7) , fea[ 4].push_back( 3) ;
    fea[ 5].push_back( 3) , fea[ 5].push_back( 2) , fea[ 5].push_back( 1) , fea[ 5].push_back( 0) ;
    fea[ 6].push_back( 0) , fea[ 6].push_back( 4) , fea[ 6].push_back( 8) , fea[ 6].push_back(12) ;
    fea[ 7].push_back(12) , fea[ 7].push_back(13) , fea[ 7].push_back(14) , fea[ 7].push_back(15) ;
    #endif
    #if 0
    for( int i = 0 ; i < FEATURESIZE ; i++ )
        for( int j = 0 ; j < fea[i].size() ; j++ )
            printf( "%d%c" , fea[i][j] , " \n"[j+1==fea[i].size()] ) ;
    getch();
    #endif
}
void PlayNRounds( )
{
    /* UCB initailize */
    Game myGame;
    dir_e dir;
    bool isGameOver;
    char cname[ 220 ] ;
    char fname[ 5 ][ 50 ] = { "AvgScore" , "AvgWinR" , "TileDis", "GameScore" , "MaxTile" } ;
    FILE *fout[ 5 ] ;
    if( !setPrint )
        for( int i = 0 ; i < 5 ; i++ )
        {
            sprintf( cname, "%s[%02d][%s][%c][M%dC%dU%d][alpha%3.0f][beta%.0f].dat"
                    , testf[setTest], ExpTs, fname[i], "AC"[cumulative], method, choose_m, update_m, alpha*10000 , beta*1000 ) ;
            printf( "File: %40s\n", cname ) ;
            fout[i] = fopen( cname , "w" ) ;
        }
    Initail();
    ClearWindow();
    int           step = 0 ;
    int       max_tile = 0 ;
    int      cur_max_t = 0 ;
    int         cur_sb = 0 ;
    double  act_reward = 0 ;
    double  last_avg_s = 0 ;
    double   avg_score = 0 ;
    double avg_winrate = 0 ;
    double      tmp[9] = { 0 } ;
    STATUS tar , next ;
    for( int i = 0 ; i < TrainTs ; i++ )
    {
        step = 0 ;
        isGameOver = false;
        while( !isGameOver )
        {
            /* Choose direction */
            /* Random */
            if( method == 0 )
                while((dir = getRandDir()) == INVALID);
            /* UCB */
            else if( method == 1 )
            {
                if( i == 0 ) myGame.doVirtualMove(0) ;
                double tol_c = 0.0 , cnt_a[4] = { 0 } , avg_s[4] = { 0 } ;
                for( int d = 0 ; d < 4 ; d++ )
                {
                    // next status
                    if( small_big )
                        tar = getBigBoardItem(myGame.getBoardValue(d));
                    else
                    {
                        tar.sum_score = 0, tar.access = 0 ;
                        for( int f = 0 ; f < FEATURESIZE ; f++ )
                        {
                            int nextid = myGame.getSmallBoardValue(d, fea[f]) ;
                            #if 0
                            if( 4 <= f && f <= 7 )
                            {
                                printf( "%2d ||" , f ) ;
                                for( int k = 0 ; k < fea[f].size() ; k++ )
                                    printf( "%2d " , fea[f][k] ) ;
                                for( int k = 0 , L = nextid ; k < 4 ; k++ )
                                    printf( "%c%2d" , " |"[k==0] , L % 18 ) , L /= 18 ;
                                printf( "| %7d %8.4f / %4d // %8.4f | %8.4f\n"
                                        , nextid
                                        , small_boards[ f ][ nextid ].sum_score
                                        , small_boards[ f ][ nextid ].access
                                        , small_boards[ f ][ nextid ].sum_score / small_boards[ f ][ nextid ].access
                                        , getSmBoardAvgValue( d, nextid ) ) ;
                                if( f == 7 )
                                    printf( "\n" ) ;
                            }
                            #endif
                            next = small_boards[ f ][ nextid ] ;
                            tar.sum_score += next.sum_score ;
                            tar.access += next.access ;
                        }
                    }
                    cnt_a[d] = ( tar.access == 0 ? 1 : tar.access ) ;
                    avg_s[d] = tar.sum_score / FEATURESIZE ;
                    //avg_s[d] = tar.sum_score / cnt_a[d] ;
                    tol_c += cnt_a[d] ;
                }
                double tops = (double)-INF*INF , curr_s = 0.0 ;
                double act_s[ 4 ] = { 0 } ;
                for( int d = 0 ; d < 4 ; d++ )
                    if( myGame.next_status[d] )
                    {
                        // Action Reward
                        tmp[0] = myGame.getNextStatus(d,4,0) ;
                        // Average value of tiles after this action
                        tmp[1] = (double)myGame.getNextStatus(d,4,2) / myGame.getNextStatus(d,4,3) ;
                        // Merge Count after this action
                        tmp[2] = myGame.getNextStatus(d,4,1) ;
                        /***************************************
                         * UCB   ----> 2 or 8(*n)
                         * TD(0) ----> 4 or 6(*n)
                         * MyF   ----> 5 or 7(*n)
                         ***************************************/
                        /* 0. Top score chooser: V(s') */
                        if( choose_m == 0 )
                            curr_s = avg_s[d] ;
                        /* 1. UCB chooser xx */
                        else if( choose_m == 1 )
                            curr_s = avg_s[d] + beta * sqrt( log(tol_c) / cnt_a[d] ) ;

                        /* 2. UCB 1 - chooser */
                        else if( choose_m == 2 )
                            curr_s = avg_s[d] + tmp[0] + beta * sqrt( log(tol_c) / cnt_a[d] ) ;
                        /* 8. UCB 2 - chooser */
                        else if( choose_m == 8 )
                            curr_s = avg_s[d] * FEATURESIZE + tmp[0] + beta * sqrt( log(tol_c) / cnt_a[d] ) ;

                        /* 4. TD0 1 - V(s') + Reward */
                        else if( choose_m == 4 )
                            curr_s = avg_s[d] + tmp[0] ;
                        /* 6. TD0 2 - V(s') * 17.0 + Reward */
                        else if( choose_m == 6 )
                            curr_s = avg_s[d] * FEATURESIZE + tmp[0] ;

                        /* 5. MyF 1 - V(s') + Reward + V(s'_Merges) + V(s'_AvgValueOfTiles) */
                        else if( choose_m == 5 )
                            curr_s = avg_s[d] + tmp[0] + tmp[2] + tmp[1] ;
                        /* 7. MyF 2 - V(s') + Reward + V(s'_Merges) + V(s'_AvgValueOfTiles) */
                        else if( choose_m == 7 )
                            curr_s = avg_s[d] * FEATURESIZE + tmp[0] + tmp[2] + tmp[1] ;

                        /* 3. MyF + UCB chooser 1 */
                        else if( choose_m == 3 )
                            curr_s = avg_s[d] * FEATURESIZE + tmp[0] + tmp[2] + tmp[1] + beta * sqrt( log(tol_c) / cnt_a[d] ) ;
                        /* 9. MyF + UCB chooser 2 */
                        else if( choose_m == 9 )
                            curr_s = avg_s[d] * FEATURESIZE + tmp[0] + tmp[2] + tmp[1] + beta * sqrt( log(tol_c) / cnt_a[d] ) ;

                        act_s[d] = curr_s ;
                        tops = max( tops , act_s[d] ) ;
                    }
                vector <int> dirs ;
                for( int d = 0 ; d < 4 ; d++ )
                    if( myGame.next_status[d] && abs(tops - act_s[d]) < 1e-7 )
                        dirs.push_back( d ) ;
                int ds = (int) dirs.size(), sel_dir = 0 ;
                if( ds > 1 )
                    sel_dir = dirs[ rand() % ds ] ;
                else if( ds == 1 )
                    sel_dir = dirs[ 0 ] ;
                dir = mapDir( sel_dir ) ;
                act_reward = (double)myGame.getNextStatus(sel_dir,4,0) ;
                #if 0
                printf( "ACT %d Reward: %f\n" , sel_dir , act_reward ) ;
                for( int d = 0 ; d < 4 ; d++ )
                    printf( "State[Rew Emp Sum Cnt][%d]: %4d %4d %4d %4d [%4d]\n"
                          , d
                          , myGame.getNextStatus(0,4,d)
                          , myGame.getNextStatus(1,4,d)
                          , myGame.getNextStatus(2,4,d)
                          , myGame.getNextStatus(3,4,d)
                          , myGame.getNextStatus(4,4,d) ) ;
                #endif
            }
            /* User select */
            else if( method == 2 )
                while((dir = getDirFromKeyboard()) == INVALID);
            if( setPrint )
                ClearWindow(), myGame.printGrid(5,0), gotoXY(0,6);
            /* Shift the board */
            myGame.insertDirection(dir, setPrint, method, fea, small_big);
            /* Check dead */
            isGameOver = myGame.isGameOver();
            /* update board St' value by 4 St+1' and 4 reword from 4 shifts */
            if( method == 1 && setTest == 0 )
            {
                #if 0
                for( int d = 0 ; d < 4 ; d++ )
                    for( int f = 4 ; f <= 7 ; f++ )
                    {
                        int nextid = myGame.getSmallBoardValue(d, fea[f]) ;
                        printf( "%2d ||" , f ) ;
                        for( int k = 0 ; k < fea[f].size() ; k++ )
                            printf( "%2d " , fea[f][k] ) ;
                        for( int k = 0 , L = nextid ; k < 4 ; k++ )
                            printf( "%c%2d" , " |"[k==0] , L % 18 ) , L /= 18 ;
                        printf( "| %7d %8.4f / %4d // %8.4f | %8.4f\n"
                                , nextid
                                , small_boards[ f ][ nextid ].sum_score
                                , small_boards[ f ][ nextid ].access
                                , small_boards[ f ][ nextid ].sum_score / small_boards[ f ][ nextid ].access
                                , getSmBoardAvgValue( d, nextid ) ) ;
                        if( f == 7 )
                            printf( "\n" ) ;
                    }
                #endif
                double as_val = 0.0 , asn_val[ 4 ] = { 0 } ;
                double tol_c = 0.0 , cnt_a[ 4 ] = { 0 } ;
                // Current board sum value = V(s')
                // Next board sum value = V(s'_next)
                for( int f = 0 ; f < FEATURESIZE ; f++ )
                {
                    if( small_big )
                    {
                        tar = getBigBoardItem( myGame.update_id ) ;
                        as_val = getBigBoardAvgValue( myGame.update_id ) ;
                        for( int d = 0 ; d < 4 ; d++ )
                        {
                            int fid = myGame.getBoardValue(d) ;
                            STATUS tmps = getBigBoardItem( fid ) ;
                            asn_val[ d ] = getBigBoardAvgValue( fid ) ;
                            cnt_a[ d ] += tmps.access ;
                            tol_c += tmps.access ;
                        }
                        break ;
                    }
                    else
                    {
                        for( int d = 0 ; d < 4 ; d++ )
                        {
                            int fid = myGame.getSmallBoardValue(d, fea[f]) ;
                            asn_val[ d ] += getSmBoardAvgValue( f, fid ) ;
                            cnt_a[ d ] += small_boards[ f ][ fid ].access ;
                            tol_c += small_boards[ f ][ fid ].access ;
                        }
                        as_val += getSmBoardAvgValue( f, myGame.pre_sm_board[f] ) ;
                        tar.sum_score = 0 , tar.access = 0 ;
                    }
                }
                // Current board average value = V(s')
                if( small_big )
                {
                    tar = getBigBoardItem( myGame.update_id ) ;
                    tmp[0] = getBigBoardAvgValue( myGame.update_id ) ;
                }
                else
                    tar.sum_score = 0 , tar.access = 0 ;
                // If the terminated board
                if( isGameOver )
                {
                    tol_c = 4 ;
                    for( int d = 0 ; d < 4 ; d++ )
                        asn_val[ d ] = 0 , cnt_a[ d ] = 1 ;
                }
                for( int f = 0 ; f < FEATURESIZE ; f++ )
                {
                    // Current board average value = V(s')[part]
                    if( !small_big )
                        tmp[0] = getSmBoardAvgValue( f, myGame.pre_sm_board[f] ) ;
                    double max_diff = (double)-INF*INF , sel_upds = 0 ;
                    double diff = 0 , upds = 0 ;
                    for( int d = 0 ; d < 4 ; d++ )
                        if( myGame.next_status[d] || isGameOver )
                        {
                            // Next board average value = V(s'_next)[part]
                            if( small_big )
                                tmp[5] = getBigBoardAvgValue( myGame.getBoardValue(d) ) ;
                            else
                                tmp[5] = getSmBoardAvgValue( f, myGame.getSmallBoardValue(d, fea[f]) ) ;
                            // Action Reward_next
                            tmp[1] = myGame.getNextStatus(d,4,0) ;

                            //tmp[0] = as_val ;
                            //tmp[5] = asn_val[d] ;
                            //tmp[1] = act_reward ;

                            // V(s'_next_AvgValueOfTiles) Average values of tiles
                            tmp[2] = (double)myGame.getNextStatus(d,4,2) / myGame.getNextStatus(d,4,3) ;
                            // V(s'_AvgValueOfTiles) Current board average values of tiles
                            tmp[4] = (double)myGame.getNextStatus(4,4,2) / myGame.getNextStatus(4,4,3) ;
                            // V(s'_next_Merges) Merge Blocks counts
                            tmp[3] = myGame.getNextStatus(d,4,1) ;

                            /********************************************************
                             * TD(0) -----> 15 or 16(myF)
                             * UCB   -----> 17 or 18(myF)
                             ********************************************************/

                            /* 15. V(s'_next) + Reward_next  */
                            /* 17. V(s') + alpha * ( Reward_next + V(s'_next) - V(s') */
                            if( update_m == 15 )
                            {
                                diff = asn_val[d] + tmp[1] ;
                                upds = tmp[0] + alpha * ( tmp[1] + asn_val[d] - as_val ) ;
                            }
                            /* 16. V(s') + alpha * ( Reward_next + V(s'_next_Merges) + V(s'_next_AvgValueOfTiles) - V(s'_AvgValueOfTiles) + V(s'_next) - V(s') */
                            else if( update_m == 16 )
                            {
                                diff = asn_val[d] + ( tmp[1] + tmp[3] + tmp[2] - tmp[4] ) ;
                                upds = tmp[0] + alpha * ( tmp[1] + tmp[3] + tmp[2] - tmp[4] + asn_val[d] - as_val ) ;
                            }
                            /* 17. V(s') + alpha * ( Reward_next + V(s'_next) - V(s') with UCB diff */
                            else if( update_m == 17 )
                            {
                                diff = asn_val[d] + tmp[1] + beta * sqrt( log(tol_c) / cnt_a[d] ) ;
                                upds = tmp[0] + alpha * ( tmp[1] + asn_val[d] - as_val ) ;
                            }
                            /* 18. V(s') + alpha * ( Reward_next + V(s'_next_Merges) + V(s'_next_AvgValueOfTiles) - V(s'_AvgValueOfTiles) + V(s'_next) - V(s') with UCB diff */
                            else if( update_m == 18 )
                            {
                                diff = asn_val[d] + tmp[1] + tmp[3] + tmp[2] - tmp[4] + beta * sqrt( log(tol_c) / cnt_a[d] ) ;
                                upds = tmp[0] + alpha * ( tmp[1] + tmp[3] + tmp[2] - tmp[4] + asn_val[d] - as_val ) ;
                            }
                            if( max_diff < diff )
                            {
                                max_diff = diff ;
                                sel_upds = upds ;
                            }
                        }
                    /************************************************
                     * C cumulative Mode
                     * A assignment Mode
                     ************************************************/
                    if( cumulative )
                    {
                        tar.sum_score += sel_upds ;
                        tar.sum_score /= 2 ;
                        tar.access += 1 ;
                        if( small_big )
                            boards[ myGame.update_id ] = tar ;
                        else
                        {
                            small_boards[ f ][ myGame.pre_sm_board[f] ].sum_score = sel_upds ;
                            small_boards[ f ][ myGame.pre_sm_board[f] ].access    += 1 ;
                            #if 1 // Better Semi-Cumulative
                            //small_boards[ f ][ myGame.pre_sm_board[f] ].sum_score /= 2 ;
                            //small_boards[ f ][ myGame.pre_sm_board[f] ].access    /= 2 ;
                            #endif
                        }
                        tar.sum_score = 0, tar.access = 0 ;
                    }
                    else
                    {
                        tar.sum_score = sel_upds , tar.access = 1 ;
                        if( small_big )
                            boards[ myGame.update_id ] = tar ;
                        else
                        {
                            small_boards[ f ][ myGame.pre_sm_board[f] ].sum_score = sel_upds ;
                            small_boards[ f ][ myGame.pre_sm_board[f] ].access    = 1 ;
                        }
                    }
                    if( small_big )
                        break ;
                }
            }
            /***************************************************/
            if( setPrint )
            {
                //gotoXY( 5,5), printf( "Previous" );
                //gotoXY(25,5), printf( "--%5s->" , dirToStr(dir) );
                //gotoXY(47,5), printf( "Current" );
                //myGame.printGrid(35,0),  gotoXY(0,22);
                /* This game status */
                printf( "tmp5 %f\n" , tmp[5] ) ;
                printf( "Round[%2d]:%8d\n", ExpTs, i+1 ) ;
                printf( "Step:      %7d\n", step++ ) ;
                printf( "Direct:    %7s\n", dirToStr(dir) ) ;
                printf( "Score:     %7d\n", myGame.getScore() ) ;
                printf( "CMax Tile: %7d\n", myGame.getMaxTile() ) ;
                #if 0
                printf( "sum_score: %16.4f\n" , tar.sum_score ) ;
                printf( "count:     %16d\n" , tar.access ) ;
                printf( "S' HASH: %18llu\n" , myGame.update_id ) ;
                unsigned long long preb = myGame.update_id ;
                for( int j = 0 ; j < 16 ; j++ )
                {
                    printf("%2d%c" , preb%16, " \n"[j%4==3] ) ;
                    preb /= 16 ;
                }
                for( int j = 4 ; j <= 7 ; j++ )
                {
                    printf( "%2d ||" , j ) ;
                    for( int k = 0 ; k < fea[j].size() ; k++ )
                        printf( "%2d " , fea[j][k] ) ;
                    int nextid = myGame.pre_sm_board[j] ;
                    for( int k = 0 , L = nextid ; k < 4 ; k++ )
                        printf( "%c%2d" , " |"[k==0] , L % 18 ) , L /= 18 ;
                    printf( " |%7d %8.4f / %4d // %8.4f | %8.4f\n"
                            , nextid
                            , small_boards[ j ][ nextid ].sum_score
                            , small_boards[ j ][ nextid ].access
                            , small_boards[ j ][ nextid ].sum_score / small_boards[ j ][ nextid ].access
                            , getSmBoardAvgValue( j, nextid ) ) ;
                }
                printf( "S'' HASH: %20llu\n" , myGame.getBoardValue(4) ) ;
                unsigned long long currid = 0 , t16 = 1 ;
                for( int j = 0 ; j < 16 ; j++ )
                {
                    printf("%2d%c" , myGame.getIndexValue(j/4, j%4), " \n"[j%4==3] ) ;
                    currid += t16 * myGame.getIndexValue(j/4, j%4) ;
                    t16 *= 16 ;
                }
                //printf( "S'' HASH: %20llu\n" , currid ) ;
                for( int d = 0 ; d < 4 ; d++ )
                    printf( "Next State[Rew Emp Sum Cnt][%d]: %4d %4d %4d %4d [%4d]\n"
                          , d
                          , myGame.getNextStatus(0,4,d)
                          , myGame.getNextStatus(1,4,d)
                          , myGame.getNextStatus(2,4,d)
                          , myGame.getNextStatus(3,4,d)
                          , myGame.getNextStatus(4,4,d) ) ;
                for( int j = 4 ; j <= 7 ; j++ )
                {
                    printf( "%2d ||" , j ) ;
                    for( int k = 0 ; k < fea[j].size() ; k++ )
                        printf( "%2d " , fea[j][k] ) ;
                    int nextid = myGame.getSmallBoardValue(4, fea[j]) ;
                    for( int k = 0 , L = nextid ; k < 4 ; k++ )
                        printf( "%c%2d" , " |"[k==0] , L % 18 ) , L /= 18 ;
                    printf( " |%7d %8.4f / %4d // %8.4f | %8.4f\n"
                            , nextid
                            , small_boards[ j ][ nextid ].sum_score
                            , small_boards[ j ][ nextid ].access
                            , small_boards[ j ][ nextid ].sum_score / small_boards[ j ][ nextid ].access
                            , getSmBoardAvgValue( j, nextid ) ) ;
                }
                getch();
                #endif
            }
        }
        if( setPrint )
            myGame.printGrid(35,0);
        /* Average game status until now */
        cur_max_t = myGame.getMaxTile();
        max_tile = max( max_tile, cur_max_t ) ;
        tile_cnt[ logtwo[ cur_max_t ] ]++ ;
        avg_score = (avg_score * i + myGame.getScore()) / (i+1) ;
        avg_winrate = (avg_winrate * i + (cur_max_t>=2048)) / (i+1) ;
        if( (i+1) % 200 == 0 )
        {
            if( setPrint ) ClearWindow();
            else gotoXY(0,0);
            //if( setPrint ) gotoXY(20,21); printf( "| Step:      %7d\n", step ) ;
            if( setPrint ) gotoXY(20,22); printf( "| [%02d][%c][M%dC%dU%d]%s\n", ExpTs, "AC"[cumulative], method, choose_m, update_m, testf[setTest]) ;
            if( setPrint ) gotoXY(20,23); printf( "| [alp%.5f][beta%.2f]\n", alpha , beta ) ;
            if( setPrint ) gotoXY(20,24); printf( "| Until [%7d] Rounds\n", i+1 ) ;
            //if( setPrint ) gotoXY(20,24); printf( "| Current Score: %7d\n", myGame.getScore() ) ;
            //if( setPrint ) gotoXY(20,26); printf( "| Current M-Tile:%7d\n", cur_max_t ) ;
            if( setPrint ) gotoXY(20,25); printf( "| Avg Score: %11.4f\n", avg_score ) ;
            if( setPrint ) gotoXY(20,26); printf( "| Improve:   %11.7f\n", ( avg_score - last_avg_s ) / 200 ) ;
            if( setPrint ) gotoXY(20,27); printf( "| Avg Winrate: %8.4f%%\n", avg_winrate * 100 ) ;
            if( setPrint ) gotoXY(20,28); printf( "| Max Tile:      %7d\n", max_tile ) ;
            if( setPrint ) gotoXY(20,29); printf( "| Max Score:     %7d\n", myGame.getMaxScore() ) ;
            //if( setPrint ) gotoXY(20,30); printf( "| Map Size:        %7d\n", boards.size() ) ;
            if( setPrint ) gotoXY(20,30); printf( "| Max Tile Distribution:\n" ) ;
            for( int k = 4 ; k < 18 ; k++ )
                printf( "[%6d]:%6d | %.4f\n", (int)pow(2,k), tile_cnt[k] , (double)tile_cnt[k]/(i+1) ) ;
            last_avg_s = avg_score ;
            #if 0
            unsigned long long preb = myGame.update_id ;
            for( int j = 0 ; j < 16 ; j++ )
            {
                printf("%2d%c" , preb%16, " \n"[j%4==3] ) ;
                preb /= 16 ;
            }
            printf( "R %d %d %d %d\n"
                  , myGame.getNextStatus(0,4,0)
                  , myGame.getNextStatus(1,4,0)
                  , myGame.getNextStatus(2,4,0)
                  , myGame.getNextStatus(3,4,0) ) ;
            printf( "HASH: %20llu\n" , myGame.getBoardValue(4) ) ;
            for( int j = 0 ; j < FEATURESIZE ; j++ )
            {
                printf( "%2d ||" , j ) ;
                for( int k = 0 ; k < fea[j].size() ; k++ )
                    printf( "%2d " , fea[j][k] ) ;
                int nextid = myGame.pre_sm_board[j] ;
                for( int k = 0 , L = nextid ; k < 4 ; k++ )
                    printf( "%c%2d" , " |"[k==0] , L % 18 ) , L /= 18 ;
                printf( " |%7d %8.4f / %7d // %8.4f\n"
                        , myGame.pre_sm_board[j]
                        , small_boards[ j ][ nextid ].sum_score
                        , small_boards[ j ][ nextid ].access
                        , small_boards[ j ][ nextid ].sum_score / small_boards[ j ][ nextid ].access ) ;
            }
            #endif
            //stop();
        }
        if( !setPrint )
        {
            fprintf( fout[0], "%.4f\n" , avg_score ) ;
            fprintf( fout[1], "%.4f\n" , avg_winrate ) ;
            fprintf( fout[3],   "%d\n" , myGame.getScore() ) ;
            fprintf( fout[4],   "%d\n" , cur_max_t ) ;
        }
        if(i < TrainTs - 1) myGame.reset();
    }
    if( !setPrint )
    {
        for( int i = 0 , j = 1 ; i < 16 ; i++ , j *= 2 )
            fprintf( fout[2], "%.4f %d %d\n", (double)tile_cnt[i] / TrainTs , j == 1 ? 0 : j , tile_cnt[i]  ) ;
        for( int i = 0 ; i < 5 ; i++ )
            fclose( fout[i] ) ;
    }
    /* Calculate average scroe and winrate */
    if( !setPrint )
    {
        FILE *Fout , *Ftrain ;
        sprintf( cname, "%s[%02d][Total][%c][M%dC%dU%d][alpha%3.0f][beta%.0f].dat"
                , testf[setTest], ExpTs, "AC"[cumulative], method, choose_m, update_m, alpha*10000 , beta*1000 ) ;
        Fout = fopen( cname , "w" ) ;
        fprintf( Fout , "[%c] - [M%dC%dU%d] - [alpha %.5f] - [beta%.5f]\n", "AC"[cumulative], method, choose_m, update_m, alpha , beta ) ;
        fprintf( Fout , "Round: [%02d]  1 ~ %7d\n", ExpTs, TrainTs ) ;
        fprintf( Fout , "Avg Score:     %14.10f\n", avg_score ) ;
        fprintf( Fout , "Avg Winrate:   %14.10f\n", avg_winrate ) ;
        fprintf( Fout , "Max Tile:        %7d\n", max_tile ) ;
        fprintf( Fout , "Max Score:       %7d\n", myGame.getMaxScore() ) ;
        fprintf( Fout , "Max Tile Distribution:\n" ) ;
        for( int i = 2 ; i < 18 ; i++ )
            fprintf( Fout, "[%6d]:%6d | %.4f\n", (int)pow(2,i), tile_cnt[i] , (double)tile_cnt[i] / TrainTs ) ;
        fclose( Fout ) ;
        if( setTest == 0 )
        {
            sprintf( cname, "%s[%02d][Feature][%c][M%dC%dU%d][alpha%3.0f][beta%.0f].dat"
                    , testf[setTest], ExpTs, "AC"[cumulative], method, choose_m, update_m, alpha*10000 , beta*1000 ) ;
            Ftrain = fopen( cname, "w" ) ;
            for( int i = 0 ; i < FEATURESIZE ; i++ )
                for( int j = 0 ; j < FEAKINDS ; j++ )
                    fprintf( Ftrain, "%.13f %d\n", small_boards[i][j].sum_score, small_boards[i][j].access ) ;
            fclose( Ftrain ) ;
        }
    }
}
void ShowExplain( )
{
    printf( "Format: ./agent_2048 ExpID TrainTimes method choose_m update_m cumulative alpha  beta setPrint setTest\n" ) ;
    printf( "Ex:   $ ./agent_2048     1    500000       1        4        1          0 0.0100  1.0        0       0\n\n" ) ;
    printf( "ExpID: customized recorded file's ID\n" ) ;
    printf( "TrainTime: Number of games for the 2048 gamer during learning process\n" ) ;
    printf( "choose_m: 0~9; Original: 0, UCB0: 1, UCB1and2: 2,8, TD1and2: 4,6, DefineFunc1and2: 5,7, DefFunc+UCB1and2: 3,9\n" ) ;
    printf( "update_m: 15~18; TD:15,16, TD+UCB diff: 17,18\n" ) ;
    printf( "cumulative mode: 0 = not cumulative, 1 = cumulative\n" ) ;
    printf( "alpha: TD learning rate: 0.0000 ~ 0.0100\n" ) ;
    printf( "beta: UCB ratio: 0.0 ~ 1.0\n" ) ;
    printf( "setPrint: 0 = not show the boards, 1 = show the boards\n" ) ;
    printf( "setTest: 0 = Training 2048 gamer, 1 = Testing gamer\n" ) ;
}
int main(int argc, char* argv[])
{
    /* Parameters setting */
    if( argc != 11 )
    {
        printf( "Parameters Error!\n" ) ;
        ShowExplain( ) ;
        return 0 ;
    }

    ExpTs      = atoi(argv[1]) ;            // - RunTime
    TrainTs    = atoi(argv[2]) ;            // - TID
    setPrint   = atoi(argv[9]) ;            // - Print board or not
    setTest    = atoi(argv[10]) ;           // - Train or Test
    method     = atoi(argv[3]) ;            // v 0 - 1 0=Random, 1=UCB, 2=Select

    choose_m   = atoi(argv[4]) ;            // v 1 - 5
    update_m   = atoi(argv[5]) ;            // v 1 - 13
    small_big  = 0             ;            // x Small=0, Big=1
    cumulative = atoi(argv[6]) ;            // v 0:=, 1:+=

    alpha      = atof(argv[7]) ;            // - TD-Learning alpha
    beta       = atof(argv[8]) ;            // - UCB-c
    srand( time( NULL ) ) ;
    PlayNRounds( ) ;
    return 0;
}
