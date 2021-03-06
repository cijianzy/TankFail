#include <websocketpp/config/asio_client.hpp>

#include <websocketpp/client.hpp>

#include <iostream>
#include <chrono>
#include <string>
#include <time.h>
#include <nlohmann/json.hpp>
#include <assert.h>
#include <set>
#include <math.h>
#include <fstream>
#include <time.h>
#include <unistd.h>
#include <chrono>
using namespace std;
using  ns = chrono::nanoseconds;
using get_time = chrono::steady_clock ;
//#define TEST
//#define SIMULATE

#define COMPETE


using namespace std;

#ifdef SIMULATE
    vector<string> tMessages;
    vector<string> tCommands;
    vector<string> tCoordinates;
#endif

using json = nlohmann::json;

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

get_time::time_point  bClock, eClock;
vector<get_time::time_point> timeIntervals;
inline void calculateClock() {
    eClock = get_time::now();
    auto diff = eClock - bClock;
    cout <<"Get Message intervel : " << chrono::duration_cast<ns>(diff).count() * 1.0 / (1000000000) << "s" << endl;
}

inline void calculateClock(get_time::time_point begin, get_time::time_point end, string msg) {
    auto diff = end - begin;
    cout << msg + ": " << chrono::duration_cast<ns>(diff).count() * 1.0 / (1000000000) << "s" << endl;
}

int preSolveBlocks[10][2] =  {{3, 6}, {0, 10}, {21, 9}, {11, 12},{17, 11}, {11, 2}, {16, 9},{14, 5}, {15, 2}, {15, 11}};

class perftest;

client m_endpoint;
websocketpp::connection_hdl ghdl;
// pull out the type of messages sent by our config
typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;
typedef client::connection_ptr connection_ptr;

int turn = 0;
int mapMultiple = 20; // 格子地图放大倍数
double trueMapWidth = 25; // 真实地图宽度
double trueMapHeight = 15; // 真实地图长度


// 2,200  和 5，400 都还不错
int searchStepLength = 3; // 探索步长
int maxExploreNumber = 400;

double bulletDWideMultiple = 1.4; // 子弹的宽度，相应扩宽一点
float tDDistance = 6; // 坦克的伤害半径
float bDDistance = 5; // 子弹的伤害远度
float maxRebornCd = 20; // 最大复活时间

int TDTYPE = -2;
int BLDTYPE = -1;
int BUDTYPE = -3;

bool isSimulateDie = false; // 是否是模拟死亡场

vector<vector<int>> radius1Circle;
vector<vector<int>> radius2Circle;
vector<vector<int>> radiusTDDistanceCircle;
vector<vector<int>> searchOrder;
vector<vector<int>> bDArea;

//perftest endpoint;
#ifdef COMPETE
std::set<std::string> myTankSet = {"ai:41"};
#else
std::set<std::string> myTankSet = {"ai:58"};
#endif

int mv4Step[4][2] = {{0,1},{1,0},{0,-1},{-1,0}};

inline tuple<float,float> mapCoordinateToTrue(int x, int y) {
    return make_pair((x * 1.0 / mapMultiple), (y * 1.0 / mapMultiple));
}

inline tuple<int,int> toMapCoordinate(double x, double y) {
    return make_pair(round(x * mapMultiple), round(y * mapMultiple));
}

double safeAcos(double x)
{
    if (x < -1.0) x = -1.0 ;
    else if (x > 1.0) x = 1.0 ;
    return acos (x) ;
}

inline float getDistance(float x1, float y1, float x2, float y2);
class Tank {
public:
    string id;
    float direction, // 坦克的方向，在 0-2π 之间，0 为水平向右，顺时针
            fireCd, // 开火冷却时间，即还有多久可再次开火，0为可立即开火
            x, // 坦克的当前位置 X
            y, // 坦克的当前位置 Y
            rebornCd = 0, // 复活冷却时间，即被击毁后还有多久可复活 null 表示在场上战斗的状态
            shieldCd = 0; // 护盾剩余时间
    bool fire;
    float radius = 1;
    int score; // 得分
    float distance;
    vector<vector<double>> historyCoordinate;
    vector<double> historyDirection;
    bool canAttackDirectly = false;

    inline bool isDied() {
        return (this->rebornCd > 0.00001);
    }

    inline bool canFire() {
        return (this->fireCd < 0.08);
    }

    inline void analyze() {

        if (isDied()) {
            historyCoordinate.clear();
            canAttackDirectly = false;
            return;
        }

        int size  = historyCoordinate.size();
        if (size >= 5) {
            double moveDistance = getDistance(historyCoordinate[size-1][0],historyCoordinate[size-1][1], historyCoordinate[size-5][0], historyCoordinate[size-5][1]);
        // 判断是不是卡死坦克或者是不是太近
            // TODO_CIJIAN  2018 ,Apr20 , Fri, 11:31
            if (moveDistance < 0.3 || distance < 3) {
                canAttackDirectly = true;
            } else {
                canAttackDirectly = false;
            }
        }

    }
};

map<string, Tank*> allTank;

