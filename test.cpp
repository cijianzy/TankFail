// basic file operations
#include <iostream>
#include <fstream>
#include <math.h>
using namespace std;

//    myfile.open ("example.txt");
//    myfile << "Writing this to a file.\n";
//    myfile << "Writing this to a file.\n";
//    myfile << "Writing this to a file.\n";
//    myfile << "Writing this to a file.\n";
//    myfile.close();
//
//    system("cd ../tank_ai");
//    system("pwd");
//    system("bash ../tank_ai/new_game.sh");
//    return 0;
//}

inline float getDistance(float x1, float y1, float x2, float y2) {
    return sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
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

inline bool hasIntersection(double x1, double y1, double x2, double y2, double x3, double y3, double r) {
    double  A = (y2 - y1) / (x2 -x1);
    double B = -1;
    double C = y1 - A * x1;

    double lineDistance = abs((A*x3 + B * y3 + C) / sqrt(A*A+B*B));

    double pointOneDistance = getDistance(x1,y1,x3,y3);

    double pointTwoDistance = getDistance(x2,y2,x3,y3);

    if (lineDistance > r) {
        return false;
    }

    // 一个点在圆，一定有交点
    if (pointOneDistance == r || pointTwoDistance == r) {
        return true;
    }

    if (min(pointOneDistance, pointTwoDistance) < r &&  max(pointOneDistance, pointTwoDistance) > r) {
        return true;
    }

    double pointDistanceSelf = getDistance(x1, y1, x2, y2);
    double beta = acos((pointOneDistance * pointOneDistance +
                        pointTwoDistance * pointTwoDistance -
                        pointDistanceSelf * pointDistanceSelf) / (2 * pointOneDistance * pointTwoDistance));

    if (pointOneDistance > r &&
        pointTwoDistance > r &&
        beta > M_PI / 2) {
        return true;
    }

    return false;
}

int main() {
    cout << hasIntersection(0, 1, 0, -1 ,0 , 0 , 0.5 ) << endl;
    cout << hasIntersection(0, 1, 0, -1 ,1.0001, 0 , 1 ) << endl;
}
//attack func myPosition: 11.5444 6.9601targetId ai:132targetPosition: 3.68668 15 targetDirection: 0.994838 result: 3.71427
