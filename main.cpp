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

using json = nlohmann::json;


using namespace std;

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;


class perftest;

client m_endpoint;
websocketpp::connection_hdl ghdl;
// pull out the type of messages sent by our config
typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;
typedef client::connection_ptr connection_ptr;

int turn = 0;
int mapMultiple = 20;
float tDDistance = 3; // 坦克的伤害半径
float bDDistance = 1; // 子弹的伤害远度

int TDTYPE = -2;
int BLDTYPE = -1;
int BUDTYPE = -3;

vector<tuple<int,int>> radius1Circle;
vector<tuple<int,int>> radius2Circle;
vector<tuple<int,int>> radiusTDDistanceCircle;
vector<tuple<int,int>> bDArea;

//perftest endpoint;
std::set<std::string> myTankSet = {"ai:58"};

inline tuple<float,float> mapCoordinateToTrue(int x, int y) {
    return make_pair((x * 1.0 / mapMultiple), (y * 1.0 / mapMultiple));
}

inline tuple<float,float> toMapCoordinate(int x, int y) {
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
};

Map *tankMap;

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


    int nx = round(x);
    int ny = round(y);
    if (isInMapRange(x,y) && tankMap->bDMap[nx][ny] != 0) {
        return false;
    } else {
        return true;
    }
}

inline bool isNowSafe() {
    return isXYSafe(myTank->x, myTank->y);
}

inline bool isDied(Tank t) {
    return (t.rebornCd < 0.00001);
}

inline void goTo(double x, double y) {

}

inline void testGoto() {
    double angle = 0;
    for (int i = 0 ; i < 20 ; i ++) {
        m_endpoint.send(ghdl, "{\"commandType\": \"direction\", \"angle\": " + to_string(angle/(2 * M_PI) * 360) + "}" , websocketpp::frame::opcode::text);
        angle += M_PI/20;
        // usleep 微秒  1e6
        usleep(16000);
    }
}

inline double xyAttackAngle(double x, double y, double x1, double y1, double direction) {

    // 追击公式  91 * x * x = y * y - 6*x*y*cos(θ)

    cout << atan2(y - y1, x - x1) << endl;
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
    return angleAns;

}

inline void attack(Tank tank) {
    double angle = xyAttackAngle(myTank->x, myTank->y, tank.x, tank.y, tank.direction);
#ifdef SIMULATE
    ofstream myfile;
    myfile.open("attack.txt");
    cout << "attack func " << "myPosition: "  << myTank->x << " " << myTank->y << "targetId "<< tank.id << "targetPosition: " << tank.x << " " << tank.y << " targetDirection: " <<
           tank.direction<< " result: " << angle << endl;
    myfile.close();
#endif
    m_endpoint.send(ghdl, "{\"commandType\": \"direction\", \"angle\": " + to_string(angle/(2 * M_PI) * 360) + "}" , websocketpp::frame::opcode::text);

    m_endpoint.send(ghdl, "{\"commandType\": \"fire\"}", websocketpp::frame::opcode::text);

    m_endpoint.send(ghdl, "{\"commandType\": \"direction\", \"angle\": -1}", websocketpp::frame::opcode::text);

}
inline void attack() {
    for (auto it = tankMap->tanks.begin(); it != tankMap->tanks.end(); ++it ) {
        if ((*it)->rebornCd < 0.00001) {
            attack(**it);
            break;
        }
    }
}
inline void solve() {
//    testGoto();
//    attack();

    if (!isNowSafe()) {
        m_endpoint.send(ghdl, "{\"commandType\": \"fire\"}", websocketpp::frame::opcode::text);
    }
}

// 每局数据处理后，用这个来填充地图危险值


inline void constructBDMap() {

#ifdef SIMULATE
    ofstream myfile;
    myfile.open("bulltes.txt");
#endif

    for(int i = 0; i < tankMap->mapWidth; ++i) {
        memset(tankMap->bDMap[i], 0, sizeof(int) * tankMap->mapHeight);
    }

    for (auto it = tankMap->bullets.begin(); it != tankMap->bullets.end(); ++it) {
        int nx = 0;
        int ny = 0;
        int x = 0;
        int y = 0;
        double alpha;
        double distnace;
        double r;
        int vsize = bDArea.size();
        for (int i = 0 ;i < vsize; i ++) {
            x = get<0>(bDArea[i]);
            y = get<1>(bDArea[i]);
            r = sqrt(x*x + y*y);
            alpha = atan2(y,x);
            alpha += (*it)->direction;
            nx = round(r * cos(alpha) + (*it)->x);
            ny = round(r * sin(alpha) + (*it)->y);
            if (isInMapRange(nx,ny)) {
                tankMap->bDMap[nx][ny] = BUDTYPE;
            }
#ifdef SIMULATE
            myfile  << nx << "," << ny << endl;
#endif
        }
    }
#ifdef SIMULATE
    myfile.close();
#endif
}