bool compareTank(Tank *t1, Tank* t2)
{
    return (t1->distance < t2->distance);
}

class Bullet {
public:
    float x, y, direction, speed;
    string from;
};

Tank *myTank;
class Block {
public:
    float x,y,radius = 1;
};

class Map {
public:
    int height;
    int width;
    int mapHeight;
    int mapWidth;
    vector<Tank *> tanks;
    vector<Block *> blocks;
    vector<Bullet *> bullets;

    int **oMap = NULL;
    int **dMap = NULL;
    int **bDMap= NULL;
    int **tDMap= NULL;
};

Map *tankMap;

inline void messageSend(string msg) {

#ifdef SIMULATE
    if (isSimulateDie) {
        return ;
    }

    tCommands.push_back(to_string(myTank->x) +  + " , " + to_string(myTank->y) +  msg);
#endif

    m_endpoint.send(ghdl, msg, websocketpp::frame::opcode::text);
}

inline void fire() {
    messageSend("{\"commandType\": \"fire\"}");
}

inline void direction(float angle) {
    messageSend("{\"commandType\": \"direction\", \"angle\": " + to_string(angle/(2 * M_PI) * 360) + "}");
}

inline void stay() {
    messageSend("{\"commandType\": \"direction\", \"angle\": -1}");
}

inline void mapClear(int **neadClearMap) {

    for(int i = 0; i < tankMap->mapWidth; ++i) {
        memset(neadClearMap[i], 0, sizeof(int) * tankMap->mapHeight);
    }
}

inline float getDistance(float x1, float y1, float x2, float y2) {
    return sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
}

inline bool isInMapRange(double x, double y) {
    if (x < 0 || y < 0) {
        return false;
    }

    if (x >= tankMap->mapWidth || y >= tankMap->mapHeight) {
        return false;
    }

    return true;
}

inline bool isInTrueMapRange(double x, double y) {
    if (x < 0 || y < 0) {
        return false;
    }

    if (x > tankMap->width+0.001 || y > tankMap->height + 0.001) {
        return false;
    }

    return true;
}


inline double angleIn2PI(double angle) {
    while (angle > 2 * M_PI) {
        angle -= 2 * M_PI;
    }

    while(angle < 0) {
        angle += M_PI * 2;
    }

    return angle;
}

inline bool isXYSafeForBullet(double x, double y) {

    tuple<int, int> tt= toMapCoordinate(x,y);

    int nx = round(get<0>(tt));
    int ny = round(get<1>(tt));

    if ((isInMapRange(nx,ny) && tankMap->bDMap[nx][ny] != 0) || !isInMapRange(nx,ny)) {
        return false;
    } else {
        return true;
    }
}

inline bool isXYSafeForTank(double x, double y) {

    tuple<int, int> tt= toMapCoordinate(x,y);

    int nx = round(get<0>(tt));
    int ny = round(get<1>(tt));

    if ((isInMapRange(nx,ny) && tankMap->tDMap[nx][ny] != 0) || !isInMapRange(nx,ny)) {
        return false;
    } else {
        return true;
    }
}

inline bool isNowSafeForBullet() {
    return isXYSafeForBullet(myTank->x, myTank->y);
}

inline bool isNowSafeForTank() {
    return isXYSafeForTank(myTank->x, myTank->y);
}

inline bool isDied(Tank t) {
    return (t.rebornCd > 0.00001);
}

inline void goTo(double x, double y) {
    double angle = angleIn2PI(atan2(y - myTank->y, x - myTank->x));
    direction(angle);
}

inline void testGoto() {
    double angle = 0;
    for (int i = 0 ; i < 20 ; i ++) {
        direction(angle);
        angle += M_PI/20;
        // usleep 微秒  1e6
        usleep(16000);
    }
}

class AttackObject {
public:
   double x, y;
   double delay = 0;
   double collideX;
   double collideY;
   double flyTime;
   Tank *target;
   bool canAttack;
   double angle;
};

void getMessage(string msg);

inline void xyAttackAngle(AttackObject *ao) {

    if (ao->target->canAttackDirectly) {
        ao->collideX = ao->target->x;
        ao->collideY = ao->target->y;
        ao->flyTime = getDistance(ao->x, ao->y, ao->collideX, ao->collideY) / 10;
        ao->angle = atan2(ao->collideY - ao->y,  ao->collideX - ao->x);
        return ;
    }

    // 追击公式  91 * x * x = y * y - 6*x*y*cos(θ)
    double x = ao->x;
    double y = ao->y;
    double x1 = ao->target->x;
    double y1 = ao->target->y;
    double direction = ao->target->direction;

    double angle = angleIn2PI(atan2(y - y1, x - x1)) - direction; // 相差角度
    angle = angleIn2PI(angle);
    if (angle > M_PI) {
        angle = M_2_PI - angle;
    }
    double distance = getDistance(x1, y1, x, y);
    double a = -91;
    double cosAngle = cos(angle);
    double b = -6 * distance * cosAngle;
    double c = distance * distance;
    int temp = b * b - 4 * a * c;
    double xAns;
    if (temp < 0){
    } else if (temp == 0) {
        xAns = -1 * (b * 1.0) / (2 * a);
    } else {
        double x1 = (-1 * b + sqrt(temp))/(2 * a);
        double x2  = (-1 * b - sqrt(temp))/(2 * a);
        if(x1>0) {
            xAns = x1;
        } else {
            xAns = x2;
        }
    }

    a = 10 * xAns;
    b = distance;
    c = 3 * xAns;
    double angleAns = 0;

        angleAns = safeAcos((b * b + a * a - c * c)/ (2 * a* b));

        double tempAngle = atan2(y1 - y, x1 - x);
        if (angleIn2PI(direction - tempAngle) > 3.14) {
            angleAns = atan2(y1 - y, x1 - x) - angleAns;
        } else {
            angleAns = atan2(y1 - y, x1 - x) + angleAns;
        }

    angleAns = angleIn2PI(angleAns);

    // 根据极坐标算出碰撞区域
    ao->collideX = a * cos(angleAns) + ao->x;
    ao->collideY = a * sin(angleAns) + ao->y;
    ao->flyTime = xAns;

    ao->angle = angleAns;
}

