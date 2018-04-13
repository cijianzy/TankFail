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
using json = nlohmann::json;

//#include <jon>

using namespace std;

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// pull out the type of messages sent by our config
typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;
typedef client::connection_ptr connection_ptr;

int turn = 0;
int mapMultiple = 20;
vector<tuple<int,int>> radius1Circle;
vector<tuple<int,int>> radius2Circle;

std::set<std::string> myTankSet = {"ai:58"};

class Tank {
public:
    string id;
    float direction, // 坦克的方向，在 0-2π 之间，0 为水平向右，顺时针
            fireCd, // 开火冷却时间，即还有多久可再次开火，0为可立即开火
            x, // 坦克的当前位置 X
            y, // 坦克的当前位置 Y
            rebornCd, // 复活冷却时间，即被击毁后还有多久可复活 null 表示在场上战斗的状态
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
    int map_height;
    int map_width;
    vector<Tank *> tanks;
    vector<Block *> blocks;
    vector<Bullet *> bullets;

    int **oMap = NULL;
    int **dMap = NULL;
};

float getDistance(float x1, float y1, float x2, float y2) {
    return sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
}

Map *tank_map;

void constructMapInfo(json msg) {

    tank_map->tanks.clear();

    // construct map
    for (json::iterator it = msg.begin(); it != msg.end(); ++it) {
        cout << it.key() << endl;

        if (it.key() == "bullets") {
            for (json::iterator  j= it.value().begin(); j != it.value().end(); ++ j) {
                for (json::iterator i = j.value().begin(); i != j.value().end(); ++i) {
                    Bullet *newBullet = new Bullet();
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

                    if (!myTankSet.count(newBullet->from)) {
                        tank_map->bullets.push_back(newBullet);
                    }
                }
            }
        }

        if (it.key() == "tanks") {
            for (json::iterator  j= it.value().begin(); j != it.value().end(); ++ j) {
                for (json::iterator i = j.value().begin(); i != j.value().end(); ++i) {
                    Tank *newTank = new Tank();
                    cout << i.value() << endl;
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
                    }

                    newTank->id = i.key();

                    if (!myTankSet.count(newTank->id)) {
                        tank_map->tanks.push_back(newTank);
                    } else {
                        myTank = newTank;
                    }
                }
            }
        }

    }
}

tuple<float,float> mapCoordinateToTrue(int x, int y) {
    return make_pair((x * 1.0 / mapMultiple), (y * 1.0 / mapMultiple));
}

void initMap(json msg) {

    tank_map = new Map ();

    for (json::iterator it = msg.begin(); it != msg.end(); ++it) {

        if (it.key() == "height") {
            tank_map->height = it.value();
        } else if (it.key() == "width"){
            tank_map->width = it.value();
        }

        if (it.key() == "blocks") {
            for (json::iterator  j= it.value().begin(); j != it.value().end(); ++ j) {
                for (json::iterator i = j.value().begin(); i != j.value().end(); ++i) {
                    Block *newBlock = new Block();
                    cout << i.value() << endl;
                    if (i.key() == "radius") {
                        newBlock->radius = i.value();
                    } else if (i.key() == "position") {
                        newBlock->x =  i.value()[0];
                        newBlock->y =  i.value()[1];
                    }

                    tank_map->blocks.push_back(newBlock);
                }
            }
        }
    }

    constructMapInfo(msg);
    tank_map->map_height = tank_map->height * mapMultiple;
    tank_map->map_width = tank_map->width * mapMultiple;
    tank_map->oMap = new int*[tank_map->map_height];

    for(int i = 0; i < tank_map->height; ++i)
        tank_map->oMap[i] = new int[tank_map->map_width];

    for(int i = )
}

void getMessage(string msg , client &cl, websocketpp::connection_hdl hdl) {
    cout << msg << endl;
    cl.send(hdl, "{\"commandType\": \"fire\"}", websocketpp::frame::opcode::text);

    auto j3 = json::parse(msg);

    for (auto& element : j3) {
        std::cout << element << '\n';
    }
    if (turn == 0) {
        for (json::iterator it = j3.begin(); it != j3.end(); ++it) {
            cout << it.key() << endl;
            if (it.key() == "data") {
                string temp = it.value();
                initMap(json::parse(temp));
            }
        }
    } else {
        constructMapInfo(j3);
    }

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
    client m_endpoint;

    std::chrono::high_resolution_clock::time_point m_start;
    std::chrono::high_resolution_clock::time_point m_socket_init;
    std::chrono::high_resolution_clock::time_point m_tls_init;
    std::chrono::high_resolution_clock::time_point m_open;
    std::chrono::high_resolution_clock::time_point m_message;
    std::chrono::high_resolution_clock::time_point m_close;
};


void beginPrepare() {
    for (int i = -2*mapMultiple; i <= 2*mapMultiple; ++i) {
        for (int j = -2*mapMultiple; j <= 2*mapMultiple; ++ j) {
            tuple<float, float> tmp = mapCoordinateToTrue(i, j);
            float distance = getDistance(get<0>tmp,get<1>tmp,0,0);

            if (distance <= 1.0001) {
                radius1Circle.push_back(make_pair(i,j));
            }

            if (distance <= 2.0001) {
                radius2Circle.push_back(make_pair(i,j));
            }
        }
    }
}
int main(int argc, char* argv[]) {
    std::string uri = "wss://tank-match.taobao.com/ai";

//    string str = "{\"width\":25,\"height\":15,\"tanks\":{\"ai:58\":{\"name\":\"流弊\",\"speed\":0,\"direction\":0,\"fireCd\":0.13299999999999929,\"fire\":true,\"position\":[10.52941026611454,5.021304607616935],\"score\":0,\"rebornCd\":null,\"shieldCd\":0}},\"bullets\":[],\"blocks\":[{\"position\":[3,6],\"radius\":1},{\"position\":[0,10],\"radius\":1},{\"position\":[21,9],\"radius\":1},{\"position\":[11,12],\"radius\":1},{\"position\":[17,11],\"radius\":1},{\"position\":[11,2],\"radius\":1},{\"position\":[16,9],\"radius\":1},{\"position\":[14,5],\"radius\":1},{\"position\":[15,2],\"radius\":1},{\"position\":[15,11],\"radius\":1}]}";
//
//    auto j3 = json::parse(str);
//    initMap(j3);


    if (argc == 2) {
        uri = argv[1];
    }

    try {
        perftest endpoint;
        endpoint.start(uri);
    } catch (const std::exception & e) {
        std::cout << e.what() << std::endl;
    } catch (websocketpp::lib::error_code e) {
        std::cout << e.message() << std::endl;
    } catch (...) {
        std::cout << "other exception" << std::endl;
    }
}