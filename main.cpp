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

//#define TEST
#define SIMULATE

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

clock_t bClock, eClock;

inline void caculateClock() {
    eClock = clock();
    cout <<"Get Message intervel : " <<(double)(eClock - bClock) / CLOCKS_PER_SEC << endl;
}


class perftest;

client m_endpoint;
websocketpp::connection_hdl ghdl;
// pull out the type of messages sent by our config
typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;
typedef client::connection_ptr connection_ptr;

int turn = 0;
int mapMultiple = 20;
float tDDistance = 6; // 坦克的伤害半径
float bDDistance = 5; // 子弹的伤害远度
float maxRebornCd = 20; // 最大复活时间

int TDTYPE = -2;
int BLDTYPE = -1;
int BUDTYPE = -3;

bool isSimulateDie = false; // 是否是模拟死亡场

vector<tuple<int,int>> radius1Circle;
vector<tuple<int,int>> radius2Circle;
vector<tuple<int,int>> radiusTDDistanceCircle;
vector<tuple<int,int>> bDArea;

//perftest endpoint;
std::set<std::string> myTankSet = {"ai:58"};

int mv4Step[4][2] = {{0,1},{1,0},{0,-1},{-1,0}};

inline tuple<float,float> mapCoordinateToTrue(int x, int y) {
    return make_pair((x * 1.0 / mapMultiple), (y * 1.0 / mapMultiple));
}

inline tuple<int,int> toMapCoordinate(double x, double y) {
    return make_pair(round(x * mapMultiple), round(y * mapMultiple));
}

class Tank {
public:
    string id;
    float direction, // 坦克的方向，在 0-2π 之间，0 为水平向右，顺时针
            fireCd, // 开火冷却时间，即还有多久可再次开火，0为可立即开火
            x, // 坦克的当前位置 X
            y, // 坦克的当前位置 Y
            rebornCd = 0, // 复活冷却时间，即被击毁后还有多久可复活 null 表示在场上战斗的状态
            shieldCd; // 护盾剩余时间
    string name;
    bool fire;
    float radius = 1;
    int score; // 得分

};

class Bullet {
public:
    float x, y, direction, speed;
    string from;
};