inline bool hasIntersection(double x1, double y1, double x2, double y2, double x3, double y3, double r) {

    // 除0
    double A = (y2 - y1) / (x2 -x1);
    double B = -1;
    double C = y1 - A * x1;

    double lineDistance = abs((A*x3 + B * y3 + C) / sqrt(A*A+B*B));

    double pointOneDistance = getDistance(x1,y1,x3,y3);

    double pointTwoDistance = getDistance(x2,y2,x3,y3);

    // 会打到石头 设置一下会不会好点?
    if (lineDistance > r + 0.01) {
        return false;
    }

    // 两个点在圆上 一定有交点
    if (pointOneDistance < r || pointTwoDistance < r) {
        return true;
    }
    // 一个点在圆，一定有交点
//    if (pointOneDistance == r || pointTwoDistance == r) {
//        return true;
//    }
//
//    if (min(pointOneDistance, pointTwoDistance) < r &&  max(pointOneDistance, pointTwoDistance) > r) {
//        return true;
//    }

    double pointDistanceSelf = getDistance(x1, y1, x2, y2);
    double beta = safeAcos((pointOneDistance * pointOneDistance +
                        pointTwoDistance * pointTwoDistance -
                        pointDistanceSelf * pointDistanceSelf) / (2 * pointOneDistance * pointTwoDistance));

    if (pointOneDistance > r &&
        pointTwoDistance > r &&
        beta > M_PI / 2) {
        return true;
    }

    return false;

}

inline bool canAttackTo(AttackObject *ao) {

    // 考虑delay的情况
//    if (ao->delay > 0.00001) {
//        double br = ao->delay * 10;
//        ao->x = ao->x + br * ;
//        ao->y = ao->y + ao->delay *
//                double tr = ao->delay * 10;
//    }

    xyAttackAngle(ao);

    ao->canAttack = true;

    // 是否有护盾 + tick 时间
    if (ao->delay + ao->flyTime - 0.11 < ao->target->shieldCd ) {
        ao->canAttack = false;
        return ao->canAttack;
    }

    // 命中点是否在区域内
    if (!isInTrueMapRange(ao->collideX, ao->collideY)) {
        ao->canAttack = false;
        return ao->canAttack;
    }

    // TODO_CIJIAN  2018 ,Apr17 , Tue, 12:37
    // 需要debug
    for (auto it = tankMap->blocks.begin() ;it != tankMap->blocks.end(); ++it) {
        if (hasIntersection(ao->x, ao->y, ao->collideX, ao->collideY, (*it)->x, (*it)->y, (*it)->radius)) {
//            cout << "can not attack" <<ao->x <<", "<< ao->y << ", " << ao->collideX << ", "<< ao->collideY << ", " << (*it)->x << ", " << (*it)->y << ", " << (*it)->radius << endl;
            ao->canAttack = false;
            break;
        }
    }

    return ao->canAttack;
}

inline AttackObject* canXYAttack(double x, double y) {

    if (myTank->isDied()) {
        return nullptr;
    }

    for (auto it = tankMap->tanks.begin(); it != tankMap->tanks.end(); ++it ) {
        if ((*it)->rebornCd < 0.00001) {
            AttackObject *ao = new AttackObject();
            ao->x = x;
            ao->y = y;
            ao->target = (*it);
            if (canAttackTo(ao)) {
                return ao;
            }
            break;
        }
    }

    return nullptr;
}

vector<vector<int>> bfsSV; // 安全区域的广度优先
vector<vector<int>> bfsAV; // 可攻击区域的广度优先
struct IntPosition {
    int x, y;
};

inline void clearBfsSV() {
    for (auto it = bfsSV.begin(); it != bfsSV.end() ; ++ it) {
        (*it).clear();
    }
    bfsSV.clear();
}

inline void clearBfsAV() {
    for (auto it = bfsAV.begin(); it != bfsAV.end() ; ++ it) {
        (*it).clear();
    }
    bfsAV.clear();
}

int **vMap = NULL;