inline void constructDMap() {
//构建危险矩阵矩阵

    // 构建子弹伤害矩阵
    constructBDMap();
    // TODO_CIJIAN  2018 ,Apr15 , Sun, 20:02


#ifdef TEST
    ofstream myfile;
    myfile.open("tanks.txt",ios::trunc);
#endif
    for(auto it = tankMap->tanks.begin(); it != tankMap->tanks.end(); ++it) {
        tuple<int, int> mapCoordinate = toMapCoordinate((*it)->x,(*it)->y);

        // 复活时间大的
        if ((*it)->rebornCd > 0.00001) {
            continue;
        }

        for( auto iter = radiusTDDistanceCircle.begin(); iter != radiusTDDistanceCircle.end(); ++iter) {
            int tx = get<0>(*iter) + get<0>(mapCoordinate);
            int ty = get<1>(*iter) + get<1>(mapCoordinate);

            if (isInMapRange(tx,ty)) {
                tankMap->dMap[tx][ty] = TDTYPE;
            }
#ifdef TEST
            myfile << tx << "," << ty << endl;
#endif
        }
    }

#ifdef TEST
    myfile.close();
#endif

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
    tankMap->mapHeight = tankMap->height * mapMultiple;
    tankMap->mapWidth = tankMap->width * mapMultiple;
    tankMap->oMap = new int*[tankMap->mapWidth];
    tankMap->dMap = new int*[tankMap->mapWidth];
    tankMap->bDMap = new int*[tankMap->mapWidth];

    // todo 完成XY对应
    for(int i = 0; i < tankMap->mapWidth; ++i) {
        tankMap->oMap[i] = new int[tankMap->mapHeight];
        tankMap->dMap[i] = new int[tankMap->mapHeight];
        tankMap->bDMap[i] = new int[tankMap->mapHeight];
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
            myfile << tx << "," << ty << endl;
#endif
        }

    }
#ifdef TEST
    myfile.close();
    //todo
    system("python3 ../tank_ai/drawer.py");
#endif
}

void getTestMessage(string msg) {
    auto j3 = json::parse(msg);

//    for (auto& element : j3) {
//        std::cout << element << '\n';
//    }
    if (turn == 0) {
        for (json::iterator it = j3.begin(); it != j3.end(); ++it) {
            if (it.key() == "data") {
                string temp = it.value();
                initMap(json::parse(temp));
            }
        }
    } else {
        constructMapInfo(j3);
    }
    turn ++;


}

inline void getMessage(string msg , client &cl, websocketpp::connection_hdl hdl) {

    auto j3 = json::parse(msg);

//    for (auto& element : j3) {
//        std::cout << element << '\n';
//    }
    if (turn == 0) {
        ghdl = hdl;
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
                initMap(json::parse(temp));
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
        getMessage(msg->get_payload(), m_endpoint, hdl);
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
            bDArea.push_back(make_pair(i,j));
        }
    }

}

void test() {

    string str = "{\"commandType\":\"REFRESH_DATA\",\"data\":\"{\\\"width\\\":25,\\\"height\\\":15,\\\"tanks\\\":{\\\"ai:58\\\":{\\\"name\\\":\\\"流弊\\\",\\\"speed\\\":0,\\\"direction\\\":0,\\\"fireCd\\\":0,\\\"fire\\\":false,\\\"position\\\":[13.925306008084853,10.266874545908701],\\\"score\\\":0,\\\"rebornCd\\\":null,\\\"shieldCd\\\":1}},\\\"bullets\\\":[],\\\"blocks\\\":[{\\\"position\\\":[3,6],\\\"radius\\\":1},{\\\"position\\\":[0,10],\\\"radius\\\":1},{\\\"position\\\":[21,9],\\\"radius\\\":1},{\\\"position\\\":[11,12],\\\"radius\\\":1},{\\\"position\\\":[17,11],\\\"radius\\\":1},{\\\"position\\\":[11,2],\\\"radius\\\":1},{\\\"position\\\":[16,9],\\\"radius\\\":1},{\\\"position\\\":[14,5],\\\"radius\\\":1},{\\\"position\\\":[15,2],\\\"radius\\\":1},{\\\"position\\\":[15,11],\\\"radius\\\":1}]}\"}";

    getTestMessage(str);
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


//    test();

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