Tank *myTank;
class Block {
public:
    float x,y,radius;
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

inline double angleIn2PI(double angle) {
    while (angle > 2 * M_PI) {
        angle -= 2 * M_PI;
    }

    while(angle < 0) {
        angle += M_PI * 2;
    }

    return angle;
}

inline bool isXYSafe(double x, double y) {

    tuple<int, int> tt= toMapCoordinate(x,y);

    int nx = round(get<0>(tt));
    int ny = round(get<1>(tt));
    // TODO_CIJIAN  2018 ,Apr16 , Mon, 07:53
    // 需要转换

    if (isInMapRange(nx,ny) && (tankMap->bDMap[nx][ny] != 0 || tankMap->tDMap[nx][ny] != 0)) {
        return false;
    } else {
        return true;
    }
}

inline bool isNowSafe() {
    return isXYSafe(myTank->x, myTank->y);
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
   Tank *target;
   bool canAttack;
   double angle;
};

void getMessage(string msg);

inline double xyAttackAngle(AttackObject *ao) {

    // 追击公式  91 * x * x = y * y - 6*x*y*cos(θ)
    double x = ao->x;
    double y = ao->y;
    double x1 = ao->target->x;
    double  y1 = ao->target->y;
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
    if (abs(a+c-b) < 0.001) {
        angleAns = 0;
    } else {
        angleAns = acos((b * b + a * a - c * c)/ (2 * a* b));
        double tempAngle = atan2(y1 - y, x1 - x);
        if (angleIn2PI(direction - tempAngle) > 3.14) {
            angleAns = atan2(y1 - y, x1 - x) - angleAns;
        } else {
            angleAns = atan2(y1 - y, x1 - x) + angleAns;
        }
    }
    angleAns = angleIn2PI(angleAns);

    ao->angle = angleAns;
    return angle;
}

inline void attack(Tank *tank) {
    AttackObject *ao = new AttackObject();
    ao->x = myTank->x;
    ao->y = myTank->y;
    ao->target = tank;
    double angle = xyAttackAngle(ao);

#ifdef SIMULATE
    ofstream myfile;
    myfile.open("attack.txt");
    cout << "attack func " << "myPosition: "  << myTank->x << " " << myTank->y << "targetId "<< tank->id << "targetPosition: " << tank->x << " " << tank->y << " targetDirection: " <<
           tank->direction<< " result: " << angle << endl;
    myfile.close();
#endif
    direction(ao->angle);
    fire();
    stay();
}

inline void tryAttack() {

}

inline void attack() {
    for (auto it = tankMap->tanks.begin(); it != tankMap->tanks.end(); ++it ) {
        if ((*it)->rebornCd < 0.00001) {
            attack(*it);
            break;
        }
    }
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
    vMap[nx][ny] = 1;
    bfsSV.push_back(tv);

    while(bBfsAV <= eBfsAV) {
        int x = bfsSV[bBfsAV][0];
        int y = bfsSV[bBfsAV][1];

        for (int i = 0 ;i < 4 ; ++ i) {
            int nx = x + mv4Step[i][0];
            int ny = y + mv4Step[i][1];

            if (isInMapRange(nx,ny) && vMap[nx][ny] == 0) {
                // TODO_CIJIAN  2018 ,Apr16 , Mon, 15:52
                if (true) {

                }

                vector<int> tv1;

                tv1.push_back(nx);
                tv1.push_back(ny);
                vMap[nx][ny] = 1;
                bfsSV.push_back(tv1);
                ++ eBfsAV;
            }
        }
        ++ bBfsAV;
    }

    return make_pair(-1,-1);
}

inline tuple<double, double> runForLife() {
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
    vMap[nx][ny] = 1;
    bfsSV.push_back(tv);


    while(bbfsSV <= ebfsSV) {
        int x = bfsSV[bbfsSV][0];
        int y = bfsSV[bbfsSV][1];

        for (int i = 0 ;i < 4 ; ++ i) {
            int nx = x + mv4Step[i][0];
            int ny = y + mv4Step[i][1];

            if (isInMapRange(nx,ny) && vMap[nx][ny] == 0) {
                // 如果发现安全区域
                //
                if (tankMap->bDMap[nx][ny] == 0 && tankMap->dMap[nx][ny] == 0 && tankMap->tDMap[nx][ny] == 0) {
                    // 反的安全区域
                    tuple<float, float> tureCoordinate = mapCoordinateToTrue(nx,ny);

                    return make_pair(get<0>(tureCoordinate), get<1>(tureCoordinate));
                }

                vector<int> tv1;

                tv1.push_back(nx);
                tv1.push_back(ny);
                vMap[nx][ny] = 1;
                bfsSV.push_back(tv1);
                ++ ebfsSV;
            }
        }
        ++ bbfsSV;
    }

    return make_pair(-1,-1);
}

inline void theLastBattle() {

}

inline void solve() {

    if (isDied(*myTank)) {

#ifdef SIMULATE

//        if (!isSimulateDie) {
//
//            ofstream myfile;
//            myfile.open("commands.txt");
//            if (turn >= 100 && myTank->rebornCd > maxRebornCd - 0.1) {
//                getMessage(tMessages[tMessages.size() - 2]);
//
//                for (int  i = 0; i <  tCommands.size() ;i ++ ) {
//                    //myfile << tCommands[i] << endl;
//                }
//
//                myfile.close();
//                isSimulateDie = true;
//                assert(0);
//            }
//        }
#endif
        return;
    }

//    attack();

    if (!isNowSafe()) {
        tuple<double,double> nearestSafeCoordinate = runForLife();

        // TODO_CIJIAN  2018 ,Apr16 , Mon, 08:19
        // 还需要判断是否找到了以及是否距离太远了
#ifdef SIMULATE
        cout <<myTank->x << ", " << myTank->y <<" scape to :" << get<0>(nearestSafeCoordinate) << " "<< get<1>(nearestSafeCoordinate) << endl;
//    if (turn >= 100 && tankMap->bullets.size() > 4) {
//        assert(0);
//    }
#endif
        goTo(get<0>(nearestSafeCoordinate), get<1>(nearestSafeCoordinate));


    } else {
       stay();
    }
}

// 每局数据处理后，用这个来填充地图危险值


inline void constructBDMap() {

#ifdef SIMULATE
    ofstream myfile;
    myfile.open("bulltes.txt");
#endif

    mapClear(tankMap->bDMap);

    for (auto it = tankMap->bullets.begin(); it != tankMap->bullets.end(); ++it) {

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
            x = get<0>(bDArea[i]);
            y = get<1>(bDArea[i]);
            // 真实坐标需要换算一下
            tuple<int, int> mapCoordinate = toMapCoordinate((*it)->x,(*it)->y);



            // 旋转坐标公式
            nx = round(x * cos((*it)->direction) - y * sin((*it)->direction) + get<0>(mapCoordinate));
            ny = round(x * sin((*it)->direction) + y * cos((*it)->direction)  + get<1>(mapCoordinate));

            if (isInMapRange(nx,ny)) {
                tankMap->bDMap[nx][ny] = BUDTYPE;
            }

#ifdef SIMULATE
            //myfile << nx << "," << ny << endl;
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
            //myfile << i << "," << j << endl;
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


inline void constructDMap() {
//构建危险矩阵矩阵
    // 构建子弹伤害矩阵
    constructBDMap();

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

        for( auto iter = radiusTDDistanceCircle.begin(); iter != radiusTDDistanceCircle.end(); ++iter) {
            int tx = get<0>(*iter) + get<0>(mapCoordinate);
            int ty = get<1>(*iter) + get<1>(mapCoordinate);

            if (isInMapRange(tx,ty)) {
                tankMap->tDMap[tx][ty] = TDTYPE;
            }
//#ifdef TEST
//            //myfile << tx << "," << ty << endl;
//#endif
        }
    }
//
//#ifdef TEST
//    myfile.close();
//#endif

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
                Tank *newTank = new Tank();
                for (json::iterator i = j.value().begin(); i != j.value().end(); ++i) {
                    if (i.key() == "direction") {
                        newTank->direction = i.value();
                    } else if (i.key() == "fire") {
                        newTank->fire = i.value();
                    } else if (i.key() == "score") {
                        newTank->score = i.value();
                    } else if (i.key() == "shieldCd") {
                        newTank->shieldCd = i.value();
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

                newTank->id = j.key();
                if (!myTankSet.count(newTank->id)) {
                    tankMap->tanks.push_back(newTank);
                } else {
                    myTank = newTank;
                }
            }
        }
    }

    constructDMap();
}

void initMap(json msg) {

    tankMap = new Map ();

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

    // 考虑边界问题，为了不溢出
    tankMap->mapHeight = (tankMap->height) * mapMultiple + 1;
    tankMap->mapWidth = (tankMap->width) * mapMultiple + 1;
    tankMap->oMap = new int*[tankMap->mapWidth];
    tankMap->dMap = new int*[tankMap->mapWidth];
    tankMap->bDMap = new int*[tankMap->mapWidth];
    tankMap->tDMap = new int*[tankMap->mapWidth];
    vMap = new int*[tankMap->mapWidth];
    for(int i = 0; i < tankMap->mapWidth; ++i) {
        tankMap->oMap[i] = new int[tankMap->mapHeight];
        tankMap->dMap[i] = new int[tankMap->mapHeight];
        tankMap->bDMap[i] = new int[tankMap->mapHeight];
        tankMap->tDMap[i] = new int[tankMap->mapHeight];

        vMap[i]= new int[tankMap->mapHeight];
    }

#ifdef TEST
    ofstream myfile;
    myfile.open("blocks.txt");
#endif
    for(auto it = tankMap->blocks.begin(); it != tankMap->blocks.end(); ++it) {
        tuple<int, int> mapCoordinate = toMapCoordinate((*it)->x,(*it)->y);
        for( auto iter = radius1Circle.begin(); iter != radius1Circle.end(); ++iter) {
            int tx = get<0>(*iter) + get<0>(mapCoordinate);
            int ty = get<1>(*iter) + get<1>(mapCoordinate);

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
}

inline void getMessage(string msg) {
#ifdef SIMULATE
    caculateClock();
    bClock = clock();
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
        m_endpoint.send(hdl, "{\"commandType\": \"aiEnterRoom\", \"roomId\": 101550, \"accessKey\": "
                             "\"248c641ededfc6d91bbc31bb2e2056ee\", \"employeeId\": 101550}", websocketpp::frame::opcode::text);
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
    for (int i = -10*mapMultiple; i <= 10*mapMultiple; ++i) {
        for (int j = -10*mapMultiple; j <= 10*mapMultiple; ++ j) {
            tuple<double, double> tmp = mapCoordinateToTrue(i, j);
            double distance = getDistance(get<0>(tmp),get<1>(tmp),0,0);

            if (distance < 0.999) {
                radius1Circle.push_back(make_pair(i,j));
            }

            if (distance <= 1.999) {
                radius2Circle.push_back(make_pair(i,j));
            }

            if (distance <= tDDistance) {
                radiusTDDistanceCircle.push_back(make_pair(i,j));
            }

        }
    }

    for (int i = -mapMultiple - 1; i <= mapMultiple + 1 ; ++i) {
        int rDistance = mapMultiple * bDDistance;
        for (int j = 0 ;j <= rDistance; j++) {
            bDArea.push_back(make_pair(j, i));
        }
    }

}

void test() {

    string str = "{\"commandType\":\"REFRESH_DATA\",\"data\":\"{\\\"width\\\":25,\\\"height\\\":15,\\\"tanks\\\":{\\\"ai:58\\\":{\\\"name\\\":\\\"流弊\\\",\\\"speed\\\":0,\\\"direction\\\":0,\\\"fireCd\\\":0,\\\"fire\\\":false,\\\"position\\\":[13.925306008084853,10.266874545908701],\\\"score\\\":0,\\\"rebornCd\\\":null,\\\"shieldCd\\\":1}},\\\"bullets\\\":[],\\\"blocks\\\":[{\\\"position\\\":[3,6],\\\"radius\\\":1},{\\\"position\\\":[0,10],\\\"radius\\\":1},{\\\"position\\\":[21,9],\\\"radius\\\":1},{\\\"position\\\":[11,12],\\\"radius\\\":1},{\\\"position\\\":[17,11],\\\"radius\\\":1},{\\\"position\\\":[11,2],\\\"radius\\\":1},{\\\"position\\\":[16,9],\\\"radius\\\":1},{\\\"position\\\":[14,5],\\\"radius\\\":1},{\\\"position\\\":[15,2],\\\"radius\\\":1},{\\\"position\\\":[15,11],\\\"radius\\\":1}]}\"}";

}
int main(int argc, char* argv[]) {
    std::string uri = "wss://tank-match.taobao.com/ai";

//    string str = "{\"width\":25,\"height\":15,\"tanks\":{\"ai:58\":{\"name\":\"流弊\",\"speed\":0,\"direction\":0,\"fireCd\":0.13299999999999929,\"fire\":true,\"position\":[10.52941026611454,5.021304607616935],\"score\":0,\"rebornCd\":null,\"shieldCd\":0}},\"bullets\":[],\"blocks\":[{\"position\":[3,6],\"radius\":1},{\"position\":[0,10],\"radius\":1},{\"position\":[21,9],\"radius\":1},{\"position\":[11,12],\"radius\":1},{\"position\":[17,11],\"radius\":1},{\"position\":[11,2],\"radius\":1},{\"position\":[16,9],\"radius\":1},{\"position\":[14,5],\"radius\":1},{\"position\":[15,2],\"radius\":1},{\"position\":[15,11],\"radius\":1}]}";
//
//    auto j3 = json::parse(str);
//    initMap(j3);

#ifdef SIMULATE
    system("open -a /Applications/iTerm\\ 2.app /bin/bash ../tank_ai/new_game.sh");
#endif
    beginPrepare();


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