inline tuple<double, double> runOutCorner() {
    return make_pair(tankMap->width/2, tankMap->height/2);
};

inline tuple<double, double> runForWin() {
    int bBfsAV  =  0;
    int eBfsAV = 0;
    clearBfsAV();
    mapClear(vMap);

    vector<int> tv;

    int nx,ny;
    tuple<int,int> co = toMapCoordinate(myTank->x, myTank->y);
    nx = get<0>(co);
    ny = get<1>(co);
    tv.push_back(nx);
    tv.push_back(ny);

    if (isInMapRange(nx,ny)) {
        vMap[nx][ny] = 1;
    }

    bfsAV.push_back(tv);

    while(bBfsAV <= eBfsAV) {
        int x = bfsAV[bBfsAV][0];
        int y = bfsAV[bBfsAV][1];

        for (int i = 0 ;i < 4 ; ++ i) {

            int nx = x + mv4Step[i][0] * searchStepLength;
            int ny = y + mv4Step[i][1] * searchStepLength;

            if (isInMapRange(nx,ny) && vMap[nx][ny] == 0) {

                tuple<float, float> trueCoordinate = mapCoordinateToTrue(nx,ny);
                AttackObject *ao = canXYAttack(get<0>(trueCoordinate), get<1>(trueCoordinate));

                if (ao != nullptr && ao->canAttack == true){
                    return make_pair(get<0>(trueCoordinate), get<1>(trueCoordinate));
                }

                if (bBfsAV > maxExploreNumber) {
#ifdef SIMULATE
#endif
                    return make_pair(16, 11);
                }

                vector<int> tv1;
                tv1.push_back(nx);
                tv1.push_back(ny);
                vMap[nx][ny] = 1;
                bfsAV.push_back(tv1);
                ++ eBfsAV;
            }
        }
        ++ bBfsAV;
    }

    return make_pair(16, 11);
}

inline tuple<double, double> runForLife(bool isRunOutTankAttack) {
    int bbfsSV  =  0;
    int ebfsSV = 0;

    clearBfsSV();
    mapClear(vMap);

    vector<int> tv;

    int nx,ny;
    tuple<int,int> co = toMapCoordinate(myTank->x, myTank->y);
    nx = get<0>(co);
    ny = get<1>(co);
    tv.push_back(nx);
    tv.push_back(ny);

    if (isInMapRange(nx,ny)) {
        vMap[nx][ny] = 1;
    }

    bfsSV.push_back(tv);

    tuple<double , double> ansCo = make_pair(myTank->x, myTank->y);

    double nearCoordinatecount = 0;
    double minDistance = 25 * 10;

    while(bbfsSV <= ebfsSV) {
        int x = bfsSV[bbfsSV][0];
        int y = bfsSV[bbfsSV][1];

        for (int i = 0 ;i < 4 ; ++ i) {
            int nx = x + mv4Step[i][0] * searchStepLength;
            int ny = y + mv4Step[i][1] * searchStepLength;

            if (isInMapRange(nx,ny) && vMap[nx][ny] == 0) {
                // 如果发现安全区域
                //
                //
                if (tankMap->dMap[nx][ny] == 0 && tankMap->bDMap[nx][ny] == 0 && (!isRunOutTankAttack || tankMap->tDMap[nx][ny] == 0)) {
                    // 反的安全区域
                    nearCoordinatecount ++;

                    tuple<float, float> tureCoordinate = mapCoordinateToTrue(nx,ny);
                    double distance = getDistance(myTank->x, myTank->y, get<0>(tureCoordinate), get<1>(tureCoordinate));
                    if (distance < minDistance) {
                        minDistance =  distance;
                        ansCo = make_pair(get<0>(tureCoordinate), get<1>(tureCoordinate));
                    }

                    if (nearCoordinatecount == maxExploreNumber) {
                        return ansCo;
                    }
                }

                // 如果是逃脱坦克攻击的话，子弹攻击区域一定不能走
                if (!isRunOutTankAttack || tankMap->bDMap[nx][ny] == 0 ) {
                    vector<int> tv1;
                    tv1.push_back(nx);
                    tv1.push_back(ny);
                    vMap[nx][ny] = 1;
                    bfsSV.push_back(tv1);
                    ++ebfsSV;
                }
            }
        }
        ++ bbfsSV;
    }

    return make_pair(tankMap->width/2, tankMap->height/2);
}

inline void theLastBattle() {

}

inline void constructTDMap() {

//#ifdef TEST
//    ofstream myfile;
//    myfile.open("tanks.txt",ios::trunc);
//#endif
    mapClear(tankMap->tDMap);
    for(auto it = tankMap->tanks.begin(); it != tankMap->tanks.end(); ++it) {
        tuple<int, int> mapCoordinate = toMapCoordinate((*it)->x,(*it)->y);

        // 复活时间大的
        if ((*it)->rebornCd > 0.00001) {
            continue;
        }

        if (getDistance(myTank->x, myTank->y, (*it)->x, (*it)->y) > tDDistance * 2) {
            continue;
        }
        int nx,ny;
        for( auto iter = radiusTDDistanceCircle.begin(); iter != radiusTDDistanceCircle.end(); ++iter) {
            nx = (*iter)[0] + get<0>(mapCoordinate);
            ny = (*iter)[1] + get<1>(mapCoordinate);

            if (isInMapRange(nx,ny)) {
                tankMap->tDMap[nx][ny] = TDTYPE;
            }
#ifdef TEST
            //myfile << tx << "," << ty << endl;
#endif
        }
    }
//
//#ifdef TEST
//    myfile.close();
//#endif

}

inline void constructBDMap() {

#ifdef SIMULATE
    ofstream myfile;
    myfile.open("bulltes.txt");
#endif

    mapClear(tankMap->bDMap);

    for (auto it = tankMap->bullets.begin(); it != tankMap->bullets.end(); ++it) {

        // 如果距离大于考虑距离的两倍，目前不用画到图上来
        if (getDistance(myTank->x, myTank->y, (*it)->x, (*it)->y) > bDDistance * 2) {
            continue;
        }

        int nx = 0;
        int ny = 0;
        int x = 0;
        int y = 0;
        double alpha;
        double distnace;
        double r;
        int vsize = bDArea.size();
        for (int i = 0 ;i < vsize; ++ i) {
            x = bDArea[i][0];
            y = bDArea[i][1];
            // 真实坐标需要换算一下
            tuple<int, int> mapCoordinate = toMapCoordinate((*it)->x,(*it)->y);

            // 旋转坐标公式
            nx = round(x * cos((*it)->direction) - y * sin((*it)->direction) + get<0>(mapCoordinate));
            ny = round(x * sin((*it)->direction) + y * cos((*it)->direction)  + get<1>(mapCoordinate));

            if (isInMapRange(nx,ny)) {
                tankMap->bDMap[nx][ny] = BUDTYPE;
            }

#ifdef SIMULATE
            if (isSimulateDie) {
                myfile << nx << "," << ny << endl;
            }
#endif
        }
    }

    int nx;
    int ny;
    // 旋转后中间会有很多小空心，填一下

    for(int i = 0; i < tankMap->mapWidth; ++i) {
        for(int j = 0; j < tankMap->mapHeight; ++j) {
            if (tankMap->bDMap[i][j] == BUDTYPE) {
                continue;
            }

            int count = 0;

            for (int lr = 0 ; lr < 4 ; ++lr) {
                nx = i + mv4Step[lr][0];
                ny = j + mv4Step[lr][1];
                if (!isInMapRange(nx,ny) || tankMap->bDMap[nx][ny]) {
                    count ++;
                } else {
                    break;
                }
            }

            if (count == 4) {
                tankMap->bDMap[i][j] = BUDTYPE;
#ifdef SIMULATE
                if (isSimulateDie) {
                    myfile << nx << "," << ny << endl;
                }
#endif
            }
        }
    }
#ifdef SIMULATE
    myfile.close();
//    if (tankMap->bullets.size() >= 2 && turn >= 100 ) {
//        assert(0);
//    }
#endif
}

inline void solve() {

    if (isDied(*myTank)) {

#ifdef SIMULATE
        cout << "posision" << myTank->x << myTank->y << endl;
        if (!isSimulateDie) {

            ofstream myfile;
            myfile.open("commands.txt");
            if (turn >= 200 && myTank->rebornCd > maxRebornCd - 0.1) {
                isSimulateDie = true;
                getMessage(tMessages[tMessages.size() - 3]);
                for (int  i = 0; i <  tCommands.size() ;i ++ ) {
                    myfile << tCommands[i] << endl;
                }
                myfile.close();

                myfile.open("messages.txt");
                for (int  i = 0; i <  tMessages.size() ;i ++ ) {
                    myfile << tMessages[i] << endl;
                }
                myfile.close();
                isSimulateDie = false;
//                assert(0);
            }
        }
#endif
    }

    auto tBClock= get_time::now();

    AttackObject *ao = canXYAttack(myTank->x, myTank->y);
    if (!myTank->isDied()) {
        if (ao != nullptr && ao->canAttack == true) {
            direction(ao->angle);
            fire();
            stay();
        }
    }

    auto tEClock = get_time::now();
    calculateClock(tBClock, tEClock, "canXYAttack");

    bool isScape = false;
    bool isAttack = false;
    // bullet 和 tank 分开讨论, 优先子弹
    // 先构建子弹伤害


    tBClock= get_time::now();

    constructBDMap();

    tEClock = get_time::now();

    calculateClock(tBClock, tEClock, "constructBDMap");

    if (!myTank->isDied() && !isNowSafeForBullet()) {
        tBClock= get_time::now();
        tuple<double, double> nearestSafeCoordinate = runForLife(false);
        tEClock = get_time::now();
        calculateClock(tBClock, tEClock, "runForLife Bullets");
        goTo(get<0>(nearestSafeCoordinate), get<1>(nearestSafeCoordinate));

        cout << "scape to" << get<0>(nearestSafeCoordinate) << ", "<< get<1>(nearestSafeCoordinate)<< endl;

        isScape = true;
    }

    if (!isScape) {

        tBClock= get_time::now();

        constructTDMap();

        tEClock = get_time::now();

        calculateClock(tBClock, tEClock, "constructTDMap");

        tBClock= get_time::now();

        if (!myTank->isDied() && !isNowSafeForTank()) {
            tuple<double, double> nearestSafeCoordinate = runForLife(true);
            goTo(get<0>(nearestSafeCoordinate), get<1>(nearestSafeCoordinate));
            isScape = true;
        }

        tEClock = get_time::now();
        calculateClock(tBClock, tEClock, "runForLife Tanks");

        if (!myTank->isDied() && !isScape) {

            tBClock= get_time::now();

            if ((ao == nullptr || ao->canAttack == false)) {
                tuple<double, double> attackPosition = runForWin();
                goTo(get<0>(attackPosition), get<1>(attackPosition));
                isAttack = true;
            }
            tEClock = get_time::now();

            calculateClock(tBClock, tEClock, "canAttack");
        }
//        else {
//                goTo(16, 10.3);
//            }
//        }
    }
}

inline void constructMapInfo(json msg) {
//   初始化，构建地图信息
    tankMap->tanks.clear();
    tankMap->bullets.clear();
    // construct map
    for (json::iterator it = msg.begin(); it != msg.end(); ++it) {

        if (it.key() == "bullets") {
            for (json::iterator  j= it.value().begin(); j != it.value().end(); ++ j) {
                Bullet *newBullet = new Bullet();
                for (json::iterator i = j.value().begin(); i != j.value().end(); ++i) {
                    if (i.key() == "speed") {
                        newBullet->speed = i.value();
                    } else if (i.key() == "position") {
                        newBullet->x =  i.value()[0];
                        newBullet->y =  i.value()[1];
                    } else if (i.key() == "direction") {
                        newBullet->direction = i.value();
                    } else if (i.key() == "from") {
                        newBullet->from = i.value();
                    }
                }

                if (!myTankSet.count(newBullet->from)) {
                    tankMap->bullets.push_back(newBullet);
                }
            }
        }

        if (it.key() == "tanks") {
            for (json::iterator  j= it.value().begin(); j != it.value().end(); ++ j) {
                string id = j.key();
                Tank *newTank;
                if (allTank.find(id) == allTank.end()) {
                    allTank[id] = new Tank();
                    allTank[id]->id = id;
                    newTank = allTank[id];
                } else {
                    newTank = allTank[id];
                }

                for (json::iterator i = j.value().begin(); i != j.value().end(); ++i) {
                    if (i.key() == "direction") {
                        newTank->direction = i.value();
                    } else if (i.key() == "fire") {
                        newTank->fire = i.value();
                    } else if (i.key() == "score") {
                        newTank->score = i.value();
                    } else if (i.key() == "shieldCd") {
                        if (i.value() == nullptr) {
                            newTank->shieldCd = 0;
                        } else {
                            newTank->shieldCd = i.value();
                        }
                    } else if (i.key() == "fireCd") {
                        if (i.value() == nullptr) {
                            newTank->fireCd = 0;
                        } else {
                            newTank->fireCd = i.value();
                        }
                    } else if (i.key() == "position") {
                        newTank->x =  i.value()[0];
                        newTank->y =  i.value()[1];


                    } else if (i.key() == "rebornCd") {
                        if (i.value() == nullptr) {
                            newTank->rebornCd = 0;
                        } else {
                            newTank->rebornCd = i.value();
                        }
                    }
                }

                if (!myTankSet.count(newTank->id)) {
                    tankMap->tanks.push_back(newTank);
                } else {
                    myTank = newTank;
                }
            }
        }

        for (auto it = tankMap->tanks.begin() ;it != tankMap->tanks.end(); ++it) {
            (*it)->distance = getDistance(myTank->x, myTank->y, (*it)->x, (*it)->y);

            vector<double> tv1;
            tv1.push_back((*it)->x);
            tv1.push_back((*it)->y);
            (*it)->historyCoordinate.push_back(tv1);
            (*it)->analyze();
        }

        // 对坦克按照距离排序
        sort(tankMap->tanks.begin(), tankMap->tanks.end(), compareTank);
    }

}

void initMap(json msg) {

    for (json::iterator it = msg.begin(); it != msg.end(); ++it) {

        if (it.key() == "height") {
            tankMap->height = it.value();
        } else if (it.key() == "width"){
            tankMap->width = it.value();
        }

        if (it.key() == "blocks") {
            for (json::iterator  j= it.value().begin(); j != it.value().end(); ++ j) {

                Block *newBlock = new Block();
                for (json::iterator i = j.value().begin(); i != j.value().end(); ++i) {
                    if (i.key() == "radius") {
                        newBlock->radius = i.value();
                    } else if (i.key() == "position") {
                        newBlock->x =  i.value()[0];
                        newBlock->y =  i.value()[1];
                    }

                }
                tankMap->blocks.push_back(newBlock);
            }
        }
    }

    constructMapInfo(msg);

}

inline void getMessage(string msg) {
#ifdef SIMULATE
    calculateClock();
    bClock = get_time::now();
    timeIntervals.push_back(get_time::now());

#endif

#ifdef SIMULATE
    tMessages.push_back(msg);
#endif

    auto j3 = json::parse(msg);

    if (turn == 0) {
        for (json::iterator it = j3.begin(); it != j3.end(); ++it) {
            if (it.key() == "data") {
                string temp = it.value();
                initMap(json::parse(temp));
            }
        }
    } else {
        for (json::iterator it = j3.begin(); it != j3.end(); ++it) {
            if (it.key() == "data") {
                string temp = it.value();
                constructMapInfo(json::parse(temp));

            }
        }
    }
    solve();

    turn ++;
#ifdef SIMULATE
    cout << turn << endl;
    if (turn == 2) {
//        system("killall python");
//        assert(0);
    }
#endif
}

class perftest {
public:
    typedef perftest type;
    typedef std::chrono::duration<int,std::micro> dur_type;

    perftest () {
        m_endpoint.set_access_channels(websocketpp::log::alevel::all);
        m_endpoint.set_error_channels(websocketpp::log::elevel::all);

        // Initialize ASIO
        m_endpoint.init_asio();

        // Register our handlers
        m_endpoint.set_socket_init_handler(bind(&type::on_socket_init,this,::_1));
        m_endpoint.set_tls_init_handler(bind(&type::on_tls_init,this,::_1));
        m_endpoint.set_message_handler(bind(&type::on_message,this,::_1,::_2));
        m_endpoint.set_open_handler(bind(&type::on_open,this,::_1));
        m_endpoint.set_close_handler(bind(&type::on_close,this,::_1));
    }

    void convertJSONToInformation(string msg) {

    }
    void start(std::string uri) {
        websocketpp::lib::error_code ec;
        client::connection_ptr con = m_endpoint.get_connection(uri, ec);

        if (ec) {
            m_endpoint.get_alog().write(websocketpp::log::alevel::app,ec.message());
        }
        //con->set_proxy("http://humupdates.uchicago.edu:8443");
        m_endpoint.connect(con);
        // Start the ASIO io_service run loop
        m_start = std::chrono::high_resolution_clock::now();
        m_endpoint.run();
    }

    void on_socket_init(websocketpp::connection_hdl hdl) {
        m_socket_init = std::chrono::high_resolution_clock::now();
    }

    context_ptr on_tls_init(websocketpp::connection_hdl hdl) {
        m_tls_init = std::chrono::high_resolution_clock::now();
        context_ptr ctx(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv1));

        try {
            ctx->set_options(boost::asio::ssl::context::default_workarounds |
                             boost::asio::ssl::context::no_sslv2 |
                             boost::asio::ssl::context::single_dh_use);
        } catch (std::exception& e) {
            std::cout << e.what() << std::endl;
        }
        return ctx;
    }

    void on_open(websocketpp::connection_hdl hdl) {
        m_open = std::chrono::high_resolution_clock::now();
#ifdef COMPETE
        m_endpoint.send(hdl, "{\"commandType\": \"aiEnterRoom\", \"roomId\": 67718, \"accessKey\": "
                             "\"96b69e5beaab0ac5dc5b6c8e9739d102\", \"employeeId\": 159806}", websocketpp::frame::opcode::text);
#else
        m_endpoint.send(hdl, "{\"commandType\": \"aiEnterRoom\", \"roomId\": 101550, \"accessKey\": "
                             "\"248c641ededfc6d91bbc31bb2e2056ee\", \"employeeId\": 101550}", websocketpp::frame::opcode::text);
#endif
    }

    void on_message(websocketpp::connection_hdl hdl, message_ptr msg) {
        m_message = std::chrono::high_resolution_clock::now();
        ghdl = hdl;
        getMessage(msg->get_payload());
    }
    void on_close(websocketpp::connection_hdl hdl) {
        m_close = std::chrono::high_resolution_clock::now();

        std::cout << "Socket Init: " << std::chrono::duration_cast<dur_type>(m_socket_init-m_start).count() << std::endl;
        std::cout << "TLS Init: " << std::chrono::duration_cast<dur_type>(m_tls_init-m_start).count() << std::endl;
        std::cout << "Open: " << std::chrono::duration_cast<dur_type>(m_open-m_start).count() << std::endl;
        std::cout << "Message: " << std::chrono::duration_cast<dur_type>(m_message-m_start).count() << std::endl;
        std::cout << "Close: " << std::chrono::duration_cast<dur_type>(m_close-m_start).count() << std::endl;
    }

private:
    std::chrono::high_resolution_clock::time_point m_start;
    std::chrono::high_resolution_clock::time_point m_socket_init;
    std::chrono::high_resolution_clock::time_point m_tls_init;
    std::chrono::high_resolution_clock::time_point m_open;
    std::chrono::high_resolution_clock::time_point m_message;
    std::chrono::high_resolution_clock::time_point m_close;
};
perftest endpoint;

void beginPrepare() {

    tankMap = new Map();
    // 考虑边界问题，为了不溢出
    tankMap->mapHeight = round(trueMapHeight) * mapMultiple;
    tankMap->mapWidth = round(trueMapWidth) * mapMultiple;
    tankMap->oMap = new int*[tankMap->mapWidth + 10];
    tankMap->dMap = new int*[tankMap->mapWidth + 10];
    tankMap->bDMap = new int*[tankMap->mapWidth + 10];
    tankMap->tDMap = new int*[tankMap->mapWidth + 10];
    vMap = new int*[tankMap->mapWidth];
    for(int i = 0; i < tankMap->mapWidth; ++i) {
        tankMap->oMap[i] = new int[tankMap->mapHeight + 10];
        tankMap->dMap[i] = new int[tankMap->mapHeight + 10];
        tankMap->bDMap[i] = new int[tankMap->mapHeight + 10];
        tankMap->tDMap[i] = new int[tankMap->mapHeight + 10];
        vMap[i]= new int[tankMap->mapHeight + 10];
    }

#ifdef TEST
    ofstream myfile;
    myfile.open("blocks.txt");
#endif
    for(auto i = 0; i < sizeof(preSolveBlocks)/(sizeof(int)*2); ++i) {
        for( auto iter = radius1Circle.begin(); iter != radius1Circle.end(); ++iter) {
            int tx = (*iter)[0] + preSolveBlocks[i][0];
            int ty = (*iter)[1] + preSolveBlocks[i][0];

            if (isInMapRange(tx,ty)) {
                tankMap->dMap[tx][ty] = BLDTYPE;
            }
#ifdef TEST
            //myfile << tx << "," << ty << endl;
#endif
        }

    }
#ifdef TEST
    myfile.close();
    system("python3 ../tank_ai/drawer.py");
#endif

    vector<int> tv(2);

    for (int i = -10*mapMultiple; i <= 10*mapMultiple; ++i) {
        for (int j = -10*mapMultiple; j <= 10*mapMultiple; ++ j) {
            tuple<double, double> tmp = mapCoordinateToTrue(i, j);
            double distance = getDistance(get<0>(tmp),get<1>(tmp),0,0);

            if (distance < 0.999) {
                tv[0] = i;
                tv[1] = j;
                radius1Circle.push_back(tv);
            }

            if (distance <= 1.999) {
                tv[0] = i;
                tv[1] = j;
                radius2Circle.push_back(tv);
            }

            if (distance <= tDDistance) {
                tv[0] = i;
                tv[1] = j;

                radiusTDDistanceCircle.push_back(tv);
            }
        }
    }

    // TODO_CIJIAN  2018 ,Apr18 , Wed, 09:45
//    for (int i =0 ;i <= mapMultiple * 25 ;i ++) {
//        double x = i - trueMapWidth/2;
//        for (int j = 1 ;j <= mapMultiple * 15 ;j ++) {
//            double y = j - trueMapHeight/2;
//
//        }
//    }

    for (int i = -bulletDWideMultiple * mapMultiple ; i <= bulletDWideMultiple * mapMultiple ; ++i) {
        int rDistance = mapMultiple * bDDistance;
        for (int j = - mapMultiple ;j <= rDistance; j++) {
            tv[0] = j;
            tv[1] = i;
            bDArea.push_back(tv);
        }
    }
}

void test() {
    string str = "{\"commandType\":\"REFRESH_DATA\",\"data\":\"{\\\"width\\\":25,\\\"height\\\":15,\\\"tanks\\\":{\\\"ai:58\\\":{\\\"name\\\":\\\"流弊\\\",\\\"speed\\\":0,\\\"direction\\\":0,\\\"fireCd\\\":0,\\\"fire\\\":false,\\\"position\\\":[13.925306008084853,10.266874545908701],\\\"score\\\":0,\\\"rebornCd\\\":null,\\\"shieldCd\\\":1}},\\\"bullets\\\":[],\\\"blocks\\\":[{\\\"position\\\":[3,6],\\\"radius\\\":1},{\\\"position\\\":[0,10],\\\"radius\\\":1},{\\\"position\\\":[21,9],\\\"radius\\\":1},{\\\"position\\\":[11,12],\\\"radius\\\":1},{\\\"position\\\":[17,11],\\\"radius\\\":1},{\\\"position\\\":[11,2],\\\"radius\\\":1},{\\\"position\\\":[16,9],\\\"radius\\\":1},{\\\"position\\\":[14,5],\\\"radius\\\":1},{\\\"position\\\":[15,2],\\\"radius\\\":1},{\\\"position\\\":[15,11],\\\"radius\\\":1}]}\"}";
}

int main(int argc, char* argv[]) {
    std::string uri = "wss://tank-match.taobao.com/ai";
#ifdef COMPETE

#else

#ifdef SIMULATE
    system("open -a /Applications/Utilities/Terminal.app /bin/bash ../tank_ai/new_game.sh");
#endif
#endif

    beginPrepare();

    bClock = get_time::now();

    if (argc == 2) {
        uri = argv[1];
    }

    try {
        endpoint.start(uri);
    } catch (const std::exception & e) {
        std::cout << e.what() << std::endl;
    } catch (websocketpp::lib::error_code e) {
        std::cout << e.message() << std::endl;
    } catch (...) {
        std::cout << "other exception" << std::endl;
    }
}